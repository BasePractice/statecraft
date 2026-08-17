/**
 * @file
 * Реализация модели цеха: см. loader_plant.h.
 *
 * Физика тут одна на всех: путь считается сантиметрами, время —
 * миллисекундами, а такт стенда — просто шаг, за который проходит
 * LOADER_TICK_MS. Автомат ни того, ни другого не знает: он читает датчики.
 */

#include <string.h>

#include "loader_plant.h"

static const int DELTA_ROW[ROUTE_DIRECTION_COUNT] = {-1, 0, 1, 0};
static const int DELTA_COL[ROUTE_DIRECTION_COUNT] = {0, 1, 0, -1};

/*
 * По клетке с меткой ехать можно в любую сторону, по горизонтальному участку —
 * только влево-вправо, по вертикальному — только вверх-вниз. Правило то же,
 * что у планировщика (route.c), и это не дублирование: планировщик решает,
 * куда ехать, а установка — что при такой команде физически произойдёт.
 * Разойдись они — и маршрут стал бы «правильным» только на бумаге.
 */
static bool cell_allows(const struct FactoryMap *map, int row, int col, int direction) {
    int value = factory_cell(map, FACTORY_LAYER_PATHS, row, col);

    if (value == FACTORY_PATH_H)
        return direction == ROUTE_LEFT || direction == ROUTE_RIGHT;
    if (value == FACTORY_PATH_V)
        return direction == ROUTE_UP || direction == ROUTE_DOWN;
    return value >= 1 && value <= FACTORY_POINT_MAX;
}

bool loader_plant_init(struct LoaderPlant *plant, const struct FactoryMap *map, int point,
                       int direction) {
    int row = 0;
    int col = 0;

    if (plant == NULL || map == NULL)
        return false;
    if (direction < 0 || direction >= ROUTE_DIRECTION_COUNT)
        return false;
    if (!factory_point_cell(map, point, &row, &col))
        return false;
    memset(plant, 0, sizeof(*plant));
    plant->map = map;
    plant->row = row;
    plant->col = col;
    plant->angle = direction;
    return true;
}

void loader_plant_place_pallet(struct LoaderPlant *plant, int point, int stack_code,
                               int pallet_code) {
    if (plant == NULL)
        return;
    plant->stack_point = point;
    plant->stack_code = stack_code;
    plant->pallet_code = pallet_code;
}

/*
 * Дальномер: сколько сантиметров свободного проезда впереди. Проезд кончается
 * там, где кончается покрытие (слой map, значение 13) — стена, стеллаж или
 * зона, куда погрузчику нельзя. Дальше LOADER_RANGE_MAX_CM датчик не видит,
 * и это тоже физика, а не упрощение: у ультразвукового дальномера предел есть.
 */
static int range_ahead(const struct LoaderPlant *plant) {
    int row = plant->row;
    int col = plant->col;
    int distance = 0;

    for (;;) {
        row += DELTA_ROW[plant->angle];
        col += DELTA_COL[plant->angle];
        if (factory_cell(plant->map, FACTORY_LAYER_MAP, row, col) != 13)
            break;
        distance += LOADER_CELL_CM;
        if (distance >= LOADER_RANGE_MAX_CM)
            return LOADER_RANGE_MAX_CM;
    }
    /* До препятствия — расстояние до края текущей клетки плюс свободные. */
    distance += LOADER_CELL_CM - plant->travel_cm;
    return distance > LOADER_RANGE_MAX_CM ? LOADER_RANGE_MAX_CM : distance;
}

void loader_plant_tick(struct LoaderPlant *plant, const struct LoaderCommands *commands) {
    int step_cm;

    if (plant == NULL || commands == NULL)
        return;
    ++plant->ticks;
    plant->clock_ms += LOADER_TICK_MS;
    plant->moved_this_tick = 0;

    if (commands->turn_left && commands->turn_right) {
        /* Привод не умеет крутиться в обе стороны сразу. Установка такую
           команду не исполняет, но и не скрывает: счётчик виден тесту. */
        ++plant->conflicts;
        plant->turn_elapsed_ms = 0;
        return;
    }

    if (commands->turn_left || commands->turn_right) {
        plant->turn_elapsed_ms += LOADER_TICK_MS;
        if (plant->turn_elapsed_ms >= LOADER_TURN_MS) {
            int shift = commands->turn_right ? 1 : ROUTE_DIRECTION_COUNT - 1;

            plant->angle = (plant->angle + shift) % ROUTE_DIRECTION_COUNT;
            plant->turn_elapsed_ms = 0;
        }
    } else {
        plant->turn_elapsed_ms = 0;
    }

    step_cm = LOADER_SPEED_CM_S * LOADER_TICK_MS / 1000;
    if (commands->gas && !commands->turn_left && !commands->turn_right && !plant->off_line) {
        plant->travel_cm += step_cm;
        plant->odometer_cm += step_cm;
        plant->moved_this_tick = 1;
        if (plant->travel_cm >= LOADER_CELL_CM) {
            int next_row = plant->row + DELTA_ROW[plant->angle];
            int next_col = plant->col + DELTA_COL[plant->angle];

            plant->travel_cm -= LOADER_CELL_CM;
            if (cell_allows(plant->map, plant->row, plant->col, plant->angle)
                && cell_allows(plant->map, next_row, next_col, plant->angle)) {
                plant->row = next_row;
                plant->col = next_col;
                ++plant->cells;
            } else {
                /* Впереди разметки нет: погрузчик утыкается и теряет линию.
                   Датчик тут же это показывает, и автомат обязан встать. */
                plant->off_line = 1;
                plant->travel_cm = 0;
            }
        }
    } else if (!commands->gas) {
        plant->travel_cm = 0;
    }

    if (commands->fork_up) {
        plant->fork_elapsed_ms += LOADER_TICK_MS;
        if (plant->fork_elapsed_ms > LOADER_FORK_MS)
            plant->fork_elapsed_ms = LOADER_FORK_MS;
    } else {
        plant->fork_elapsed_ms = 0;
    }
}

void loader_plant_sensors(const struct LoaderPlant *plant, struct LoaderSensors *sensors) {
    int at_stack;

    if (plant == NULL || sensors == NULL)
        return;
    memset(sensors, 0, sizeof(*sensors));
    sensors->line = plant->off_line ? 0 : 1;
    /*
     * Метка читается, пока погрузчик в её зоне: считыватель видит RFID не в
     * точке, а на отрезке. Отсюда и «зона метки» — половина клетки.
     */
    sensors->point = plant->travel_cm * 2 < LOADER_CELL_CM ? loader_plant_point(plant) : 0;
    sensors->angle = plant->angle;
    sensors->odometer = plant->odometer_cm;
    sensors->range = range_ahead(plant);
    sensors->motion = plant->moved_this_tick;

    at_stack = plant->stack_code != 0 && loader_plant_point(plant) == plant->stack_point;
    if (at_stack) {
        sensors->stack = plant->stack_code;
        /* Сканер на вилах читает код паллеты, когда вилы подошли к ней, а
           тензодатчик отзывается позже — когда паллета оказалась на вилах. */
        if (plant->fork_elapsed_ms * 2 >= LOADER_FORK_MS)
            sensors->pallet = plant->pallet_code;
        if (plant->fork_elapsed_ms >= LOADER_FORK_MS)
            sensors->load = 1;
    }
}

int loader_plant_point(const struct LoaderPlant *plant) {
    if (plant == NULL)
        return 0;
    return factory_point_at(plant->map, plant->row, plant->col);
}
