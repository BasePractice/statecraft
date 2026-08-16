#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thompson.h"

/*
 * Лекция 5. Регулярное выражение → ε-НКА конструкцией Томпсона.
 *
 *     05-kleene "(a|b)*abb"            описание автомата в текстовом формате
 *     05-kleene --dot "(a|b)*abb"      то же в формате Graphviz
 *     05-kleene --check "(a|b)*abb" aabb   проверить слово прямым
 *                                          моделированием НКА
 *
 * Вывод без ключей читается практикой лекции 4:
 *
 *     05-kleene "(a|b)*abb" | 04-dfa-nfa --minimize
 */

static void usage(void) {
    fprintf(stderr, "Использование:\n");
    fprintf(stderr, "  05-kleene <выражение>              описание ε-НКА\n");
    fprintf(stderr, "  05-kleene --dot <выражение>        то же в формате Graphviz\n");
    fprintf(stderr, "  05-kleene --check <выражение> <слово>\n");
}

int main(int argc, char **argv) {
    struct Nfa nfa;

    if (argc == 2) {
        if (!thompson_build(&nfa, argv[1]))
            return EXIT_FAILURE;
        nfa_print(&nfa, stdout);
        return EXIT_SUCCESS;
    }

    if (argc == 3 && strcmp(argv[1], "--dot") == 0) {
        if (!thompson_build(&nfa, argv[2]))
            return EXIT_FAILURE;
        nfa_print_dot(&nfa, stdout);
        return EXIT_SUCCESS;
    }

    if (argc == 4 && strcmp(argv[1], "--check") == 0) {
        if (!thompson_build(&nfa, argv[2]))
            return EXIT_FAILURE;
        if (nfa_accepts(&nfa, argv[3])) {
            printf("«%s» ∈ L(%s)\n", argv[3], argv[2]);
            return EXIT_SUCCESS;
        }
        printf("«%s» ∉ L(%s)\n", argv[3], argv[2]);
        return EXIT_FAILURE;
    }

    usage();
    return EXIT_FAILURE;
}
