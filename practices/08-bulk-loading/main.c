/*
 * Лекция 8. Загрузка сыпучих материалов: автомат управления и модель
 * установки.
 *
 *   08-bulk-loading [рецепт] [--quiet]
 *
 * Рецепт — десятичное число: младшая цифра задаёт первый малый цикл, каждая
 * цифра — маска резервуаров (бит 0 — первый, бит 1 — второй, бит 2 — третий).
 * По умолчанию 5151: циклы «первый и третий», «второй», «первый и третий»,
 * «второй».
 *
 * Печатается циклограмма: по строке на такт — состояние автомата, поднятые
 * команды и положение контейнера.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bulk_plant.h"

#define TICK_LIMIT 400

static void print_header(const struct BulkFsm *fsm) {
    int i;

    printf("Рецепт по циклам:\n");
    for (i = 0; i < BULK_CYCLE_COUNT; ++i) {
        int tank;

        printf("  цикл %d: резервуары", i + 1);
        if (bulk_recipe_mask(fsm, i) == 0) {
            printf(" — (пропуск)");
        }
        for (tank = 0; tank < BULK_TANK_COUNT; ++tank) {
            if ((bulk_recipe_mask(fsm, i) & (1U << tank)) != 0) {
                printf(" %d", tank + 1);
            }
        }
        printf("\n");
    }
    printf("\n%-5s %-20s %-8s %s\n", "такт", "состояние", "позиция", "команды");
}

static void print_tick(const struct BulkFsm *fsm, const struct BulkPlant *plant) {
    int i;

    printf("%-5lu %-20s %-8d", fsm->tick, bulk_state_name(fsm->state), plant->position);
    for (i = 0; i < BULK_OUTPUT_COUNT; ++i) {
        if (bulk_output(fsm, (enum BulkOutput)i)) {
            printf(" [%s]", bulk_output_name((enum BulkOutput)i));
        }
    }
    printf("\n");
}

int main(int argc, char **argv) {
    struct BulkFsm fsm;
    struct BulkPlant plant;
    unsigned long recipe = 5151UL;
    bool quiet = false;
    int i;
    int tank;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else {
            char *end;
            unsigned long value = strtoul(argv[i], &end, 10);

            if (end == argv[i] || *end != '\0') {
                fprintf(stderr, "рецепт — десятичное число, а не «%s»\n", argv[i]);
                return 1;
            }
            recipe = value;
        }
    }

    if (!bulk_fsm_init(&fsm, recipe)) {
        fprintf(stderr,
                "рецепт %lu задан неверно: в нём не более %d цифр, каждая — маска\n"
                "резервуаров от 0 до %d\n",
                recipe, BULK_CYCLE_COUNT, (1 << BULK_TANK_COUNT) - 1);
        return 1;
    }
    bulk_plant_init(&plant);

    if (!quiet) {
        print_header(&fsm);
    }

    for (i = 0; i < TICK_LIMIT && !bulk_fsm_finished(&fsm); ++i) {
        /* Команда на малый цикл приходит извне — её даёт оператор; здесь
           она подаётся всегда, когда автомат её ждёт. */
        bulk_input_set(&fsm, BULK_IN_START_CYCLE, fsm.state == BULK_CYCLE_IDLE);
        bulk_fsm_tick(&fsm);
        bulk_plant_tick(&plant, &fsm);
        if (!quiet) {
            print_tick(&fsm, &plant);
        }
    }

    printf("\nИтог: %s, тактов %lu\n", bulk_state_name(fsm.state), fsm.tick);
    for (tank = 0; tank < BULK_TANK_COUNT; ++tank) {
        printf("  резервуар %d отдал материал %d раз(а)\n", tank + 1, plant.filled[tank]);
    }
    return bulk_fsm_finished(&fsm) && fsm.state == BULK_FINISHED ? 0 : 1;
}
