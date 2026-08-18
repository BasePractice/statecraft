// lectures/template/appendix.typ — «Приложение А» с кириллической нумерацией.
//
// Использование в лекции:
//   #show: appendix
//   = Программа машины Тьюринга для y = x + 1
// — далее обычные заголовки первого уровня.

#import "i18n.typ": L, appendix-numbering
#import "theme.typ": fonts, palette, sizes

// В сводном томе (book.typ) лекция — раздел первого уровня, а её собственные
// приложения оказываются подразделами. Буквенная нумерация им там не нужна и
// только сбивала бы сквозную: `counter(heading).update(0)` ниже — глобальный
// сброс, из-за которого после лекции 10 нумерация частей тома начиналась
// заново. Признак тома выставляет book().
#let book-mode = state("statecraft-book-mode", false)

#let appendix(doc) = context if book-mode.get() { doc } else {
  counter(heading).update(0)
  set heading(numbering: appendix-numbering)
  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    block(above: 0pt, below: 14pt)[
      #set text(font: fonts.head, size: sizes.h1, weight: 600)
      #L.appendix~#context text(
        fill: palette.heading-number,
        counter(heading).display(appendix-numbering),
      )
      #linebreak()
      #it.body
    ]
  }
  doc
}
