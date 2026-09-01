#!/usr/bin/env bash
# lectures/scripts/fetch-fonts.sh [--force]
#
# Кладёт Fira Code, PT Serif, PT Sans и osifont (чертёжный шрифт по
# ГОСТ 2.304-81 / ISO 3098 — им набираются подписи в диаграммах) в
# lectures/fonts/.
# Идемпотентно, без sudo, ничего не ставит в систему.
#
# Если сети нет, скрипт не роняет сборку: в шаблоне прописан fallback на
# встроенный в typst Libertinus Serif, который полностью покрывает кириллицу.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

FORCE=0
[ "${1:-}" = "--force" ] && FORCE=1

FIRA_URL="https://github.com/tonsky/FiraCode/releases/download/6.2/Fira_Code_v6.2.zip"
FIRA_SHA="0949915ba8eb24d89fd93d10a7ff623f42830d7c5ffc3ecbf960e4ecad3e3e79"

# osifont — свободная (LGPL3 + font exception) реализация чертёжного шрифта
# ISO 3098, совместимого с ГОСТ 2.304-81. Ветка master изменяема, поэтому
# архив берётся по конкретному коммиту.
OSIFONT_REF="2e9aa86a8a09b044e08c00f5a4a2505dc3ca9f6e"
OSIFONT_URL="https://github.com/hikikomori82/osifont/archive/${OSIFONT_REF}.zip"
OSIFONT_SHA="a3724c58ab614e1b9ec8d74895d28559d8a20e4620ad5d66bdb644bda1534a4c"

# google/fonts: ветка main изменяема, поэтому пин на конкретный коммит.
GF_REF="80327115aa6e63ea8947558e5fb676f5287878ba"
GF_BASE="https://raw.githubusercontent.com/google/fonts/${GF_REF}/ofl"

PT_FILES="
ptserif/PT_Serif-Web-Regular.ttf
ptserif/PT_Serif-Web-Bold.ttf
ptserif/PT_Serif-Web-Italic.ttf
ptserif/PT_Serif-Web-BoldItalic.ttf
ptsans/PT_Sans-Web-Regular.ttf
ptsans/PT_Sans-Web-Bold.ttf
ptsans/PT_Sans-Web-Italic.ttf
ptsans/PT_Sans-Web-BoldItalic.ttf
"

sha256_of() {
  if command -v shasum >/dev/null 2>&1; then shasum -a 256 "$1" | cut -d' ' -f1
  else sha256sum "$1" | cut -d' ' -f1; fi
}

mkdir -p "$FONT_DIR"

# ---------------------------- Fira Code ------------------------------------
if [ "$FORCE" = 0 ] && ls "$FONT_DIR"/FiraCode-Regular.ttf >/dev/null 2>&1; then
  ok "Fira Code уже в fonts/"
else
  info "Скачиваю Fira Code 6.2 (~2.4 МБ)…"
  tmp="$(mktemp -d "${TMPDIR:-/tmp}/firacode.XXXXXX")"
  trap 'rm -rf "$tmp"' EXIT
  if curl -fsSL --retry 3 --connect-timeout 15 -o "$tmp/fira.zip" "$FIRA_URL"; then
    got="$(sha256_of "$tmp/fira.zip")"
    if [ "$got" != "$FIRA_SHA" ]; then
      die "sha256 архива Fira Code не совпал: $got, ожидался $FIRA_SHA"
    fi
    # Берём статические начертания: variable-версия имеет Default weight 300 (Light).
    unzip -o -j -q "$tmp/fira.zip" 'ttf/*.ttf' -d "$FONT_DIR" \
      || die "не удалось распаковать ttf/*.ttf из архива Fira Code"
    ok "Fira Code установлен в fonts/"
  else
    warn "не удалось скачать Fira Code ($FIRA_URL)."
    warn "Положите файлы вручную в $FONT_DIR или установите шрифт в систему."
  fi
  rm -rf "$tmp"; trap - EXIT
fi

# ------------------------------ osifont -------------------------------------
# Чертёжный шрифт для подписей в диаграммах (ГОСТ 2.304-81 / ISO 3098).
if [ "$FORCE" = 0 ] && ls "$FONT_DIR"/osifont-lgpl3fe.ttf >/dev/null 2>&1; then
  ok "osifont уже в fonts/"
else
  info "Скачиваю osifont (~1 МБ)…"
  tmp="$(mktemp -d "${TMPDIR:-/tmp}/osifont.XXXXXX")"
  trap 'rm -rf "$tmp"' EXIT
  if curl -fsSL --retry 3 --connect-timeout 15 -o "$tmp/osifont.zip" "$OSIFONT_URL"; then
    got="$(sha256_of "$tmp/osifont.zip")"
    if [ "$got" != "$OSIFONT_SHA" ]; then
      die "sha256 архива osifont не совпал: $got, ожидался $OSIFONT_SHA"
    fi
    unzip -o -j -q "$tmp/osifont.zip" \
      'osifont-*/osifont-lgpl3fe.ttf' 'osifont-*/osifont-italic.ttf' -d "$FONT_DIR" \
      || die "не удалось распаковать osifont"
    ok "osifont установлен в fonts/"
  else
    warn "не удалось скачать osifont ($OSIFONT_URL)."
    warn "Подписи в диаграммах будут набраны основным шрифтом лекции."
  fi
  rm -rf "$tmp"; trap - EXIT
fi

# ------------------------- PT Serif / PT Sans ------------------------------
# Кириллический основной текст. На macOS обычно уже есть в системе,
# на Linux и в CI — нет.
printf '%s\n' "$PT_FILES" | while read -r rel; do
  [ -z "${rel:-}" ] && continue
  base="$(basename "$rel")"
  dst="$FONT_DIR/$base"
  if [ "$FORCE" = 0 ] && [ -f "$dst" ]; then continue; fi
  info "Скачиваю ${base}…"
  if curl -fsSL --retry 3 --connect-timeout 15 -o "$dst.part" "$GF_BASE/$rel"; then
    mv -f "$dst.part" "$dst"
  else
    rm -f "$dst.part"
    warn "не удалось скачать $base — будет использован встроенный Libertinus Serif"
  fi
done

echo
ok "Шрифты, доступные сборке (fonts/ + встроенные):"
"$TYPST_BIN" fonts --font-path "$FONT_DIR" --ignore-system-fonts \
  | grep -E '^(Fira Code|PT Serif|PT Sans|osifont|Libertinus Serif|New Computer Modern.*)$' \
  | sed 's/^/       /' || true
