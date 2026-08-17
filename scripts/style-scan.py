#!/usr/bin/env python3
# scripts/style-scan.py <корень> <длина строки> <каталог отчётов>
#
# Построчные проверки оформления, которые не выражаются через .clang-format.
# Список файлов приходит на stdin по одному в строке; результат раскладывается
# по файлам отчётов в указанном каталоге (tabs, long, slashes, bom, crlf,
# trail, eof) — их читает scripts/check-style.sh.
#
# Почему не grep. Проверки табуляции и комментариев `//` требуют регулярных
# выражений PCRE (`grep -P`), а BSD grep в macOS ключа -P не имеет: там
# `grep -P ... 2>/dev/null` молча не находил ничего, и две проверки из семи
# на машине разработчика были пустышкой — расхождение вскрылось только в CI.
# Разбор `//` регулярным выражением к тому же неверен по существу: внутри
# блочного комментария и строкового литерала это не комментарий (на слове
# «комментарии //» в тексте пояснения проверка ложно срабатывала).

import sys

# Больше трёх нарушений одного вида на файл не показываем: остальное всё равно
# правится тем же способом, а вывод остаётся читаемым.
MAX_PER_FILE = 3

CODE, STRING, CHAR, BLOCK = 0, 1, 2, 3


def find_line_comments(text):
    """Номера строк, где начинается комментарий `//` вне строк и блоков.

    Разбор посимвольный: регулярное выражение не отличает `//` в коде от
    того же сочетания внутри /* ... */ или "http://".
    """
    found = []
    state = CODE
    line = 1
    i = 0
    size = len(text)
    while i < size:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < size else ""
        if ch == "\n":
            line += 1
            i += 1
            continue
        if state == CODE:
            if ch == "/" and nxt == "*":
                state = BLOCK
                i += 2
                continue
            if ch == "/" and nxt == "/":
                found.append(line)
                # До конца строки разбирать нечего.
                while i < size and text[i] != "\n":
                    i += 1
                continue
            if ch == '"':
                state = STRING
            elif ch == "'":
                state = CHAR
        elif state == BLOCK:
            if ch == "*" and nxt == "/":
                state = CODE
                i += 2
                continue
        else:  # STRING, CHAR
            if ch == "\\":
                # Экранированная кавычка литерал не закрывает.
                if nxt == "\n":
                    line += 1
                i += 2
                continue
            if (state == STRING and ch == '"') or (state == CHAR and ch == "'"):
                state = CODE
        i += 1
    return found


def main():
    root, limit, out_dir = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    reports = {name: [] for name in
               ("tabs", "long", "slashes", "bom", "crlf", "trail", "eof")}

    for path in (ln.rstrip("\n") for ln in sys.stdin if ln.strip()):
        rel = path[len(root) + 1:] if path.startswith(root + "/") else path
        with open(path, "rb") as fh:
            raw = fh.read()

        if raw.startswith(b"\xef\xbb\xbf"):
            reports["bom"].append("%s: BOM в начале файла" % rel)
        if b"\r" in raw:
            reports["crlf"].append("%s: концы строк CRLF" % rel)
        if raw and not raw.endswith(b"\n"):
            reports["eof"].append("%s: нет перевода строки в конце файла" % rel)

        text = raw.decode("utf-8", errors="replace")
        lines = text.split("\n")
        if lines and lines[-1] == "":
            lines.pop()

        shown = {"tabs": 0, "long": 0, "trail": 0}
        for number, line in enumerate(lines, 1):
            if "\t" in line and shown["tabs"] < MAX_PER_FILE:
                reports["tabs"].append("%s:%d: %s" % (rel, number, line.strip()))
                shown["tabs"] += 1
            # Длина считается в символах, а не в байтах: комментарии на русском.
            if len(line) > limit and shown["long"] < MAX_PER_FILE:
                reports["long"].append("%s:%d: %d символов" % (rel, number, len(line)))
                shown["long"] += 1
            if line and line[-1] in " \t" and shown["trail"] < MAX_PER_FILE:
                reports["trail"].append("%s:%d: пробелы в конце строки" % (rel, number))
                shown["trail"] += 1

        for number in find_line_comments(text)[:MAX_PER_FILE]:
            body = lines[number - 1].strip() if number <= len(lines) else ""
            reports["slashes"].append("%s:%d: %s" % (rel, number, body))

    for name, items in reports.items():
        with open("%s/%s" % (out_dir, name), "w", encoding="utf-8") as fh:
            for item in items:
                fh.write(item + "\n")


if __name__ == "__main__":
    main()
