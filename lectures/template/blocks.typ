// lectures/template/blocks.typ — определение, теорема, лемма, следствие,
// пример, замечание, задание, доказательство, todo.
//
// Утверждения реализованы через figure с kind: "statement". Это даёт:
//   - сквозную нумерацию по всем видам утверждений (одна «Лемма 7» на лекцию,
//     а не «Лемма 1» рядом с «Определение 1»);
//   - работающие ссылки: `#theorem[...] <thm:kleene>` и далее `@thm:kleene`
//     печатается как «Теорема 7».
// Счётчик figure шагает по первому уровню, поэтому нумерация сквозная по
// документу, а не по разделам, — так номер в ссылке однозначен.

#import "theme.typ": palette
#import "i18n.typ": L

#let stmt-selector = figure.where(kind: "statement")
#let _proofmark = sym.square.stroked

#let _numbered(kind, color, name, body) = figure(
  block(
    width: 100%,
    breakable: true,
    inset: (left: 11pt, right: 8pt, y: 8pt),
    fill: color.lighten(93%),
    stroke: (left: 2.5pt + color),
    radius: (right: 2pt),
  )[
    #set align(left)
    #text(fill: color.darken(20%), weight: 600, {
      kind
      [ ]
      context counter(stmt-selector).display("1")
      if name != none [ (#emph(name))]
    })#[.]
    #body
  ],
  kind: "statement",
  supplement: kind,
  numbering: "1",
  outlined: false,
)

#let definition(name: none, body) = _numbered(L.definition, palette.define, name, body)
#let theorem(name: none, body) = _numbered(L.theorem, palette.accent, name, body)
#let lemma(name: none, body) = _numbered(L.lemma, palette.accent, name, body)
#let corollary(name: none, body) = _numbered(L.corollary, palette.accent, name, body)
#let example(name: none, body) = _numbered(L.example, palette.example, name, body)
#let remark(name: none, body) = _numbered(L.remark, palette.remark, name, body)
#let exercise(name: none, body) = _numbered(L.exercise, palette.task, name, body)

#let proof(body) = block(width: 100%, inset: (left: 11pt, y: 4pt))[
  #emph(L.proof + ".") #body #h(1fr) #_proofmark
]

// Незаполненный фрагмент. Виден в PDF, пока course.show-todo = true, — чтобы
// дыры из REPORT.md не терялись при вычитке. Перед раздачей студентам
// выключается одним флагом в course.typ.
#let todo(body, show-todo: true) = {
  if not show-todo { return }
  block(
    width: 100%,
    breakable: true,
    inset: (left: 11pt, right: 8pt, y: 7pt),
    fill: palette.todo.lighten(92%),
    stroke: (
      left: 2.5pt + palette.todo,
      rest: (paint: palette.todo.lighten(60%), thickness: 0.5pt, dash: "dashed"),
    ),
    radius: (right: 2pt),
  )[
    #text(fill: palette.todo, weight: 700, size: 0.85em)[#L.todo.] #body
  ]
}

// Врезка «откуда взят материал и что в нём исправлено».
#let sources(body) = block(
  width: 100%,
  inset: (left: 10pt, right: 8pt, y: 6pt),
  fill: luma(246),
  stroke: (left: 2pt + palette.muted),
)[
  #text(size: 0.85em, fill: palette.muted)[#strong(L.sources + ".") #body]
]
