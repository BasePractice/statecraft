/*
 * Эволюционное программирование по статье Angeline и Pollack (1993).
 *
 * Что взято из статьи и в каком виде здесь воспроизведено:
 *
 *   постановка       тропа Санта-Фе, 89 клеток еды, 200 тактов, потолок в
 *                    32 состояния автомата;
 *   приспособленность формула (4): food + 0.01 * (1 - t / 200), где t — такт,
 *                    на котором съедена последняя клетка;
 *   мутации          равный шанс изменить состояние или переход; при выборе
 *                    состояния вероятность удаления — формула (5),
 *                    P(delete) = numstates / maxstates, иначе состояние
 *                    добавляется;
 *   число мутаций    формула (6): 1 + round(|N(0, T) * size|), где size —
 *                    число переходов родителя, а T — «температура»
 *                    приспособленности: чем ближе автомат к решению, тем
 *                    осторожнее его правят;
 *   отбор            состязательный по Фогелю: каждая особь сравнивается с
 *                    несколькими случайными соперниками, при равенстве
 *                    победитель выбирается жребием; верхняя половина
 *                    популяции даёт по одному потомку;
 *   заморозка        главный предмет статьи: часть автомата объявляется
 *                    модулем и перестаёт мутировать. Оператор compress
 *                    замораживает состояние и до пяти переходов, оператор
 *                    expand размораживает; при создании потомка вероятность
 *                    сжатия 10 %, расширения 20 %, и заморожено может быть не
 *                    более 75 % состояний и 75 % переходов.
 *
 * Чего в статье нет и что решено здесь: способ начальной инициализации
 * популяции (взяты случайные автоматы малого размера, чтобы дать место
 * росту) и точный вид «температуры» (взята доля несъеденной еды).
 */

#include "ant_ep93.h"

#include <stdlib.h>
#include <string.h>

#include "ant_random.h"

/* Сколько переходов захватывает один оператор сжатия — «up to 5» из статьи. */
#define EP93_MODULE_LINKS 5

#define EP93_COMPRESS_PERCENT 10
#define EP93_EXPAND_PERCENT 20

/* Не более 75 % состояний и переходов заморожено одновременно. */
#define EP93_FROZEN_NUMERATOR 3
#define EP93_FROZEN_DENOMINATOR 4

struct Individual {
    struct AntFsm fsm;
    unsigned char state_frozen[ANT_FSM_MAX_STATES];
    unsigned char trans_frozen[ANT_FSM_MAX_STATES][2];
    struct TrailRun run;
    double fitness;
    int wins;
};

/* --- вспомогательное ------------------------------------------------------- */

static enum AntAction random_action(struct AntRandom *r) {
    switch (ant_random_below(r, 3)) {
    case 0:
        return ANT_STEP;
    case 1:
        return ANT_TURN_LEFT;
    default:
        return ANT_TURN_RIGHT;
    }
}

static void individual_random(struct Individual *ind, int state_count, struct AntRandom *r) {
    int i;

    memset(ind, 0, sizeof(*ind));
    ind->fsm.state_count = state_count;
    for (i = 0; i < state_count; ++i) {
        ind->fsm.action[i][0] = random_action(r);
        ind->fsm.action[i][1] = random_action(r);
        ind->fsm.next[i][0] = ant_random_below(r, state_count);
        ind->fsm.next[i][1] = ant_random_below(r, state_count);
    }
}

static void individual_from(struct Individual *ind, const struct AntFsm *fsm) {
    memset(ind, 0, sizeof(*ind));
    ind->fsm = *fsm;
}

static int frozen_states(const struct Individual *ind) {
    int i;
    int count = 0;

    for (i = 0; i < ind->fsm.state_count; ++i) {
        count += ind->state_frozen[i] ? 1 : 0;
    }
    return count;
}

static int frozen_transitions(const struct Individual *ind) {
    int i;
    int count = 0;

    for (i = 0; i < ind->fsm.state_count; ++i) {
        count += ind->trans_frozen[i][0] ? 1 : 0;
        count += ind->trans_frozen[i][1] ? 1 : 0;
    }
    return count;
}

/* --- мутации --------------------------------------------------------------- */

/*
 * Правка перехода: меняется либо действие, либо состояние-приёмник.
 * Замороженные переходы не трогаются; если случайный выбор попал в
 * замороженный, делается несколько попыток и на этом всё — так проще, чем
 * строить список свободных переходов, и на поведение поиска не влияет.
 */
static void mutate_transition(struct Individual *ind, struct AntRandom *r) {
    int attempt;

    for (attempt = 0; attempt < 8; ++attempt) {
        int state = ant_random_below(r, ind->fsm.state_count);
        int input = ant_random_below(r, 2);

        if (ind->trans_frozen[state][input]) {
            continue;
        }
        if (ant_random_below(r, 2) == 0) {
            ind->fsm.action[state][input] = random_action(r);
        } else {
            ind->fsm.next[state][input] = ant_random_below(r, ind->fsm.state_count);
        }
        return;
    }
}

static void add_state(struct Individual *ind, int max_states, struct AntRandom *r) {
    int added = ind->fsm.state_count;
    int attempt;

    if (added >= max_states) {
        return;
    }
    ind->fsm.action[added][0] = random_action(r);
    ind->fsm.action[added][1] = random_action(r);
    ind->fsm.next[added][0] = ant_random_below(r, added + 1);
    ind->fsm.next[added][1] = ant_random_below(r, added + 1);
    ind->state_frozen[added] = 0;
    ind->trans_frozen[added][0] = 0;
    ind->trans_frozen[added][1] = 0;
    ind->fsm.state_count = added + 1;

    /* Недостижимое состояние поиску бесполезно: заводим на него одну ссылку. */
    for (attempt = 0; attempt < 8; ++attempt) {
        int from = ant_random_below(r, added);
        int input = ant_random_below(r, 2);

        if (!ind->trans_frozen[from][input]) {
            ind->fsm.next[from][input] = added;
            return;
        }
    }
}

/*
 * Удаление состояния: состояние 0 начальное и не удаляется, замороженные — по
 * определению тоже. Переходы, которые вели в удалённое состояние,
 * перенаправляются случайно; индексы состояний за удалённым сдвигаются.
 */
static void delete_state(struct Individual *ind, struct AntRandom *r) {
    int count = ind->fsm.state_count;
    int victim = -1;
    int attempt;
    int i;
    int input;

    if (count <= 1) {
        return;
    }
    for (attempt = 0; attempt < 8; ++attempt) {
        int candidate = 1 + ant_random_below(r, count - 1);
        if (!ind->state_frozen[candidate]) {
            victim = candidate;
            break;
        }
    }
    if (victim < 0) {
        return;
    }

    for (i = victim; i < count - 1; ++i) {
        ind->fsm.action[i][0] = ind->fsm.action[i + 1][0];
        ind->fsm.action[i][1] = ind->fsm.action[i + 1][1];
        ind->fsm.next[i][0] = ind->fsm.next[i + 1][0];
        ind->fsm.next[i][1] = ind->fsm.next[i + 1][1];
        ind->state_frozen[i] = ind->state_frozen[i + 1];
        ind->trans_frozen[i][0] = ind->trans_frozen[i + 1][0];
        ind->trans_frozen[i][1] = ind->trans_frozen[i + 1][1];
    }
    ind->fsm.state_count = count - 1;

    for (i = 0; i < ind->fsm.state_count; ++i) {
        for (input = 0; input < 2; ++input) {
            int target = ind->fsm.next[i][input];
            if (target == victim) {
                ind->fsm.next[i][input] = ant_random_below(r, ind->fsm.state_count);
            } else if (target > victim) {
                ind->fsm.next[i][input] = target - 1;
            }
        }
    }
}

/* Формула (5): чем ближе автомат к потолку размера, тем охотнее состояния
   удаляются, а не добавляются. */
static void mutate_state(struct Individual *ind, int max_states, struct AntRandom *r) {
    int percent = (ind->fsm.state_count * 100) / max_states;

    if (ant_random_chance(r, percent)) {
        delete_state(ind, r);
    } else {
        add_state(ind, max_states, r);
    }
}

/* --- заморозка модулей ------------------------------------------------------ */

static void compress(struct Individual *ind, struct AntRandom *r) {
    int count = ind->fsm.state_count;
    int limit_states = (count * EP93_FROZEN_NUMERATOR) / EP93_FROZEN_DENOMINATOR;
    int limit_trans = (2 * count * EP93_FROZEN_NUMERATOR) / EP93_FROZEN_DENOMINATOR;
    int i;

    if (frozen_states(ind) < limit_states) {
        int state = ant_random_below(r, count);
        ind->state_frozen[state] = 1;
    }
    for (i = 0; i < EP93_MODULE_LINKS; ++i) {
        int state = ant_random_below(r, count);
        int input = ant_random_below(r, 2);

        if (frozen_transitions(ind) >= limit_trans) {
            return;
        }
        ind->trans_frozen[state][input] = 1;
    }
}

static void expand(struct Individual *ind, struct AntRandom *r) {
    int count = ind->fsm.state_count;
    int i;

    for (i = 0; i < 8; ++i) {
        int state = ant_random_below(r, count);
        if (ind->state_frozen[state]) {
            ind->state_frozen[state] = 0;
            break;
        }
    }
    for (i = 0; i < EP93_MODULE_LINKS; ++i) {
        int state = ant_random_below(r, count);
        int input = ant_random_below(r, 2);
        ind->trans_frozen[state][input] = 0;
    }
}

/* --- размножение ------------------------------------------------------------ */

/*
 * Формула (6): число правок тем больше, чем дальше родитель от решения.
 * «Температура» T — доля несъеденной еды; при T = 0 потомок отличается ровно
 * одной правкой.
 */
static int mutation_count(const struct Individual *parent, int steps_limit, struct AntRandom *r) {
    double temperature;
    double size;
    double value;
    int count;

    (void)steps_limit;
    if (parent->run.total <= 0) {
        return 1;
    }
    temperature = 1.0 - (double)parent->run.eaten / (double)parent->run.total;
    size = (double)(2 * parent->fsm.state_count);
    value = ant_random_normal(r) * temperature * size;
    if (value < 0.0) {
        value = -value;
    }
    count = 1 + (int)(value + 0.5);
    if (count > 2 * parent->fsm.state_count) {
        count = 2 * parent->fsm.state_count;
    }
    return count;
}

static void reproduce(struct Individual *child, const struct Individual *parent,
                      const struct AntSearch *cfg, struct AntRandom *r, long *compressions) {
    int mutations;
    int i;

    *child = *parent;
    child->wins = 0;

    /* Сжатие и расширение выполняются до прочих мутаций — как в статье. */
    if (cfg->compression) {
        if (ant_random_chance(r, EP93_COMPRESS_PERCENT)) {
            compress(child, r);
            ++(*compressions);
        }
        if (ant_random_chance(r, EP93_EXPAND_PERCENT)) {
            expand(child, r);
        }
    }

    mutations = mutation_count(parent, cfg->steps_limit, r);
    for (i = 0; i < mutations; ++i) {
        if (ant_random_below(r, 2) == 0) {
            mutate_state(child, cfg->max_states, r);
        } else {
            mutate_transition(child, r);
        }
    }
}

/* --- отбор ------------------------------------------------------------------ */

static int compare_wins(const void *a, const void *b) {
    const struct Individual *const *left = (const struct Individual *const *)a;
    const struct Individual *const *right = (const struct Individual *const *)b;

    if ((*left)->wins != (*right)->wins) {
        return (*right)->wins - (*left)->wins;
    }
    /* При равном числе побед вперёд идёт более приспособленный: иначе порядок
       зависел бы от реализации qsort, и поиск перестал бы воспроизводиться. */
    if ((*left)->fitness > (*right)->fitness) {
        return -1;
    }
    if ((*left)->fitness < (*right)->fitness) {
        return 1;
    }
    return 0;
}

static void competition(struct Individual *population, int size, int tournaments,
                        struct AntRandom *r) {
    int i;
    int t;

    for (i = 0; i < size; ++i) {
        population[i].wins = 0;
    }
    for (i = 0; i < size; ++i) {
        for (t = 0; t < tournaments; ++t) {
            int rival = ant_random_below(r, size);

            if (rival == i) {
                continue;
            }
            if (population[i].fitness > population[rival].fitness) {
                ++population[i].wins;
            } else if (population[i].fitness == population[rival].fitness
                       && ant_random_below(r, 2) == 0) {
                ++population[i].wins;
            }
        }
    }
}

/* --- поиск ------------------------------------------------------------------ */

static void evaluate(struct Individual *ind, const struct AntSearch *cfg,
                     struct AntSearchResult *result) {
    ind->run = ant_trail_run(&ind->fsm, cfg->steps_limit);
    ind->fitness = ant_fitness_ep93(&ind->run, cfg->steps_limit);
    ++result->evaluations;
    if (ind->run.finished && result->solved_at == 0) {
        result->solved_at = result->evaluations;
    }
}

static void remember_best(const struct Individual *ind, struct AntSearchResult *out,
                          const struct AntSearchLog *log) {
    if (out->evaluations > 0 && !ant_run_better(&ind->run, &out->run)) {
        return;
    }
    out->best = ind->fsm;
    out->run = ind->run;
    ++out->improvements;
    if (log != NULL && log->improved != NULL) {
        (*log->improved)(&out->best, &out->run, out->evaluations, log->user);
    }
}

bool ant_ep93_run(const struct AntSearch *cfg, const struct AntFsm *start,
                  struct AntSearchResult *out, const struct AntSearchLog *log) {
    struct AntRandom random;
    struct AntSearchResult result;
    struct Individual *current;
    struct Individual *next;
    struct Individual **order;
    int size = cfg->population;
    int half;
    int i;
    bool solved = false;

    if (size < 2 || cfg->tournaments < 1) {
        return false;
    }
    current = (struct Individual *)malloc((size_t)size * sizeof(*current));
    next = (struct Individual *)malloc((size_t)size * sizeof(*next));
    order = (struct Individual **)malloc((size_t)size * sizeof(*order));
    if (current == NULL || next == NULL || order == NULL) {
        free(current);
        free(next);
        free(order);
        return false;
    }

    ant_random_init(&random, cfg->seed);
    memset(&result, 0, sizeof(result));
    half = size / 2;

    for (i = 0; i < size; ++i) {
        if (start != NULL) {
            individual_from(&current[i], start);
        } else {
            /* Начальный размер невелик: пусть автоматы растут отбором, а не
               достаются готовыми — иначе не видно работы формулы (5). */
            int states = 1 + ant_random_below(&random, cfg->max_states / 4 + 1);
            individual_random(&current[i], states, &random);
        }
        evaluate(&current[i], cfg, &result);
        remember_best(&current[i], &result, log);
        if (current[i].run.finished) {
            solved = true;
        }
    }

    while (!solved && result.evaluations < cfg->iterations) {
        competition(current, size, cfg->tournaments, &random);
        for (i = 0; i < size; ++i) {
            order[i] = &current[i];
        }
        qsort(order, (size_t)size, sizeof(*order), compare_wins);

        /* Верхняя половина переходит в следующее поколение как есть и даёт по
           одному потомку — так популяция не теряет найденного. */
        for (i = 0; i < half; ++i) {
            next[i] = *order[i];
        }
        for (i = half; i < size; ++i) {
            reproduce(&next[i], order[i - half], cfg, &random, &result.compressions);
            evaluate(&next[i], cfg, &result);
            remember_best(&next[i], &result, log);
            if (next[i].run.finished) {
                solved = true;
            }
        }
        memcpy(current, next, (size_t)size * sizeof(*current));
        ++result.generations;
    }

    result.iterations_done = result.evaluations;
    *out = result;

    free(current);
    free(next);
    free(order);
    return true;
}
