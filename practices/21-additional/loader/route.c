/**
 * @file
 * Поиск маршрута: алгоритм Дейкстры по состояниям «клетка + направление».
 *
 * Почему не поиск в ширину по клеткам, как в оригинале. Пока все шаги стоят
 * одинаково, ширина даёт кратчайший путь, но кратчайший в клетках — не то же
 * самое, что быстрейший: поворот погрузчика на 90° занимает время, сравнимое
 * с несколькими клетками пути. Как только у поворота появляется цена, граф
 * перестаёт быть однородным, и правильный ответ даёт Дейкстра по расширенному
 * состоянию: в вершину «клетка» приходят с разных сторон, и цена дальнейшего
 * пути от стороны зависит.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "route.h"

#include "loader_plant.h"

/* Смещения по направлениям: индексы совпадают с enum RouteDirection. */
static const int DELTA_ROW[ROUTE_DIRECTION_COUNT] = {-1, 0, 1, 0};
static const int DELTA_COL[ROUTE_DIRECTION_COUNT] = {0, 1, 0, -1};

struct HeapItem {
    int cost;
    int state;
};

struct Heap {
    struct HeapItem *item;
    int count;
    int capacity;
};

static bool heap_push(struct Heap *heap, int cost, int state) {
    int child;

    if (heap->count == heap->capacity) {
        int next = heap->capacity == 0 ? 256 : heap->capacity * 2;
        struct HeapItem *grown
                = (struct HeapItem *)realloc(heap->item, (size_t)next * sizeof(struct HeapItem));

        if (grown == NULL)
            return false;
        heap->item = grown;
        heap->capacity = next;
    }
    child = heap->count++;
    heap->item[child].cost = cost;
    heap->item[child].state = state;
    while (child > 0) {
        int parent = (child - 1) / 2;

        if (heap->item[parent].cost <= heap->item[child].cost)
            break;
        {
            struct HeapItem swap = heap->item[parent];

            heap->item[parent] = heap->item[child];
            heap->item[child] = swap;
        }
        child = parent;
    }
    return true;
}

static bool heap_pop(struct Heap *heap, struct HeapItem *out) {
    int parent = 0;

    if (heap->count == 0)
        return false;
    *out = heap->item[0];
    heap->item[0] = heap->item[--heap->count];
    for (;;) {
        int left = parent * 2 + 1;
        int right = left + 1;
        int smallest = parent;

        if (left < heap->count && heap->item[left].cost < heap->item[smallest].cost)
            smallest = left;
        if (right < heap->count && heap->item[right].cost < heap->item[smallest].cost)
            smallest = right;
        if (smallest == parent)
            break;
        {
            struct HeapItem swap = heap->item[parent];

            heap->item[parent] = heap->item[smallest];
            heap->item[smallest] = swap;
        }
        parent = smallest;
    }
    return true;
}

/* Пускает ли клетка движение в этом направлении. */
static bool cell_allows(const struct FactoryMap *map, int row, int col, int direction) {
    int value = factory_cell(map, FACTORY_LAYER_PATHS, row, col);

    if (value == 0)
        return false;
    if (value == FACTORY_PATH_H)
        return direction == ROUTE_LEFT || direction == ROUTE_RIGHT;
    if (value == FACTORY_PATH_V)
        return direction == ROUTE_UP || direction == ROUTE_DOWN;
    return value >= 1 && value <= FACTORY_POINT_MAX; /* метка: любое направление */
}

/* Поворачивать можно только там, где стоит метка: см. route.h. */
static bool cell_allows_turn(const struct FactoryMap *map, int row, int col) {
    return factory_point_at(map, row, col) != 0;
}

int route_turn_count(int from_direction, int to_direction) {
    int diff = (to_direction - from_direction) % ROUTE_DIRECTION_COUNT;

    if (diff < 0)
        diff += ROUTE_DIRECTION_COUNT;
    return diff <= 2 ? diff : ROUTE_DIRECTION_COUNT - diff;
}

const char *route_direction_name(int direction) {
    switch (direction) {
    case ROUTE_UP:
        return "вверх";
    case ROUTE_RIGHT:
        return "вправо";
    case ROUTE_DOWN:
        return "вниз";
    case ROUTE_LEFT:
        return "влево";
    default:
        return "?";
    }
}

/*
 * Восстанавливает маршрут по массиву предшественников и разворачивает его в
 * отрезки «ехать до метки». Отрезок кончается на любой встреченной метке, а не
 * только на конечной: автомат сверяет каждую метку с той, которую ждёт, и
 * пропущенная в плане метка для него — авария.
 */
static bool collect_steps(const struct FactoryMap *map, const int *previous, int final_state,
                          struct Route *route) {
    int chain[ROUTE_MAX_STEPS * FACTORY_MAX_SIDE];
    int length = 0;
    int state = final_state;
    int i;
    int cells = 0;
    int direction = route->start_direction;

    while (state >= 0) {
        if (length >= (int)(sizeof(chain) / sizeof(chain[0])))
            return false;
        chain[length++] = state;
        state = previous[state];
    }
    /* chain лежит от конца к началу — идём по нему в обратную сторону. */
    route->count = 0;
    route->cells = 0;
    route->turns = 0;
    for (i = length - 2; i >= 0; --i) {
        int current = chain[i];
        int previous_state = chain[i + 1];
        int current_cell = current / ROUTE_DIRECTION_COUNT;
        int previous_cell = previous_state / ROUTE_DIRECTION_COUNT;
        int current_direction = current % ROUTE_DIRECTION_COUNT;
        int previous_direction = previous_state % ROUTE_DIRECTION_COUNT;

        if (current_cell == previous_cell) {
            /* поворот на месте */
            route->turns += route_turn_count(previous_direction, current_direction);
            direction = current_direction;
            continue;
        }
        ++cells;
        route->cells += 1;
        direction = current_direction;
        {
            int row = current_cell / map->cols;
            int col = current_cell % map->cols;
            int point = factory_point_at(map, row, col);

            if (point != 0) {
                if (route->count >= ROUTE_MAX_STEPS)
                    return false;
                route->step[route->count].point = point;
                route->step[route->count].direction = direction;
                route->step[route->count].cells = cells;
                ++route->count;
                cells = 0;
            }
        }
    }
    return cells == 0; /* маршрут обязан заканчиваться меткой */
}

bool route_find(const struct FactoryMap *map, int from, int to, int start_direction, int turn_cost,
                struct Route *route) {
    int start_row = 0;
    int start_col = 0;
    int cell_count;
    int state_count;
    int *distance;
    int *previous;
    struct Heap heap;
    int best_state = -1;
    int best_cost = INT_MAX;
    bool ok = false;

    if (map == NULL || route == NULL || map->rows <= 0 || map->cols <= 0)
        return false;
    if (start_direction < 0 || start_direction >= ROUTE_DIRECTION_COUNT || turn_cost < 0)
        return false;
    if (!factory_point_cell(map, from, &start_row, &start_col))
        return false;
    if (!factory_point_cell(map, to, NULL, NULL))
        return false;

    memset(route, 0, sizeof(*route));
    route->from = from;
    route->to = to;
    route->start_direction = start_direction;
    if (from == to)
        return true; /* уже на месте: маршрут пуст, но это не отказ */

    cell_count = map->rows * map->cols;
    state_count = cell_count * ROUTE_DIRECTION_COUNT;
    distance = (int *)malloc((size_t)state_count * sizeof(int));
    previous = (int *)malloc((size_t)state_count * sizeof(int));
    if (distance == NULL || previous == NULL) {
        free(distance);
        free(previous);
        return false;
    }
    {
        int i;

        for (i = 0; i < state_count; ++i) {
            distance[i] = INT_MAX;
            previous[i] = -1;
        }
    }
    heap.item = NULL;
    heap.count = 0;
    heap.capacity = 0;

    {
        int start_state
                = (start_row * map->cols + start_col) * ROUTE_DIRECTION_COUNT + start_direction;

        distance[start_state] = 0;
        if (!heap_push(&heap, 0, start_state)) {
            free(distance);
            free(previous);
            free(heap.item);
            return false;
        }
    }

    while (heap.count > 0) {
        struct HeapItem item;
        int row;
        int col;
        int direction;
        int cell;

        if (!heap_pop(&heap, &item))
            break;
        if (item.cost > distance[item.state])
            continue; /* устаревшая запись: в кучу кладут, а не понижают ключ */
        cell = item.state / ROUTE_DIRECTION_COUNT;
        direction = item.state % ROUTE_DIRECTION_COUNT;
        row = cell / map->cols;
        col = cell % map->cols;
        if (factory_point_at(map, row, col) == to) {
            if (item.cost < best_cost) {
                best_cost = item.cost;
                best_state = item.state;
            }
            continue; /* дальше цели идти незачем */
        }
        /* движение вперёд */
        {
            int next_row = row + DELTA_ROW[direction];
            int next_col = col + DELTA_COL[direction];

            if (cell_allows(map, row, col, direction)
                && cell_allows(map, next_row, next_col, direction)) {
                int next_state
                        = (next_row * map->cols + next_col) * ROUTE_DIRECTION_COUNT + direction;

                if (distance[item.state] + 1 < distance[next_state]) {
                    distance[next_state] = distance[item.state] + 1;
                    previous[next_state] = item.state;
                    if (!heap_push(&heap, distance[next_state], next_state))
                        break;
                }
            }
        }
        /* поворот на месте — только на метке */
        if (cell_allows_turn(map, row, col)) {
            int turn;

            for (turn = -1; turn <= 1; turn += 2) {
                int next_direction
                        = (direction + turn + ROUTE_DIRECTION_COUNT) % ROUTE_DIRECTION_COUNT;
                int next_state = cell * ROUTE_DIRECTION_COUNT + next_direction;

                if (distance[item.state] + turn_cost < distance[next_state]) {
                    distance[next_state] = distance[item.state] + turn_cost;
                    previous[next_state] = item.state;
                    if (!heap_push(&heap, distance[next_state], next_state))
                        break;
                }
            }
        }
    }

    if (best_state >= 0)
        ok = collect_steps(map, previous, best_state, route);
    if (!ok)
        memset(route, 0, sizeof(*route));
    free(distance);
    free(previous);
    free(heap.item);
    return ok;
}

const char *plan_command_name(int code) {
    switch (code) {
    case LOADER_CMD_NONE:
        return "нет команды";
    case LOADER_CMD_TURN_LEFT:
        return "повернуть налево";
    case LOADER_CMD_TURN_RIGHT:
        return "повернуть направо";
    case LOADER_CMD_DRIVE:
        return "ехать до метки";
    case LOADER_CMD_LIFT:
        return "поднять груз";
    default:
        return "?";
    }
}

void loader_timing_default(struct LoaderTiming *timing) {
    if (timing == NULL)
        return;
    /*
     * Числа взяты из паспорта установки (loader_plant.h): клетка — полметра,
     * маршевая скорость — метр в секунду, поворот — 0.6 с, подъём вил — 0.8 с.
     * Здесь они записаны временем, а не тактами: планировщик задаёт срок в
     * физических величинах, потому что его исполняет физическая машина.
     */
    timing->cell_ms = LOADER_CELL_CM * 1000 / LOADER_SPEED_CM_S;
    timing->turn_ms = LOADER_TURN_MS;
    timing->lift_ms = LOADER_FORK_MS;
    timing->margin_ms = 1000;
}

void plan_options_default(struct PlanOptions *options) {
    if (options == NULL)
        return;
    memset(options, 0, sizeof(*options));
    loader_timing_default(&options->timing);
}

static bool plan_add(struct Plan *plan, int code, int point, int extra, int timeout_ms) {
    if (plan->count >= PLAN_MAX_STEPS)
        return false;
    plan->step[plan->count].code = code;
    plan->step[plan->count].point = point;
    plan->step[plan->count].extra = extra;
    plan->step[plan->count].timeout_ms = timeout_ms;
    ++plan->count;
    return true;
}

bool plan_build(const struct Route *route, const struct PlanOptions *options, struct Plan *plan) {
    struct PlanOptions defaults;
    int direction;
    int i;

    if (route == NULL || plan == NULL)
        return false;
    if (options == NULL) {
        plan_options_default(&defaults);
        options = &defaults;
    }
    memset(plan, 0, sizeof(*plan));
    direction = route->start_direction;
    for (i = 0; i < route->count; ++i) {
        int wanted = route->step[i].direction;
        int clockwise = (wanted - direction + ROUTE_DIRECTION_COUNT) % ROUTE_DIRECTION_COUNT;
        int turns;
        int code;

        /*
         * Разворот (clockwise == 2) стоит одинаково в обе стороны; крутим
         * направо. Оригинал в этом месте считал число поворотов циклом
         * `while ((int) angle != direction) angle += 90`, который зацикливался,
         * если угол по какой-то причине оказывался не кратен 90.
         */
        if (clockwise == 3) {
            turns = 1;
            code = LOADER_CMD_TURN_LEFT;
        } else {
            turns = clockwise;
            code = LOADER_CMD_TURN_RIGHT;
        }
        while (turns-- > 0) {
            if (!plan_add(plan, code, 0, 0, options->timing.turn_ms + options->timing.margin_ms))
                return false;
        }
        direction = wanted;
        /*
         * Срок переезда считается по длине отрезка: столько клеток по столько
         * миллисекунд плюс запас. Это и есть «предрассчитанное время», по
         * которому автомат сторожит исполнение команды.
         */
        if (!plan_add(plan, LOADER_CMD_DRIVE, route->step[i].point, 0,
                      route->step[i].cells * options->timing.cell_ms + options->timing.margin_ms))
            return false;
    }
    if (options->lift_pallet != 0
        && !plan_add(plan, LOADER_CMD_LIFT, options->lift_pallet, options->lift_stack,
                     options->timing.lift_ms + options->timing.margin_ms))
        return false;
    return true;
}
