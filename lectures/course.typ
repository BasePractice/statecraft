// lectures/course.typ
//
// ЕДИНСТВЕННЫЙ файл, который правят при смене автора, института, кафедры,
// дисциплины, даты или версии курса. Ни один src/**/main.typ не дублирует
// эти сведения — там указывается только `id` лекции.
//
// Этот же файл читают bash-скрипты через `typst eval`, поэтому реестр лекций
// нигде больше не повторяется.

#let course = (
  discipline: "Автоматное программирование систем управления",
  short:      "АПСУ",
  institute:  "",   // TODO: заполнить (в исходных материалах не указано)
  department: "",   // TODO: заполнить (в исходных материалах не указано)
  city:       "Москва",   // TODO: заполнить
  authors:    ("Хлебников Андрей",),
  email:      "viruszold@gmail.com",
  version:    "2026.1",
  year:       2026,
  repo:       "https://github.com/BasePractice/statecraft",

  // Библиография: путь абсолютный относительно --root (то есть от lectures/)
  bib:        "/bib/references.bib",
  // ЕДИНСТВЕННЫЙ ГОСТ-стиль, встроенный в typst 0.15
  bib-style:  "gost-r-705-2008-numeric",

  // Что печатать в конце каждой лекции по умолчанию
  with-questions: true,
  with-tasks:     true,
  with-glossary:  false,

  // Показывать ли блоки #todo[...] в PDF. Выключить перед раздачей студентам.
  show-todo:      true,
)

// Реестр лекций. Номер, название и дата берутся отсюда и шаблоном, и скриптами.
//
// ВНИМАНИЕ: нумерация провизорная — она сведена из трёх источников (лекции
// репозитория articles, md-лекции statecraft и план REPORT.md §6) и
// утверждается отдельно. Перенумерация — правка этого реестра плюс `git mv`
// каталога в src/.
#let lectures = (
  (n:  1, id: "01-intro",         title: "Введение. Дискретные системы, алфавиты, слова и языки",
      date: datetime(year: 2026, month: 9, day: 1)),
  (n:  2, id: "02-fsm",           title: "Конечный автомат. Модели Мили и Мура",
      date: datetime(year: 2026, month: 9, day: 8)),
  (n:  3, id: "03-synthesis",     title: "Синтез автоматов и автоматные схемы",
      date: datetime(year: 2026, month: 9, day: 15)),
  (n:  4, id: "04-dfa-nfa",       title: "Автоматы-акцепторы. ДКА и НКА",
      date: datetime(year: 2026, month: 9, day: 22)),
  (n:  5, id: "05-kleene",        title: "Регулярные события и теорема Клини",
      date: datetime(year: 2026, month: 9, day: 29)),
  (n:  6, id: "06-regexp",        title: "Регулярные выражения и лексический анализ",
      date: datetime(year: 2026, month: 10, day: 6)),
  (n:  7, id: "07-automata-prog", title: "Автоматное программирование",
      date: datetime(year: 2026, month: 10, day: 13)),
  (n:  8, id: "08-statecharts",   title: "Иерархические автоматы и кодогенерация",
      date: datetime(year: 2026, month: 10, day: 20)),
  (n:  9, id: "09-verification",  title: "Верификация и тестирование автоматных программ",
      date: datetime(year: 2026, month: 10, day: 27)),
  (n: 10, id: "10-limits",        title: "Границы модели. Машина Тьюринга и вычислимость",
      date: datetime(year: 2026, month: 11, day: 3)),
)

#let lecture-meta(id) = {
  if id == "_template" {
    return (n: 0, id: "_template", title: "Болванка новой лекции",
            date: datetime(year: course.year, month: 1, day: 1))
  }
  let m = lectures.find(l => l.id == id)
  if m == none { panic("course.typ: нет лекции с id «" + id + "»") }
  m
}
