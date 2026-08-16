#!/usr/bin/env bash
# scripts/check-style.sh [--fix] [путь ...]
#
# Проверка оформления кода практикума по .clang-format и правилам курса
# (см. practices/README.md, раздел «Требования к коду»).
# Запускается вручную, целью CMake style-check и в CI.
#
#   без аргументов   проверить весь код в practices/
#   --fix            переформатировать (clang-format) и нормализовать файлы
#   путь ...         проверить только указанные файлы или каталоги
#
# Проверяется:
#   1. форматирование по .clang-format (если clang-format установлен);
#   2. отсутствие табуляции — отступ только пробелами;
#   3. длина строки не больше 100 символов;
#   4. отсутствие комментариев // — их нет в ISO C90;
#   5. концы строк LF и отсутствие BOM;
#   6. отсутствие пробелов в конце строки и перевод строки в конце файла.

set -uo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
FIX=0
PATHS=()
MAX_LINE=100

while [ $# -gt 0 ]; do
  case "$1" in
    --fix) FIX=1 ;;
    -h|--help) sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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
# На macOS clang-format обычно есть в толчейне Xcode, но не в PATH.
CLANG_FORMAT="${CLANG_FORMAT:-}"
if [ -z "$CLANG_FORMAT" ]; then
  if command -v clang-format >/dev/null 2>&1; then
    CLANG_FORMAT="clang-format"
  elif command -v xcrun >/dev/null 2>&1 && xcrun -f clang-format >/dev/null 2>&1; then
    CLANG_FORMAT="$(xcrun -f clang-format)"
  fi
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
else
  warn "clang-format не установлен — проверка форматирования пропущена"
  warn "  macOS: brew install clang-format, Debian/Ubuntu: apt install clang-format"
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

for f in "${FILES[@]}"; do
  rel="${f#$ROOT/}"

  if [ "$FIX" = 1 ]; then
    perl -pi -e 's/\r$//; s/[ \t]+$//;' "$f"
    perl -0pi -e 's/^\xEF\xBB\xBF//' "$f"
    tail -c1 "$f" | od -An -c | grep -q '\\n' || printf '\n' >> "$f"
    continue
  fi

  grep -nP '\t' "$f" 2>/dev/null | head -3 | sed "s|^|$rel:|" >> "$TMP/tabs"
  # Длина считается в символах, а не в байтах: комментарии на русском.
  python3 -c '
import sys
path, rel, limit = sys.argv[1], sys.argv[2], int(sys.argv[3])
with open(path, encoding="utf-8", errors="replace") as fh:
    shown = 0
    for n, line in enumerate(fh, 1):
        text = line.rstrip("\n")
        if len(text) > limit and shown < 3:
            print("%s:%d: %d символов" % (rel, n, len(text)))
            shown += 1
' "$f" "$rel" "$MAX_LINE" >> "$TMP/long"
  grep -nP '^(?:[^"'\''/\n]|"(?:[^"\\]|\\.)*"|'\''(?:[^'\''\\]|\\.)*'\'')*//' "$f" 2>/dev/null \
      | head -3 | sed "s|^|$rel:|" >> "$TMP/slashes"
  if head -c3 "$f" | grep -q $'\xEF\xBB\xBF'; then echo "$rel: BOM в начале файла" >> "$TMP/bom"; fi
  if grep -qU $'\r' "$f" 2>/dev/null; then echo "$rel: концы строк CRLF" >> "$TMP/crlf"; fi
  grep -n '[ 	]$' "$f" | head -3 | sed "s|^|$rel:|" >> "$TMP/trail"
  if [ -s "$f" ] && [ "$(tail -c1 "$f" | wc -l | tr -d ' ')" = "0" ]; then
    echo "$rel: нет перевода строки в конце файла" >> "$TMP/eof"
  fi
done

if [ "$FIX" = 1 ]; then
  ok "нормализованы концы строк, BOM, пробелы в конце строк"
  echo
  ok "готово; проверить результат: scripts/check-style.sh"
  exit 0
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
