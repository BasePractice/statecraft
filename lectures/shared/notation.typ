// lectures/shared/notation.typ — единая нотация курса.
//
// Снимает разнобой, зафиксированный в REPORT.md §3.4: в исходных четырёх
// лекциях начальное состояние обозначалось q₀ (Конечные автоматы), q₁
// (Машина Тьюринга), q₁ при q₀ заключительном (Регулярные события) и 'A'
// (Автоматное программирование); пустое слово — Λ, e, λ, ε вперемешку.
//
// Здесь это зафиксировано раз и навсегда:
//   q₀ — начальное состояние, F — множество заключительных состояний,
//   ε  — пустое слово, δ — функция переходов, λ — функция выходов.
//
// ВНИМАНИЕ: в typst 0.15 `angle.l` / `bracket.angle.l` дают
// «unknown symbol modifier». Угловые скобки — только `chevron.l` / `chevron.r`.

#let states = $Q$
#let inputs = $X$
#let outputs = $Y$
#let trans = $delta$
#let out-fn = $lambda$
#let q0 = $q_0$
#let finals = $F$
#let alphabet = $Sigma$
#let eps = $epsilon.alt$
#let empty = $nothing$

// Автомат-преобразователь: Мили и Мур различаются не составом, а сигнатурой
// функции выходов, поэтому шестёрка одна и та же.
#let mealy = $chevron.l Q, X, Y, delta, lambda, q_0 chevron.r$
#let moore = $chevron.l Q, X, Y, delta, lambda, q_0 chevron.r$

// Автомат-акцептор
#let dfa = $chevron.l Q, Sigma, delta, q_0, F chevron.r$
#let nfa = $chevron.l Q, Sigma, Delta, q_0, F chevron.r$

// Машина Тьюринга
#let tm = $chevron.l Q, Gamma, b, Sigma, delta, q_0, F chevron.r$

// Автомат с магазинной памятью
#let pda = $chevron.l Q, Sigma, Gamma, delta, q_0, Z_0, F chevron.r$

// Операции над языками и событиями
#let star(x) = $#x^*$
#let plus(x) = $#x^+$
#let concat = $dot$

// Темпоральная логика (лекция о верификации)
#let models = $tack.r.r$
#let always = $square$
#let eventually = $diamond$
#let until = $sans("U")$
#let next = $circle.small$
