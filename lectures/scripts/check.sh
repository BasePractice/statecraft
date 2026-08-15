#!/usr/bin/env bash
# lectures/scripts/check.sh [--fix]
#
# Проверяет всё, что нужно для сборки лекций:
#   1. typst нужной версии;
#   2. шрифты (с --fix — скачивает недостающие);
#   3. структуру каталога;
#   4. что у каждой лекции из реестра course.typ есть main.typ;
#   5. что все пути в img(...), image(...) и code-file(...) существуют;
#   6. что @preview-пакеты либо не нужны, либо доступны;
#   7. что шаблон реально компилируется.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

FIX=0
[ "${1:-}" = "--fix" ] && FIX=1

FAILFILE="$(mktemp "${TMPDIR:-/tmp}/lectures-check.XXXXXX")"
trap 'rm -f "$FAILFILE"' EXIT
fail() { printf '%s FAIL %s%s\n' "$C_RED" "$C_OFF" "$*" >&2; echo x >>"$FAILFILE"; }

echo "== 1. typst =="
command -v "$TYPST_BIN" >/dev/null 2>&1 \
  || die "typst не найден. Установка: brew install typst (macOS) или https://github.com/typst/typst"
TV="$("$TYPST_BIN" --version | awk '{print $2}')"
ok "typst $TV ($(command -v "$TYPST_BIN"))"
case "$TV" in
  0.1[5-9]*|0.[2-9][0-9]*|[1-9]*) ;;
  *) fail "нужен typst >= 0.15: шаблон использует typst eval и first-line-indent: (amount: ..)" ;;
esac

echo
echo "== 2. шрифты =="
mkdir -p "$FONT_DIR"
need_fonts=0
for f in "Fira Code" "PT Serif" "PT Sans"; do
  if "$TYPST_BIN" fonts --font-path "$FONT_DIR" | grep -qxF "$f"; then
    ok "$f найден"
  else
    warn "$f не найден"
    need_fonts=1
  fi
done
if [ "$need_fonts" = 1 ]; then
  if [ "$FIX" = 1 ]; then
    "$SCRIPT_DIR/fetch-fonts.sh"
  else
    warn "недостающие шрифты ставятся так: scripts/fetch-fonts.sh (или check.sh --fix)"
  fi
fi
if "$TYPST_BIN" fonts --ignore-system-fonts | grep -qx 'Libertinus Serif'; then
  ok "встроенный запасной шрифт Libertinus Serif доступен"
else
  fail "у typst нет встроенных шрифтов — сборка невозможна"
fi

echo
echo "== 3. структура =="
for f in course.typ template/lecture.typ bib/references.bib \
         shared/notation.typ shared/glossary.typ shared/questions.typ shared/tasks.typ; do
  if [ -f "$ROOT/$f" ]; then ok "$f"; else fail "нет $f"; fi
done
for d in template shared bib src scripts; do
  [ -d "$ROOT/$d" ] || fail "нет каталога $d/"
done

echo
echo "== 4. лекции из course.typ =="
LECT="$(list_lectures)" || die "не удалось прочитать course.typ — проверьте синтаксис"
while IFS="$(printf '\t')" read -r id n title; do
  [ -z "${id:-}" ] && continue
  if [ -f "$SRC_DIR/$id/main.typ" ]; then
    ok "$(printf '%2s' "$n")  $id — $title"
  else
    fail "$(printf '%2s' "$n")  $id — нет src/$id/main.typ (болванка: src/_template/)"
  fi
done <<EOF
$LECT
EOF

echo
echo "== 5. пути к картинкам и коду =="
python3 - "$ROOT" >"$FAILFILE.paths" <<'PY' || true
import os, re, sys
root = sys.argv[1]
pat = re.compile(r'(?:image|code-file|img)\(\s*"([^"]+)"')
for base in ("src", "template", "shared"):
    for dirpath, _, files in os.walk(os.path.join(root, base)):
        for fn in files:
            if not fn.endswith(".typ"):
                continue
            path = os.path.join(dirpath, fn)
            try:
                text = open(path, encoding="utf-8").read()
            except OSError:
                continue
            for line in text.splitlines():
                stripped = line.lstrip()
                if stripped.startswith("//"):
                    continue
                for m in pat.finditer(line):
                    p = m.group(1)
                    full = os.path.join(root, p.lstrip("/")) if p.startswith("/") \
                        else os.path.join(dirpath, p)
                    if not os.path.exists(full):
                        print("%s|%s" % (os.path.relpath(path, root), p))
PY
if [ -s "$FAILFILE.paths" ]; then
  while IFS='|' read -r f p; do
    [ -n "${f:-}" ] && fail "$f: нет файла «$p»"
  done <"$FAILFILE.paths"
else
  ok "битых путей нет"
fi
rm -f "$FAILFILE.paths"

echo
echo "== 6. пакеты @preview =="
if grep -rqs '@preview/' "$ROOT/template" "$ROOT/src" 2>/dev/null; then
  warn "используются внешние пакеты:"
  grep -rhoE '@preview/[a-z0-9-]+:[0-9.]+' "$ROOT/template" "$ROOT/src" | sort -u | sed 's/^/       /'
  if [ -d "$PKG_CACHE/preview" ]; then
    ok "локальный кэш .typst-packages — сборка работает офлайн"
  elif curl -fsS -o /dev/null --max-time 8 https://packages.typst.org/preview/index.json 2>/dev/null; then
    ok "сеть до packages.typst.org есть, первая сборка скачает пакеты"
    [ "$FIX" = 1 ] && "$SCRIPT_DIR/vendor-packages.sh"
  else
    fail "ни кэша, ни сети: сборка с @preview невозможна, см. scripts/vendor-packages.sh"
  fi
else
  ok "базовый шаблон не зависит от @preview и собирается офлайн"
fi

echo
echo "== 7. пробная компиляция =="
typst_args
if [ -f "$SRC_DIR/_template/main.typ" ]; then
  if "$TYPST_BIN" compile "${TYPST_ARGS[@]}" -f pdf \
       "$SRC_DIR/_template/main.typ" /dev/null 2>"$FAILFILE.log"; then
    ok "шаблон компилируется"
  else
    fail "шаблон НЕ компилируется:"
    sed 's/^/       /' "$FAILFILE.log" >&2
  fi
  rm -f "$FAILFILE.log"
else
  fail "нет src/_template/main.typ"
fi

echo
if [ -s "$FAILFILE" ]; then
  die "проверка не пройдена: $(wc -l <"$FAILFILE" | tr -d ' ') замечаний"
else
  ok "окружение готово, можно собирать: scripts/build.sh"
fi
