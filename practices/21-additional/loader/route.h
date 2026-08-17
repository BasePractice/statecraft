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
    int cells; /**< клеток линии до этой метки */
};

#define ROUTE_MAX_STEPS 64

struct Route {
    int from;
    int to;
    int start_direction;
    struct RouteStep step[ROUTE_MAX_STEPS];
    int count;
    int cells; /**< длина маршрута в клетках */
    int turns; /**< сколько раз пришлось повернуть на 90° */
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
    LOADER_CMD_DRIVE = 3, /**< ехать до метки, номер — в аргументе */
    LOADER_CMD_LIFT = 4,  /**< взять паллету: код паллеты и код места */
    LOADER_CMD_PLACE = 5  /**< поставить паллету на свободное место */
};

struct PlanStep {
    int code;
    int point;      /**< «ехать» — номер метки, «взять» — код паллеты */
    int extra;      /**< «взять» — код места (штабеля), иначе 0 */
    int timeout_ms; /**< сколько отведено на команду: см. LoaderTiming */
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
    int cell_ms;   /**< проезд одной клетки разметки */
    int turn_ms;   /**< поворот на 90°               */
    int lift_ms;   /**< подъём вил до захвата        */
    int margin_ms; /**< запас на разгон и торможение */
};

/** Значения из паспорта установки (loader_plant.h). */
void loader_timing_default(struct LoaderTiming *timing);

/** Что добавить к маршруту сверх переездов. */
struct PlanOptions {
    int lift_pallet; /**< код паллеты; 0 — подъём не нужен  */
    int lift_stack;  /**< код места, у которого её берут    */
    int place_stack; /**< код места, куда её ставят; 0 — не ставим */
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

/**
 * То же, но команды дописываются в конец уже начатого плана: так задание из
 * нескольких перегонов («доехать, взять, отвезти, поставить») собирается в
 * один план, который автомат исполняет командой за командой.
 */
bool plan_append(const struct Route *route, const struct PlanOptions *options, struct Plan *plan);

/* --- задание целиком -------------------------------------------------------- */

/**
 * Задание складской системы: откуда выехать, где взять паллету и куда её
 * поставить. Планировщик разворачивает его в один план команд.
 */
struct Mission {
    char name[64];
    int start_point;
    int start_direction;
    int pick_point;      /**< метка у места, где стоит паллета */
    int pick_stack;      /**< код этого места                  */
    int pick_pallet;     /**< код паллеты                      */
    int place_point;     /**< метка у свободного места; 0 — только взять */
    int place_stack;     /**< код свободного места             */
    int jam_after_cells; /**< отладка: заклинить привод после N клеток; -1 — нет */
    int block_point;     /**< отладка: перегородить проход у этой метки; 0 — нет */
};

void mission_default(struct Mission *mission);

/** Читает задание из файла (подмножество JSON). */
bool mission_read_file(struct Mission *mission, const char *file_name, char *error,
                       size_t error_size);

/**
 * Разворачивает задание в план: переезд к месту, «взять», переезд к
 * свободному месту, «поставить».
 */
bool mission_plan(const struct FactoryMap *map, const struct Mission *mission,
                  const struct LoaderTiming *timing, struct Plan *plan);

const char *plan_command_name(int code);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ROUTE_H */
