#!/usr/bin/env bash
# lectures/scripts/course.sh [--version | --check ТЕГ | --lectures | --appendices]
#
# Сведения о курсе из course.typ — единственного источника правды. Скрипт
# нужен там, где эти сведения читает не typst, а сборка: проверка тега при
# публикации релиза и заметки к нему (ТД-3).
#
#   --version    напечатать версию курса (например, 1.0.31); режим по умолчанию
#   --check ТЕГ  сверить версию с тегом релиза вида v{MAJOR}.{MINOR}.{BUILD}
#                (префикс v необязателен)
#   --lectures   напечатать реестр лекций: «номер<TAB>название»
#   --appendices напечатать реестр приложений курса: «id<TAB>название»
#   --labs       напечатать реестр лабораторных работ: «номер<TAB>название»
#
# Версия печатается на титульном листе каждой лекции, поэтому её смена
# означает переиздание всего комплекта, а расхождение с тегом — что выложены
# PDF не той версии, которую обещает релиз.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

MODE="version"
CHECK=""
while [ $# -gt 0 ]; do
  case "$1" in
    --version)  MODE="version" ;;
    --lectures) MODE="lectures" ;;
    --appendices) MODE="appendices" ;;
    --labs)     MODE="labs" ;;
    --check)    MODE="check"; CHECK="${2:-}"; shift ;;
    -h|--help)  sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) die "неизвестный аргумент: $1" ;;
  esac
  shift
done

command -v "$TYPST_BIN" >/dev/null 2>&1 || die "typst не найден, запустите scripts/check.sh"

if [ "$MODE" = "lectures" ]; then
  list_lectures | cut -f2,3
  exit 0
fi

if [ "$MODE" = "appendices" ]; then
  list_appendices
  exit 0
fi

if [ "$MODE" = "labs" ]; then
  list_labs | cut -f2,3
  exit 0
fi

VERSION="$(course_version)" || die "не удалось прочитать версию из course.typ"
[ -n "$VERSION" ] || die "поле version в course.typ пусто"

if [ "$MODE" = "version" ]; then
  printf '%s\n' "$VERSION"
  exit 0
fi

[ -n "$CHECK" ] || die "ключ --check требует тега"

TAG="${CHECK#v}"
if ! [[ "$TAG" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  die "тег «$CHECK» не имеет вида v{MAJOR}.{MINOR}.{BUILD}"
fi

if [ "$TAG" != "$VERSION" ]; then
  die "тег «$CHECK» не совпадает с версией курса «$VERSION» (lectures/course.typ)"
fi

ok "версия курса $VERSION совпадает с тегом $CHECK"
