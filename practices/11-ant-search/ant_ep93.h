#ifndef STATECRAFT_ANT_EP93_H
#define STATECRAFT_ANT_EP93_H

/*
 * Внутренний заголовок практики: эволюционное программирование по статье
 * Angeline P. J., Pollack J. B. «Evolutionary Module Acquisition» (Proc. of
 * the 2nd Annual Conference on Evolutionary Programming, 1993).
 *
 * Наружу выходит только ant_search_run() из ant_search.h, которая выбирает
 * алгоритм по настройкам; отдельный заголовок нужен, чтобы популяционный
 * алгоритм лежал в своём файле и не мешался с простым (1 + 1)-поиском.
 */

#include "ant_search.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool ant_ep93_run(const struct AntSearch *cfg, const struct AntFsm *start,
                  struct AntSearchResult *out, const struct AntSearchLog *log);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ANT_EP93_H */
