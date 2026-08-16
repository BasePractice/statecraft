// lectures/template/theme.typ — шрифты, цвета, кегли, поля.

// ---------------------------------------------------------------------------
// ПЕРЕКЛЮЧАТЕЛЬ ОСНОВНОГО ШРИФТА
//
// true  — весь текст лекции набирается Fira Code (требование заказчика курса);
// false — основной текст набирается PT Serif, Fira Code остаётся только у кода.
//
// Fira Code моноширинная, поэтому при true кегль и интерлиньяж основного текста
// берутся из `mono-body-*` ниже: та же полоса набора вмещает сопоставимое число
// знаков, а строки не слипаются.
// ---------------------------------------------------------------------------
#let body-in-mono = true

// Кегль и интерлиньяж основного текста при `body-in-mono: true`. Средняя ширина
// знака Fira Code — 0.6em против 0.47em у PT Serif, поэтому 11pt дают строку
// примерно на четверть короче; 9.6pt возвращают привычные ~72 знака в строке.
#let mono-body-size = 9.6pt
#let mono-body-leading = 0.85em

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
  body: if body-in-mono { mono-body-size } else { 11pt },
  small: if body-in-mono { 8.4pt } else { 9.5pt },
  code: 8.8pt,
  h1: if body-in-mono { 14pt } else { 16pt },
  h2: if body-in-mono { 11.5pt } else { 13pt },
  h3: if body-in-mono { 10.2pt } else { 11.5pt },
  title: if body-in-mono { 20pt } else { 24pt },
  subtitle: if body-in-mono { 13pt } else { 15pt },
)

// Начертание основного текста: у Fira Code вес по умолчанию — Light (300).
#let body-weight = if body-in-mono { mono-weight } else { 400 }

#let page-setup = (
  paper: "a4",
  margin: (top: 25mm, bottom: 22mm, inside: 28mm, outside: 20mm),
)

#let par-setup = (
  // Моноширинный набор не сжимается и не растягивается по ширине знака, поэтому
  // выключка по формату при `body-in-mono` даёт дыры между словами — оставляем
  // выключку влево.
  justify: not body-in-mono,
  leading: if body-in-mono { mono-body-leading } else { 0.72em },
  spacing: 0.95em,
  first-line-indent: (amount: 1.25em, all: true),
)
