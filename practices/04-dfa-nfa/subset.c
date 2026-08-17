#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "subset.h"

static int last_visited = 0;

int subset_last_visited(void) {
    return last_visited;
}

/* --- детерминизация --------------------------------------------------- */

/*
 * Подмножества состояний НКА хранятся как массивы флагов: множество —
 * строка таблицы sets. Так алгоритм читается буквально по определению, а
 * битовые маски не отвлекают от сути.
 */
struct SubsetTable {
    char sets[FSM_MAX_DFA_STATES][FSM_MAX_STATES];
    int count;
};

static int subset_find(const struct SubsetTable *table, const char *set, int state_count) {
    int i;

    for (i = 0; i < table->count; ++i) {
        if (memcmp(table->sets[i], set, (size_t)state_count) == 0)
            return i;
    }
    return FSM_NO_STATE;
}

static int subset_add(struct SubsetTable *table, const char *set, int state_count) {
    int index = subset_find(table, set, state_count);

    if (index != FSM_NO_STATE)
        return index;
    if (table->count >= FSM_MAX_DFA_STATES)
        return FSM_NO_STATE;
    memcpy(table->sets[table->count], set, (size_t)state_count);
    return table->count++;
}

static bool subset_is_empty(const char *set, int state_count) {
    int i;

    for (i = 0; i < state_count; ++i) {
        if (set[i])
            return false;
    }
    return true;
}

bool subset_construction(struct Dfa *dfa, const struct Nfa *nfa) {
    struct SubsetTable table;
    char set[FSM_MAX_STATES];
    int queue_head = 0;
    int s;

    assert(dfa != NULL && nfa != NULL);
    dfa_init(dfa);
    table.count = 0;
    last_visited = 0;

    if (nfa->start == FSM_NO_STATE) {
        fprintf(stderr, "subset: у НКА не задано начальное состояние\n");
        return false;
    }

    dfa->symbol_count = nfa->symbol_count;
    memcpy(dfa->symbols, nfa->symbols, sizeof(dfa->symbols));

    /* Начальное состояние ДКА — ε-замыкание {q0}. */
    memset(set, 0, sizeof(set));
    set[nfa->start] = 1;
    nfa_epsilon_closure(nfa, set);
    dfa->start = subset_add(&table, set, nfa->state_count);

    /* Очередь — сама таблица подмножеств: новые попадают в её конец. */
    while (queue_head < table.count) {
        int current = queue_head++;

        for (s = 0; s < nfa->symbol_count; ++s) {
            char next[FSM_MAX_STATES];
            int from;
            int to;
            int index;

            memset(next, 0, sizeof(next));
            for (from = 0; from < nfa->state_count; ++from) {
                if (!table.sets[current][from])
                    continue;
                for (to = 0; to < nfa->state_count; ++to) {
                    if (nfa->trans[from][s][to])
                        next[to] = 1;
                }
            }
            nfa_epsilon_closure(nfa, next);

            /* Пустое подмножество — это «дьявольское» состояние. Оно не
               меняет языка, поэтому переход просто не создаётся: ДКА
               остаётся неполным, зато не растёт. */
            if (subset_is_empty(next, nfa->state_count))
                continue;

            index = subset_add(&table, next, nfa->state_count);
            if (index == FSM_NO_STATE) {
                fprintf(stderr, "subset: подмножеств больше %d\n", FSM_MAX_DFA_STATES);
                return false;
            }
            dfa->trans[current][s] = index;
        }
    }

    dfa->state_count = table.count;
    last_visited = table.count;

    /* Заключительные — те подмножества, что пересекаются с F. */
    {
        int i;
        int q;
        for (i = 0; i < table.count; ++i) {
            for (q = 0; q < nfa->state_count; ++q) {
                if (table.sets[i][q] && nfa->final[q]) {
                    dfa->final[i] = 1;
                    break;
                }
            }
        }
    }
    return true;
}

/* --- минимизация ------------------------------------------------------ */

static void mark_reachable(const struct Dfa *dfa, char *reachable) {
    int changed = 1;
    int i;
    int s;

    memset(reachable, 0, (size_t)FSM_MAX_DFA_STATES);
    if (dfa->start == FSM_NO_STATE)
        return;
    reachable[dfa->start] = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < dfa->state_count; ++i) {
            if (!reachable[i])
                continue;
            for (s = 0; s < dfa->symbol_count; ++s) {
                int to = dfa->trans[i][s];
                if (to != FSM_NO_STATE && !reachable[to]) {
                    reachable[to] = 1;
                    changed = 1;
                }
            }
        }
    }
}

bool dfa_minimize(struct Dfa *out, const struct Dfa *in) {
    char reachable[FSM_MAX_DFA_STATES];
    int class_of[FSM_MAX_DFA_STATES];
    int next_class[FSM_MAX_DFA_STATES];
    int representative[FSM_MAX_DFA_STATES];
    /* Инициализация не для компилятора, а для читателя и анализатора:
       значение задаётся первым же проходом цикла, но доказать это статически
       нельзя, и cppcheck справедливо сообщает о чтении неинициализированного. */
    int class_count = 0;
    int changed = 1;
    int i;
    int j;
    int s;

    assert(out != NULL && in != NULL);
    mark_reachable(in, reachable);

    /* Начальное разбиение: заключительные и все остальные. */
    for (i = 0; i < in->state_count; ++i) {
        class_of[i] = reachable[i] ? (in->final[i] ? 1 : 0) : FSM_NO_STATE;
    }

    while (changed) {
        changed = 0;
        class_count = 0;
        for (i = 0; i < in->state_count; ++i) {
            next_class[i] = FSM_NO_STATE;
        }

        for (i = 0; i < in->state_count; ++i) {
            if (class_of[i] == FSM_NO_STATE)
                continue;
            if (next_class[i] != FSM_NO_STATE)
                continue;

            /* i задаёт новый класс; собираем в него всех, кто неотличим
               от i за один шаг: тот же старый класс и те же классы
               преемников по каждому символу. */
            next_class[i] = class_count;
            for (j = i + 1; j < in->state_count; ++j) {
                bool same = true;

                if (class_of[j] == FSM_NO_STATE || next_class[j] != FSM_NO_STATE)
                    continue;
                if (class_of[j] != class_of[i])
                    continue;

                for (s = 0; s < in->symbol_count && same; ++s) {
                    int ti = in->trans[i][s];
                    int tj = in->trans[j][s];
                    int ci = (ti == FSM_NO_STATE) ? FSM_NO_STATE : class_of[ti];
                    int cj = (tj == FSM_NO_STATE) ? FSM_NO_STATE : class_of[tj];
                    if (ci != cj)
                        same = false;
                }
                if (same)
                    next_class[j] = class_count;
            }
            ++class_count;
        }

        for (i = 0; i < in->state_count; ++i) {
            if (class_of[i] != next_class[i]) {
                changed = 1;
            }
            class_of[i] = next_class[i];
        }
    }

    /* Класс → состояние минимального автомата. */
    dfa_init(out);
    out->symbol_count = in->symbol_count;
    memcpy(out->symbols, in->symbols, sizeof(out->symbols));
    for (i = 0; i < class_count; ++i) {
        representative[i] = FSM_NO_STATE;
    }
    for (i = 0; i < in->state_count; ++i) {
        if (class_of[i] != FSM_NO_STATE && representative[class_of[i]] == FSM_NO_STATE)
            representative[class_of[i]] = i;
    }

    out->state_count = class_count;
    for (i = 0; i < class_count; ++i) {
        int rep = representative[i];
        out->final[i] = in->final[rep];
        for (s = 0; s < in->symbol_count; ++s) {
            int to = in->trans[rep][s];
            out->trans[i][s] = (to == FSM_NO_STATE) ? FSM_NO_STATE : class_of[to];
        }
    }
    out->start = class_of[in->start];
    return true;
}
