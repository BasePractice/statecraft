#!/bin/sh
# Порождение векторных иллюстраций лекции 11.
#
# Картинки лекции считает та же программа, что и автоматы: рисунок не может
# разойтись с моделью, потому что берётся из неё. Файлы кладутся в
# lectures/src/11-cells/images/ и коммитятся — typst их только вставляет.
#
# Использование:
#   practices/11-cells/render-figures.sh [путь к 11-cells] [каталог назначения]
#
# По умолчанию берётся build/practices/11-cells/11-cells; собрать его можно
# обычным способом:
#   cmake -S . -B build && cmake --build build --target 11-cells

set -eu

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${1:-"$ROOT/build/practices/11-cells/11-cells"}
OUT=${2:-"$ROOT/lectures/src/11-cells/images"}

if [ ! -x "$BIN" ]; then
    echo "не найден исполняемый файл практики: $BIN" >&2
    echo "соберите его: cmake -S . -B build && cmake --build build --target 11-cells" >&2
    exit 1
fi
mkdir -p "$OUT"

# --- одномерные автоматы Вольфрама -----------------------------------------
"$BIN" svg rule 90 41 20 "$OUT/wolfram-90.svg"
"$BIN" svg rule 110 41 20 "$OUT/wolfram-110.svg"
"$BIN" svg rule 30 41 20 "$OUT/wolfram-30.svg"

# --- «Жизнь»: натюрморты, осцилляторы, корабли, рост ------------------------
# Кадры выбраны так, чтобы по ленте читалось поведение: у осциллятора — полный
# период, у корабля — сдвиг, у ружья — вылет очередного планера.
"$BIN" svg life block 0,1,2 "$OUT/life-block.svg" 12
"$BIN" svg life beehive 0,1,2 "$OUT/life-beehive.svg" 12
"$BIN" svg life blinker 0,1,2 "$OUT/life-blinker.svg" 12
"$BIN" svg life toad 0,1,2 "$OUT/life-toad.svg" 12
"$BIN" svg life beacon 0,1,2 "$OUT/life-beacon.svg" 14
"$BIN" svg life pulsar 0,1,2,3 "$OUT/life-pulsar.svg" 24
# Пентадекатлон задан рядом из десяти клеток: цикл начинается со второго хода.
"$BIN" svg life pentadecathlon 2,5,8,11,14,17 "$OUT/life-pentadecathlon.svg" 24
"$BIN" svg life glider 0,1,2,3,4 "$OUT/life-glider.svg" 16
"$BIN" svg life lwss 0,2,4 "$OUT/life-lwss.svg" 24
"$BIN" svg life gosper-gun 0,30,60,90 "$OUT/life-gosper-gun.svg" 56
"$BIN" svg life r-pentomino 0,20,50,100 "$OUT/life-r-pentomino.svg" 48
"$BIN" svg life diehard 0,50,100,130 "$OUT/life-diehard.svg" 32
"$BIN" svg life acorn 0,50,100,200 "$OUT/life-acorn.svg" 48

# --- муравей Лэнгтона --------------------------------------------------------
# Беспорядок первых тысяч шагов и внезапное «шоссе» после ~10 000.
"$BIN" svg langton 200,2000,7000,11000 "$OUT/langton-highway.svg" 96

# --- пожиратель встречает планер ----------------------------------------------
"$BIN" svg scene eater-vs-glider 0,4,8,12,16 "$OUT/life-eater-vs-glider.svg"

# --- задача об умном муравье --------------------------------------------------
# По одной ленте на стратегию: видно и то, как она ищет продолжение тропы, и
# то, где именно она встаёт.
"$BIN" svg trail scan 0,80,180,315 "$OUT/trail-scan.svg"
"$BIN" svg trail probe 0,100,300,600 "$OUT/trail-probe.svg"
"$BIN" svg trail lookaround 0,50,150,600 "$OUT/trail-lookaround.svg"
"$BIN" svg trail evolved 0,60,120,181 "$OUT/trail-evolved.svg"

# --- диаграммы автоматов муравья ----------------------------------------------
# Стратегии, придуманные вручную, и автоматы из журнала эволюционного поиска:
# 315 тактов (пять состояний) → 255 → 235 → 181 (семнадцать состояний).
# Ряд показывает, чем поиск платит за такты, — числом состояний.
"$BIN" svg fsm lookaround "$OUT/fsm-lookaround.svg"
"$BIN" svg fsm probe "$OUT/fsm-probe.svg"
"$BIN" svg fsm scan "$OUT/fsm-scan.svg"
"$BIN" svg fsm 3.0.0.1:3.4.0.2:3.0.0.3:3.5.0.4:3.0.2.0:3.1.2.4 "$OUT/fsm-255.svg"
"$BIN" svg fsm 3.0.0.1:3.5.0.2:3.0.0.3:3.5.0.4:3.0.2.0:3.4.2.4 "$OUT/fsm-235.svg"
"$BIN" svg fsm evolved "$OUT/fsm-evolved.svg"

echo "готово: $OUT"
