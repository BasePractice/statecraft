/*
 * Реализация 2: таблица переходов.
 *
 * Автомат описан данными: двумерный массив «состояние × событие». Управляющая
 * функция состоит из одной выборки по индексу, а всё поведение читается прямо
 * из таблицы — её можно сверить с диаграммой, распечатать, породить
 * генератором или проверить на полноту программой.
 *
 * Цена — косвенность: действия вынесены в отдельные функции, и чтобы понять
 * переход, нужно перейти по указателю.
 */

#include "barrier.h"

void barrier_log_action(struct BarrierTrace *trace, enum BarrierAction action);
void barrier_log_state(struct BarrierTrace *trace, enum BarrierState state);

/*
 * Действие клетки. Возвращает состояние, в которое автомат переходит на самом
 * деле: обычно это `next` из таблицы, но клетка со сторожевым условием
 * (OPEN по tick) может вернуть текущее состояние и оставить автомат на месте.
 */
typedef enum BarrierState (*BarrierAct)(struct BarrierTable *fsm,
                                        enum BarrierState next);

struct BarrierCell {
    enum BarrierState next;
    BarrierAct action; /* NULL — переход без действий */
};

static enum BarrierState act_open(struct BarrierTable *fsm, enum BarrierState next) {
    barrier_log_action(fsm->trace, BARRIER_LAMP_ON);
    barrier_log_action(fsm->trace, BARRIER_MOTOR_UP);
    return next;
}

static enum BarrierState act_reverse(struct BarrierTable *fsm, enum BarrierState next) {
    barrier_log_action(fsm->trace, BARRIER_MOTOR_UP);
    return next;
}

static enum BarrierState act_opened(struct BarrierTable *fsm, enum BarrierState next) {
    barrier_log_action(fsm->trace, BARRIER_MOTOR_STOP);
    fsm->open_ticks = 0;
    return next;
}

static enum BarrierState act_close(struct BarrierTable *fsm, enum BarrierState next) {
    barrier_log_action(fsm->trace, BARRIER_MOTOR_DOWN);
    return next;
}

static enum BarrierState act_closed(struct BarrierTable *fsm, enum BarrierState next) {
    barrier_log_action(fsm->trace, BARRIER_MOTOR_STOP);
    barrier_log_action(fsm->trace, BARRIER_LAMP_OFF);
    return next;
}

/* Единственное сторожевое условие автомата: выдержка в состоянии OPEN. */
static enum BarrierState act_tick(struct BarrierTable *fsm, enum BarrierState next) {
    ++fsm->open_ticks;
    if (fsm->open_ticks < BARRIER_OPEN_TIMEOUT) {
        return BARRIER_OPEN;
    }
    barrier_log_action(fsm->trace, BARRIER_MOTOR_DOWN);
    return next;
}

/*
 * Вся логика автомата — в этой таблице. Строки: состояния, столбцы: события
 * (card, opened, passed, closed, tick). Клетка «остаться на месте без
 * действий» задана явно: пустых клеток в таблице нет.
 */
static const struct BarrierCell TABLE[BARRIER_STATE_COUNT][BARRIER_EVENT_COUNT] = {
    /* CLOSED  */ {{BARRIER_OPENING, act_open},
                   {BARRIER_CLOSED, NULL},
                   {BARRIER_CLOSED, NULL},
                   {BARRIER_CLOSED, NULL},
                   {BARRIER_CLOSED, NULL}},
    /* OPENING */ {{BARRIER_OPENING, NULL},
                   {BARRIER_OPEN, act_opened},
                   {BARRIER_OPENING, NULL},
                   {BARRIER_OPENING, NULL},
                   {BARRIER_OPENING, NULL}},
    /* OPEN    */ {{BARRIER_OPEN, NULL},
                   {BARRIER_OPEN, NULL},
                   {BARRIER_CLOSING, act_close},
                   {BARRIER_OPEN, NULL},
                   {BARRIER_CLOSING, act_tick}},
    /* CLOSING */ {{BARRIER_OPENING, act_reverse},
                   {BARRIER_CLOSING, NULL},
                   {BARRIER_CLOSING, NULL},
                   {BARRIER_CLOSED, act_closed},
                   {BARRIER_CLOSING, NULL}}};

void barrier_table_init(struct BarrierTable *fsm, struct BarrierTrace *trace) {
    int state;
    int event;

    fsm->state = BARRIER_CLOSED;
    fsm->open_ticks = 0;
    fsm->trace = trace;
    for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
        for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
            fsm->covered[state][event] = 0;
        }
    }
    barrier_log_state(trace, fsm->state);
}

void barrier_table_event(struct BarrierTable *fsm, enum BarrierEvent event) {
    const struct BarrierCell *cell = &TABLE[fsm->state][event];

    fsm->covered[fsm->state][event] += 1;
    if (cell->action != NULL) {
        fsm->state = cell->action(fsm, cell->next);
    } else {
        fsm->state = cell->next;
    }
    barrier_log_state(fsm->trace, fsm->state);
}

int barrier_table_coverage(const struct BarrierTable *fsm) {
    int state;
    int event;
    int hit = 0;

    for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
        for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
            if (fsm->covered[state][event] > 0) {
                ++hit;
            }
        }
    }
    return hit * 100 / (BARRIER_STATE_COUNT * BARRIER_EVENT_COUNT);
}

void barrier_table_print_uncovered(const struct BarrierTable *fsm, FILE *out) {
    int state;
    int event;

    for (state = 0; state < BARRIER_STATE_COUNT; ++state) {
        for (event = 0; event < BARRIER_EVENT_COUNT; ++event) {
            if (fsm->covered[state][event] == 0) {
                fprintf(out, "  %-8s %s\n", barrier_state_name((enum BarrierState)state),
                        barrier_event_name((enum BarrierEvent)event));
            }
        }
    }
}
