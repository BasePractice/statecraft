// lectures/template/lecture.typ — публичный API шаблона.
//
// В файле лекции достаточно двух строк:
//     #import "/template/lecture.typ": *
//     #show: lecture.with(id: "02-fsm")
// Всё остальное — номер, название, дата, автор, институт — берётся из course.typ.

#import "/course.typ": course as _course, lecture-meta
#import "theme.typ": fonts, palette, sizes, page-setup, par-setup
#import "i18n.typ" as i18n
#import "i18n.typ": L, ru-date
#import "blocks.typ": *
#import "blocks.typ" as blocks
#import "code.typ": raw-show-rules, code-file, listing
#import "figures.typ": figure-rules, img, karnaugh-map, subfigures
#import "frontmatter.typ": title-page, front-outlines
#import "appendix.typ": appendix
#import "/shared/glossary.typ": glossary-section
#import "/shared/questions.typ": questions-section
#import "/shared/tasks.typ": tasks-section
#import "/shared/notation.typ" as N

// Ре-экспорт, чтобы в лекции был ровно один импорт.
#let nota = N
#let course = _course

// #todo[...] печатается только при course.show-todo = true.
#let todo(body) = blocks.todo(body, show-todo: _course.show-todo)

#let lecture(
  id: none,
  // Точечные переопределения — обычно не нужны.
  number: none,
  title: none,
  date: none,
  authors: none,
  institute: none,
  department: none,
  discipline: none,
  version: none,
  // Структурные переключатели.
  contents: true,
  figures-list: true,
  tables-list: true,
  listings-list: true,
  bibliography-file: none,
  bib-style: none,
  glossary: none,
  questions: none,
  tasks: none,
  draft: false,
  body,
) = {
  let m = if id != none {
    lecture-meta(id)
  } else {
    (n: number, id: "-", title: title, date: date)
  }
  let n = if number != none { number } else { m.n }
  let ttl = if title != none { title } else { m.title }
  let dt = if date != none { date } else { m.date }
  let c = (
    institute: if institute != none { institute } else { _course.institute },
    department: if department != none { department } else { _course.department },
    discipline: if discipline != none { discipline } else { _course.discipline },
    authors: if authors != none { authors } else { _course.authors },
    version: if version != none { version } else { _course.version },
    city: _course.city,
  )
  let bibf = if bibliography-file != none { bibliography-file } else { _course.bib }
  let bibs = if bib-style != none { bib-style } else { _course.bib-style }
  let g = if glossary != none { glossary } else { _course.with-glossary }
  let q = if questions != none { questions } else { _course.with-questions }
  let t = if tasks != none { tasks } else { _course.with-tasks }

  set document(title: ttl, author: c.authors, date: dt)

  set text(
    font: fonts.text,
    lang: "ru",
    region: "RU",
    size: sizes.body,
    fill: palette.ink,
    hyphenate: true,
  )
  show math.equation: set text(font: fonts.math)
  set par(..par-setup)
  set page(..page-setup)

  set heading(numbering: "1.1")
  show heading: set text(font: fonts.head, fill: palette.ink)
  show heading.where(level: 1): it => block(
    above: 20pt, below: 11pt, text(size: sizes.h1, weight: 600, it),
  )
  show heading.where(level: 2): set text(size: sizes.h2, weight: 600)
  show heading.where(level: 3): set text(size: sizes.h3, weight: 600)

  show link: set text(fill: palette.accent)
  show ref: set text(fill: palette.accent)

  show: figure-rules
  show: raw-show-rules
  show table: set table(stroke: 0.5pt + palette.rule, inset: 5pt)
  // Утверждения занимают всю ширину и не центрируются как обычные figure.
  show blocks.stmt-selector: set figure(gap: 0pt)
  show blocks.stmt-selector: set block(width: 100%)

  // Титул
  title-page((n: n, title: ttl, date: dt), c)

  // Фронт-материя, римская нумерация
  set page(numbering: "i", number-align: center)
  counter(page).update(1)
  front-outlines(
    contents: contents,
    figures: figures-list,
    tables: tables-list,
    listings: listings-list,
  )
  if g { glossary-section(only: m.id) }

  // Основной текст, арабская нумерация с единицы
  pagebreak(weak: true)
  set page(
    numbering: "1",
    number-align: center,
    header: context {
      // Только нумерованные заголовки: иначе в колонтитул попадают
      // «Содержание», «Список рисунков» и прочая фронт-материя.
      let hs = query(selector(heading.where(level: 1)).before(here()))
        .filter(h => h.numbering != none)
      let cur = if hs.len() > 0 { hs.last() } else { none }
      set text(font: fonts.head, size: 8.5pt, fill: palette.muted)
      grid(
        columns: (1fr, auto),
        align: (left, right),
        [#L.lecture~#n.~#ttl],
        if cur != none {
          let num = counter(heading).at(cur.location())
          [#numbering(cur.numbering, ..num)~#cur.body]
        } else { [] },
      )
      v(-6pt)
      line(length: 100%, stroke: 0.4pt + palette.rule)
    },
    background: if draft {
      rotate(-45deg, text(size: 90pt, fill: rgb(0, 0, 0, 18), weight: 700)[#L.draft])
    } else { none },
  )
  counter(page).update(1)

  body

  if q { questions-section(m.id) }
  if t { tasks-section(m.id) }

  if bibf != none {
    pagebreak(weak: true)
    bibliography(bibf, style: bibs, title: [#L.refs])
  }
}
