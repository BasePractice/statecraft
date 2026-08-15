#!/usr/bin/env bash
# lectures/scripts/clean.sh [--all]
#
#   без ключа  чистит out/
#   --all      чистит ещё fonts/ и .typst-packages/ (потребуют повторной загрузки)

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

# Страховка: удаляем только заведомо свои каталоги внутри ROOT.
safe_rm() {
  local d="$1"
  case "$d" in
    "$ROOT"/out|"$ROOT"/fonts|"$ROOT"/.typst-packages) ;;
    *) die "отказываюсь удалять «$d»" ;;
  esac
  if [ -d "$d" ]; then
    rm -rf -- "$d"
    mkdir -p -- "$d"
    ok "очищено: ${d#$ROOT/}/"
  fi
}

safe_rm "$OUT_DIR"

if [ "${1:-}" = "--all" ]; then
  safe_rm "$FONT_DIR"
  safe_rm "$PKG_CACHE"
  warn "шрифты и пакеты удалены: перед сборкой запустите scripts/check.sh --fix"
fi

find "$ROOT" -name '.DS_Store' -delete 2>/dev/null || true
