#!/usr/bin/env bash
# scripts/check-misra.sh [--strict] [--raw ФАЙЛ] [путь ...]
#
# Оценка кода практикума по правилам MISRA C:2012 (ТД-5). Проверка
# ФАКУЛЬТАТИВНАЯ: курс написан на ISO C90 и учит автоматному программированию,
# а не сертификации, поэтому нарушения здесь — предмет разбора, а не запрета.
# Скрипт всегда завершается успешно, если анализ прошёл; список находок он
# печатает, а решение по каждой — в docs/misra-report.md.
#
#   без аргументов   проверить весь код в practices/
#   --strict         считать отсутствие cppcheck ошибкой (так стоит в CI)
#   --raw ФАЙЛ       сохранить полный вывод анализатора
#   путь ...         проверить только указанные файлы или каталоги
#
# Анализатор — cppcheck с аддоном misra. Это ПЕРЕЛОЖЕНИЕ правил, а не
# первоисточник: официальная публикация MISRA распространяется по платной
# лицензии. Тексты правил в выводе — тоже переложение, они лежат в
# docs/misra-rules.txt. Подробнее — docs/misra-report.md.

set -uo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
STRICT=0
RAW=""
PATHS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --strict) STRICT=1 ;;
    --raw) RAW="${2:-}"; shift ;;
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

CPPCHECK="${CPPCHECK:-cppcheck}"
if ! command -v "$CPPCHECK" >/dev/null 2>&1; then
  warn "cppcheck не установлен — проверка MISRA пропущена"
  warn "  macOS: brew install cppcheck, Debian/Ubuntu: apt install cppcheck"
  [ "$STRICT" = 1 ] && { bad "нужен cppcheck"; exit 1; }
  exit 0
fi

# Аддон misra написан на python и запускается самим cppcheck.
if ! command -v python3 >/dev/null 2>&1; then
  warn "python3 не найден — аддон misra запустить нечем"
  [ "$STRICT" = 1 ] && { bad "нужен python3"; exit 1; }
  exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Сторонний и порождённый код под правила курса не подпадает: тот же список
# исключений, что в scripts/check-style.sh.
FILES="$TMP/files.txt"
for p in "${PATHS[@]}"; do
  if [ -f "$p" ]; then
    printf '%s\n' "$p"
  elif [ -d "$p" ]; then
    find "$p" -name '*.c' -type f \
      -not -path '*/catch2/*' -not -path '*/resources/*' \
      -not -path '*/generated/*' -not -path '*/build/*' -not -path '*/cmake-build*/*'
  fi
done | sort > "$FILES"

COUNT="$(wc -l < "$FILES" | tr -d ' ')"
if [ "$COUNT" = "0" ]; then
  warn "нечего проверять"
  exit 0
fi

echo "== файлов на проверке: $COUNT ($("$CPPCHECK" --version))"

# Тексты правил передаются аддону конфигурацией: ключа --rule-texts у самого
# cppcheck нет, он принадлежит аддону.
ADDON="$TMP/misra.json"
RULES="$ROOT/docs/misra-rules.txt"
if [ -f "$RULES" ]; then
  printf '{"script": "misra", "args": ["--rule-texts=%s"]}\n' "$RULES" > "$ADDON"
else
  warn "нет docs/misra-rules.txt — вывод будет без формулировок правил"
  printf '{"script": "misra"}\n' > "$ADDON"
fi

# Каталоги практик добавляются в пути поиска заголовков: каждая практика
# включает свои заголовки по имени, без пути.
INCLUDES=( -I "$ROOT/practices/common" -I "$ROOT/practices/common/network" )
while IFS= read -r dir; do
  INCLUDES+=( -I "$dir" )
done < <(find "$ROOT/practices" -mindepth 1 -maxdepth 2 -type d \
           -not -path '*/catch2*' -not -path '*/resources*' -not -path '*/generated*' | sort)

OUT="$TMP/misra.txt"
"$CPPCHECK" --addon="$ADDON" --std=c89 --quiet --enable=style --inline-suppr \
    "${INCLUDES[@]}" --file-list="$FILES" 2> "$OUT"
STATUS=$?

if [ -n "$RAW" ]; then
  cp "$OUT" "$RAW"
  ok "полный вывод: $RAW"
fi

if [ "$STATUS" != 0 ]; then
  bad "cppcheck завершился с кодом $STATUS"
  sed -n '1,20p' "$OUT" >&2
  exit 1
fi

TOTAL="$(grep -c 'misra-c2012-' "$OUT" 2>/dev/null || true)"
echo
echo "== нарушения по правилам (всего $TOTAL)"
grep -o 'misra-c2012-[0-9.]*' "$OUT" 2>/dev/null \
  | sed 's/misra-c2012-//' \
  | sort | uniq -c | sort -rn \
  | awk '{ printf "   %5d  MISRA %s\n", $1, $2 }'

# Находки самого cppcheck (вне MISRA) ценны отдельно: это уже не «стиль
# промышленного набора правил», а обычные дефекты — неверный формат printf,
# чтение неинициализированного, выход за границы.
echo
echo "== прочие находки анализатора"
if grep -E ': (error|warning|portability|performance):' "$OUT" \
     | grep -v 'misra' | sed 's|^'"$ROOT"'/||' | sort; then
  :
else
  echo "   нет"
fi

echo
ok "проверка MISRA выполнена; разбор находок — docs/misra-report.md"
