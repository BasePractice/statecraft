/*
 * Модель установки: рельс, контейнер, заслонки, таймер. Назначение и границы
 * модели — в bulk_plant.h.
 */

#include "bulk_plant.h"

#include <string.h>

int bulk_tank_position(int tank) {
    return BULK_HOME_POSITION - (tank + 1) * BULK_TANK_STEP;
}

void bulk_plant_init(struct BulkPlant *plant) {
    memset(plant, 0, sizeof(*plant));
    /* Установка начинает работу не в исходном положении: где остановились в
       прошлый раз, никто не знает — за этим и нужен выход в исходное. */
    plant->position = bulk_tank_position(BULK_TANK_COUNT - 1);
    plant->timer = -1;
    plant->running = 1;
}

/* Перемещение: клетка проходится за BULK_MOVE_TICKS тактов. */
static void move(struct BulkPlant *plant, int direction) {
    ++plant->move_progress;
    if (plant->move_progress >= BULK_MOVE_TICKS) {
        plant->move_progress = 0;
        plant->position += direction;
    }
}

static void gate_step(struct BulkPlant *plant, int tank, bool open_command, bool close_command) {
    enum BulkGate *gate = &plant->gate[tank];

    if (open_command && (*gate == BULK_GATE_CLOSED || *gate == BULK_GATE_CLOSING)) {
        *gate = BULK_GATE_OPENING;
        plant->gate_progress = 0;
    }
    if (close_command && (*gate == BULK_GATE_OPEN || *gate == BULK_GATE_OPENING)) {
        *gate = BULK_GATE_CLOSING;
        plant->gate_progress = 0;
    }

    if (*gate == BULK_GATE_OPENING || *gate == BULK_GATE_CLOSING) {
        ++plant->gate_progress;
        if (plant->gate_progress >= BULK_GATE_TICKS) {
            plant->gate_progress = 0;
            if (*gate == BULK_GATE_OPENING) {
                *gate = BULK_GATE_OPEN;
                ++plant->filled[tank];
            } else {
                *gate = BULK_GATE_CLOSED;
            }
        }
    }
}

/* Какой резервуар сейчас «свой»: тот, к которому автомат обращается. */
static int current_tank(const struct BulkFsm *fsm) {
    if (fsm->tank < 0 || fsm->tank >= BULK_TANK_COUNT) {
        return 0;
    }
    return fsm->tank;
}

void bulk_plant_tick(struct BulkPlant *plant, struct BulkFsm *fsm) {
    int tank = current_tank(fsm);

    /* --- исполнение команд ------------------------------------------------ */
    if (bulk_output(fsm, BULK_OUT_MOVE_RIGHT) && bulk_output(fsm, BULK_OUT_MOVE_LEFT)) {
        /* Две команды движения сразу — неисправность управления, а не
           «поедем куда-нибудь»: установка обязана встать. */
        plant->fault = 1;
    } else if (bulk_output(fsm, BULK_OUT_MOVE_RIGHT)) {
        if (plant->position < BULK_HOME_POSITION) {
            move(plant, 1);
        }
    } else if (bulk_output(fsm, BULK_OUT_MOVE_LEFT)) {
        move(plant, -1);
        if (plant->position < bulk_tank_position(BULK_TANK_COUNT - 1) - BULK_TANK_STEP) {
            /* Уехали за последний резервуар — упор. */
            plant->fault = 1;
        }
    } else {
        plant->move_progress = 0;
    }

    gate_step(plant, tank, bulk_output(fsm, BULK_OUT_OPEN_GATE),
              bulk_output(fsm, BULK_OUT_CLOSE_GATE));

    if (bulk_output(fsm, BULK_OUT_START_TIMER)) {
        if (plant->timer < 0) {
            plant->timer = BULK_FILL_TICKS;
            plant->timer_done = 0;
        } else if (plant->timer > 0) {
            --plant->timer;
            if (plant->timer == 0) {
                plant->timer_done = 1;
            }
        }
    } else {
        plant->timer = -1;
        plant->timer_done = 0;
    }

    /* --- показания датчиков ------------------------------------------------ */
    bulk_input_set(fsm, BULK_IN_RUNNING, plant->running != 0);
    bulk_input_set(fsm, BULK_IN_FAULT, plant->fault != 0);
    bulk_input_set(fsm, BULK_IN_AT_HOME, plant->position == BULK_HOME_POSITION);
    bulk_input_set(fsm, BULK_IN_AT_TANK, plant->position == bulk_tank_position(tank));
    bulk_input_set(fsm, BULK_IN_GATE_OPEN, plant->gate[tank] == BULK_GATE_OPEN);
    bulk_input_set(fsm, BULK_IN_GATE_CLOSED, plant->gate[tank] == BULK_GATE_CLOSED);
    bulk_input_set(fsm, BULK_IN_TIMER_DONE, plant->timer_done != 0);
}
