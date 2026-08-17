#ifndef STATECRAFT_LOADER_RUNNER_H
#define STATECRAFT_LOADER_RUNNER_H

/**
 * @file
 * Связка «автомат — установка»: порождённая из model/loader.takt система
 * управления, модель цеха и план маршрута, сведённые в один прогон.
 *
 * Модуль вынесен отдельно, потому что прогон нужен двоим — консольному
 * драйверу (loader_main.c) и графическому приложению (gui/loader_gui.cpp).
 * Логика связи портов автомата с датчиками установки должна быть одна: две её
 * копии неизбежно разойдутся, и тогда картинка станет показывать не то, что
 * проверяет тест.
 *
 * Это единственная часть практики, зависящая от компилятора Takt: заголовок
 * автомата порождается `taktc` и написан на C99. Всё остальное —
 * конфигурация, планировщик, модель цеха — обычный ISO C90 и собирается
 * всегда.
 */

#include <stdbool.h>
#include <stdint.h>

#include "factory_map.h"
#include "loader_plant.h"
#include "route.h"
#include "scenario.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Порождённый заголовок включается внутри extern "C": `taktc` его такой
 * обвязкой не снабжает, и при сборке графического приложения (C++) функции
 * автомата получили бы C++-линковку. Стандартные заголовки, которые он тянет,
 * включены выше — внутрь блока не должно попасть ничего из библиотеки C++.
 */
#include "loader.h" /* порождается taktc из model/loader.takt */

struct LoaderRunner {
    const struct FactoryMap *map; /**< не владеет: карту держит вызывающая сторона */
    struct Route route;
    struct Plan plan;
    struct LoaderPlant plant;
    Loader model;

    /* входы автомата */
    int cmd_valid;
    int cmd_code;
    int cmd_point;
    int cmd_extra;
    int cmd_timeout_ms;
    int reset;
    struct LoaderSensors sensors;

    /* выходы автомата */
    int cmd_ack;
    int cmd_done;
    int fault;
    struct LoaderCommands commands;

    int step;            /**< какая команда плана исполняется */
    int target;          /**< конечная метка маршрута */
    unsigned long ticks; /**< тактов прогона */

    /*
     * Режим сценария: входы берутся из файла, а не от установки. Нужен для
     * разбора отказов, которых от исправной установки не дождёшься. Пока он
     * включён, модель цеха не тикает — иначе показания двух источников
     * противоречили бы друг другу.
     */
    const struct Scenario *scenario;
    int scenario_step;
};

/**
 * Готовит прогон: ищет маршрут @p from → @p to для погрузчика, стоящего в
 * @p direction, разворачивает его в план и ставит погрузчик на метку @p from.
 * Возвращает false, если маршрута нет или метка неизвестна.
 */
bool loader_runner_init(struct LoaderRunner *runner, const struct FactoryMap *map, int from, int to,
                        int direction, const struct PlanOptions *options);

/**
 * Один такт: команда плана предъявляется автомату, автомат делает шаг, его
 * команды исполняет модель цеха, датчики обновляются, квитирование продвигает
 * план.
 */
void loader_runner_tick(struct LoaderRunner *runner);

/** План исполнен целиком. */
bool loader_runner_done(const struct LoaderRunner *runner);

/** Квитирование аварии: автомат вернётся к ожиданию команды. */
void loader_runner_reset_fault(struct LoaderRunner *runner);

/**
 * Включает режим сценария: входы автомата берутся из @p scenario по шагу за
 * такт, установка не тикает. @p scenario допускает NULL — тогда прогон
 * возвращается к модели цеха. Владение сценарием остаётся у вызывающей стороны.
 */
void loader_runner_set_scenario(struct LoaderRunner *runner, const struct Scenario *scenario);

/** Сценарий доигран до конца (в обычном режиме — всегда false). */
bool loader_runner_scenario_done(const struct LoaderRunner *runner);

/**
 * Готовит прогон по заданию: план из двух перегонов с «взять» и «поставить»,
 * места хранения и паллета расставлены на стенде, при @c jam_after_cells
 * привод заклинивает после стольких клеток.
 */
bool loader_runner_init_mission(struct LoaderRunner *runner, const struct FactoryMap *map,
                                const struct Mission *mission);

/** Название текущей команды плана — для трассы и панели состояния. */
const char *loader_runner_command_name(const struct LoaderRunner *runner);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_LOADER_RUNNER_H */
