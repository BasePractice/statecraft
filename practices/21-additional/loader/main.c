/**
 * @file
 * Инструмент планировщика: схема цеха, маршруты и план команд погрузчику.
 *
 * Здесь нет автомата: система управления написана на языке Takt
 * (model/loader.takt), а её прогон вместе с моделью цеха делает драйвер
 * loader_main.c, который собирается только при наличии компилятора `taktc`.
 * Планировщик же — обычный код на ISO C90, и он нужен всегда: маршрут можно
 * посчитать и посмотреть, не имея ни автомата, ни установки.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader_plant.h"
#include "route.h"

static int usage(void) {
    printf("Использование:\n");
    printf("  21-additional map [файл]\n");
    printf("      схема цеха: пол, стеллажи, разметка и метки\n");
    printf("  21-additional points [файл]\n");
    printf("      метки RFID и их координаты\n");
    printf("  21-additional route ОТКУДА КУДА [ключи]\n");
    printf("      маршрут между метками и план команд погрузчику\n");
    printf("\nКлючи команды route:\n");
    printf("  --file ФАЙЛ        конфигурация цеха (по умолчанию factory.json)\n");
    printf("  --dir НАПРАВЛЕНИЕ  как стоит погрузчик: up, right, down, left\n");
    printf("  --turn-cost N      во сколько клеток пути обходится поворот (по умолчанию 3)\n");
    printf("  --lift             добавить в конец команду подъёма груза\n");
    return EXIT_FAILURE;
}

static bool parse_direction(const char *text, int *direction) {
    if (strcmp(text, "up") == 0) {
        *direction = ROUTE_UP;
    } else if (strcmp(text, "right") == 0) {
        *direction = ROUTE_RIGHT;
    } else if (strcmp(text, "down") == 0) {
        *direction = ROUTE_DOWN;
    } else if (strcmp(text, "left") == 0) {
        *direction = ROUTE_LEFT;
    } else {
        return false;
    }
    return true;
}

static bool load(struct FactoryMap *map, const char *file_name) {
    char error[256];

    if (factory_map_read_file(map, file_name, error, sizeof(error)))
        return true;
    fprintf(stderr, "не удалось прочитать %s: %s\n", file_name, error);
    return false;
}

static void print_points(const struct FactoryMap *map) {
    int point;

    printf("Меток: %d\n", factory_point_count(map));
    for (point = 1; point <= FACTORY_POINT_MAX; ++point) {
        int row;
        int col;

        if (factory_point_cell(map, point, &row, &col))
            printf("  метка %2d: строка %2d, столбец %2d\n", point, row, col);
    }
}

static int command_route(int argc, char **argv) {
    const char *file_name = "factory.json";
    struct FactoryMap map;
    struct Route route;
    struct Plan plan;
    struct PlanOptions options;
    int from;
    int to;
    int direction = ROUTE_UP;
    int turn_cost = 3;
    bool lift = false;
    int i;

    if (argc < 4)
        return usage();
    from = atoi(argv[2]);
    to = atoi(argv[3]);
    for (i = 4; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            file_name = argv[++i];
        } else if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            if (!parse_direction(argv[++i], &direction)) {
                fprintf(stderr, "неизвестное направление: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--turn-cost") == 0 && i + 1 < argc) {
            turn_cost = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--lift") == 0) {
            lift = true;
        } else {
            return usage();
        }
    }

    if (!load(&map, file_name))
        return EXIT_FAILURE;
    if (!route_find(&map, from, to, direction, turn_cost, &route)) {
        fprintf(stderr, "маршрут от метки %d до метки %d не найден\n", from, to);
        factory_map_destroy(&map);
        return EXIT_FAILURE;
    }
    printf("Маршрут %d → %d: %d клеток, %d поворотов, стоит %s\n", from, to, route.cells,
           route.turns, route_direction_name(route.start_direction));
    for (i = 0; i < route.count; ++i) {
        printf("  до метки %2d — %s, %d клеток\n", route.step[i].point,
               route_direction_name(route.step[i].direction), route.step[i].cells);
    }
    plan_options_default(&options);
    if (lift) {
        /* Учебные коды: сканер вил читает паллету, сканер места — штабель. */
        options.lift_pallet = 101;
        options.lift_stack = 5001;
    }
    if (!plan_build(&route, &options, &plan)) {
        fprintf(stderr, "план не построен: слишком длинный маршрут\n");
        factory_map_destroy(&map);
        return EXIT_FAILURE;
    }
    printf("\nПлан команд (%d), в скобках — отведённое время:\n", plan.count);
    for (i = 0; i < plan.count; ++i) {
        const struct PlanStep *step = &plan.step[i];

        if (step->code == LOADER_CMD_DRIVE) {
            printf("  %2d. %s %d (%d мс)\n", i + 1, plan_command_name(step->code), step->point,
                   step->timeout_ms);
        } else if (step->code == LOADER_CMD_LIFT) {
            printf("  %2d. %s: паллета %d у места %d (%d мс)\n", i + 1,
                   plan_command_name(step->code), step->point, step->extra, step->timeout_ms);
        } else {
            printf("  %2d. %s (%d мс)\n", i + 1, plan_command_name(step->code), step->timeout_ms);
        }
    }
    factory_map_destroy(&map);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    struct FactoryMap map;
    const char *file_name = "factory.json";

    if (argc < 2)
        return usage();
    if (strcmp(argv[1], "route") == 0)
        return command_route(argc, argv);
    if (argc > 2)
        file_name = argv[2];
    if (strcmp(argv[1], "map") == 0) {
        if (!load(&map, file_name))
            return EXIT_FAILURE;
        printf("Конфигурация %s, версия %s, %d × %d\n", file_name, map.version, map.rows, map.cols);
        factory_map_print(&map, stdout);
        factory_map_destroy(&map);
        return EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "points") == 0) {
        if (!load(&map, file_name))
            return EXIT_FAILURE;
        print_points(&map);
        factory_map_destroy(&map);
        return EXIT_SUCCESS;
    }
    return usage();
}
