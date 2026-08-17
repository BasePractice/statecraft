#ifndef STATECRAFT_ROUTE_H
#define STATECRAFT_ROUTE_H

/**
 * @file
 * Планирование маршрута погрузчика по разметке цеха.
 *
 * Система управления погрузчиком — автомат (model/loader.takt) — умеет ровно
 * три вещи: повернуться, доехать до следующей метки и поднять груз. Куда
 * ехать, автомат не решает: последовательность команд для него готовит этот
 * планировщик, и это разделение принципиально. Поиск пути — задача с памятью
 * (очередь, пометки посещённого), конечным автоматом она не решается; попытка
 * впихнуть её в автомат управления и даёт те «автоматы» на сотню состояний, о
 * которых лекция 7 говорит как о симптоме.
 *
 * Отличия от оригинала (c_fsm, PathLine::search в transport_loader_gui.cpp):
 *
 *  - оригинал читал клетку до проверки границ (`matrix_get(path, row, col)`
 *    вызывался, а уже потом сравнивался `col >= 0 && col < size`), то есть
 *    выходил за пределы матрицы на каждой клетке у края карты;
 *  - оригинал ходил по любой непустой клетке слоя paths, не различая
 *    горизонтальные и вертикальные участки, хотя сам же их и кодировал;
 *  - оригинал считал шаги, но не считал повороты, а поворот погрузчика стоит
 *    дороже клетки пути; здесь цена поворота — параметр поиска;
 *  - оригинал разворачивал найденный путь в команды прямо в обработчике кадра
 *    графического приложения, поэтому проверить его было нечем.
 *
 * Правила движения:
 *
 *  - ехать можно по клетке с меткой в любую сторону, по горизонтальному
 *    участку — только влево-вправо, по вертикальному — только вверх-вниз;
 *  - поворачивать разрешено только на метке. В конфигурации цеха метки стоят
 *    на пересечениях магистрали с проходами, то есть ровно там, где линия и
 *    поворачивает, а метка нужна ещё и для того, чтобы автомат знал, где он.
 */

#include "base_types.h"
#include "factory_map.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Направление отсчитывается по часовой стрелке от «вверх». */
enum RouteDirection {
    ROUTE_UP = 0,
    ROUTE_RIGHT = 1,
    ROUTE_DOWN = 2,
    ROUTE_LEFT = 3,
    ROUTE_DIRECTION_COUNT = 4
};

/** Отрезок маршрута: доехать до метки @c point, двигаясь в @c direction. */
struct RouteStep {
    int point;
    int direction;
    int cells; /* клеток линии до этой метки */
};

#define ROUTE_MAX_STEPS 64

struct Route {
    int from;
    int to;
    int start_direction;
    struct RouteStep step[ROUTE_MAX_STEPS];
    int count;
    int cells; /* длина маршрута в клетках */
    int turns; /* сколько раз пришлось повернуть на 90° */
};

/**
 * Ищет маршрут от метки @p from до метки @p to для погрузчика, стоящего
 * в @p start_direction. @p turn_cost — во сколько клеток пути обходится
 * поворот на 90°; 0 означает «повороты бесплатны», и тогда из маршрутов
 * равной длины выбирается произвольный.
 *
 * Возвращает false, если метки нет в конфигурации, маршрут не существует или
 * не помещается в ROUTE_MAX_STEPS.
 */
bool route_find(const struct FactoryMap *map, int from, int to, int start_direction, int turn_cost,
                struct Route *route);

const char *route_direction_name(int direction);

/** На сколько поворотов по часовой стрелке отличаются направления (0…3). */
int route_turn_count(int from_direction, int to_direction);

/* --- план команд для системы управления ------------------------------------ */

/**
 * Коды команд. Те же числа автомат получает во входном порту cmd_code
 * (model/loader.takt) — менять их надо в двух местах сразу.
 */
enum LoaderCommandCode {
    LOADER_CMD_NONE = 0,
    LOADER_CMD_TURN_LEFT = 1,
    LOADER_CMD_TURN_RIGHT = 2,
    LOADER_CMD_DRIVE = 3, /* ехать до метки, номер — в аргументе */
    LOADER_CMD_LIFT = 4
};

struct PlanStep {
    int code;
    int point;      /* «ехать» — номер метки, «взять» — код паллеты */
    int extra;      /* «взять» — код места (штабеля), иначе 0 */
    int timeout_ms; /* сколько отведено на команду: см. LoaderTiming */
};

#define PLAN_MAX_STEPS 192

struct Plan {
    struct PlanStep step[PLAN_MAX_STEPS];
    int count;
};

/**
 * Паспортные времена машины. Планировать время обязана система верхнего
 * уровня: она знает длину перегона и характеристики погрузчика. Автомат срок
 * не вычисляет — он получает его с командой и сторожит по нему исполнение
 * (модель CommandWatchdog в model/loader.takt).
 */
struct LoaderTiming {
    int cell_ms;   /* проезд одной клетки разметки */
    int turn_ms;   /* поворот на 90°               */
    int lift_ms;   /* подъём вил до захвата        */
    int margin_ms; /* запас на разгон и торможение */
};

/** Значения из паспорта установки (loader_plant.h). */
void loader_timing_default(struct LoaderTiming *timing);

/** Что добавить к маршруту сверх переездов. */
struct PlanOptions {
    int lift_pallet; /* код паллеты; 0 — подъём не нужен */
    int lift_stack;  /* код места, у которого её берут   */
    struct LoaderTiming timing;
};

/** Заполняет @p options значениями по умолчанию: без подъёма, паспортные времена. */
void plan_options_default(struct PlanOptions *options);

/**
 * Разворачивает маршрут в команды: повороты в нужную сторону (выбирается
 * ближайшая, разворот — два поворота направо), переезды от метки к метке и,
 * если задан код паллеты, подъём груза в конце. Каждой команде проставляется
 * расчётный срок исполнения.
 *
 * @p options допускает NULL — тогда берутся значения по умолчанию.
 */
bool plan_build(const struct Route *route, const struct PlanOptions *options, struct Plan *plan);

const char *plan_command_name(int code);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ROUTE_H */
