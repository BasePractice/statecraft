// lectures/template/figures.typ — подписи «Рисунок N — …», картинки, подрисунки.

#import "i18n.typ": L

#let figure-rules(doc) = {
  set figure(numbering: "1")
  show figure.where(kind: image): set figure(supplement: [#L.figure])
  show figure.where(kind: table): set figure(supplement: [#L.table])
  show figure.where(kind: raw): set figure(supplement: [#L.listing])
  show figure.where(kind: table): set figure(placement: none)
  // ГОСТ: «Рисунок 3 — Автомат Мили», через тире, а не двоеточие.
  set figure.caption(separator: [ --- ])
  show figure.caption: it => [
    #set text(size: 9.5pt)
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
