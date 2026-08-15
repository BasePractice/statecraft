#!/usr/bin/env bash
# lectures/scripts/vendor-packages.sh
#
# Прогревает локальный кэш пакетов @preview в lectures/.typst-packages,
# чтобы сборка работала без сети. Список пакетов собирается прямо из
# исходников, отдельно его вести не нужно.
#
# Базовый шаблон намеренно не зависит ни от одного @preview-пакета: внешние
# импорты допускаются только в template/diagrams.typ (векторные диаграммы
# автоматов). Если diagrams.typ не используется, этот скрипт не нужен.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

PKGS="$(grep -rhoE '@preview/[a-z0-9_-]+:[0-9]+\.[0-9]+\.[0-9]+' \
          "$ROOT/template" "$ROOT/src" "$ROOT/shared" 2>/dev/null | sort -u || true)"

if [ -z "$PKGS" ]; then
  ok "внешние пакеты не используются, прогревать нечего"
  exit 0
fi

mkdir -p "$PKG_CACHE"
info "пакеты, найденные в исходниках:"
printf '%s\n' "$PKGS" | sed 's/^/       /'

# Пробный файл кладём внутрь --root: typst требует, чтобы исходник лежал
# в корне проекта.
tmp="$(mktemp -d "$ROOT/.warm.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT

{
  printf '%s\n' "$PKGS" | while read -r p; do
    [ -z "$p" ] && continue
    printf '#import "%s"\n' "$p"
  done
  printf '#set page(width: 10pt, height: 10pt)\n'
} >"$tmp/warm.typ"

typst_args
if "$TYPST_BIN" compile "${TYPST_ARGS[@]}" -f pdf "$tmp/warm.typ" /dev/null 2>"$tmp/log"; then
  ok "кэш прогрет: $PKG_CACHE"
else
  sed 's/^/       /' "$tmp/log" >&2
  die "не удалось скачать пакеты (нет сети?)"
fi
