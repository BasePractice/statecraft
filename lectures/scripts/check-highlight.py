#!/usr/bin/env python3
"""Проверка подсветки листингов в собранных PDF.

Смотрит не в исходники, а в результат: находит плашки листингов (заливка
palette.code-bg) и считает, сколько цветов текста внутри. Плашка, где нет
ничего, кроме основного цвета и серых номеров строк, — листинг без подсветки.

Так ловится то, чего не видно по исходнику: язык указан, но typst его не
знает; язык не указан вовсе (например, у вставок #raw(...)); свой синтаксис
из syntaxes/ перестал применяться.

Часть листингов одноцветна законно — вывод программ, поля клеточных
автоматов, ленты машины Тьюринга. Они помечены языком «text», и скрипт
печатает их отдельным списком, а ошибкой не считает: отличить вывод от кода
может только человек, и это решение записано в самом документе языком блока.

    ./scripts/check-highlight.py out/*.pdf

Требует pymupdf. Без него проверка мягко пропускается: это инструмент
разработчика, а не условие сборки.
"""

import sys
import glob

CODE_BG = 248 / 255                  # palette.code-bg = luma(248)
NEUTRAL = {0x000000, 0x1a1a1a,       # palette.ink
           0x9b9b9b}                 # номера строк, luma(155)
MIN_LINES = 2                        # плашки короче — врезки, а не листинги


def plates(page, pymupdf):
    """Строки текста, лежащие на плашках листингов, сгруппированные по плашкам."""
    rects = [d["rect"] for d in page.get_drawings()
             if d.get("fill") and len(d["fill"]) == 3
             and all(abs(c - CODE_BG) < 0.003 for c in d["fill"])]
    out = []
    for rc in rects:
        lines = []
        for block in page.get_text("dict")["blocks"]:
            for line in block.get("lines", []):
                if pymupdf.Rect(line["bbox"]).intersects(rc):
                    text = "".join(s["text"] for s in line.get("spans", []))
                    if text.strip():
                        lines.append((text.strip(),
                                      {s["color"] for s in line["spans"]}))
        if len(lines) >= MIN_LINES:
            out.append(lines)
    return out


def main(paths):
    try:
        import pymupdf
    except ImportError:
        print(" warn pymupdf не установлен — проверка подсветки пропущена")
        print("      установить: python3 -m pip install pymupdf")
        return 0

    total = colored = 0
    plain = []
    for path in paths:
        doc = pymupdf.open(path)
        for number, page in enumerate(doc, 1):
            for lines in plates(page, pymupdf):
                total += 1
                colors = set().union(*[c for _, c in lines])
                if colors - NEUTRAL:
                    colored += 1
                else:
                    plain.append((path, number, lines[0][0][:60]))
        doc.close()

    if total == 0:
        print(" warn листингов не найдено: PDF собраны?")
        return 0

    print(f"  ok  листингов: {total}, с подсветкой: {colored}, одноцветных: {len(plain)}")
    for path, number, head in plain:
        print(f"       {path}:{number}  {head}")
    if plain:
        print("      одноцветный листинг — либо вывод программы (тогда так и"
              " задумано), либо язык не указан или неизвестен typst")
    return 0


if __name__ == "__main__":
    args = sys.argv[1:] or sorted(glob.glob("out/*.pdf"))
    sys.exit(main([a for a in args if " — " not in a]))
