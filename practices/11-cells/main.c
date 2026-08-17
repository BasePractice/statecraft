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
#include "render.h"

static int usage(void) {
    printf("Использование:\n");
    printf("  11-cells rule <0..255> [ширина] [шагов]\n");
    printf("  11-cells life <block|blinker|toad|glider|lwss|r-pentomino> [шагов]\n");
    printf("  11-cells ant [шагов]\n");
    printf("  11-cells trail <lookaround|probe|scan|evolved|таблица> [тактов]\n");
    printf("                               задача об умном муравье, тропа Санта-Фе\n");
    printf("  11-cells patterns            список фигур «Жизни»\n");
    printf("  11-cells svg life <фигура> <поколения через запятую> <файл> [сторона]\n");
    printf("  11-cells svg trail <стратегия> <такты через запятую> <файл>\n");
    printf("  11-cells svg fsm <стратегия|таблица> <файл>\n");
    printf("  11-cells svg scene eater-vs-glider <поколения> <файл>\n");
    printf("  11-cells svg langton <шаги через запятую> <файл> [сторона]\n");
    printf("  11-cells svg rule <0..255> <ширина> <шагов> <файл>\n");
    printf("                               векторные ленты кадров для лекции\n");
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

/* Задача об умном муравье: прогон выбранной стратегии по тропе Санта-Фе. */
static int run_trail(int argc, char **argv) {
    struct AntFsm fsm;
    const char *name = "scan";
    int steps = TRAIL_STEPS;
    char spec[ANT_FSM_SPEC_SIZE];

    if (argc > 2) {
        name = argv[2];
    }
    if (argc > 3) {
        steps = (int)strtol(argv[3], NULL, 10);
    }
    /* Имя стратегии либо готовая запись автомата: так можно прогнать
       найденный поиском автомат, не пересобирая программу. */
    if (!ant_fsm_by_name(name, &fsm) && !ant_fsm_parse(&fsm, name)) {
        fprintf(stderr, "неизвестная стратегия «%s»\n", name);
        return 1;
    }
    printf("Автомат из %d состояний, лимит %d тактов\n", fsm.state_count, steps);
    if (ant_fsm_format(&fsm, spec, sizeof(spec))) {
        printf("Таблица: %s\n", spec);
    }
    printf("\n");
    ant_trail_print(&fsm, steps, stdout);
    return 0;
}

static int run_patterns(void) {
    int i;
    const char *name;

    printf("Фигуры «Жизни»:\n");
    for (i = 0; (name = life_pattern_name(i)) != NULL; ++i) {
        printf("  %s\n", name);
    }
    return 0;
}

/* Разбор списка кадров «12,24,36». Возвращает число разобранных значений или
   ноль, если список пуст либо испорчен. */
static int parse_frames(const char *text, int *frames, int capacity) {
    int count = 0;

    while (*text != '\0' && count < capacity) {
        char *end;
        long value = strtol(text, &end, 10);

        if (end == text || value < 0) {
            return 0;
        }
        frames[count++] = (int)value;
        text = end;
        if (*text == ',') {
            ++text;
        } else if (*text != '\0') {
            return 0;
        }
    }
    return count;
}

#define MAX_FRAMES 16

/* Векторная лента кадров в файл: картинки лекции порождает та же программа,
   которая считает автоматы. */
static int run_svg(int argc, char **argv) {
    int frames[MAX_FRAMES];
    int count;
    FILE *out;
    bool ok = false;

    if (argc < 4) {
        return usage();
    }

    if (strcmp(argv[2], "life") == 0 && argc >= 6) {
        /* Сторона поля — необязательный параметр: ружью Госпера 24 клеток
           мало, а натюрморту столько и не нужно. */
        int side = (argc > 6) ? (int)strtol(argv[6], NULL, 10) : 24;

        count = parse_frames(argv[4], frames, MAX_FRAMES);
        if (count == 0) {
            fprintf(stderr, "список поколений не разобран: «%s»\n", argv[4]);
            return 1;
        }
        if (side < 4 || side > LIFE_MAX_SIDE) {
            fprintf(stderr, "сторона поля — от 4 до %d\n", LIFE_MAX_SIDE);
            return 1;
        }
        out = fopen(argv[5], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[5]);
            return 1;
        }
        ok = render_life_svg(argv[3], side, side, frames, count, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "неизвестная фигура «%s»\n", argv[3]);
            return 1;
        }
        printf("Записано: %s (%d кадров фигуры «%s»)\n", argv[5], count, argv[3]);
        return 0;
    }

    if (strcmp(argv[2], "trail") == 0 && argc >= 6) {
        struct AntFsm fsm;

        if (!ant_fsm_by_name(argv[3], &fsm) && !ant_fsm_parse(&fsm, argv[3])) {
            fprintf(stderr, "неизвестная стратегия «%s»\n", argv[3]);
            return 1;
        }
        count = parse_frames(argv[4], frames, MAX_FRAMES);
        if (count == 0) {
            fprintf(stderr, "список тактов не разобран: «%s»\n", argv[4]);
            return 1;
        }
        out = fopen(argv[5], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[5]);
            return 1;
        }
        ok = render_trail_svg(&fsm, frames, count, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "лента не построена\n");
            return 1;
        }
        printf("Записано: %s (%d кадров стратегии «%s»)\n", argv[5], count, argv[3]);
        return 0;
    }

    if (strcmp(argv[2], "fsm") == 0 && argc >= 5) {
        struct AntFsm fsm;

        if (!ant_fsm_by_name(argv[3], &fsm) && !ant_fsm_parse(&fsm, argv[3])) {
            fprintf(stderr, "неизвестная стратегия или запись автомата «%s»\n", argv[3]);
            return 1;
        }
        out = fopen(argv[4], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[4]);
            return 1;
        }
        ok = render_fsm_svg(&fsm, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "диаграмма не построена\n");
            return 1;
        }
        printf("Записано: %s (автомат из %d состояний)\n", argv[4], fsm.state_count);
        return 0;
    }

    if (strcmp(argv[2], "scene") == 0 && argc >= 6) {
        count = parse_frames(argv[4], frames, MAX_FRAMES);
        if (count == 0) {
            fprintf(stderr, "список поколений не разобран: «%s»\n", argv[4]);
            return 1;
        }
        out = fopen(argv[5], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[5]);
            return 1;
        }
        ok = render_scene_svg(argv[3], frames, count, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "неизвестная сцена «%s»\n", argv[3]);
            return 1;
        }
        printf("Записано: %s (%d кадров сцены «%s»)\n", argv[5], count, argv[3]);
        return 0;
    }

    if (strcmp(argv[2], "langton") == 0 && argc >= 5) {
        long langton_steps[MAX_FRAMES];
        int side = (argc > 5) ? (int)strtol(argv[5], NULL, 10) : 96;
        int i;

        count = parse_frames(argv[3], frames, MAX_FRAMES);
        if (count == 0) {
            fprintf(stderr, "список шагов не разобран: «%s»\n", argv[3]);
            return 1;
        }
        for (i = 0; i < count; ++i) {
            langton_steps[i] = frames[i];
        }
        out = fopen(argv[4], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[4]);
            return 1;
        }
        ok = render_langton_svg(side, langton_steps, count, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "сторона поля — от 4 до %d\n", ANT_MAX_SIDE);
            return 1;
        }
        printf("Записано: %s (%d кадров муравья Лэнгтона)\n", argv[4], count);
        return 0;
    }

    if (strcmp(argv[2], "rule") == 0 && argc >= 7) {
        long rule = strtol(argv[3], NULL, 10);
        int width = (int)strtol(argv[4], NULL, 10);
        int steps = (int)strtol(argv[5], NULL, 10);

        if (rule < 0 || rule > 255) {
            fprintf(stderr, "номер правила — от 0 до 255\n");
            return 1;
        }
        out = fopen(argv[6], "w");
        if (out == NULL) {
            fprintf(stderr, "не открыть файл «%s»\n", argv[6]);
            return 1;
        }
        ok = render_elementary_svg((unsigned char)rule, width, steps, NULL, out);
        fclose(out);
        if (!ok) {
            fprintf(stderr, "ширина 3..%d, шагов не меньше одного\n", CELLS_MAX_WIDTH);
            return 1;
        }
        printf("Записано: %s (правило %ld, %d шагов)\n", argv[6], rule, steps);
        return 0;
    }

    return usage();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return usage();
    }
    if (strcmp(argv[1], "rule") == 0) {
        return run_rule(argc, argv);
    }
    if (strcmp(argv[1], "patterns") == 0) {
        return run_patterns();
    }
    if (strcmp(argv[1], "svg") == 0) {
        return run_svg(argc, argv);
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
