/*
 * Лекция 7. Один автомат — три реализации.
 *
 *   07-three-ways                     сценарий по умолчанию, трассы трёх
 *                                     реализаций и их сравнение
 *   07-three-ways card opened passed closed
 *                                     свой сценарий: события через пробел
 *   07-three-ways --coverage          покрытие переходов набором сценариев
 *
 * Программа не управляет железом: действия пишутся в трассу. Смысл примера в
 * том, что три разных по устройству программы дают одну и ту же трассу.
 */

#include <stdio.h>
#include <string.h>

#include "barrier.h"

static void print_trace(const char *title, const struct BarrierTrace *trace) {
    size_t i;

    printf("%s\n", title);
    printf("  состояния:");
    for (i = 0; i < trace->state_count; ++i) {
        printf(" %s", barrier_state_name(trace->states[i]));
    }
    printf("\n  действия: ");
    if (trace->action_count == 0) {
        printf(" (нет)");
    }
    for (i = 0; i < trace->action_count; ++i) {
        printf(" %s", barrier_action_name(trace->actions[i]));
    }
    printf("\n");
}

static bool traces_equal(const struct BarrierTrace *a, const struct BarrierTrace *b) {
    size_t i;

    if (a->action_count != b->action_count || a->state_count != b->state_count) {
        return false;
    }
    for (i = 0; i < a->action_count; ++i) {
        if (a->actions[i] != b->actions[i]) {
            return false;
        }
    }
    for (i = 0; i < a->state_count; ++i) {
        if (a->states[i] != b->states[i]) {
            return false;
        }
    }
    return true;
}

/* Подать событие всем трём реализациям сразу. */
static void feed(struct BarrierSwitch *sw, struct BarrierTable *tb, struct BarrierObject *ob,
                 enum BarrierEvent event) {
    barrier_switch_event(sw, event);
    barrier_table_event(tb, event);
    barrier_object_event(ob, event);
}

static bool run_words(int count, const char *const *words, struct BarrierTable *table,
                      struct BarrierTrace *ts, struct BarrierTrace *tt, struct BarrierTrace *to,
                      bool verbose) {
    struct BarrierSwitch sw;
    struct BarrierObject ob;
    enum BarrierEvent event;
    int i;

    barrier_trace_init(ts);
    barrier_trace_init(tt);
    barrier_trace_init(to);
    barrier_switch_init(&sw, ts);
    barrier_table_init(table, tt);
    barrier_object_init(&ob, to);

    for (i = 0; i < count; ++i) {
        if (!barrier_event_by_name(words[i], &event)) {
            fprintf(stderr,
                    "неизвестное событие «%s»: ожидались card, opened, passed, "
                    "closed, tick\n",
                    words[i]);
            return false;
        }
        if (verbose) {
            printf("%-8s → %s\n", words[i], barrier_state_name(sw.state));
        }
        feed(&sw, table, &ob, event);
    }
    return true;
}

/* Разбор строки сценария в массив слов. */
#define MAX_WORDS 32

static int split(char *line, const char **words) {
    int count = 0;
    char *p = line;

    while (*p != '\0' && count < MAX_WORDS) {
        while (*p == ' ') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        words[count++] = p;
        while (*p != '\0' && *p != ' ') {
            ++p;
        }
        if (*p == ' ') {
            *p = '\0';
            ++p;
        }
    }
    return count;
}

static int coverage_report(void) {
    struct BarrierTable table;
    struct BarrierTrace ts;
    struct BarrierTrace tt;
    struct BarrierTrace to;
    unsigned total[BARRIER_STATE_COUNT][BARRIER_EVENT_COUNT];
    char buffer[128];
    const char *words[MAX_WORDS];
    int state;
    int event;
    int i;
    int covered = 0;

    for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
        for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
            total[state][event] = 0;
        }
    }

    for (i = 0; i < BARRIER_SCENARIO_COUNT; ++i) {
        int count;
        strncpy(buffer, BARRIER_SCENARIOS[i], sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        count = split(buffer, words);
        if (!run_words(count, words, &table, &ts, &tt, &to, false)) {
            return 1;
        }
        printf("  сценарий %d: %-52s покрытие %3d %%\n", i + 1, BARRIER_SCENARIOS[i],
               barrier_table_coverage(&table));
        for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
            for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
                total[state][event] += table.covered[state][event];
            }
        }
    }

    for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
        for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
            if (total[state][event] > 0) {
                ++covered;
            }
        }
    }
    printf("\nвсего клеток таблицы: %d, покрыто: %d (%d %%)\n",
           BARRIER_STATE_COUNT * BARRIER_EVENT_COUNT, covered,
           covered * 100 / (BARRIER_STATE_COUNT * BARRIER_EVENT_COUNT));
    if (covered < BARRIER_STATE_COUNT * BARRIER_EVENT_COUNT) {
        printf("непокрытые переходы:\n");
        for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
            for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
                if (total[state][event] == 0) {
                    printf("  %-8s %s\n", barrier_state_name((enum BarrierState)state),
                           barrier_event_name((enum BarrierEvent)event));
                }
            }
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    struct BarrierTable table;
    struct BarrierTrace ts;
    struct BarrierTrace tt;
    struct BarrierTrace to;
    char buffer[128];
    const char *words[MAX_WORDS];
    int count;

    if (argc > 1 && strcmp(argv[1], "--coverage") == 0) {
        printf("Покрытие переходов набором сценариев\n\n");
        return coverage_report();
    }

    if (argc > 1) {
        int i;
        count = argc - 1;
        if (count > MAX_WORDS) {
            count = MAX_WORDS;
        }
        for (i = 0; i < count; ++i) {
            words[i] = argv[i + 1];
        }
    } else {
        strncpy(buffer, BARRIER_SCENARIOS[2], sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        count = split(buffer, words);
        printf("Сценарий по умолчанию: %s\n\n", BARRIER_SCENARIOS[2]);
    }

    printf("событие    состояние до события\n");
    if (!run_words(count, words, &table, &ts, &tt, &to, true)) {
        return 1;
    }
    printf("\n");

    print_trace("switch:", &ts);
    print_trace("таблица:", &tt);
    print_trace("состояние-объект:", &to);

    printf("\nтрассы совпадают: %s\n",
           (traces_equal(&ts, &tt) && traces_equal(&ts, &to)) ? "да" : "НЕТ");
    printf("покрытие переходов этим сценарием: %d %%\n", barrier_table_coverage(&table));
    printf("непокрытые переходы:\n");
    barrier_table_print_uncovered(&table, stdout);
    return 0;
}
