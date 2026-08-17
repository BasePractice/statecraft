/**
 * @file
 * Консольный драйвер погрузчика: прогоняет систему управления вместе с
 * моделью цеха и печатает потактовую трассу.
 *
 * Вся связка «автомат — установка» живёт в loader_runner.h; здесь только
 * разбор ключей, печать и проверка исхода. Тем же прогоном пользуется
 * графическое приложение (../loader-gui), поэтому логика связи портов не
 * дублируется.
 *
 * Порождённый код — C99, поэтому и драйвер собирается как C99: заголовок
 * автомата приносит `uint8_t` и `bool`, которых в C90 нет. Это та же
 * документированная поблажка, что и в practices/12-takt, и причина у неё
 * внешняя — интерфейс чужого генератора.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader_runner.h"

static int usage(void) {
    printf("Использование: 21-additional-run [ключи]\n");
    printf("  --file ФАЙЛ        конфигурация цеха (по умолчанию factory.json)\n");
    printf("  --from МЕТКА       где стоит погрузчик (по умолчанию 1)\n");
    printf("  --to МЕТКА         куда ехать (по умолчанию 10)\n");
    printf("  --dir НАПРАВЛЕНИЕ  up, right, down, left (по умолчанию right)\n");
    printf("  --lift             взять паллету в конце маршрута\n");
    printf("  --pallet КОД       код паллеты для сканера вил (по умолчанию 101)\n");
    printf("  --stack КОД        код места для сканера штабеля (по умолчанию 5001)\n");
    printf("  --no-pallet        не ставить паллету на месте: команда «взять» "
           "закончится аварией\n");
    printf("  --limit N          предел тактов прогона (по умолчанию 4000)\n");
    printf("  --quiet            без потактовой трассы\n");
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

int main(int argc, char **argv) {
    const char *file_name = "factory.json";
    int from = 1;
    int to = 10;
    int direction = ROUTE_RIGHT;
    int limit = 4000;
    bool lift = false;
    bool quiet = false;
    bool place_pallet = true;
    int pallet_code = 101;
    int stack_code = 5001;
    char error[256];

    struct FactoryMap map;
    struct LoaderRunner runner;
    struct PlanOptions options;
    int result = EXIT_SUCCESS;
    int reached;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            file_name = argv[++i];
        } else if (strcmp(argv[i], "--from") == 0 && i + 1 < argc) {
            from = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) {
            to = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            if (!parse_direction(argv[++i], &direction))
                return usage();
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--lift") == 0) {
            lift = true;
        } else if (strcmp(argv[i], "--pallet") == 0 && i + 1 < argc) {
            pallet_code = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--stack") == 0 && i + 1 < argc) {
            stack_code = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-pallet") == 0) {
            place_pallet = false;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else {
            return usage();
        }
    }

    if (!factory_map_read_file(&map, file_name, error, sizeof(error))) {
        fprintf(stderr, "не удалось прочитать %s: %s\n", file_name, error);
        return EXIT_FAILURE;
    }
    plan_options_default(&options);
    if (lift) {
        options.lift_pallet = pallet_code;
        options.lift_stack = stack_code;
    }
    if (!loader_runner_init(&runner, &map, from, to, direction, &options)) {
        fprintf(stderr, "маршрут от метки %d до метки %d не построен\n", from, to);
        factory_map_destroy(&map);
        return EXIT_FAILURE;
    }
    /* Паллета кладётся на конечную метку: без неё сканеры ничего не прочитают,
       и команда «взять» закончится аварией — как и должна. */
    if (lift && place_pallet)
        loader_plant_place_pallet(&runner.plant, to, stack_code, pallet_code);

    printf("Маршрут %d → %d: %d клеток, %d поворотов, %d команд\n", from, to, runner.route.cells,
           runner.route.turns, runner.plan.count);

    while ((int)runner.ticks < limit && !loader_runner_done(&runner)) {
        loader_runner_tick(&runner);
        if (!quiet) {
            printf("%6lu мс: метка %2d, %-6s, путь %4d см, впереди %3d см | газ %d поворот %d%d "
                   "вилы %d | ack %d done %d авария %d | %s\n",
                   runner.plant.clock_ms, runner.sensors.point,
                   route_direction_name(runner.sensors.angle), runner.sensors.odometer,
                   runner.sensors.range, runner.commands.gas, runner.commands.turn_left,
                   runner.commands.turn_right, runner.commands.fork_up, runner.cmd_ack,
                   runner.cmd_done, runner.fault, loader_runner_command_name(&runner));
        }
        if (runner.fault) {
            fprintf(stderr, "ОШИБКА: автомат сообщил об аварии на такте %lu (команда %d из %d)\n",
                    runner.ticks, runner.step + 1, runner.plan.count);
            result = EXIT_FAILURE;
            break;
        }
    }

    reached = loader_plant_point(&runner.plant);
    if (result == EXIT_SUCCESS) {
        printf("Исполнено команд: %d из %d, время %lu мс, путь %lu клеток (%d см), "
               "погрузчик на метке %d\n",
               runner.step, runner.plan.count, runner.plant.clock_ms, runner.plant.cells,
               runner.plant.odometer_cm, reached);
        if (!loader_runner_done(&runner)) {
            fprintf(stderr, "ОШИБКА: план не доигран за %d тактов\n", limit);
            result = EXIT_FAILURE;
        } else if (reached != to) {
            fprintf(stderr, "ОШИБКА: погрузчик на метке %d, а ожидалась %d\n", reached, to);
            result = EXIT_FAILURE;
        }
    }
    factory_map_destroy(&map);
    return result;
}
