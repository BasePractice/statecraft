#!/usr/bin/env bash
# scripts/check-promela.sh [--verify] [файл ...]
#
# Проверка моделей Promela из practices/09-promela и модели сварочной линии,
# порождаемой практикой 20-welding-line.
#
#   без аргументов   разобрать все модели: spin -a должен построить
#                    верификатор без ошибок
#   --verify         сверх разбора собрать pan и прогнать проверку;
#                    ожидаемый исход модель задаёт сама строкой
#                    «verify: ok|violation|skip» в шапке — часть примеров
#                    курса демонстрирует именно нарушение
#   файл ...         проверить только указанные модели
#
# SPIN — внешний инструмент. Если его нет в системе, скрипт печатает
# предупреждение и завершается успешно: сборка курса не должна зависеть от
# наличия чужого пакета (так же ведут себя проверки clang-format и taktc).
#
#   macOS:          brew install spin
#   Debian/Ubuntu:  apt install spin
#   Windows:        сборка лежит в practices/resources/spin/windows/

set -uo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
MODELS_DIR="$ROOT/practices/09-promela"
VERIFY=0
FILES=()

while [ $# -gt 0 ]; do
  case "$1" in
    --verify) VERIFY=1 ;;
    -h|--help) sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) FILES+=( "$1" ) ;;
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

SPIN_BIN="${SPIN:-}"
if [ -z "$SPIN_BIN" ] && command -v spin >/dev/null 2>&1; then
  SPIN_BIN="spin"
fi

if [ -z "$SPIN_BIN" ]; then
  warn "spin не установлен — модели Promela не проверялись"
  warn "  macOS: brew install spin, Debian/Ubuntu: apt install spin"
  exit 0
fi

if [ "${#FILES[@]}" -eq 0 ]; then
  while IFS= read -r f; do
    FILES+=( "$f" )
  done < <(find "$MODELS_DIR" -name '*.pml' -type f | sort)
fi

if [ "${#FILES[@]}" -eq 0 ]; then
  warn "моделей не найдено в $MODELS_DIR"
  exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "== моделей на проверке: ${#FILES[@]} ($("$SPIN_BIN" -V 2>&1 | head -1))"
ERRORS=0

for model in "${FILES[@]}"; do
  rel="${model#$ROOT/}"
  name="$(basename "$model")"

  cp "$model" "$WORK/$name"
  if ! (cd "$WORK" && "$SPIN_BIN" -a "$name" >"$WORK/spin.log" 2>&1); then
    bad "$rel: spin -a не построил верификатор"
    sed 's/^/       /' "$WORK/spin.log" >&2
    ERRORS=$((ERRORS + 1))
    continue
  fi

  if [ "$VERIFY" = 0 ]; then
    ok "$rel"
    continue
  fi

  # Модель с LTL-свойствами проверяется иначе: -DSAFETY отключает поиск
  # ациклических нарушений, а именно им проверяется живость. Такие модели
  # собираются без -DSAFETY и прогоняются по одному свойству за запуск
  # (`pan -N имя`), с предположением о слабой справедливости (`-f`) —
  # без него любое свойство живости нарушается тривиально: процесс просто
  # никогда не выполняется.
  LTL_NAMES="$(grep -oE '^[[:space:]]*ltl[[:space:]]+[A-Za-z_][A-Za-z_0-9]*' "$model" \
    | awk '{print $2}' || true)"

  cc_flags="-DSAFETY"
  [ -n "$LTL_NAMES" ] && cc_flags=""

  if ! (cd "$WORK" && cc $cc_flags -o pan pan.c >"$WORK/cc.log" 2>&1); then
    bad "$rel: не собрался верификатор pan"
    sed 's/^/       /' "$WORK/cc.log" >&2
    ERRORS=$((ERRORS + 1))
    continue
  fi
  # Ожидаемый исход задаёт сама модель строкой «verify: ok|violation|skip»
  # в шапке. Часть примеров курса демонстрирует именно нарушение — для них
  # ошибкой является его отсутствие.
  expect="ok"
  case "$(head -5 "$model" | grep -oE 'verify: *(ok|violation|skip)' | head -1)" in
    *violation) expect="violation" ;;
    *skip)      expect="skip" ;;
  esac

  if [ "$expect" = "skip" ]; then
    ok "$rel — верификация пропущена по пометке в модели"
    continue
  fi

  if [ -n "$LTL_NAMES" ]; then
    found="ok"
    for claim in $LTL_NAMES; do
      (cd "$WORK" && ./pan -a -f -N "$claim" >"$WORK/pan.log" 2>&1) || true
      if grep -qiE "errors: [1-9]" "$WORK/pan.log"; then
        found="violation"
        warn "$rel: свойство «$claim» нарушено"
        break
      fi
      ok "$rel — свойство «$claim» выполняется"
    done
    # Итоговую строку печатать не нужно: по свойству на строку уже напечатано.
    if [ "$found" = "$expect" ]; then
      continue
    fi
  else
    (cd "$WORK" && ./pan >"$WORK/pan.log" 2>&1) || true
    if grep -qiE "errors: [1-9]" "$WORK/pan.log"; then
      found="violation"
    else
      found="ok"
    fi
  fi

  if [ "$found" = "$expect" ]; then
    if [ "$expect" = "violation" ]; then
      ok "$rel — нарушение найдено, как и заявлено моделью"
    else
      ok "$rel — нарушений не найдено"
    fi
  elif [ "$expect" = "violation" ]; then
    bad "$rel: модель заявляет нарушение, но верификатор его не нашёл"
    ERRORS=$((ERRORS + 1))
  else
    bad "$rel: верификатор нашёл нарушение"
    grep -iE "errors:|assertion|invalid|pan:1" "$WORK/pan.log" | sed 's/^/       /' >&2
    ERRORS=$((ERRORS + 1))
  fi
  rm -f "$WORK/pan" "$WORK/pan."*
done

echo
if [ "$ERRORS" -gt 0 ]; then
  bad "моделей с ошибками: $ERRORS"
  exit 1
fi
ok "все модели Promela разбираются"
