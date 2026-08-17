#!/usr/bin/env bash
# scripts/check-style.sh [--fix] [--require-clang-format] [путь ...]
#
# Проверка оформления кода практикума по .clang-format и правилам курса
# (см. practices/README.md, раздел «Требования к коду»).
# Запускается вручную, целью CMake style-check и в CI.
#
#   без аргументов          проверить весь код в practices/
#   --fix                   переформатировать (clang-format) и нормализовать
#   --require-clang-format  падать, если подходящей версии clang-format нет
#   путь ...                проверить только указанные файлы или каталоги
#
# Проверяется:
#   1. форматирование по .clang-format (если clang-format установлен);
#   2. отсутствие табуляции — отступ только пробелами;
#   3. длина строки не больше 100 символов;
#   4. отсутствие комментариев // — их нет в ISO C90;
#   5. концы строк LF и отсутствие BOM;
#   6. отсутствие пробелов в конце строки и перевод строки в конце файла.
#
# Проверки 2–6 выполняет scripts/style-scan.py — там же объяснено, почему не
# grep.

set -uo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
FIX=0
REQUIRE_FORMAT=0
PATHS=()
MAX_LINE=100

# Наименьшая пригодная версия clang-format. Начиная с 19-й завершающие
# комментарии выравниваются в столбик и после `#define`, и в перечислениях;
# 18-я и старее их сжимает до одного пробела, то есть та же команда --fix на
# разных машинах даёт разный код. Проверять оформление такой версией нельзя:
# она объявит расходящимися файлы, оформленные правильно.
CLANG_FORMAT_MIN_MAJOR=19

while [ $# -gt 0 ]; do
  case "$1" in
    --fix) FIX=1 ;;
    --require-clang-format) REQUIRE_FORMAT=1 ;;
    -h|--help) sed -n '2,22p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) PATHS+=( "$1" ) ;;
  esac
  shift
done

[ "${#PATHS[@]}" -eq 0 ] && PATHS=( "$ROOT/practices" )

if command -v tput >/dev/null 2>&1 && [ -t 1 ]; then
  C_RED="$(tput setaf 1)"; C_GRN="$(tput setaf 2)"; C_YEL="$(tput setaf 3)"; C_OFF="$(tput sgr0)"
else
  C_RED=""; C_GRN=""; C_YEL=""; C_OFF=""
fi
ok()   { printf '%s  ok  %s%s\n' "$C_GRN" "$C_OFF" "$*"; }
warn() { printf '%s warn %s%s\n' "$C_YEL" "$C_OFF" "$*" >&2; }
bad()  { printf '%s FAIL %s%s\n' "$C_RED" "$C_OFF" "$*" >&2; }

# Сторонний и сгенерированный код под требования курса не подпадает.
is_excluded() {
  case "$1" in
    */catch2/*|*/resources/spin/*|*/generated/*|*/cmake-build*/*|*/build/*) return 0 ;;
    *) return 1 ;;
  esac
}

FILES=()
for p in "${PATHS[@]}"; do
  if [ -f "$p" ]; then
    is_excluded "$p" || FILES+=( "$p" )
  elif [ -d "$p" ]; then
    while IFS= read -r f; do
      is_excluded "$f" || FILES+=( "$f" )
    done < <(find "$p" \( -name '*.c' -o -name '*.h' \) -type f | sort)
  fi
done

if [ "${#FILES[@]}" -eq 0 ]; then
  warn "нечего проверять"
  exit 0
fi

echo "== файлов на проверке: ${#FILES[@]}"
ERRORS=0

# --- 1. clang-format --------------------------------------------------------
# Кандидаты в порядке предпочтения: заданный вручную, версия с номером в имени
# (так пакеты называются в Debian и Ubuntu), просто clang-format, и наконец
# толчейн Xcode — на macOS clang-format есть в ней, но не в PATH.
format_major() { # format_major <бинарь> -> мажорная версия или пустая строка
  "$1" --version 2>/dev/null | sed -n 's/.*version \([0-9][0-9]*\)\..*/\1/p' | head -1
}

CLANG_FORMAT="${CLANG_FORMAT:-}"
CLANG_FORMAT_MAJOR=""
CLANG_FORMAT_OLD=0
if [ -n "$CLANG_FORMAT" ]; then
  CLANG_FORMAT_MAJOR="$(format_major "$CLANG_FORMAT")"
else
  CANDIDATES=( clang-format-21 clang-format-20 clang-format-19 clang-format )
  if command -v xcrun >/dev/null 2>&1 && xcrun -f clang-format >/dev/null 2>&1; then
    CANDIDATES+=( "$(xcrun -f clang-format)" )
  fi
  for cand in "${CANDIDATES[@]}"; do
    command -v "$cand" >/dev/null 2>&1 || continue
    major="$(format_major "$cand")"
    [ -n "$major" ] || continue
    # Первый найденный запоминаем в любом случае: если подходящего так и не
    # окажется, о нём будет сказано в предупреждении.
    if [ -z "$CLANG_FORMAT" ]; then
      CLANG_FORMAT="$cand"; CLANG_FORMAT_MAJOR="$major"
    fi
    if [ "$major" -ge "$CLANG_FORMAT_MIN_MAJOR" ]; then
      CLANG_FORMAT="$cand"; CLANG_FORMAT_MAJOR="$major"
      break
    fi
  done
fi

# Версия старше нужной проверку не выполняет, а сбивает с толку: она найдёт
# «нарушения» в правильно оформленном коде. Поэтому такая же ситуация, как
# отсутствие clang-format вовсе, — предупреждение и пропуск; в CI пропуск
# запрещён ключом --require-clang-format.
if [ -n "$CLANG_FORMAT" ] && [ -n "$CLANG_FORMAT_MAJOR" ] \
    && [ "$CLANG_FORMAT_MAJOR" -lt "$CLANG_FORMAT_MIN_MAJOR" ]; then
  warn "clang-format $CLANG_FORMAT_MAJOR старше требуемой $CLANG_FORMAT_MIN_MAJOR — проверка форматирования пропущена"
  warn "  разные версии по-разному выравнивают завершающие комментарии"
  if [ "$REQUIRE_FORMAT" = 1 ]; then
    bad "нужен clang-format $CLANG_FORMAT_MIN_MAJOR или новее"
    exit 1
  fi
  CLANG_FORMAT=""
  CLANG_FORMAT_OLD=1
fi

if [ -n "$CLANG_FORMAT" ]; then
  if [ "$FIX" = 1 ]; then
    "$CLANG_FORMAT" -i --style=file "${FILES[@]}"
    ok "clang-format: файлы переформатированы"
  else
    FMT_BAD=()
    for f in "${FILES[@]}"; do
      if ! "$CLANG_FORMAT" --style=file --dry-run -Werror "$f" >/dev/null 2>&1; then
        FMT_BAD+=( "${f#$ROOT/}" )
      fi
    done
    if [ "${#FMT_BAD[@]}" -gt 0 ]; then
      bad "clang-format: расходится с .clang-format в ${#FMT_BAD[@]} файлах"
      printf '       %s\n' "${FMT_BAD[@]}" >&2
      echo "       поправить: scripts/check-style.sh --fix" >&2
      ERRORS=$((ERRORS + 1))
    else
      ok "clang-format: расхождений нет"
    fi
  fi
elif [ "$CLANG_FORMAT_OLD" = 0 ]; then
  warn "clang-format не установлен — проверка форматирования пропущена"
  warn "  macOS: brew install llvm, Debian/Ubuntu: apt install clang-format-19"
  warn "  где угодно: python3 -m venv .venv && .venv/bin/pip install clang-format"
  if [ "$REQUIRE_FORMAT" = 1 ]; then
    bad "нужен clang-format $CLANG_FORMAT_MIN_MAJOR или новее"
    exit 1
  fi
fi

# --- 2..6. собственные проверки --------------------------------------------
report() { # report <заголовок> <файл со списком нарушений>
  local title="$1" list="$2"
  if [ -s "$list" ]; then
    bad "$title"
    sed 's/^/       /' "$list" >&2
    ERRORS=$((ERRORS + 1))
  else
    ok "$title — нарушений нет"
  fi
}

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ "$FIX" = 1 ]; then
  for f in "${FILES[@]}"; do
    perl -pi -e 's/\r$//; s/[ \t]+$//;' "$f"
    perl -0pi -e 's/^\xEF\xBB\xBF//' "$f"
    tail -c1 "$f" | od -An -c | grep -q '\\n' || printf '\n' >> "$f"
  done
  ok "нормализованы концы строк, BOM, пробелы в конце строк"
  echo
  ok "готово; проверить результат: scripts/check-style.sh"
  exit 0
fi

if ! printf '%s\n' "${FILES[@]}" \
    | python3 "$ROOT/scripts/style-scan.py" "$ROOT" "$MAX_LINE" "$TMP"; then
  bad "построчные проверки не отработали (scripts/style-scan.py)"
  exit 1
fi

report "табуляция в отступах"                          "$TMP/tabs"
report "строки длиннее $MAX_LINE символов"             "$TMP/long"
report "комментарии // (в ISO C90 их нет)"             "$TMP/slashes"
report "BOM в начале файла"                            "$TMP/bom"
report "концы строк CRLF"                              "$TMP/crlf"
report "пробелы в конце строки"                        "$TMP/trail"
report "нет перевода строки в конце файла"             "$TMP/eof"

echo
if [ "$ERRORS" -gt 0 ]; then
  bad "проверок с нарушениями: $ERRORS"
  exit 1
fi
ok "оформление кода соответствует правилам курса"
