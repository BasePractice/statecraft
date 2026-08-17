#ifndef STATECRAFT_ANT_RANDOM_H
#define STATECRAFT_ANT_RANDOM_H

/*
 * Датчик случайных чисел поиска.
 *
 * Своя реализация нужна по одной причине: поиск обязан воспроизводиться по
 * зерну, а библиотечный rand() на разных платформах даёт разные
 * последовательности. Состояние датчика лежит в структуре, поэтому два поиска
 * в одном процессе не мешают друг другу — в отличие от исходного кода в
 * репозитории c_fsm, где всё состояние было глобальным.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct AntRandom {
    uint32_t state;
};

void ant_random_init(struct AntRandom *r, uint32_t seed);

/* Равномерное целое из [0, bound). bound должен быть положительным. */
int ant_random_below(struct AntRandom *r, int bound);

/* Истина с вероятностью percent процентов. */
bool ant_random_chance(struct AntRandom *r, int percent);

/*
 * Приближение нормального распределения N(0, 1) суммой двенадцати
 * равномерных величин. Точности хватает: величина нужна только как масштаб
 * числа мутаций, а не как статистическая модель.
 */
double ant_random_normal(struct AntRandom *r);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ANT_RANDOM_H */
