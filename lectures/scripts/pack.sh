#!/usr/bin/env bash
# lectures/scripts/pack.sh
#
# Комплект курса для раздачи: PDF всех лекций под читаемыми именами, сложенные
# в один архив `statecraft-lectures-<версия>.zip`. Тем же комплектом
# публикуется релиз (ТД-3), поэтому собирается он одним кодом и вручную, и в
# GitHub Actions.
#
# Каталог результата задаётся переменной OUT_DIR (в Makefile — OUT=<каталог>).

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

command -v "$TYPST_BIN" >/dev/null 2>&1 || die "typst не найден, запустите scripts/check.sh"

VERSION="$(course_version)" || die "не удалось прочитать версию из course.typ"
[ -n "$VERSION" ] || die "поле version в course.typ пусто"

# Сборка идёт в отдельный каталог, а не в общий OUT_DIR: там могут лежать
# PDF от прежних прогонов — черновики, лекции под старыми названиями, — и они
# попали бы в комплект наравне со свежими.
BUILD_DIR="$OUT_DIR/pack"
rm -rf "$BUILD_DIR"
"$SCRIPT_DIR/build.sh" --pretty --out "$BUILD_DIR" || die "лекции не собрались, комплект не готовим"

DIST_ROOT="$OUT_DIR/dist"
DIST="$DIST_ROOT/Лекции АПСУ $VERSION"
rm -rf "$DIST_ROOT"
mkdir -p "$DIST"

# В каталоге сборки два набора: `08-statecharts.pdf` для сборочных нужд и
# «08 — Иерархические автоматы и кодогенерация.pdf» для людей. В комплект идёт
# второй; отличить их можно по разделителю в имени.
count=0
for pdf in "$BUILD_DIR"/*.pdf; do
  [ -f "$pdf" ] || continue
  case "$(basename "$pdf")" in
    *" — "*) cp "$pdf" "$DIST/"; count=$((count + 1)) ;;
  esac
done
[ "$count" -gt 0 ] || die "в $BUILD_DIR нет PDF под читаемыми именами"

ARCHIVE="$OUT_DIR/statecraft-lectures-$VERSION.zip"
rm -f "$ARCHIVE"

# Упаковка python, а не утилитой zip. Причина одна: имена файлов русские, а
# Info-ZIP помечает их кодировкой CP437, из-за чего «Проводник» Windows
# показывает вместо названий мусор. Модуль zipfile ставит для не-ASCII имён
# флаг UTF-8, и архив читается везде одинаково. Заодно не приходится гадать,
# какие ключи поддерживает zip конкретной системы: у Apple он свой.
python3 - "$ARCHIVE" "$DIST_ROOT" <<'PY' || die "архив не собран"
import os
import sys
import zipfile

archive, root = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zf:
    for dirpath, dirnames, names in os.walk(root):
        dirnames.sort()
        for name in sorted(names):
            full = os.path.join(dirpath, name)
            zf.write(full, os.path.relpath(full, root))
PY

ok "комплект: ${ARCHIVE#"$ROOT/"} ($count PDF, версия $VERSION)"
