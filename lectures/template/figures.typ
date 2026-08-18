// lectures/template/figures.typ — подписи «Рисунок N — …», картинки, подрисунки.

#import "i18n.typ": L
#import "theme.typ": justify-in-captions

#let figure-rules(doc) = {
  set figure(numbering: "1")
  show figure.where(kind: image): set figure(supplement: [#L.figure])
  show figure.where(kind: table): set figure(supplement: [#L.table])
  show figure.where(kind: raw): set figure(supplement: [#L.listing])
  show figure.where(kind: table): set figure(placement: none)
  // Листинг длиннее страницы обязан разрываться. Блок `raw` сам по себе
  // breakable, но figure вокруг него — нет, и длинный листинг просто вылезал
  // за нижнее поле: подпись и колонцифра оказывались НАПЕЧАТАНЫ ПОВЕРХ кода
  // (было на девяти страницах тома). Таблицы разрываются по той же причине:
  // таблица переходов на сотню строк в лекции 10 не помещается на страницу.
  show figure.where(kind: raw): set block(breakable: true)
  show figure.where(kind: table): set block(breakable: true)
  // ГОСТ: «Рисунок 3 — Автомат Мили», через тире, а не двоеточие.
  set figure.caption(separator: [ --- ])
  show figure.caption: it => [
    #set text(size: 9.5pt)
    // Подпись короткая и центрированная: выключка по формату дала бы в ней
    // растянутые пробелы (см. `justify-in-captions` в theme.typ).
    #set par(justify: justify-in-captions)
    #strong[#it.supplement~#context it.counter.display(it.numbering)]#it.separator#it.body
  ]
  // Ссылка @eq:canon печатается как «(1)», а не «Уравнение 1».
  set math.equation(numbering: "(1)", supplement: none)

  // В подписи стоит полное слово («Рисунок 3 — …»), а в ссылке — сокращение,
  // чтобы фразы вида «приведена на рис. 3» и «см. в табл. 2» оставались
  // грамматичными. При полном слове получалось «приведена на Рисунок 3».
  show ref: it => {
    let el = it.element
    if el == none or el.func() != figure { return it }
    let short = if el.kind == image { L.figure-short } else if el.kind == table {
      L.table-short
    } else if el.kind == raw { L.listing-short } else { none }
    if short == none { return it }
    link(
      el.location(),
      [#short~#numbering(el.numbering, ..counter(figure.where(kind: el.kind)).at(el.location()))],
    )
  }
  doc
}

// ВАЖНО: как и у code-file, path разрешается относительно template/figures.typ,
// поэтому указывайте абсолютный путь от --root: "/src/02-fsm/images/foo.png".
// Внутри самого файла лекции обычный `image("images/foo.png")` работает как
// относительный — так и сделано в перенесённых лекциях.
#let img(path, caption: none, width: 80%, label: none) = {
  let f = figure(image(path, width: width), caption: caption)
  if label == none { f } else { [#f #label] }
}

// Замена subfig/subfloat: сетка подрисунков внутри одного figure.
// items — массив словарей (body: …, caption: …).
#let subfigures(items, caption: none, columns: 2, gutter: 8pt, label: none) = {
  let cells = items.map(it => [
    #it.at("body")
    #v(3pt)
    #text(size: 9pt, emph(it.at("caption", default: none)))
  ])
  let f = figure(
    grid(columns: columns, gutter: gutter, align: center, ..cells),
    caption: caption,
  )
  if label == none { f } else { [#f #label] }
}

// Карта Карно функции трёх переменных: один разряд входа против двух
// разрядов состояния.
//
// name  — обозначение функции, например $phi_1$
// cells — 2×4 массив значений: строки — x1 = 0, 1; столбцы — (q1 q2) = 00,
//         01, 11, 10 (код Грея); значение none печатается звёздочкой.
#let karnaugh-map3(name, cells, caption: none, label: none) = {
  let cell(v) = if v == none { $*$ } else { [#v] }
  let gray-q1 = (0, 0, 1, 1)
  let gray-q2 = (0, 1, 1, 0)
  let f = figure(
    table(
      columns: (auto, auto, auto, auto, auto, auto),
      align: center,
      stroke: none,
      inset: 5pt,
      table.vline(x: 2, stroke: 0.5pt),
      name, [$q_1$], ..gray-q1.map(v => [#v]),
      [], [$q_2$], ..gray-q2.map(v => [#v]),
      [$x_1$], [], table.cell(colspan: 4)[],
      table.hline(y: 3, stroke: 0.5pt),
      ..(0, 1)
        .map(x => ([#x], [], ..cells.at(x).map(cell)))
        .flatten(),
    ),
    caption: caption,
    kind: table,
  )
  if label == none { f } else { [#f #label] }
}

// Карта Карно функции четырёх переменных.
//
// name  — обозначение функции, например $phi_1$
// cells — 4×4 массив значений в порядке строк (x1 x2) = 00, 01, 11, 10
//         и столбцов (q1 q2) = 00, 01, 11, 10 (код Грея);
//         значение none печатается звёздочкой (набор недостижим).
#let karnaugh-map(name, cells, caption: none, label: none) = {
  let cell(v) = if v == none { $*$ } else { [#v] }
  let gray-q1 = (0, 0, 1, 1)
  let gray-q2 = (0, 1, 1, 0)
  let gray-x = ((0, 0), (0, 1), (1, 1), (1, 0))
  let f = figure(
    table(
      columns: (auto, auto, auto, auto, auto, auto),
      align: center,
      stroke: none,
      inset: 5pt,
      table.vline(x: 2, stroke: 0.5pt),
      name, [$q_1$], ..gray-q1.map(v => [#v]),
      [], [$q_2$], ..gray-q2.map(v => [#v]),
      [$x_1$], [$x_2$], table.cell(colspan: 4)[],
      table.hline(y: 3, stroke: 0.5pt),
      ..gray-x
        .enumerate()
        .map(((i, x)) => ([#x.at(0)], [#x.at(1)], ..cells.at(i).map(cell)))
        .flatten(),
    ),
    caption: caption,
    kind: table,
  )
  if label == none { f } else { [#f #label] }
}
