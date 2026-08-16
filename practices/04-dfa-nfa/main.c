#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "subset.h"

/*
 * Лекция 4. Читает описание НКА со стандартного ввода, детерминизирует его
 * и печатает результат.
 *
 *     04-dfa-nfa < nfa.txt                    ДКА в текстовом формате
 *     04-dfa-nfa --minimize < nfa.txt         минимальный ДКА
 *     04-dfa-nfa --minimize --dot < nfa.txt   он же для Graphviz
 *     04-dfa-nfa --accept aabb < nfa.txt      проверить слово
 *
 * Вход берётся из практики лекции 5:
 *
 *     05-kleene "(a|b)*abb" | 04-dfa-nfa --minimize --dot | dot -Tpng > dfa.png
 */

static void usage(void) {
    fprintf(stderr, "Использование: 04-dfa-nfa [--minimize] [--dot|--accept СЛОВО] < nfa.txt\n");
}

int main(int argc, char **argv) {
    struct Nfa nfa;
    struct Dfa dfa;
    struct Dfa minimal;
    struct Dfa *result;
    bool minimize = false;
    bool dot = false;
    const char *word = NULL;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--minimize") == 0) {
            minimize = true;
        } else if (strcmp(argv[i], "--dot") == 0) {
            dot = true;
        } else if (strcmp(argv[i], "--accept") == 0 && i + 1 < argc) {
            word = argv[++i];
        } else {
            usage();
            return EXIT_FAILURE;
        }
    }

    if (!nfa_read(&nfa, stdin))
        return EXIT_FAILURE;
    if (!subset_construction(&dfa, &nfa))
        return EXIT_FAILURE;

    result = &dfa;
    if (minimize) {
        if (!dfa_minimize(&minimal, &dfa))
            return EXIT_FAILURE;
        result = &minimal;
        fprintf(stderr, "04-dfa-nfa: НКА %d состояний → ДКА %d → минимальный %d\n", nfa.state_count,
                dfa.state_count, minimal.state_count);
    } else {
        fprintf(stderr, "04-dfa-nfa: НКА %d состояний → ДКА %d\n", nfa.state_count,
                dfa.state_count);
    }

    if (word != NULL) {
        if (dfa_accepts(result, word)) {
            printf("«%s» допускается\n", word);
            return EXIT_SUCCESS;
        }
        printf("«%s» не допускается\n", word);
        return EXIT_FAILURE;
    }

    if (dot) {
        dfa_print_dot(result, stdout);
    } else {
        dfa_print(result, stdout);
    }
    return EXIT_SUCCESS;
}
