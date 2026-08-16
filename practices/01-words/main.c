#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "words.h"

/*
 * Лекция 1. Демонстрация операций над словами и языками.
 *
 *     01-words                       разбор примеров лекции
 *     01-words star ab ba 4          L* для L = {ab, ba}, слова до длины 4
 *     01-words concat a b c          произведение {a} и {b, c}
 */

static void demo_words(void) {
    char buffer[WORDS_MAX_WORD + 1];
    const char *alpha = "ab";
    const char *beta = "ba";

    printf("=== слова ===\n");
    printf("|%s| = %lu\n", alpha, (unsigned long)word_length(alpha));
    printf("|ε| = %lu\n", (unsigned long)word_length(""));

    word_concat(buffer, sizeof(buffer), alpha, beta);
    printf("%s · %s = %s\n", alpha, beta, buffer);

    word_power(buffer, sizeof(buffer), alpha, 3);
    printf("%s^3 = %s\n", alpha, buffer);

    word_power(buffer, sizeof(buffer), alpha, 0);
    printf("%s^0 = «%s» (пустое слово)\n", alpha, buffer);

    word_reverse(buffer, sizeof(buffer), "abbc");
    printf("обращение abbc = %s\n", buffer);

    word_prefix(buffer, sizeof(buffer), "abbc", 2);
    printf("префикс длины 2 слова abbc = %s\n", buffer);

    printf("ab — префикс abbc? %s\n", word_is_prefix("ab", "abbc") ? "да" : "нет");
    printf("bc — суффикс abbc? %s\n", word_is_suffix("bc", "abbc") ? "да" : "нет");
    printf("bb — подслово abbc? %s\n", word_is_subword("bb", "abbc") ? "да" : "нет");
    printf("ε — префикс любого слова? %s\n", word_is_prefix("", "abbc") ? "да" : "нет");
}

static void demo_languages(void) {
    struct Language a;
    struct Language b;
    struct Language result;

    printf("\n=== языки ===\n");
    language_init(&a);
    language_add(&a, "a");
    language_add(&a, "ab");
    printf("A = ");
    language_print(&a, stdout);

    language_init(&b);
    language_add(&b, "b");
    language_add(&b, "");
    printf("B = ");
    language_print(&b, stdout);

    language_union(&result, &a, &b);
    printf("A ∪ B = ");
    language_print(&result, stdout);

    language_concat(&result, &a, &b);
    printf("A · B = ");
    language_print(&result, stdout);

    language_power(&result, &a, 2);
    printf("A^2  = ");
    language_print(&result, stdout);

    language_star(&result, &a, 4);
    printf("A* (слова до длины 4) = ");
    language_print(&result, stdout);

    printf("\nОбратите внимание: ε ∈ B, поэтому A ⊆ A · B — произведение с\n");
    printf("языком, содержащим пустое слово, сохраняет исходные слова.\n");
}

int main(int argc, char **argv) {
    struct Language language;
    struct Language other;
    struct Language result;
    int i;

    if (argc == 1) {
        demo_words();
        demo_languages();
        return EXIT_SUCCESS;
    }

    if (argc >= 4 && strcmp(argv[1], "star") == 0) {
        int limit = atoi(argv[argc - 1]);

        language_init(&language);
        for (i = 2; i < argc - 1; ++i) {
            if (!language_add(&language, argv[i])) {
                fprintf(stderr, "01-words: язык переполнен\n");
                return EXIT_FAILURE;
            }
        }
        if (!language_star(&result, &language, (size_t)limit)) {
            fprintf(stderr, "01-words: результат не помещается в память языка\n");
            return EXIT_FAILURE;
        }
        language_print(&result, stdout);
        return EXIT_SUCCESS;
    }

    if (argc >= 4 && strcmp(argv[1], "concat") == 0) {
        language_init(&language);
        language_init(&other);
        if (!language_add(&language, argv[2]))
            return EXIT_FAILURE;
        for (i = 3; i < argc; ++i) {
            if (!language_add(&other, argv[i]))
                return EXIT_FAILURE;
        }
        if (!language_concat(&result, &language, &other))
            return EXIT_FAILURE;
        language_print(&result, stdout);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Использование:\n");
    fprintf(stderr, "  01-words                     примеры лекции\n");
    fprintf(stderr, "  01-words star СЛОВО... ДЛИНА итерация языка\n");
    fprintf(stderr, "  01-words concat СЛОВО СЛОВО... произведение языков\n");
    return EXIT_FAILURE;
}
