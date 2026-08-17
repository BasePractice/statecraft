#ifndef SIMPLE_REGEXP_H_
#define SIMPLE_REGEXP_H_

/**
 * @file
 * Лекция 6. Два учебных выражения, записанных автоматом вручную.
 *
 * Смысл примера в том, что распознаватель регулярного выражения не требует
 * ни библиотеки, ни памяти: это конечный автомат, и его можно выписать
 * прямо по выражению. Прикладные распознаватели той же лекции — в
 * `applied_dfa.h`, там автомат задан таблицей, а не кодом.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Какое выражение распознавать. */
enum Type {
    ONE_ZERO_PLUS, /**< `10+` — единица и хотя бы один нуль */
    XYZ,           /**< `xy*` — «x», за которым сколько угодно «y» */
    OTHER          /**< выражение не задано: #match всегда вернёт false */
};

/** Принимает ли выражение @p type строку @p text целиком. */
bool match(enum Type type, const char *text);

#if defined(__cplusplus)
}
#endif

#endif /* SIMPLE_REGEXP_H_ */
