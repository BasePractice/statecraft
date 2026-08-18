// lectures/book.typ — сводный том курса: все лекции и приложения одним PDF.
//
// Содержание и оформление берутся из тех же файлов, что и отдельные лекции
// (`src/<id>/main.body.typ`), поэтому расхождение между томом и выпуском
// отдельных лекций невозможно: правится один текст.
//
// Единственное, что приходится перечислять здесь руками, — пути включаемых
// тел: `include` в typst принимает только литеральный путь, вычислить его из
// реестра нельзя. Номера, названия и порядок при этом всё равно берутся из
// `course.typ` — здесь только пути.
//
//   typst compile --root . book.typ out/statecraft-book.pdf

#import "/template/book.typ": *
#import "/course.typ": lecture-meta, appendix-meta, lab-meta

#show: book

// Часть тома: заголовок собирается из реестра, тело приходит включением.
// Контрольные вопросы и задачи печатаются в конце своей лекции, как и в
// отдельном выпуске.
#let lec(id, body) = {
  let m = lecture-meta(id)
  part([#L.lecture~#m.n. #m.title], body, number: m.n, questions: id, tasks: id)
}

#let app(id, body) = {
  let m = appendix-meta(id)
  part([#L.appendix. #m.title], body)
}

// Лабораторная работа в томе: та же часть, что и приложение, но с номером
// работы в заголовке. Задания печатаются после справочных приложений —
// сначала то, чем пользуются при чтении, потом то, что делают руками.
#let lab(id, body) = {
  let m = lab-meta(id)
  part([#L.lab~#m.n. #m.title], body)
}

#lec("01-intro", include "src/01-intro/main.body.typ")
#lec("02-fsm", include "src/02-fsm/main.body.typ")
#lec("03-synthesis", include "src/03-synthesis/main.body.typ")
#lec("04-dfa-nfa", include "src/04-dfa-nfa/main.body.typ")
#lec("05-kleene", include "src/05-kleene/main.body.typ")
#lec("06-regexp", include "src/06-regexp/main.body.typ")
#lec("07-automata-prog", include "src/07-automata-prog/main.body.typ")
#lec("08-statecharts", include "src/08-statecharts/main.body.typ")
#lec("09-verification", include "src/09-verification/main.body.typ")
// У лекции 10 приложение с задачами вынесено в отдельный файл: в томе оно
// идёт следом за телом, как и в отдельном выпуске.
#lec("10-limits", {
  include "src/10-limits/main.body.typ"
  include "src/10-limits/tasks.body.typ"
})
#lec("11-cells", include "src/11-cells/main.body.typ")
#lec("12-takt", include "src/12-takt/main.body.typ")

// Дальше — приложения курса: нумерация частей становится буквенной.
#show: appendix-parts

#app("a1-version-control", include "src/a1-version-control/main.body.typ")
#app("a2-code-style", include "src/a2-code-style/main.body.typ")

#lab("lab-1-synthesis", include "src/lab-1-synthesis/main.body.typ")
#lab("lab-2-dfa", include "src/lab-2-dfa/main.body.typ")
#lab("lab-3-format", include "src/lab-3-format/main.body.typ")
#lab("lab-4-turing", include "src/lab-4-turing/main.body.typ")
#lab("lab-5-three-ways", include "src/lab-5-three-ways/main.body.typ")
