#ifndef STATECRAFT_SUBSET_H
#define STATECRAFT_SUBSET_H

/*
 * Лекция 4. Два алгоритма, которыми конечный автомат приводится к
 * каноническому виду:
 *
 *   subset_construction — детерминизация (построение по подмножествам);
 *   dfa_minimize        — минимизация разбиением классов эквивалентности.
 *
 * Оба работают со структурами из common/fsm.h, поэтому вход можно получить
 * из практики лекции 5:
 *
 *     05-kleene "(a|b)*abb" | 04-dfa-nfa --minimize --dot
 */

#include "base_types.h"
#include "fsm.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Детерминизация. Состояние ДКА — множество состояний НКА, замкнутое по
 * ε-переходам. Возвращает false, если подмножеств оказалось больше
 * FSM_MAX_DFA_STATES.
 */
bool subset_construction(struct Dfa *dfa, const struct Nfa *nfa);

/*
 * Минимизация разбиением: классы дробятся, пока в одном классе остаются
 * состояния, которые по какому-либо символу ведут в разные классы.
 * Недостижимые состояния удаляются до начала работы.
 */
bool dfa_minimize(struct Dfa *out, const struct Dfa *in);

/*
 * Сколько подмножеств породила последняя детерминизация до удаления
 * недостижимых — для наблюдения за экспоненциальным взрывом.
 */
int subset_last_visited(void);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_SUBSET_H */
