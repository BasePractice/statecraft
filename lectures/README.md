# Лекции курса АПСУ

Десять лекций в формате [Typst](https://typst.app) под единым шаблоном.
Автор, институт, кафедра, дата и номера лекций правятся в одном файле —
[`course.typ`](course.typ).

## Быстрый старт

```bash
./scripts/check.sh --fix   # проверить окружение и доустановить недостающее
./scripts/build.sh         # собрать все лекции в out/
```

Или через `make`:

```bash
make fix && make
```

## Требования

| Что | Зачем | Установка |
|---|---|---|
| `typst` ≥ 0.15 | сборка | `brew install typst` |
| `pandoc` | только для `tex2typ.sh` | `brew install pandoc` |
| Fira Code, PT Serif, PT Sans | шрифты | `./scripts/fetch-fonts.sh` |

Шрифты не обязательны: шаблон падает обратно на встроенный в typst
Libertinus Serif, который полностью покрывает кириллицу. Внешних
typst-пакетов базовый шаблон не использует, поэтому собирается офлайн.

## Команды

```bash
./scripts/build.sh                  # все лекции
./scripts/build.sh 03-synthesis     # одну
./scripts/build.sh -w 03-synthesis  # пересборка при изменении
./scripts/build.sh --pretty         # копии под читаемыми именами
./scripts/build.sh --draft          # с водяным знаком «ЧЕРНОВИК»
./scripts/clean.sh                  # очистить out/
```

`make help` печатает то же самое.

## Структура

```
course.typ         метаданные курса и реестр лекций — единственное место правки
template/          шаблон: тема, подписи, блоки, листинги, титул, приложения
shared/            общая нотация, глоссарий, контрольные вопросы, задачник
bib/references.bib единая библиография (стиль gost-r-705-2008-numeric)
src/<id>/          лекция: main.typ, main.body.typ, images/, code/
scripts/           check, build, clean, fetch-fonts, vendor-packages, tex2typ
out/               собранные PDF (в .gitignore)
```

## Как добавить лекцию

1. Добавить запись в реестр `lectures` в `course.typ`.
2. Скопировать `src/_template/` в `src/<новый-id>/`.
3. Заменить `id` в первой строке `main.typ`.

Больше нигде номер, название, дату и автора указывать не нужно.

## Что доступно в лекции

Один импорт `#import "/template/lecture.typ": *` даёт:

- блоки `#definition`, `#theorem`, `#lemma`, `#corollary`, `#example`,
  `#remark`, `#exercise`, `#proof` — со сквозной нумерацией и работающими
  ссылками (`#theorem[…] <thm:kleene>`, далее `@thm:kleene`);
- `#todo[…]` — видимая в PDF метка ненаписанного фрагмента; выключается
  флагом `show-todo` в `course.typ`;
- `#sources[…]` — врезка о происхождении материала и внесённых правках;
- `#code-file("/src/<id>/code/foo.c", lang: "c", from: 1, to: 40)` — листинг
  из файла (путь **абсолютный от корня** `lectures/`);
- `#img(...)`, `#subfigures(...)`, `#listing(...)`, `#karnaugh-map(...)`;
- `#nota.*` — единая нотация курса из `shared/notation.typ`;
- `#show: appendix` — дальше заголовки нумеруются «Приложение А».

## Векторные диаграммы автоматов

`template/diagrams.typ` — единственный файл шаблона с внешней зависимостью
(`@preview/fletcher`). Базовый шаблон его не импортирует, поэтому лекция без
диаграмм собирается офлайн. Лекции, которым диаграммы нужны, подключают его
явно:

```typst
#import "/template/diagrams.typ": fsm-diagram
```

Перед первой такой сборкой прогрейте локальный кэш пакетов:

```bash
./scripts/vendor-packages.sh
```

Пример использования — диаграмма Мура разменного аппарата в
`src/03-synthesis/main.body.typ`.

## Перенос лекции из LaTeX

```bash
./scripts/tex2typ.sh "/путь/к/index.tex" 03-synthesis
```

Скрипт копирует картинки и код, прогоняет `pandoc` и чинит то, что pandoc
ломает (кавычки-ёлочки, `image()` без расширения, теоремы с именем,
`\lstinputlisting`, `\appendix`). Перезаписывается только `main.body.typ`,
поэтому конвертер можно перезапускать. Список мест для ручной доводки
пишется в `src/<id>/TODO.md`.

## Замечания по typst 0.15

- Угловые скобки — только `chevron.l` / `chevron.r`; `angle.l` даёт
  «unknown symbol modifier».
- Единственный встроенный ГОСТ-стиль библиографии — `gost-r-705-2008-numeric`.
- Fira Code — variable-шрифт с начертанием по умолчанию Light (300), поэтому
  в шаблоне вес задан явно.
- `read()` и `image()` внутри шаблона разрешают относительные пути от файла
  шаблона, а не от лекции: в `code-file` и `img` указывайте абсолютный путь
  от корня.
