#!/usr/bin/env bash
# lectures/scripts/sync-code.sh [--check]
#
# Держит листинги лекций и код практикума одним и тем же текстом.
#
# Зачем: typst читает файлы только внутри своего --root (каталога lectures/),
# поэтому код, который показывается в лекции, лежит копией в
# lectures/src/<id>/code/. Копии молча расходятся с практикумом — так уже
# случилось: в лекции 2 показывался delay_fsm.c с типом InsertingEngine из
# совсем другой практики, хотя в самом практикуме тип давно исправлен.
#
# Без ключа: копирует источники поверх копий.
# --check:   ничего не меняет, сравнивает и завершается с ошибкой при
#            расхождении. Этот режим стоит в CI.
#
# Соответствие «источник → копия» задано в scripts/code-map.txt.

. "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/lib.sh"

REPO="$(cd -- "$ROOT/.." && pwd -P)"
MAP="$SCRIPT_DIR/code-map.txt"

CHECK=0
[ "${1:-}" = "--check" ] && CHECK=1

[ -f "$MAP" ] || die "не найден $MAP"

changed=0
missing=0
total=0

while read -r src dst; do
  case "$src" in
  '' | \#*) continue ;;
  esac
  [ -n "$dst" ] || die "в code-map.txt строка без назначения: $src"

  total=$((total + 1))
  if [ ! -f "$REPO/$src" ]; then
    warn "нет источника: $src"
    missing=$((missing + 1))
    continue
  fi

  if [ -f "$REPO/$dst" ] && cmp -s "$REPO/$src" "$REPO/$dst"; then
    continue
  fi

  changed=$((changed + 1))
  if [ "$CHECK" = 1 ]; then
    if [ -f "$REPO/$dst" ]; then
      warn "копия разошлась с источником: $dst"
    else
      warn "копии нет: $dst"
    fi
  else
    mkdir -p "$(dirname -- "$REPO/$dst")"
    cp "$REPO/$src" "$REPO/$dst"
    info "обновлено: $dst"
  fi
done <"$MAP"

if [ "$missing" -gt 0 ]; then
  die "источников не найдено: $missing (проверьте scripts/code-map.txt)"
fi

if [ "$CHECK" = 1 ]; then
  if [ "$changed" -gt 0 ]; then
    die "листинги лекций разошлись с практикумом: $changed из $total. Выполните ./scripts/sync-code.sh"
  fi
  ok "листинги совпадают с практикумом: $total файлов"
else
  if [ "$changed" -gt 0 ]; then
    ok "обновлено копий: $changed из $total"
    warn "листинги с указанием from/to могли поехать — проверьте PDF лекций 8 и 12"
  else
    ok "всё уже синхронно: $total файлов"
  fi
fi
