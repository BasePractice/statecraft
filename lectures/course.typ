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
  city:       "Москва",
  authors:    ("Хлебников Андрей",),
  email:      "viruszold@gmail.com",
  // Версия курса и версия релиза — одно и то же число: тег вида
  // v{MAJOR}.{MINOR}.{BUILD} обязан совпадать с этой строкой (ТД-3).
  version:    "1.8.0",
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

  // Печатать ли ответы к задачам. Экземпляр преподавателя собирается с true,
  // раздаточный — с false: условия остаются, ответы исчезают.
  show-answers:   true,
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
  // Клеточные автоматы выделены из лекции 10 отдельной лекцией: вместе они
  // занимали 104 страницы, и это две разные темы.
  (n: 11, id: "11-cells",         title: "Клеточные автоматы и самовоспроизведение",
      date: datetime(year: 2026, month: 11, day: 10)),
  // Тематически лекция примыкает к 8-й (кодогенерация): язык описывает
  // автоматы и порождает C, ST для ПЛК, Rust и SystemVerilog.
  (n: 12, id: "12-takt",          title: "Язык Takt: описание автоматов и порождение кода",
      date: datetime(year: 2026, month: 11, day: 17)),
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

// Приложения курса. Это не лекции: они не читаются на занятии, не имеют
// номера в расписании и нужны как справка — по инструменту, по оформлению
// работ. Собираются той же командой, что и лекции, и попадают в комплект
// (`make pack`), поэтому реестр у них тоже здесь, а не в скриптах.
#let appendices = (
  (id: "a1-version-control", title: "Система управления версиями Git",
      date: datetime(year: 2026, month: 9, day: 1)),
  (id: "a2-code-style", title: "Требования к коду курса",
      date: datetime(year: 2026, month: 9, day: 1)),
  (id: "a3-c-standard", title: "Стандарт языка Си: справочник курса",
      date: datetime(year: 2026, month: 9, day: 1)),
)

#let appendix-meta(id) = {
  let m = appendices.find(a => a.id == id)
  if m == none { panic("course.typ: нет приложения с id «" + id + "»") }
  m
}

// Лабораторные работы. Это задания, а не чтение: у работы свой номер, лекция,
// к которой она относится, и срок сдачи. Реестр здесь по той же причине, что и
// у приложений: второй реестр — второй источник правды, а course.typ заведён
// ровно затем, чтобы его не было. В сводный том работы не входят (том — это
// лекции и приложения), но в комплект `make pack` попадают: студенту нужны и
// текст курса, и задания.
#let labs = (
  (n: 1, id: "lab-1-synthesis", lecture: 3,
      title: "Синтез автомата и его схемы",
      date: datetime(year: 2026, month: 9, day: 15)),
  (n: 2, id: "lab-2-dfa", lecture: 4,
      title: "От регулярного выражения к минимальному автомату",
      date: datetime(year: 2026, month: 9, day: 22)),
  (n: 3, id: "lab-3-format", lecture: 6,
      title: "Распознаватель формата как конечный автомат",
      date: datetime(year: 2026, month: 10, day: 6)),
  (n: 4, id: "lab-4-turing", lecture: 10,
      title: "Программа для машины Тьюринга",
      date: datetime(year: 2026, month: 11, day: 3)),
  (n: 5, id: "lab-5-three-ways", lecture: 7,
      title: "Прикладной автомат в трёх реализациях",
      date: datetime(year: 2026, month: 10, day: 13)),
)

#let lab-meta(id) = {
  let m = labs.find(l => l.id == id)
  if m == none { panic("course.typ: нет лабораторной работы с id «" + id + "»") }
  m
}
