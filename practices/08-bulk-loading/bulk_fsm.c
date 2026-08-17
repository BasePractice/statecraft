/*
 * Автомат управления загрузкой сыпучих материалов.
 *
 * Устройство и договорённости — в bulk_fsm.h. Здесь важно одно: функция такта
 * не делает ничего, кроме чтения регистра входов, смены состояния и записи
 * регистра выходов. Ни печати, ни файлов, ни ожиданий внутри — иначе автомат
 * нельзя ни проверить тестом, ни подключить к другой установке.
 */

#include "bulk_fsm.h"

#include <string.h>

static void output_set(struct BulkFsm *fsm, enum BulkOutput output, bool value) {
    if (value) {
        fsm->outputs |= 1UL << (int)output;
    } else {
        fsm->outputs &= ~(1UL << (int)output);
    }
}

void bulk_input_set(struct BulkFsm *fsm, enum BulkInput input, bool value) {
    if (value) {
        fsm->inputs |= 1UL << (int)input;
    } else {
        fsm->inputs &= ~(1UL << (int)input);
    }
}

bool bulk_input(const struct BulkFsm *fsm, enum BulkInput input) {
    return (fsm->inputs & (1UL << (int)input)) != 0;
}

bool bulk_output(const struct BulkFsm *fsm, enum BulkOutput output) {
    return (fsm->outputs & (1UL << (int)output)) != 0;
}

bool bulk_fsm_init(struct BulkFsm *fsm, unsigned long recipe) {
    int i;

    memset(fsm, 0, sizeof(*fsm));
    fsm->state = BULK_POWER_ON;

    for (i = 0; i < BULK_CYCLE_COUNT; ++i) {
        unsigned long digit = recipe % 10UL;

        /* Цифра рецепта — маска резервуаров. Если в ней есть бит, которому не
           соответствует резервуар, рецепт задан неверно: молча отбросить его
           нельзя, установка исполнит не то, что просили. */
        if (digit >= (1UL << BULK_TANK_COUNT)) {
            memset(fsm, 0, sizeof(*fsm));
            return false;
        }
        fsm->recipe[i] = (unsigned char)digit;
        recipe /= 10UL;
    }
    /* Оставшиеся разряды — тоже часть рецепта, а значит, лишние циклы. */
    if (recipe != 0UL) {
        memset(fsm, 0, sizeof(*fsm));
        return false;
    }
    return true;
}

unsigned char bulk_recipe_mask(const struct BulkFsm *fsm, int cycle) {
    if (cycle < 0 || cycle >= BULK_CYCLE_COUNT) {
        return 0;
    }
    return fsm->recipe[cycle];
}

/* Нужен ли резервуару tank материал в цикле cycle. */
static bool recipe_wants(const struct BulkFsm *fsm, int cycle, int tank) {
    if (tank < 0 || tank >= BULK_TANK_COUNT) {
        return false;
    }
    return (bulk_recipe_mask(fsm, cycle) & (unsigned char)(1U << tank)) != 0;
}

/* Первый резервуар цикла, начиная с from; BULK_TANK_COUNT — если больше нет. */
static int next_tank(const struct BulkFsm *fsm, int cycle, int from) {
    int tank;

    for (tank = from; tank < BULK_TANK_COUNT; ++tank) {
        if (recipe_wants(fsm, cycle, tank)) {
            return tank;
        }
    }
    return BULK_TANK_COUNT;
}

void bulk_fsm_tick(struct BulkFsm *fsm) {
    if (fsm == NULL) {
        return;
    }
    ++fsm->tick;

    /*
     * Авария и снятие питания обрабатываются до разбора состояний: иначе
     * пришлось бы повторять эту проверку в каждой ветви, а забытая ветвь
     * означала бы, что установка продолжает работу после аварии.
     */
    if (fsm->state != BULK_POWER_ON && fsm->state != BULK_FINISHED && fsm->state != BULK_FAULT) {
        if (bulk_input(fsm, BULK_IN_FAULT) || !bulk_input(fsm, BULK_IN_RUNNING)) {
            fsm->outputs = 0UL;
            fsm->state = BULK_FAULT;
            return;
        }
    }

    switch (fsm->state) {
    case BULK_POWER_ON:
        /* Ждём напряжения; до него никаких команд не выдаём. */
        if (bulk_input(fsm, BULK_IN_RUNNING)) {
            fsm->state = BULK_HOMING;
        }
        break;

    case BULK_HOMING:
        /* Выход в исходное положение: где остановились в прошлый раз,
           автомат не знает, и знать не должен. */
        if (bulk_input(fsm, BULK_IN_AT_HOME)) {
            output_set(fsm, BULK_OUT_MOVE_RIGHT, false);
            fsm->cycle = 0;
            fsm->state = BULK_CYCLE_IDLE;
        } else {
            output_set(fsm, BULK_OUT_MOVE_RIGHT, true);
        }
        break;

    case BULK_CYCLE_IDLE:
        output_set(fsm, BULK_OUT_CYCLE_END, false);
        if (bulk_input(fsm, BULK_IN_START_CYCLE)) {
            fsm->tank = 0;
            fsm->state = BULK_CYCLE_PICK;
        }
        break;

    case BULK_CYCLE_PICK: {
        int tank = next_tank(fsm, fsm->cycle, fsm->tank);

        if (tank >= BULK_TANK_COUNT) {
            /* В этом цикле резервуаров больше нет. */
            fsm->state = BULK_CYCLE_DONE;
        } else {
            fsm->tank = tank;
            fsm->state = BULK_MOVING;
        }
        break;
    }

    case BULK_MOVING:
        if (bulk_input(fsm, BULK_IN_AT_TANK)) {
            output_set(fsm, BULK_OUT_MOVE_LEFT, false);
            output_set(fsm, BULK_OUT_OPEN_GATE, true);
            fsm->state = BULK_OPENING;
        } else {
            output_set(fsm, BULK_OUT_MOVE_LEFT, true);
        }
        break;

    case BULK_OPENING:
        if (bulk_input(fsm, BULK_IN_GATE_OPEN)) {
            output_set(fsm, BULK_OUT_OPEN_GATE, false);
            output_set(fsm, BULK_OUT_START_TIMER, true);
            fsm->state = BULK_FILLING;
        }
        break;

    case BULK_FILLING:
        if (bulk_input(fsm, BULK_IN_TIMER_DONE)) {
            output_set(fsm, BULK_OUT_START_TIMER, false);
            /*
             * Здесь у оригинала дефект: по истечении выдержки он снова
             * подавал команду «открыть заслонку», а сигнал «закрыть» был
             * объявлен и не использован ни разу. Установка закрывалась
             * случайно — на переходе следующего состояния.
             */
            output_set(fsm, BULK_OUT_CLOSE_GATE, true);
            fsm->state = BULK_CLOSING;
        }
        break;

    case BULK_CLOSING:
        if (bulk_input(fsm, BULK_IN_GATE_CLOSED)) {
            output_set(fsm, BULK_OUT_CLOSE_GATE, false);
            /* Следующий резервуар того же цикла — если он в рецепте есть. */
            ++fsm->tank;
            fsm->state = BULK_CYCLE_PICK;
        }
        break;

    case BULK_CYCLE_DONE:
        /*
         * Набранное надо отвезти: контейнер разгружают в исходном положении.
         * Возврат нужен и по другой причине — резервуары стоят слева друг за
         * другом, и следующий цикл всегда начинается с самого правого из
         * нужных. Без возврата автомат погнал бы контейнер влево из позиции,
         * которая уже левее цели, и упёрся бы в упор: у него нет координаты,
         * только датчик «под резервуаром».
         */
        output_set(fsm, BULK_OUT_CYCLE_END, true);
        fsm->state = BULK_RETURN;
        break;

    case BULK_RETURN:
        if (bulk_input(fsm, BULK_IN_AT_HOME)) {
            output_set(fsm, BULK_OUT_MOVE_RIGHT, false);
            ++fsm->cycle;
            if (fsm->cycle >= BULK_CYCLE_COUNT) {
                output_set(fsm, BULK_OUT_DONE, true);
                fsm->state = BULK_FINISHED;
            } else {
                fsm->state = BULK_CYCLE_IDLE;
            }
        } else {
            output_set(fsm, BULK_OUT_MOVE_RIGHT, true);
        }
        break;

    case BULK_FINISHED:
    case BULK_FAULT:
    default:
        /* Оба состояния конечные: из них автомат сам не выходит. */
        break;
    }
}

bool bulk_fsm_finished(const struct BulkFsm *fsm) {
    return fsm->state == BULK_FINISHED || fsm->state == BULK_FAULT;
}

const char *bulk_state_name(enum BulkState state) {
    switch (state) {
    case BULK_POWER_ON:
        return "включение";
    case BULK_HOMING:
        return "исходное положение";
    case BULK_CYCLE_IDLE:
        return "ожидание цикла";
    case BULK_CYCLE_PICK:
        return "выбор резервуара";
    case BULK_MOVING:
        return "движение";
    case BULK_OPENING:
        return "открытие заслонки";
    case BULK_FILLING:
        return "наполнение";
    case BULK_CLOSING:
        return "закрытие заслонки";
    case BULK_CYCLE_DONE:
        return "цикл завершён";
    case BULK_RETURN:
        return "возврат";
    case BULK_FINISHED:
        return "рецепт исполнен";
    case BULK_FAULT:
        return "авария";
    default:
        return "?";
    }
}

const char *bulk_input_name(enum BulkInput input) {
    switch (input) {
    case BULK_IN_RUNNING:
        return "питание";
    case BULK_IN_START_CYCLE:
        return "пуск цикла";
    case BULK_IN_AT_HOME:
        return "исходное";
    case BULK_IN_AT_TANK:
        return "под резервуаром";
    case BULK_IN_GATE_OPEN:
        return "заслонка открыта";
    case BULK_IN_GATE_CLOSED:
        return "заслонка закрыта";
    case BULK_IN_TIMER_DONE:
        return "выдержка истекла";
    case BULK_IN_FAULT:
        return "авария";
    default:
        return "?";
    }
}

const char *bulk_output_name(enum BulkOutput output) {
    switch (output) {
    case BULK_OUT_MOVE_RIGHT:
        return "вправо";
    case BULK_OUT_MOVE_LEFT:
        return "влево";
    case BULK_OUT_OPEN_GATE:
        return "открыть";
    case BULK_OUT_CLOSE_GATE:
        return "закрыть";
    case BULK_OUT_START_TIMER:
        return "пуск выдержки";
    case BULK_OUT_CYCLE_END:
        return "конец цикла";
    case BULK_OUT_DONE:
        return "рецепт исполнен";
    default:
        return "?";
    }
}
