#!/usr/bin/env bash
# lectures/scripts/lib.sh — общие определения. Подключается из остальных скриптов.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"

SRC_DIR="$ROOT/src"
# Сводный том: все лекции и приложения одним PDF (lectures/book.typ). Это не
# лекция и не приложение, поэтому в реестрах course.typ его нет — только здесь.
BOOK_ID="${BOOK_ID:-book}"
BOOK_SRC="$ROOT/book.typ"
# Каталог результата переопределяется извне: так сборка из CMake кладёт PDF в
# build/lectures, а ручная — в lectures/out, и обе идут по одному коду.
OUT_DIR="${OUT_DIR:-$ROOT/out}"
FONT_DIR="$ROOT/fonts"
PKG_CACHE="$ROOT/.typst-packages"

TYPST_BIN="${TYPST_BIN:-typst}"
# STRICT_FONTS=1 добавляет --ignore-system-fonts: сборка использует только
# lectures/fonts/ и встроенные шрифты typst, то есть даёт одинаковый PDF
# на любой машине. Имеет смысл только когда fonts/ наполнен.
STRICT_FONTS="${STRICT_FONTS:-0}"

if command -v tput >/dev/null 2>&1 && [ -t 1 ]; then
  C_RED="$(tput setaf 1)"; C_GRN="$(tput setaf 2)"; C_YEL="$(tput setaf 3)"
  C_BLU="$(tput setaf 4)"; C_OFF="$(tput sgr0)"
else
  C_RED=""; C_GRN=""; C_YEL=""; C_BLU=""; C_OFF=""
fi

ok()   { printf '%s  ok  %s%s\n' "$C_GRN" "$C_OFF" "$*"; }
warn() { printf '%s warn %s%s\n' "$C_YEL" "$C_OFF" "$*" >&2; }
info() { printf '%s  ..  %s%s\n' "$C_BLU" "$C_OFF" "$*"; }
die()  { printf '%s FAIL %s%s\n' "$C_RED" "$C_OFF" "$*" >&2; exit 1; }

ncpu() {
  if command -v nproc >/dev/null 2>&1; then nproc
  elif command -v sysctl >/dev/null 2>&1; then sysctl -n hw.ncpu
  else echo 4; fi
}

# Заполняет глобальный массив TYPST_ARGS общими аргументами typst.
typst_args() {
  TYPST_ARGS=( --root "$ROOT" )
  if [ -d "$FONT_DIR" ]; then
    TYPST_ARGS+=( --font-path "$FONT_DIR" )
    if [ "$STRICT_FONTS" = "1" ]; then
      TYPST_ARGS+=( --ignore-system-fonts )
    fi
  fi
  if [ -d "$PKG_CACHE" ]; then
    TYPST_ARGS+=( --package-cache-path "$PKG_CACHE" )
  fi
}

# Реестр лекций из course.typ — единственный источник правды.
# Печатает строки вида "id<TAB>номер<TAB>название".
list_lectures() {
  typst_args
  "$TYPST_BIN" eval "${TYPST_ARGS[@]}" --format json '{
    import "/course.typ": lectures
    lectures.map(l => l.id + "\t" + str(l.n) + "\t" + l.title).join("\n")
  }' | python3 -c 'import json,sys; sys.stdout.write(json.load(sys.stdin) + "\n")'
}

# Реестр приложений курса из course.typ. Приложения — не лекции: у них нет
# номера, но собираются они тем же способом и попадают в тот же комплект.
# Печатает строки вида "id<TAB>название".
list_appendices() {
  typst_args
  "$TYPST_BIN" eval "${TYPST_ARGS[@]}" --format json '{
    import "/course.typ": appendices
    appendices.map(a => a.id + "\t" + a.title).join("\n")
  }' | python3 -c 'import json,sys; sys.stdout.write(json.load(sys.stdin) + "\n")'
}

# Реестр лабораторных работ из course.typ. Работа — не лекция и не приложение:
# у неё есть номер и лекция, к которой она относится.
# Печатает строки вида "id<TAB>номер<TAB>название".
list_labs() {
  typst_args
  "$TYPST_BIN" eval "${TYPST_ARGS[@]}" --format json '{
    import "/course.typ": labs
    labs.map(l => l.id + "\t" + str(l.n) + "\t" + l.title).join("\n")
  }' | python3 -c 'import json,sys; sys.stdout.write(json.load(sys.stdin) + "\n")'
}

# Название дисциплины из course.typ: им подписывается сводный том.
course_discipline() {
  typst_args
  "$TYPST_BIN" eval "${TYPST_ARGS[@]}" --format json '{
    import "/course.typ": course
    course.discipline
  }' | python3 -c 'import json,sys; sys.stdout.write(json.load(sys.stdin) + "\n")'
}

# Проверка, что id есть в реестре.
lecture_exists() {
  list_lectures | cut -f1 | grep -qxF -- "$1"
}

# Версия курса из course.typ. Она же версия релиза: тег v{MAJOR}.{MINOR}.{BUILD}
# обязан совпадать с ней (ТД-3), и это проверяется при публикации.
course_version() {
  typst_args
  "$TYPST_BIN" eval "${TYPST_ARGS[@]}" --format json '{
    import "/course.typ": course
    course.version
  }' | python3 -c 'import json,sys; sys.stdout.write(json.load(sys.stdin) + "\n")'
}
