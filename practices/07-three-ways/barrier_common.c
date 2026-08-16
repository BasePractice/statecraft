/*
 * Общая часть трёх реализаций: имена, трасса, разбор событий.
 *
 * Действия автомата не трогают железо, а пишутся в трассу: тесты сравнивают
 * трассы трёх реализаций между собой, а без общего журнала сравнивать было бы
 * нечего.
 */

#include "barrier.h"

#include <string.h>

/*
 * Сценарии подобраны так, чтобы вместе покрыть все клетки таблицы переходов:
 * четыре игнорируемых события в каждом состоянии плюс шесть содержательных
 * переходов. Проверку покрытия делает тест, а не человек глазами.
 */
const char *const BARRIER_SCENARIOS[] = {
    "opened passed closed tick",                  /* игноры в CLOSED */
    "card card passed closed tick opened",        /* игноры в OPENING */
    "card opened opened card closed passed closed", /* игноры в OPEN */
    "card opened passed opened passed tick closed", /* игноры в CLOSING */
    "card opened passed card opened passed closed", /* реверс из CLOSING */
    "card opened tick tick tick closed"};         /* закрытие по выдержке */

const int BARRIER_SCENARIO_COUNT =
    (int)(sizeof(BARRIER_SCENARIOS) / sizeof(BARRIER_SCENARIOS[0]));

void barrier_trace_init(struct BarrierTrace *trace) {
    memset(trace, 0, sizeof(*trace));
}

/* Внутренние функции журнала — общие для всех реализаций. */
void barrier_log_action(struct BarrierTrace *trace, enum BarrierAction action);
void barrier_log_state(struct BarrierTrace *trace, enum BarrierState state);

void barrier_log_action(struct BarrierTrace *trace, enum BarrierAction action) {
    if (trace == NULL) {
        return;
    }
    if (trace->action_count >= BARRIER_TRACE_MAX) {
        trace->overflow = true;
        return;
    }
    trace->actions[trace->action_count++] = action;
}

void barrier_log_state(struct BarrierTrace *trace, enum BarrierState state) {
    if (trace == NULL) {
        return;
    }
    if (trace->state_count >= BARRIER_TRACE_MAX) {
        trace->overflow = true;
        return;
    }
    trace->states[trace->state_count++] = state;
}

const char *barrier_state_name(enum BarrierState state) {
    switch (state) {
    case BARRIER_CLOSED:
        return "CLOSED";
    case BARRIER_OPENING:
        return "OPENING";
    case BARRIER_OPEN:
        return "OPEN";
    case BARRIER_CLOSING:
        return "CLOSING";
    default:
        return "?";
    }
}

const char *barrier_event_name(enum BarrierEvent event) {
    switch (event) {
    case BARRIER_CARD:
        return "card";
    case BARRIER_OPENED:
        return "opened";
    case BARRIER_PASSED:
        return "passed";
    case BARRIER_CLOSED_LIMIT:
        return "closed";
    case BARRIER_TICK:
        return "tick";
    default:
        return "?";
    }
}

const char *barrier_action_name(enum BarrierAction action) {
    switch (action) {
    case BARRIER_MOTOR_UP:
        return "motor_up";
    case BARRIER_MOTOR_DOWN:
        return "motor_down";
    case BARRIER_MOTOR_STOP:
        return "motor_stop";
    case BARRIER_LAMP_ON:
        return "lamp_on";
    case BARRIER_LAMP_OFF:
        return "lamp_off";
    default:
        return "?";
    }
}

bool barrier_event_by_name(const char *name, enum BarrierEvent *event) {
    int i;
    for (i = 0; i < BARRIER_EVENT_COUNT; ++i) {
        enum BarrierEvent candidate = (enum BarrierEvent)i;
        if (strcmp(name, barrier_event_name(candidate)) == 0) {
            *event = candidate;
            return true;
        }
    }
    return false;
}
