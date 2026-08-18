// lectures/template/frontmatter.typ — титульный лист и оглавления.

#import "theme.typ": fonts, palette, sizes
#import "i18n.typ": L, ru-date

#let title-page(meta, course) = page(
  numbering: none,
  header: none,
  footer: none,
)[
  #set align(center)
  #set text(font: fonts.head)
  #v(12mm)
  #if course.institute != "" {
    text(size: 12pt, tracking: 0.14em, weight: 600, upper(course.institute))
    v(2mm)
  }
  #if course.department != "" {
    text(size: 10pt, fill: palette.muted, course.department)
  }
  #v(38mm)
  #text(size: 10.5pt, tracking: 0.22em, fill: palette.muted, upper(course.discipline))
  #v(5mm)
  #line(length: 62%, stroke: 0.7pt + palette.rule)
  #v(6mm)
  // У приложений номера в расписании нет: вместо «Лекция N» печатается
  // «Приложение».
  #text(size: sizes.subtitle, fill: palette.muted)[#if meta.n == none {
    L.appendix
  } else {
    [#L.lecture~#meta.n]
  }]
  #v(3mm)
  #text(size: sizes.title, weight: 600, meta.title)
  #v(1fr)
  #set align(right)
  #set text(size: 10.5pt)
  #for a in course.authors [#a \ ]
  #v(2mm)
  #text(fill: palette.muted)[#ru-date(meta.date)]
  #linebreak()
  #text(fill: palette.muted, size: 9pt)[#L.version #course.version]
  #if course.city != "" {
    v(6mm)
    text(fill: palette.muted, size: 9pt)[#course.city]
  }
]

#let front-outlines(
  contents: true,
  figures: true,
  tables: true,
  listings: true,
  depth: 3,
  // separate: каждый список — с новой страницы. Нужно там, где списки длинные
  // (сводный том: только рисунков четыре страницы) и заголовок следующего
  // списка иначе прилипает к последней строке предыдущего. В отдельной лекции
  // списки короткие, и разрыв дал бы полупустые страницы, поэтому там они
  // разделяются отбивкой.
  separate: false,
) = {
  set outline.entry(fill: repeat[.#h(3pt)])
  show outline.entry.where(level: 1): it => { v(6pt, weak: true); strong(it) }
  let gap = if separate { pagebreak(weak: true) } else { v(20pt, weak: true) }
  // Пустые списки не печатаются: в исходных лекциях «Список таблиц»
  // выводился заголовком без единой строки.
  let non-empty(target, title) = context {
    if query(target).len() > 0 {
      gap
      outline(title: title, target: target)
    }
  }
  if contents { outline(title: [#L.contents], depth: depth) }
  if figures { non-empty(figure.where(kind: image), [#L.figures]) }
  if tables { non-empty(figure.where(kind: table), [#L.tables]) }
  if listings { non-empty(figure.where(kind: raw), [#L.listings]) }
}
