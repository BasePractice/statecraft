/*
 * Реализация 3: паттерн State (GoF).
 *
 * Каждое состояние — отдельный объект с обработчиками; смена состояния — это
 * подмена указателя на структуру методов. В C++ вместо структуры была бы
 * иерархия классов с виртуальными функциями, суть та же.
 *
 * Плюс подхода — обработчики состояния собраны рядом и не смешиваются:
 * добавление состояния не трогает код остальных. Минус — переходы снова
 * размазаны, теперь по объектам, и таблицу переходов из кода не видно.
 * Ещё появляется действие «при входе в состояние» (on_enter), которого нет ни
 * в switch-, ни в табличной реализации: это уже автомат Мура.
 */

#include "barrier.h"

void barrier_log_action(struct BarrierTrace *trace, enum BarrierAction action);
void barrier_log_state(struct BarrierTrace *trace, enum BarrierState state);

static const struct BarrierStateVtbl CLOSED_VTBL;
static const struct BarrierStateVtbl OPENING_VTBL;
static const struct BarrierStateVtbl OPEN_VTBL;
static const struct BarrierStateVtbl CLOSING_VTBL;

static void go(struct BarrierObject *self, const struct BarrierStateVtbl *next) {
    self->state = next;
    if (next->on_enter != NULL) {
        next->on_enter(self);
    }
}

/* --- CLOSED --------------------------------------------------------------- */

static void closed_event(struct BarrierObject *self, enum BarrierEvent event) {
    if (event == BARRIER_CARD) {
        barrier_log_action(self->trace, BARRIER_LAMP_ON);
        go(self, &OPENING_VTBL);
    }
}

/* --- OPENING -------------------------------------------------------------- */

static void opening_enter(struct BarrierObject *self) {
    barrier_log_action(self->trace, BARRIER_MOTOR_UP);
}

static void opening_event(struct BarrierObject *self, enum BarrierEvent event) {
    if (event == BARRIER_OPENED) {
        go(self, &OPEN_VTBL);
    }
}

/* --- OPEN ----------------------------------------------------------------- */

static void open_enter(struct BarrierObject *self) {
    barrier_log_action(self->trace, BARRIER_MOTOR_STOP);
    self->open_ticks = 0;
}

static void open_event(struct BarrierObject *self, enum BarrierEvent event) {
    if (event == BARRIER_PASSED) {
        go(self, &CLOSING_VTBL);
    } else if (event == BARRIER_TICK) {
        ++self->open_ticks;
        if (self->open_ticks >= BARRIER_OPEN_TIMEOUT) {
            go(self, &CLOSING_VTBL);
        }
    }
}

/* --- CLOSING -------------------------------------------------------------- */

static void closing_enter(struct BarrierObject *self) {
    barrier_log_action(self->trace, BARRIER_MOTOR_DOWN);
}

static void closing_event(struct BarrierObject *self, enum BarrierEvent event) {
    if (event == BARRIER_CLOSED_LIMIT) {
        go(self, &CLOSED_VTBL);
    } else if (event == BARRIER_CARD) {
        go(self, &OPENING_VTBL);
    }
}

static void closed_enter(struct BarrierObject *self) {
    /* Вход в CLOSED бывает только из CLOSING: створка встала внизу. */
    barrier_log_action(self->trace, BARRIER_MOTOR_STOP);
    barrier_log_action(self->trace, BARRIER_LAMP_OFF);
}

static const struct BarrierStateVtbl CLOSED_VTBL = {
    BARRIER_CLOSED, closed_enter, closed_event};
static const struct BarrierStateVtbl OPENING_VTBL = {
    BARRIER_OPENING, opening_enter, opening_event};
static const struct BarrierStateVtbl OPEN_VTBL = {
    BARRIER_OPEN, open_enter, open_event};
static const struct BarrierStateVtbl CLOSING_VTBL = {
    BARRIER_CLOSING, closing_enter, closing_event};

void barrier_object_init(struct BarrierObject *fsm, struct BarrierTrace *trace) {
    fsm->state = &CLOSED_VTBL;
    fsm->open_ticks = 0;
    fsm->trace = trace;
    /* on_enter начального состояния не вызывается: мотор уже стоит, лампа
       погашена — это исходное положение железа, а не результат перехода. */
    barrier_log_state(trace, fsm->state->id);
}

void barrier_object_event(struct BarrierObject *fsm, enum BarrierEvent event) {
    fsm->state->on_event(fsm, event);
    barrier_log_state(fsm->trace, fsm->state->id);
}

enum BarrierState barrier_object_state(const struct BarrierObject *fsm) {
    return fsm->state->id;
}
