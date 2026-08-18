// lectures/template/i18n.typ — русские подписи, даты, обозначения приложений.

#let L = (
  contents: "Содержание",
  figures: "Список рисунков",
  tables: "Список таблиц",
  listings: "Список листингов",
  refs: "Список литературы",
  figure: "Рисунок",
  table: "Таблица",
  listing: "Листинг",
  figure-short: "рис.",
  table-short: "табл.",
  listing-short: "листинг",
  equation: "Формула",
  appendix: "Приложение",
  lecture: "Лекция",
  glossary: "Обозначения и сокращения",
  questions: "Контрольные вопросы",
  tasks: "Задачи для самостоятельной работы",
  sources: "Материал лекции",
  definition: "Определение",
  theorem: "Теорема",
  lemma: "Лемма",
  corollary: "Следствие",
  example: "Пример",
  remark: "Замечание",
  proof: "Доказательство",
  exercise: "Задание",
  todo: "НЕ НАПИСАНО",
  version: "версия",
  // Подзаголовок сводного тома: он не лекция и не приложение, а весь курс.
  book-subtitle: "Полный курс: лекции и приложения",
  draft: "ЧЕРНОВИК",
)

#let months-gen = (
  "января", "февраля", "марта", "апреля", "мая", "июня",
  "июля", "августа", "сентября", "октября", "ноября", "декабря",
)

#let ru-date(d) = [#d.day() #months-gen.at(d.month() - 1) #d.year() г.]

// ГОСТ 7.32: в обозначениях приложений не используются Ё, З, Й, О, Ч, Ь, Ы, Ъ.
#let ru-appendix-letters = (
  "А", "Б", "В", "Г", "Д", "Е", "Ж", "И", "К", "Л", "М", "Н", "П",
  "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ш", "Щ", "Э", "Ю", "Я",
)

#let appendix-numbering = (..nums) => {
  let ns = nums.pos()
  let head = ru-appendix-letters.at(ns.first() - 1)
  if ns.len() == 1 { head } else { head + "." + ns.slice(1).map(str).join(".") }
}
