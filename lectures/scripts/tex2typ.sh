#!/usr/bin/env bash
# lectures/scripts/tex2typ.sh <исходный.tex> <id-лекции>
#
# Полуавтоматический перенос лекции из LaTeX в typst. Обёртка вокруг
# `pandoc -f latex -t typst` с пред- и постобработкой того, что pandoc теряет.
#
# Пример:
#   scripts/tex2typ.sh "/Users/pastor/github/articles/Конечные Автоматы/index.tex" 03-synthesis
#
# Результат:
#   src/<id>/main.body.typ  — сконвертированное тело (перезаписывается);
#   src/<id>/TODO.md        — список мест, требующих ручной доводки;
#   src/<id>/images/, code/ — скопированные картинки и исходники.
#
# main.typ пишется руками и НЕ перезаписывается, поэтому конвертер можно
# перезапускать, не теряя ручных правок обвязки.
#
# Что pandoc переносит хорошо: заголовки, \cite{K} → @K, сноски, списки,
# tabular, minted, инлайновую математику.
# Что чинит этот скрипт: babel-кавычки <<>>, image() без расширения (typst
# падает «file not found»), theorem/definition с именем, \lstinputlisting,
# \appendix, angle.l → chevron.l, шумные автометки.
# Что остаётся руками: subfloat (pandoc теряет картинки целиком), cases
# (переэкранирование), \includepdf, breqn/dmath.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

[ $# -eq 2 ] || die "использование: tex2typ.sh <исходный.tex> <id-лекции>"
TEX="$1"; ID="$2"
[ -f "$TEX" ] || die "нет файла: $TEX"
command -v pandoc >/dev/null 2>&1 || die "нужен pandoc (brew install pandoc)"

TEXDIR="$(cd -- "$(dirname -- "$TEX")" && pwd -P)"
DST="$SRC_DIR/$ID"
mkdir -p "$DST/images" "$DST/code"

# --- 0. картинки и исходный код -------------------------------------------
copy_tree() {
  local from="$1" to="$2"
  [ -d "$from" ] || return 0
  if command -v rsync >/dev/null 2>&1; then
    rsync -a --exclude '.DS_Store' "$from/" "$to/"
  else
    (cd "$from" && tar cf - .) | (cd "$to" && tar xf -)
    find "$to" -name '.DS_Store' -delete 2>/dev/null || true
  fi
}
copy_tree "$TEXDIR/images" "$DST/images"
copy_tree "$TEXDIR/source" "$DST/code"
ok "скопировано: $(ls -1 "$DST/images" 2>/dev/null | wc -l | tr -d ' ') картинок"

# --- 1. предобработка LaTeX ------------------------------------------------
pre="$(mktemp "${TMPDIR:-/tmp}/tex2typ-pre.XXXXXX")"
post="$(mktemp "${TMPDIR:-/tmp}/tex2typ-body.XXXXXX")"
trap 'rm -f "$pre" "$post"' EXIT

perl -0777 -pe '
  s/\\begin\{(theorem|definition|lemma|example|remark)\}(?:\[([^\]]*)\])?/\n\nTYPSTBLOCKBEGIN|$1|$2|\n\n/g;
  s/\\end\{(theorem|definition|lemma|example|remark)\}/\n\nTYPSTBLOCKEND\n\n/g;
  s/\\lstinputlisting(?:\[[^\]]*\])?\{([^}]*)\}/\n\nTYPSTCODE|$1|\n\n/g;
  s/\\appendix/\n\nTYPSTAPPENDIX\n\n/g;
  s/\\includepdf(?:\[[^\]]*\])?\{([^}]*)\}/\n\nTYPSTINCLUDEPDF|$1|\n\n/g;
  s/\\input\{structure\.tex\}//g;
  s/\\lehead\{.*?\}\n//g;
  s/\\(tableofcontents|listoffigures|listoftables|maketitle)\b//g;
  s/\\pagestyle\{[^}]*\}//g;
  s/\\setcounter\{[^}]*\}\{[^}]*\}//g;
' "$TEX" >"$pre"

# --- 2. pandoc -------------------------------------------------------------
pandoc -f latex -t typst --wrap=preserve "$pre" -o "$post" 2>"$DST/pandoc.log" || {
  warn "pandoc завершился с ошибкой, см. $DST/pandoc.log"
}

# --- 3. постобработка typst ------------------------------------------------
OUT="$DST/main.body.typ"
perl -0777 -pe '
  s/\\<\\</«/g;  s/\\>\\>/»/g;
  s/<<\s*/«/g;   s/\s*>>/»/g;
  s/^<[^>\n]*[а-яё][^>\n]*>\n//gmi;
  s/\bangle\.l\b/chevron.l/g;  s/\bangle\.r\b/chevron.r/g;
  s/\bbracket\.angle\.l\b/chevron.l/g;  s/\bbracket\.angle\.r\b/chevron.r/g;
  s/TYPSTBLOCKBEGIN\|theorem\|(.*?)\|\s*/"#theorem(" . ($1 ne "" ? "name: \"$1\"" : "") . ")[\n"/ge;
  s/TYPSTBLOCKBEGIN\|definition\|(.*?)\|\s*/"#definition(" . ($1 ne "" ? "name: \"$1\"" : "") . ")[\n"/ge;
  s/TYPSTBLOCKBEGIN\|lemma\|(.*?)\|\s*/"#lemma(" . ($1 ne "" ? "name: \"$1\"" : "") . ")[\n"/ge;
  s/TYPSTBLOCKBEGIN\|example\|(.*?)\|\s*/"#example(" . ($1 ne "" ? "name: \"$1\"" : "") . ")[\n"/ge;
  s/TYPSTBLOCKBEGIN\|remark\|(.*?)\|\s*/"#remark(" . ($1 ne "" ? "name: \"$1\"" : "") . ")[\n"/ge;
  s/TYPSTBLOCKEND\s*/]\n/g;
  s{TYPSTCODE\|([^|]*)\|}{"#code-file(\"code/" . (split m|/|, $1)[-1] . "\", lang: \"c\") // TODO: from/to/caption"}ge;
  s/TYPSTAPPENDIX/\n#show: appendix\n/g;
  s/TYPSTINCLUDEPDF\|(.*?)\|/"\/\/ TODO: $1 вставлялся как готовый PDF; перенести отдельной лекцией или разделом"/ge;
' "$post" >"$OUT"

# --- 4. дописать расширения к image("…") -----------------------------------
python3 - "$OUT" "$DST/images" <<'PY'
import os, re, sys
out, imgdir = sys.argv[1], sys.argv[2]
index = {}
if os.path.isdir(imgdir):
    for fn in os.listdir(imgdir):
        stem, ext = os.path.splitext(fn)
        if ext.lower() in ('.png', '.jpg', '.jpeg', '.pdf', '.svg', '.gif'):
            index.setdefault(stem, fn)
            index.setdefault(stem.lower(), fn)
src = open(out, encoding='utf-8').read()

def fix(m):
    p = m.group(1)
    if os.path.splitext(p)[1]:
        return m.group(0)
    base = os.path.basename(p)
    fn = index.get(base) or index.get(base.lower())
    if fn:
        return 'image("images/%s"' % fn
    return m.group(0) + ' /* TODO: файл не найден */'

src = re.sub(r'image\("([^"]+)"', fix, src)
open(out, 'w', encoding='utf-8').write(src)
PY

# --- 5. отчёт о ручной доводке ---------------------------------------------
cnt() { grep -cE "$1" "$TEX" 2>/dev/null || echo 0; }
{
  echo "# Ручная доводка лекции $ID"
  echo
  echo "Источник: \`$TEX\`"
  echo
  echo "## Метки TODO в main.body.typ"
  grep -n 'TODO' "$OUT" 2>/dev/null | sed 's/^/- /' || echo "- нет"
  echo
  echo "## Что было в исходном .tex (проверьте перенос вручную)"
  echo "- subfloat (pandoc теряет картинки целиком): $(cnt '\\subfloat')"
  echo "- minted-блоков: $(cnt '\\begin\{minted\}')"
  echo "- dmath/breqn: $(cnt '\\begin\{dmath')"
  echo "- cases (проверьте экранирование): $(cnt '\\begin\{cases\}')"
  echo "- multline/multiline: $(cnt '\\begin\{multl?ine')"
  echo "- includepdf: $(cnt '\\includepdf')"
  echo "- lstinputlisting: $(cnt '\\lstinputlisting')"
  echo
  echo "## Обязательно сверить с REPORT.md и SOURCE-REPORT.md"
  echo "- содержательные ошибки Приоритета 1 (К-1 … К-6, С-1 … С-13);"
  echo "- пустые разделы: оформить скелетом с #todo[...];"
  echo "- нотацию привести к shared/notation.typ."
} >"$DST/TODO.md"

ok "готово: ${OUT#$ROOT/}"
info "см. ${DST#$ROOT/}/TODO.md и ${DST#$ROOT/}/pandoc.log"
