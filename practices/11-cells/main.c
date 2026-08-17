/*
 * Лекция 11. Клеточные автоматы: одномерные правила Вольфрама, игра «Жизнь»,
 * муравей Лэнгтона.
 *
 *   11-cells rule 110 [ширина] [шагов]   узор одномерного автомата
 *   11-cells life glider [шагов]         поле «Жизни» с заданной фигурой
 *   11-cells ant [шагов]                 муравей Лэнгтона
 *
 * Вывод текстовый: '#' — живая клетка, '.' — пустая, '@' — муравей.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ant_trail.h"
#include "cells.h"

static int usage(void) {
    printf("Использование:\n");
    printf("  11-cells rule <0..255> [ширина] [шагов]\n");
    printf("  11-cells life <block|blinker|toad|glider|lwss|r-pentomino> [шагов]\n");
    printf("  11-cells ant [шагов]\n");
    printf("  11-cells trail [тактов]      задача об умном муравье\n");
    return 1;
}

static int run_rule(int argc, char **argv) {
    struct Elementary ca;
    long rule;
    int width = 79;
    int steps = 32;
    int i;

    if (argc < 3) {
        return usage();
    }
    rule = strtol(argv[2], NULL, 10);
    if (rule < 0 || rule > 255) {
        fprintf(stderr, "номер правила — от 0 до 255\n");
        return 1;
    }
    if (argc > 3) {
        width = (int)strtol(argv[3], NULL, 10);
    }
    if (argc > 4) {
        steps = (int)strtol(argv[4], NULL, 10);
    }
    if (width < 3 || width > CELLS_MAX_WIDTH || steps < 1) {
        fprintf(stderr, "ширина 3..%d, шагов не меньше одного\n", CELLS_MAX_WIDTH);
        return 1;
    }

    elementary_init(&ca, (unsigned char)rule, width);
    printf("Правило %ld, ширина %d, одна живая клетка посередине\n\n", rule, width);
    elementary_print(&ca, stdout);
    for (i = 0; i < steps; ++i) {
        elementary_step(&ca);
        elementary_print(&ca, stdout);
    }
    return 0;
}

static int run_life(int argc, char **argv) {
    struct Life life;
    int steps = 4;
    int i;

    if (argc < 3) {
        return usage();
    }
    if (argc > 3) {
        steps = (int)strtol(argv[3], NULL, 10);
    }

    life_init(&life, 24, 16);
    if (!life_place(&life, argv[2], 4, 4)) {
        fprintf(stderr, "неизвестная фигура «%s»\n", argv[2]);
        return 1;
    }

    printf("Поколение 0, живых клеток: %d\n", life_population(&life));
    life_print(&life, stdout);
    for (i = 1; i <= steps; ++i) {
        life_step(&life);
        printf("\nПоколение %d, живых клеток: %d\n", i, life_population(&life));
        life_print(&life, stdout);
    }
    return 0;
}

static int run_ant(int argc, char **argv) {
    struct Ant ant;
    long steps = 11000;
    long i;

    if (argc > 2) {
        steps = strtol(argv[2], NULL, 10);
    }

    ant_init(&ant, 96);
    for (i = 0; i < steps; ++i) {
        if (!ant_step(&ant)) {
            printf("Муравей ушёл за край поля на шаге %ld\n", ant.steps);
            break;
        }
    }
    printf("Шагов: %ld, чёрных клеток: %d\n\n", ant.steps, ant_black_count(&ant));
    ant_print(&ant, stdout);
    return 0;
}

/* Задача об умном муравье: прогон эталонного автомата по учебной тропе. */
static int run_trail(int argc, char **argv) {
    struct AntFsm fsm;
    int steps = TRAIL_STEPS;

    if (argc > 2) {
        steps = (int)strtol(argv[2], NULL, 10);
    }
    ant_fsm_reference(&fsm);
    printf("Автомат из %d состояний, лимит %d тактов\n\n", fsm.state_count, steps);
    ant_trail_print(&fsm, steps, stdout);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return usage();
    }
    if (strcmp(argv[1], "rule") == 0) {
        return run_rule(argc, argv);
    }
    if (strcmp(argv[1], "life") == 0) {
        return run_life(argc, argv);
    }
    if (strcmp(argv[1], "ant") == 0) {
        return run_ant(argc, argv);
    }
    if (strcmp(argv[1], "trail") == 0) {
        return run_trail(argc, argv);
    }
    return usage();
}
