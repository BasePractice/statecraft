// lectures/template/theme.typ — шрифты, цвета, кегли, поля.

// ---------------------------------------------------------------------------
// ПЕРЕКЛЮЧАТЕЛЬ ОСНОВНОГО ШРИФТА
//
// false — основной текст набирается PT Serif (по умолчанию);
// true  — основной текст тоже набирается Fira Code.
//
// Fira Code — моноширинная гарнитура; на объёме лекции сплошной моноширинный
// набор читается заметно хуже, поэтому по умолчанию она используется для кода,
// листингов и акцентов. Если нужен Fira Code во всём документе — поставьте true.
// ---------------------------------------------------------------------------
#let body-in-mono = false

// Fira Code — variable-шрифт, Weight 300–700, Default 300. Без явного
// weight: 400 весь код выйдет в начертании Light.
#let mono-weight = 400

#let _serif = ("PT Serif", "Libertinus Serif", "New Computer Modern")
#let _sans = ("PT Sans", "PT Serif", "Libertinus Serif")
#let _mono = ("Fira Code", "PT Mono", "DejaVu Sans Mono")

#let fonts = (
  // Libertinus Serif ВСТРОЕН в typst и полностью покрывает кириллицу,
  // поэтому документ соберётся и без единого установленного шрифта.
  text: if body-in-mono { _mono } else { _serif },
  head: if body-in-mono { _mono } else { _sans },
  mono: _mono,
  math: ("New Computer Modern Math",),
)

#let palette = (
  ink: rgb("#1a1a1a"),
  muted: luma(105),
  rule: luma(180),
  accent: rgb("#2f5d8c"), // теорема, лемма, следствие
  define: rgb("#2f7a4f"), // определение
  example: rgb("#7a5a1f"), // пример
  remark: rgb("#5a5a6e"), // замечание
  task: rgb("#8c2f4a"), // задание
  todo: rgb("#a03000"), // блок #todo[...]
  code-bg: luma(248),
  code-rule: luma(205),
)

#let sizes = (
  body: 11pt,
  small: 9.5pt,
  code: 8.8pt,
  h1: 16pt,
  h2: 13pt,
  h3: 11.5pt,
  title: 24pt,
  subtitle: 15pt,
)

#let page-setup = (
  paper: "a4",
  margin: (top: 25mm, bottom: 22mm, inside: 28mm, outside: 20mm),
)

#let par-setup = (
  justify: true,
  leading: 0.72em,
  spacing: 0.95em,
  first-line-indent: (amount: 1.25em, all: true),
)
