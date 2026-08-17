#!/usr/bin/env bash
# scripts/build-docs.sh [--strict]
#
# Документация практикума по комментариям Doxygen (ТД-4). Запускается целью
# CMake `docs`, в CI и вручную:
#
#   ./scripts/build-docs.sh            собрать в build/docs/html
#   ./scripts/build-docs.sh --strict   считать отсутствие doxygen ошибкой
#
# По умолчанию отсутствие doxygen — предупреждение, а не ошибка: инструмент
# внешний, а сборка курса не должна от него зависеть. В CI, где документация
# и проверяется, стоит --strict.
#
# Версия документации берётся из lectures/course.typ — той же строки, что
# печатается на титульных листах лекций. Без typst версия остаётся пустой:
# документация от этого не ломается.

set -uo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
STRICT=0

while [ $# -gt 0 ]; do
  case "$1" in
    --strict) STRICT=1 ;;
    -h|--help) sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "неизвестный аргумент: $1" >&2; exit 1 ;;
  esac
  shift
done

if command -v tput >/dev/null 2>&1 && [ -t 1 ]; then
  C_RED="$(tput setaf 1)"; C_GRN="$(tput setaf 2)"; C_YEL="$(tput setaf 3)"; C_OFF="$(tput sgr0)"
else
  C_RED=""; C_GRN=""; C_YEL=""; C_OFF=""
fi
ok()   { printf '%s  ok  %s%s\n' "$C_GRN" "$C_OFF" "$*"; }
warn() { printf '%s warn %s%s\n' "$C_YEL" "$C_OFF" "$*" >&2; }
bad()  { printf '%s FAIL %s%s\n' "$C_RED" "$C_OFF" "$*" >&2; }

DOXYGEN="${DOXYGEN:-doxygen}"
if ! command -v "$DOXYGEN" >/dev/null 2>&1; then
  warn "doxygen не установлен — документация не собрана"
  warn "  macOS: brew install doxygen, Debian/Ubuntu: apt install doxygen"
  [ "$STRICT" = 1 ] && { bad "нужен doxygen"; exit 1; }
  exit 0
fi

STATECRAFT_VERSION=""
if command -v typst >/dev/null 2>&1; then
  STATECRAFT_VERSION="$(cd "$ROOT/lectures" && ./scripts/course.sh --version 2>/dev/null)" || true
fi
export STATECRAFT_VERSION

cd "$ROOT" || exit 1
# Каталог создаётся заранее: doxygen 1.9 не заводит вложенный путь, если
# промежуточного каталога нет, и падает с «Output directory does not exist».
# Локально каталог обычно уже создан сборкой, поэтому дефект вылез в CI.
mkdir -p build/docs || exit 1

if ! "$DOXYGEN" Doxyfile; then
  bad "doxygen: документация не собрана"
  exit 1
fi

ok "документация: build/docs/html/index.html${STATECRAFT_VERSION:+ (версия $STATECRAFT_VERSION)}"
