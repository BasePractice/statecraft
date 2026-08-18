// lectures/template/book.typ — сводный том курса.
//
// Тот же материал, что и в отдельных лекциях, но одним документом: единое
// оглавление, сквозная нумерация разделов, рисунков, таблиц и листингов, один
// список литературы в конце. Лекция становится разделом первого уровня, её
// собственные разделы уезжают на уровень ниже (`set heading(offset: 1)`).
//
// Почему не через lecture(): у lecture() своя фронт-материя, свой титул и своя
// библиография на документ — в сборнике всё это нужно ровно по одному разу.
// Общее с ней — оформление (тема, блоки, листинги, рисунки), поэтому show- и
// set-правила ниже повторяют lecture() и должны меняться вместе с ней.

#import "/course.typ": course as _course, lectures, appendices
#import "theme.typ": fonts, palette, sizes, page-setup, par-setup, body-weight, heading-number-gap
#import "i18n.typ" as i18n
#import "i18n.typ": L, ru-date
#import "blocks.typ": *
#import "blocks.typ" as blocks
#import "code.typ": raw-show-rules, code-file, listing
#import "figures.typ": figure-rules, img, karnaugh-map, karnaugh-map3, subfigures
#import "frontmatter.typ": front-outlines
#import "appendix.typ": book-mode
#import "/shared/glossary.typ": glossary-section
#import "/shared/questions.typ": questions-section
#import "/shared/tasks.typ": tasks-section
#import "/shared/notation.typ" as N

#let nota = N
#let course = _course
#let todo(body) = blocks.todo(body, show-todo: _course.show-todo)

// Титул сборника: без номера лекции и без даты занятия — у тома их нет.
#let book-title-page(c) = page(
  numbering: none,
  header: none,
  footer: none,
)[
  #set align(center)
  #set text(font: fonts.head)
  #v(12mm)
  #if c.institute != "" {
    text(size: 12pt, tracking: 0.14em, weight: 600, upper(c.institute))
    v(2mm)
  }
  #if c.department != "" {
    text(size: 10pt, fill: palette.muted, c.department)
  }
  #v(38mm)
  #text(size: 10.5pt, tracking: 0.22em, fill: palette.muted, upper(c.short))
  #v(5mm)
  #line(length: 62%, stroke: 0.7pt + palette.rule)
  #v(6mm)
  #text(size: sizes.title, weight: 600, c.discipline)
  #v(3mm)
  #text(size: sizes.subtitle, fill: palette.muted)[#L.book-subtitle]
  #v(1fr)
  #set align(right)
  #set text(size: 10.5pt)
  #for a in c.authors [#a \ ]
  #v(2mm)
  #text(fill: palette.muted, size: 9pt)[#L.version #c.version]
  #if c.city != "" {
    v(6mm)
    text(fill: palette.muted, size: 9pt)[#c.city]
  }
]

// Одна часть тома: заголовок «Лекция N. Название» (или «Приложение. Название»)
// и тело, включённое со сдвигом заголовков на уровень вниз.
//
// Тело оборачивается в блок контента: set и show внутри лекции (например
// `#show: appendix` в лекции 10, переключающий нумерацию на буквенную) не
// должны утекать в следующие части тома.
#let part(title, body, number: none, questions: none, tasks: none) = {
  pagebreak(weak: true)
  // Номер части задаётся явно: внутри лекций встречаются собственные
  // приложения со своим счётчиком, и полагаться на автоинкремент нельзя.
  if number != none { counter(heading).update(number - 1) }
  heading(level: 1, title)
  [
    #set heading(offset: 1)
    #body
    #if questions != none { questions-section(questions) }
    #if tasks != none { tasks-section(tasks) }
  ]
}

// Приложения тома нумеруются буквами по ГОСТ 7.32 — как и приложения внутри
// лекции. Применяется один раз, show-правилом на остаток документа.
#let appendix-parts(doc) = {
  counter(heading).update(0)
  set heading(numbering: i18n.appendix-numbering)
  doc
}

#let book(
  contents: true,
  figures-list: true,
  tables-list: true,
  listings-list: true,
  glossary: true,
  body,
) = {
  let c = (
    institute: _course.institute,
    department: _course.department,
    discipline: _course.discipline,
    short: _course.short,
    authors: _course.authors,
    version: _course.version,
    city: _course.city,
  )

  set document(title: _course.discipline, author: _course.authors)

  set text(
    font: fonts.text,
    lang: "ru",
    region: "RU",
    size: sizes.body,
    weight: body-weight,
    fill: palette.ink,
    hyphenate: true,
  )
  show math.equation: set text(font: fonts.math)
  set par(..par-setup)
  set page(..page-setup)

  set heading(numbering: "1.1")
  show heading: set text(font: fonts.head, fill: palette.ink)
  let head-number(it) = if it.numbering == none {
    none
  } else {
    text(fill: palette.heading-number, numbering(
      it.numbering, ..counter(heading).at(it.location()),
    )) + h(heading-number-gap)
  }
  // Заголовок первого уровня в томе — целая лекция, поэтому он крупнее, чем
  // раздел внутри неё, и начинает страницу.
  show heading.where(level: 1): it => block(
    above: 0pt, below: 16pt,
    text(size: sizes.title, weight: 600, head-number(it) + it.body),
  )
  show heading.where(level: 2): it => block(
    above: 20pt, below: 11pt,
    text(size: sizes.h1, weight: 600, head-number(it) + it.body),
  )
  show heading.where(level: 3): it => block(
    above: 14pt, below: 7pt,
    text(size: sizes.h2, weight: 600, head-number(it) + it.body),
  )
  show heading.where(level: 4): it => block(
    above: 12pt, below: 6pt,
    text(size: sizes.h3, weight: 600, head-number(it) + it.body),
  )

  show link: set text(fill: palette.accent)
  show ref: set text(fill: palette.accent)

  show: figure-rules
  show: raw-show-rules
  show table: set table(stroke: 0.5pt + palette.rule, inset: 5pt)
  show blocks.stmt-selector: set figure(gap: 0pt)
  show blocks.stmt-selector: set block(width: 100%)

  book-mode.update(true)

  book-title-page(c)

  set page(numbering: "i", number-align: center)
  counter(page).update(1)
  front-outlines(
    contents: contents,
    figures: figures-list,
    tables: tables-list,
    listings: listings-list,
    depth: 2,
  )
  if glossary { glossary-section() }

  pagebreak(weak: true)
  set page(
    numbering: "1",
    number-align: center,
    header: context {
      // Слева — текущая часть тома (лекция или приложение), справа — раздел
      // внутри неё: в сборнике без этого непонятно, что читаешь.
      //
      // Считается по страницам, а не по `before(here())`: колонтитул рисуется
      // раньше содержимого страницы, поэтому на первой странице лекции
      // «before» видит ещё предыдущую и печатает чужое название.
      let page-now = here().page()
      let parts = query(heading.where(level: 1))
        .filter(h => h.location().page() <= page-now)
      let secs = query(heading.where(level: 2))
        .filter(h => h.numbering != none and h.location().page() <= page-now)
      // Раздел, оставшийся от предыдущей части, в колонтитул не попадает.
      if parts.len() > 0 {
        secs = secs.filter(h => h.location().page() >= parts.last().location().page())
      }
      set text(font: fonts.head, size: 8.5pt, fill: palette.muted)
      grid(
        columns: (1fr, auto),
        align: (left, right),
        if parts.len() > 0 { parts.last().body } else { [#_course.discipline] },
        if secs.len() > 0 {
          let h = secs.last()
          let num = counter(heading).at(h.location())
          [#numbering(h.numbering, ..num)~#h.body]
        } else { [] },
      )
      v(-6pt)
      line(length: 100%, stroke: 0.4pt + palette.rule)
    },
  )
  counter(page).update(1)

  body

  if _course.bib != none {
    pagebreak(weak: true)
    bibliography(_course.bib, style: _course.bib-style, title: [#L.refs])
  }
}
