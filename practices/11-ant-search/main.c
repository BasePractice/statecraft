/*
 * Лекция 11. Поиск автомата для задачи об умном муравье.
 *
 *   11-ant-search [ключи]
 *
 *     --algorithm hill|ep93  какой алгоритм поиска (по умолчанию hill)
 *     --states N        потолок числа состояний
 *     --iterations N    бюджет: мутаций для hill, оценённых автоматов для ep93
 *     --steps N         лимит тактов на прогон муравья
 *     --seed N          зерно датчика: один и тот же поиск воспроизводится
 *     --grow N          hill: после N неудач подряд добавлять состояние
 *     --population N    ep93: размер популяции (в статье 300)
 *     --tournaments N   ep93: состязаний на особь при отборе (в статье 5)
 *     --no-compression  ep93: выключить заморозку модулей
 *     --from <таблица>  начать с готового автомата в записи «3.0.0.1:…»
 *                       или с именованной стратегии практики 11-cells
 *                       (lookaround, probe, scan, evolved)
 *     --quiet           не печатать журнал улучшений
 *
 * Ключ --algorithm ep93 переключает поиск на постановку статьи Angeline и
 * Pollack (1993): 32 состояния, лимит 200 тактов, популяция и заморозка
 * модулей. Прочие ключи, заданные после него, эти значения перекрывают.
 *
 * Печатается журнал улучшений в том же виде, в каком он приводится в лекции:
 *
 *   [315] 3.0.0.1:3.0.0.2:3.0.0.3:3.0.0.4:3.0.2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ant_search.h"

static int usage(void) {
    printf("Использование:\n");
    printf("  11-ant-search [--algorithm hill|ep93] [--states N] [--iterations N]\n");
    printf("                [--steps N] [--seed N] [--grow N]\n");
    printf("                [--population N] [--tournaments N] [--no-compression]\n");
    printf("                [--from <таблица|стратегия>] [--quiet]\n");
    return 1;
}

static void print_improvement(const struct AntFsm *fsm, const struct TrailRun *run, long iteration,
                              void *user) {
    char spec[ANT_FSM_SPEC_SIZE];

    (void)user;
    if (!ant_fsm_format(fsm, spec, sizeof(spec))) {
        return;
    }
    if (run->finished) {
        printf("[%3d] %s\n", run->steps, spec);
    } else {
        printf("[ -- ] съедено %d из %d, оценок %ld: %s\n", run->eaten, run->total, iteration,
               spec);
    }
}

/* Разбор числового ключа: возвращает false, если значения нет или оно не число. */
static bool number_arg(int argc, char **argv, int *i, long *value) {
    char *end;
    long parsed;

    if (*i + 1 >= argc) {
        return false;
    }
    parsed = strtol(argv[*i + 1], &end, 10);
    if (end == argv[*i + 1] || *end != '\0') {
        return false;
    }
    *value = parsed;
    ++(*i);
    return true;
}

int main(int argc, char **argv) {
    struct AntSearch cfg;
    struct AntSearchResult result;
    struct AntSearchLog log;
    struct AntFsm start;
    bool has_start = false;
    bool quiet = false;
    char spec[ANT_FSM_SPEC_SIZE];
    long value;
    int i;

    ant_search_defaults(&cfg);

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            ++i;
            if (strcmp(argv[i], "ep93") == 0) {
                ant_search_defaults_ep93(&cfg);
            } else if (strcmp(argv[i], "hill") != 0) {
                fprintf(stderr, "неизвестный алгоритм «%s»\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--states") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.max_states = (int)value;
        } else if (strcmp(argv[i], "--iterations") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.iterations = value;
        } else if (strcmp(argv[i], "--steps") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.steps_limit = (int)value;
        } else if (strcmp(argv[i], "--seed") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.seed = (uint32_t)value;
        } else if (strcmp(argv[i], "--grow") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.stall_limit = value;
        } else if (strcmp(argv[i], "--population") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.population = (int)value;
        } else if (strcmp(argv[i], "--tournaments") == 0 && number_arg(argc, argv, &i, &value)) {
            cfg.tournaments = (int)value;
        } else if (strcmp(argv[i], "--no-compression") == 0) {
            cfg.compression = false;
        } else if (strcmp(argv[i], "--from") == 0 && i + 1 < argc) {
            ++i;
            if (!ant_fsm_by_name(argv[i], &start) && !ant_fsm_parse(&start, argv[i])) {
                fprintf(stderr, "не разобран начальный автомат «%s»\n", argv[i]);
                return 1;
            }
            has_start = true;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else {
            return usage();
        }
    }

    log.improved = quiet ? NULL : print_improvement;
    log.user = NULL;

    if (cfg.algorithm == ANT_SEARCH_EP93) {
        printf("Поиск (Angeline, Pollack, 1993): популяция %d, состязаний %d, заморозка %s\n",
               cfg.population, cfg.tournaments, cfg.compression ? "включена" : "выключена");
        printf("До %d состояний, бюджет %ld оценок, зерно %lu, лимит %d тактов\n\n", cfg.max_states,
               cfg.iterations, (unsigned long)cfg.seed, cfg.steps_limit);
    } else {
        printf("Поиск ((1 + 1)-ЭС): до %d состояний, %ld итераций, зерно %lu, лимит %d тактов\n\n",
               cfg.max_states, cfg.iterations, (unsigned long)cfg.seed, cfg.steps_limit);
    }

    if (!ant_search_run(&cfg, has_start ? &start : NULL, &result, &log)) {
        fprintf(stderr, "негодные настройки поиска\n");
        return 1;
    }

    printf("\nОценено автоматов: %ld", result.evaluations);
    if (cfg.algorithm == ANT_SEARCH_EP93) {
        printf(", поколений: %ld, заморозок: %ld", result.generations, result.compressions);
    }
    printf(", улучшений: %ld\n", result.improvements);
    if (result.solved_at > 0) {
        printf("Тропа впервые пройдена на оценке № %ld\n", result.solved_at);
    }
    printf("Съедено %d из %d за %d тактов%s\n", result.run.eaten, result.run.total,
           result.run.steps, result.run.finished ? " (тропа пройдена)" : "");
    if (ant_fsm_format(&result.best, spec, sizeof(spec))) {
        printf("Автомат из %d состояний: %s\n", result.best.state_count, spec);
    }
    /* Найденный автомат проверяется прогоном, а не принимается на веру:
       это тот же порядок, что в лекции 7 — таблице переходов, порождённой
       машиной, «на глаз» доверять нельзя. */
    printf("\n");
    ant_trail_print(&result.best, cfg.steps_limit, stdout);
    return 0;
}
