/*
 * Реализация 1: вложенный switch.
 *
 * Самый прямой способ: внешний switch по состоянию, внутренний — по событию.
 * Переходы видны в коде, но нигде не собраны в одном месте: чтобы ответить на
 * вопрос «куда ведёт CLOSING по card», приходится читать всю функцию.
 */

#include "barrier.h"

void barrier_log_action(struct BarrierTrace *trace, enum BarrierAction action);
void barrier_log_state(struct BarrierTrace *trace, enum BarrierState state);

void barrier_switch_init(struct BarrierSwitch *fsm, struct BarrierTrace *trace) {
    fsm->state = BARRIER_CLOSED;
    fsm->open_ticks = 0;
    fsm->trace = trace;
    barrier_log_state(trace, fsm->state);
}

void barrier_switch_event(struct BarrierSwitch *fsm, enum BarrierEvent event) {
    switch (fsm->state) {
    case BARRIER_CLOSED:
        switch (event) {
        case BARRIER_CARD:
            barrier_log_action(fsm->trace, BARRIER_LAMP_ON);
            barrier_log_action(fsm->trace, BARRIER_MOTOR_UP);
            fsm->state = BARRIER_OPENING;
            break;
        default:
            break;
        }
        break;

    case BARRIER_OPENING:
        switch (event) {
        case BARRIER_OPENED:
            barrier_log_action(fsm->trace, BARRIER_MOTOR_STOP);
            fsm->open_ticks = 0;
            fsm->state = BARRIER_OPEN;
            break;
        default:
            break;
        }
        break;

    case BARRIER_OPEN:
        switch (event) {
        case BARRIER_PASSED:
            barrier_log_action(fsm->trace, BARRIER_MOTOR_DOWN);
            fsm->state = BARRIER_CLOSING;
            break;
        case BARRIER_TICK:
            ++fsm->open_ticks;
            if (fsm->open_ticks >= BARRIER_OPEN_TIMEOUT) {
                barrier_log_action(fsm->trace, BARRIER_MOTOR_DOWN);
                fsm->state = BARRIER_CLOSING;
            }
            break;
        default:
            break;
        }
        break;

    case BARRIER_CLOSING:
        switch (event) {
        case BARRIER_CLOSED_LIMIT:
            barrier_log_action(fsm->trace, BARRIER_MOTOR_STOP);
            barrier_log_action(fsm->trace, BARRIER_LAMP_OFF);
            fsm->state = BARRIER_CLOSED;
            break;
        case BARRIER_CARD:
            /* Машина подъехала, пока створка шла вниз: реверс. */
            barrier_log_action(fsm->trace, BARRIER_MOTOR_UP);
            fsm->state = BARRIER_OPENING;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    barrier_log_state(fsm->trace, fsm->state);
}
