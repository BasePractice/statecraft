// lectures/shared/glossary.typ — обозначения и сокращения курса.
//
// Закрывает замечание REPORT.md §3.4: четыре исходные лекции использовали
// несогласованную терминологию и нотацию. Термин добавляется сюда один раз,
// в лекциях печатается через glossary-section("<id>").

#import "/template/i18n.typ": L
#import "notation.typ" as N

#let terms = (
  (
    key: "alphabet", term: "Алфавит", full: "конечное непустое множество символов",
    sym: N.alphabet,
    desc: [Символы алфавита называются буквами. Слово — конечная последовательность букв.],
    lectures: ("01-intro", "05-kleene"),
  ),
  (
    key: "eps", term: "Пустое слово", full: "слово нулевой длины", sym: N.eps,
    desc: [В исходных материалах обозначалось Λ, e и λ; в курсе принято $epsilon.alt$.],
    lectures: ("01-intro", "05-kleene"),
  ),
  (
    key: "mealy", term: "Автомат Мили", full: "автомат с выходами на переходах",
    sym: N.mealy,
    desc: [$delta: Q times X -> Q$, $lambda: Q times X -> Y$: выход зависит от состояния и входного символа.],
    lectures: ("02-fsm", "03-synthesis", "07-automata-prog"),
  ),
  (
    key: "moore", term: "Автомат Мура", full: "автомат с выходами в состояниях",
    sym: N.moore,
    desc: [$delta: Q times X -> Q$, $lambda: Q -> Y$: выход определяется только состоянием.],
    lectures: ("02-fsm", "03-synthesis"),
  ),
  (
    key: "dfa", term: "ДКА", full: "детерминированный конечный автомат", sym: N.dfa,
    desc: [Автомат-акцептор, у которого $delta$ — всюду определённая функция $Q times Sigma -> Q$.],
    lectures: ("04-dfa-nfa", "05-kleene", "06-regexp"),
  ),
  (
    key: "nfa", term: "НКА", full: "недетерминированный конечный автомат", sym: N.nfa,
    desc: [Функция переходов — отношение $Delta subset.eq Q times Sigma times Q$; допускаются $epsilon.alt$-переходы.],
    lectures: ("04-dfa-nfa", "05-kleene"),
  ),
  (
    key: "q0", term: "Начальное состояние", full: "состояние перед первым тактом",
    sym: N.q0,
    desc: [В исходных лекциях обозначалось q₀, q₁ и даже 'A'; в курсе принято $q_0$.],
    lectures: ("02-fsm", "04-dfa-nfa"),
  ),
  (
    key: "finals", term: "Заключительные состояния", full: "множество допускающих состояний",
    sym: N.finals,
    desc: [Слово допускается, если после его обработки автомат оказался в состоянии из $F$.],
    lectures: ("04-dfa-nfa", "05-kleene"),
  ),
  (
    key: "event", term: "Событие", full: "множество слов в алфавите", sym: none,
    desc: [Терминология теории автоматов; в теории формальных языков то же называют языком.],
    lectures: ("05-kleene",),
  ),
  (
    key: "regular", term: "Регулярное событие", full: "событие, построенное из элементарных операциями объединения, произведения и итерации",
    sym: none,
    desc: [Итерация $M^+$ — именно итерация, а не произведение (исправление К-6 из REPORT.md).],
    lectures: ("05-kleene", "06-regexp"),
  ),
  (
    key: "pda", term: "МП-автомат", full: "автомат с магазинной памятью", sym: N.pda,
    desc: [Конечный автомат со стеком; распознаёт контекстно-свободные языки, в том числе $a^n b^n$.],
    lectures: ("10-limits",),
  ),
  (
    key: "tm", term: "МТ", full: "машина Тьюринга", sym: N.tm,
    desc: [Конечное управление плюс бесконечная лента с возможностью записи и движения головки.],
    lectures: ("10-limits",),
  ),
  (
    key: "statechart", term: "Statechart", full: "иерархический конечный автомат Харела",
    sym: none,
    desc: [Расширение автомата вложенностью состояний, ортогональными регионами и историей.],
    lectures: ("08-statecharts",),
  ),
  (
    key: "ltl", term: "LTL", full: "линейная темпоральная логика", sym: none,
    desc: [Операторы $square$ (всегда), $diamond$ (когда-нибудь), $sans("U")$ (до тех пор пока).],
    lectures: ("09-verification",),
  ),
)

#let glossary-section(only: none) = {
  let items = if only == none { terms } else {
    terms.filter(t => only in t.at("lectures", default: ()))
  }
  if items.len() == 0 { return }
  heading(level: 1, numbering: none, L.glossary)
  for t in items {
    grid(
      columns: (3.6cm, 1fr),
      gutter: 10pt,
      align: (left + top, left + top),
      strong(t.term),
      [#emph(t.full).#if t.sym != none [ #h(0.4em) #t.sym] #linebreak() #t.desc],
    )
    v(5pt)
  }
}
