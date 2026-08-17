#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H

/**
 * @file
 * Лекция 2. Автомат «Задержка» — простейший автомат Мили.
 *
 * Выход равен входу предыдущего такта: `y(t) = x(t-1)`. Состояние и есть
 * запомненный символ, поэтому автомат состоит из двух состояний, а вся
 * реализация — из одного присваивания. На нём же построена модель
 * SimInTech, лежащая рядом: обе стороны обязаны давать одну и ту же
 * последовательность выходов.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Состояние: символ, поступивший на предыдущем такте. */
enum DelayState { DELAY_STATE_ZERO, DELAY_STATE_ONE };

/** Входной алфавит. */
enum DelayInputSymbol { DELAY_INPUT_ZERO = 0, DELAY_INPUT_ONE = 1 };

/** Выходной алфавит. */
enum DelayOutputSymbol { DELAY_OUTPUT_ZERO = 0, DELAY_OUTPUT_ONE = 1 };

struct DelayEngine {
    enum DelayState state;
};

/** Начальное состояние `q0`: до первого такта автомат помнит нуль. */
#define DEFAULT_DELAY_STATE DELAY_STATE_ZERO

/** Приводит автомат в #DEFAULT_DELAY_STATE. false — если @p engine равен NULL. */
bool delay_init(struct DelayEngine *engine);

/** То же, что #delay_init: возврат в начальное состояние посреди прогона. */
bool delay_reset(struct DelayEngine *engine);

/**
 * Такт работы: принимает входной символ и выдаёт выходной.
 *
 * Выдаётся символ, полученный на предыдущем такте, после чего состояние
 * заменяется текущим символом — то самое каноническое уравнение из лекции.
 */
enum DelayOutputSymbol delay_engine(struct DelayEngine *engine, enum DelayInputSymbol symbol);

#if defined(__cplusplus)
}
#endif

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H */
