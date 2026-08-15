// lectures/template/appendix.typ — «Приложение А» с кириллической нумерацией.
//
// Использование в лекции:
//   #show: appendix
//   = Программа машины Тьюринга для y = x + 1
// — далее обычные заголовки первого уровня.

#import "i18n.typ": L, appendix-numbering
#import "theme.typ": fonts, sizes

#let appendix(doc) = {
  counter(heading).update(0)
  set heading(numbering: appendix-numbering)
  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    block(above: 0pt, below: 14pt)[
      #set text(font: fonts.head, size: sizes.h1, weight: 600)
      #L.appendix~#context counter(heading).display(appendix-numbering)
      #linebreak()
      #it.body
    ]
  }
  doc
}
