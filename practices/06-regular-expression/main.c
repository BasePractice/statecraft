/*
 * Лекция 6. Распознаватели форматов: регулярное выражение работает автоматом.
 *
 *   06-regular-expression                      список распознавателей
 *   06-regular-expression <имя> <строка>...    прогон по строкам
 *   06-regular-expression --table <имя>        таблица переходов автомата
 *
 * Имена: isbn, tag, number, utf8. Например:
 *
 *   06-regular-expression isbn ISBN:012345678X 12345
 *   06-regular-expression tag '<kill><</kill>'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "applied_dfa.h"

static void print_list(void) {
    const struct Dfa *dfa;
    int i;

    printf("Распознаватели:\n");
    for (i = 0; (dfa = dfa_by_index(i)) != NULL; ++i) {
        printf("  %-8s %-40s состояний: %d\n", dfa->name, dfa->pattern, dfa_state_count(dfa));
    }
}

/* Печать таблицы переходов: автомат — это данные, и посмотреть на них
   нужно уметь без отладчика. */
static void print_table(const struct Dfa *dfa) {
    int i;

    printf("%s: %s\n", dfa->name, dfa->pattern);
    printf("Начальное состояние: %d, состояний всего: %d\n", dfa->start, dfa_state_count(dfa));
    printf("Принимающие состояния:");
    for (i = 0; dfa->accepting[i] >= 0; ++i) {
        printf(" %d", dfa->accepting[i]);
    }
    printf("\n\nПереходы (просматриваются сверху вниз, срабатывает первый подходящий):\n");
    for (i = 0; i < dfa->rule_count; ++i) {
        const char *set = dfa->rules[i].set;

        if (set[0] == '^' && set[1] == '\0') {
            printf("  %3d --[любой символ]--> %d\n", dfa->rules[i].from, dfa->rules[i].to);
        } else if (set[0] == '^') {
            printf("  %3d --[кроме %s]--> %d\n", dfa->rules[i].from, set + 1, dfa->rules[i].to);
        } else {
            printf("  %3d --[%s]--> %d\n", dfa->rules[i].from, set, dfa->rules[i].to);
        }
    }
}

int main(int argc, char **argv) {
    const struct Dfa *dfa;
    int i;

    if (argc < 2) {
        print_list();
        return 0;
    }

    if (strcmp(argv[1], "--table") == 0) {
        if (argc < 3) {
            fprintf(stderr, "укажите распознаватель: --table <имя>\n");
            return 1;
        }
        dfa = dfa_by_name(argv[2]);
        if (dfa == NULL) {
            fprintf(stderr, "неизвестный распознаватель «%s»\n", argv[2]);
            return 1;
        }
        print_table(dfa);
        return 0;
    }

    dfa = dfa_by_name(argv[1]);
    if (dfa == NULL) {
        fprintf(stderr, "неизвестный распознаватель «%s»\n", argv[1]);
        print_list();
        return 1;
    }
    if (argc < 3) {
        fprintf(stderr, "укажите хотя бы одну строку\n");
        return 1;
    }

    for (i = 2; i < argc; ++i) {
        if (dfa_match(dfa, argv[i])) {
            printf("%-28s подходит\n", argv[i]);
        } else {
            /* Где именно разошлось — существеннее самого отказа: по этому
               числу видно, на каком знаке формата сломался разбор. */
            printf("%-28s не подходит (прочитано знаков: %d)\n", argv[i],
                   dfa_match_prefix(dfa, argv[i]));
        }
    }
    return 0;
}
