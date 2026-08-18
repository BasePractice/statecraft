// lectures/template/frontmatter.typ — титульный лист и оглавления.

#import "theme.typ": fonts, palette, sizes, heading-gaps
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
  // Что за документ: у приложения номера в расписании нет, у лабораторной
  // работы номер свой. Вид приходит полем `kind`.
  #text(size: sizes.subtitle, fill: palette.muted)[#{
    let kind = meta.at("kind", default: if meta.n == none { "appendix" } else { "lecture" })
    if kind == "appendix" {
      L.appendix
    } else if kind == "lab" {
      [#L.lab~#meta.n]
    } else {
      [#L.lecture~#meta.n]
    }
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
  // Заголовок печатается отдельно от списка, а не параметром `title`, и за ним
  // ставится явная отбивка. Причина: строка списка первого уровня начинается с
  // `v(6pt, weak: true)` (правило выше), а слабая отбивка в typst не берёт
  // максимум с блочной, а вытесняет её — из-за этого `below` у заголовка
  // пропадал и первая строка списка прилипала к заголовку, сколько ни
  // увеличивай отступ. Явная (не weak) отбивка вытеснению не поддаётся.
  // `outlined: false` сохраняет прежнее поведение: сами списки в «Содержание»
  // не попадают, но закладка в PDF остаётся.
  let head(title) = {
    heading(level: 1, numbering: none, outlined: false, bookmarked: true, title)
    v(heading-gaps.service.below)
  }
  // Пустые списки не печатаются: в исходных лекциях «Список таблиц»
  // выводился заголовком без единой строки.
  let non-empty(target, title, kind: none, short: none) = context {
    if query(target).len() > 0 {
      gap
      head(title)
      // В списке слово «Рисунок» повторялось бы в каждой строке и съедало
      // место, поэтому там оно сокращается («Рис. 3»). Подпись под самим
      // рисунком остаётся полной: сокращение задано здесь, а не supplement'ом.
      if short == none {
        outline(title: none, target: target)
      } else {
        show outline.entry.where(level: 1): it => {
          v(6pt, weak: true)
          strong(it.indented(
            [#short~#numbering(
              it.element.numbering,
              ..counter(figure.where(kind: kind)).at(it.element.location()),
            )],
            it.inner(),
          ))
        }
        outline(title: none, target: target)
      }
    }
  }
  if contents {
    head([#L.contents])
    outline(title: none, depth: depth)
  }
  if figures {
    non-empty(figure.where(kind: image), [#L.figures], kind: image, short: L.figure-list)
  }
  if tables { non-empty(figure.where(kind: table), [#L.tables]) }
  if listings { non-empty(figure.where(kind: raw), [#L.listings]) }
}
