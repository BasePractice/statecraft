#!/usr/bin/env bash
# lectures/scripts/build.sh [опции] [id ...]
#
#   без аргументов   собрать все лекции из реестра course.typ
#   id ...           собрать только указанные (например: 03-synthesis 10-limits)
#
#   -w, --watch      пересборка при изменении (только для одной лекции)
#   -o, --open       открыть PDF после сборки
#   -j N             параллельность (по умолчанию — число ядер)
#   -d, --draft      водяной знак «ЧЕРНОВИК»
#   -p, --pretty     дополнительно положить копию под читаемым именем
#                    «03 — Синтез автоматов и автоматные схемы.pdf»
#       --out DIR    каталог результата (по умолчанию lectures/out);
#                    этим ключом пользуется сборка из CMake
#   -h, --help       эта справка

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

# --- внутренний режим: сборка одной лекции, вызывается из xargs ------------
if [ "${1:-}" = "--build-one" ]; then
  id="$2"
  draft="${3:-0}"
  typst_args
  src="$SRC_DIR/$id/main.typ"
  if [ ! -f "$src" ]; then
    warn "$id: нет src/$id/main.typ, пропускаю"
    exit 0
  fi
  extra=()
  [ "$draft" = 1 ] && extra=( --input draft=1 )
  mkdir -p "$OUT_DIR"
  if "$TYPST_BIN" compile "${TYPST_ARGS[@]}" ${extra[@]+"${extra[@]}"} "$src" "$OUT_DIR/$id.pdf"; then
    ok "$id.pdf"
    exit 0
  else
    warn "$id: сборка не удалась"
    exit 1
  fi
fi
# ---------------------------------------------------------------------------

WATCH=0; OPEN=0; DRAFT=0; PRETTY=0; JOBS=""
IDS=()

while [ $# -gt 0 ]; do
  case "$1" in
    -w|--watch)  WATCH=1 ;;
    -o|--open)   OPEN=1 ;;
    -d|--draft)  DRAFT=1 ;;
    -p|--pretty) PRETTY=1 ;;
    -j)          JOBS="${2:-}"; shift ;;
    --out)       OUT_DIR="${2:-}"; shift ;;
    --all)       ;;
    -h|--help)   sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*)          die "неизвестный ключ: $1" ;;
    *)           IDS+=( "$1" ) ;;
  esac
  shift
done

command -v "$TYPST_BIN" >/dev/null 2>&1 || die "typst не найден, запустите scripts/check.sh"
mkdir -p "$OUT_DIR"
typst_args

LECT="$(list_lectures)" || die "не удалось прочитать реестр из course.typ"

if [ "${#IDS[@]}" -eq 0 ]; then
  while IFS="$(printf '\t')" read -r id n title; do
    [ -n "${id:-}" ] && IDS+=( "$id" )
  done <<EOF
$LECT
EOF
fi

# Название лекции, пригодное для имени файла.
#
# В именах файлов Windows запрещены : " < > | * ? \ /. Двоеточие в названии
# («Язык Takt: описание автоматов и порождение кода») делает файл несохранимым
# на NTFS, а GitHub такой артефакт и вовсе не принимает — на этом падала
# выкладка PDF в CI. Двоеточие заменяется на тире (курс раздаётся студентам,
# читаемость имени важнее буквальности), остальные запрещённые знаки — на
# дефис.
safe_title() {
  local t="$1" ch
  t="${t//: / — }"
  t="${t//:/ —}"
  for ch in '"' '<' '>' '|' '*' '?' '\' '/'; do
    t="${t//"$ch"/-}"
  done
  printf '%s' "$t"
}

# Читаемое имя PDF по id.
pretty_name() {
  local want="$1"
  while IFS="$(printf '\t')" read -r id n title; do
    if [ "$id" = "$want" ]; then
      printf '%02d — %s.pdf' "$n" "$(safe_title "$title")"
      return
    fi
  done <<EOF
$LECT
EOF
  printf '%s.pdf' "$want"
}

if [ "$WATCH" = 1 ]; then
  [ "${#IDS[@]}" -eq 1 ] || die "режим --watch работает с одной лекцией: build.sh -w 03-synthesis"
  id="${IDS[0]}"
  extra=()
  [ "$DRAFT" = 1 ] && extra=( --input draft=1 )
  info "watch: $id (Ctrl+C для выхода)"
  exec "$TYPST_BIN" watch "${TYPST_ARGS[@]}" ${extra[@]+"${extra[@]}"} \
    "$SRC_DIR/$id/main.typ" "$OUT_DIR/$id.pdf"
fi

[ -z "$JOBS" ] && JOBS="$(ncpu)"

# Каждый вызов typst однопоточен по документу, поэтому параллелим по документам.
FAILED=0
export OUT_DIR
printf '%s\0' "${IDS[@]}" | xargs -0 -P "$JOBS" -I{} \
  bash "$0" --build-one {} "$DRAFT" || FAILED=1

if [ "$PRETTY" = 1 ]; then
  for id in "${IDS[@]}"; do
    [ -f "$OUT_DIR/$id.pdf" ] && cp -f "$OUT_DIR/$id.pdf" "$OUT_DIR/$(pretty_name "$id")"
  done
  ok "копии под читаемыми именами: ${OUT_DIR#"$ROOT/"}"
fi

if [ "$OPEN" = 1 ]; then
  for id in "${IDS[@]}"; do
    [ -f "$OUT_DIR/$id.pdf" ] || continue
    if command -v open >/dev/null 2>&1; then open "$OUT_DIR/$id.pdf"
    elif command -v xdg-open >/dev/null 2>&1; then xdg-open "$OUT_DIR/$id.pdf"
    fi
  done
fi

echo
ok "собрано PDF в ${OUT_DIR#"$ROOT/"}: $(ls -1 "$OUT_DIR"/*.pdf 2>/dev/null | wc -l | tr -d ' ')"
[ "$FAILED" = 0 ] || die "часть лекций не собралась"
