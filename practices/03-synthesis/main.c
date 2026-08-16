#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "synthesis.h"

/*
 * Лекция 3. Синтез автоматной схемы по таблице переходов.
 *
 *     03-synthesis < machine/coin.fsm            формулы синтеза
 *     03-synthesis --table < machine/coin.fsm    исходная таблица переходов
 *     03-synthesis --code  < machine/coin.fsm    реализация на C
 *
 * Пример machine/coin.fsm — тот самый разменный аппарат, который на лекции
 * синтезируется вручную через карты Карно.
 */

static void usage(void) {
    fprintf(stderr, "Использование: 03-synthesis [--table|--code] < machine/coin.fsm\n");
}

int main(int argc, char **argv) {
    struct Machine machine;
    struct Synthesis synthesis;
    bool show_table = false;
    bool show_code = false;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--table") == 0) {
            show_table = true;
        } else if (strcmp(argv[i], "--code") == 0) {
            show_code = true;
        } else {
            usage();
            return EXIT_FAILURE;
        }
    }

    if (!machine_read(&machine, stdin))
        return EXIT_FAILURE;

    if (show_table) {
        machine_print_table(&machine, stdout);
        return EXIT_SUCCESS;
    }

    if (!synthesis_run(&synthesis, &machine))
        return EXIT_FAILURE;

    if (!synthesis_verify(&synthesis, &machine)) {
        fprintf(stderr, "03-synthesis: формулы не воспроизводят таблицу переходов —\n");
        fprintf(stderr, "              это ошибка в минимизации, а не в описании автомата\n");
        return EXIT_FAILURE;
    }

    if (show_code) {
        synthesis_print_c(&synthesis, &machine, stdout);
    } else {
        synthesis_print(&synthesis, &machine, stdout);
        printf("\nПроверка: формулы воспроизводят таблицу переходов на всех\n");
        printf("заданных наборах.\n");
    }
    return EXIT_SUCCESS;
}
