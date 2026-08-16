#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "words.h"

size_t word_length(const char *word) {
    assert(word != NULL);
    return strlen(word);
}

bool word_concat(char *result, size_t capacity, const char *left, const char *right) {
    size_t total;

    assert(result != NULL && left != NULL && right != NULL);
    total = strlen(left) + strlen(right);
    if (total + 1 > capacity)
        return false;
    strcpy(result, left);
    strcat(result, right);
    return true;
}

bool word_power(char *result, size_t capacity, const char *word, int n) {
    int i;
    size_t total;

    assert(result != NULL && word != NULL);
    if (n < 0)
        return false;
    total = strlen(word) * (size_t)n;
    if (total + 1 > capacity)
        return false;

    result[0] = '\0';
    for (i = 0; i < n; ++i) {
        strcat(result, word);
    }
    return true;
}

bool word_reverse(char *result, size_t capacity, const char *word) {
    size_t length;
    size_t i;

    assert(result != NULL && word != NULL);
    length = strlen(word);
    if (length + 1 > capacity)
        return false;
    for (i = 0; i < length; ++i) {
        result[i] = word[length - 1 - i];
    }
    result[length] = '\0';
    return true;
}

bool word_is_prefix(const char *prefix, const char *word) {
    size_t length;

    assert(prefix != NULL && word != NULL);
    length = strlen(prefix);
    if (length > strlen(word))
        return false;
    return strncmp(prefix, word, length) == 0 ? true : false;
}

bool word_is_suffix(const char *suffix, const char *word) {
    size_t suffix_length;
    size_t word_length_value;

    assert(suffix != NULL && word != NULL);
    suffix_length = strlen(suffix);
    word_length_value = strlen(word);
    if (suffix_length > word_length_value)
        return false;
    return strcmp(word + word_length_value - suffix_length, suffix) == 0 ? true : false;
}

bool word_is_subword(const char *subword, const char *word) {
    assert(subword != NULL && word != NULL);
    return strstr(word, subword) != NULL ? true : false;
}

bool word_prefix(char *result, size_t capacity, const char *word, size_t n) {
    size_t length;

    assert(result != NULL && word != NULL);
    length = strlen(word);
    if (n > length)
        n = length;
    if (n + 1 > capacity)
        return false;
    memcpy(result, word, n);
    result[n] = '\0';
    return true;
}

void language_init(struct Language *language) {
    assert(language != NULL);
    language->count = 0;
}

bool language_contains(const struct Language *language, const char *word) {
    int i;

    assert(language != NULL && word != NULL);
    for (i = 0; i < language->count; ++i) {
        if (strcmp(language->words[i], word) == 0)
            return true;
    }
    return false;
}

bool language_add(struct Language *language, const char *word) {
    assert(language != NULL && word != NULL);
    if (strlen(word) > WORDS_MAX_WORD)
        return false;
    if (language_contains(language, word))
        return true; /* язык — множество: повтор не ошибка */
    if (language->count >= WORDS_MAX_LANGUAGE)
        return false;
    strcpy(language->words[language->count], word);
    ++language->count;
    return true;
}

bool language_union(struct Language *result, const struct Language *a, const struct Language *b) {
    struct Language tmp;
    int i;

    assert(result != NULL && a != NULL && b != NULL);
    language_init(&tmp);
    for (i = 0; i < a->count; ++i) {
        if (!language_add(&tmp, a->words[i]))
            return false;
    }
    for (i = 0; i < b->count; ++i) {
        if (!language_add(&tmp, b->words[i]))
            return false;
    }
    *result = tmp;
    return true;
}

bool language_intersect(struct Language *result, const struct Language *a,
                        const struct Language *b) {
    struct Language tmp;
    int i;

    assert(result != NULL && a != NULL && b != NULL);
    language_init(&tmp);
    for (i = 0; i < a->count; ++i) {
        if (language_contains(b, a->words[i])) {
            if (!language_add(&tmp, a->words[i]))
                return false;
        }
    }
    *result = tmp;
    return true;
}

bool language_difference(struct Language *result, const struct Language *a,
                         const struct Language *b) {
    struct Language tmp;
    int i;

    assert(result != NULL && a != NULL && b != NULL);
    language_init(&tmp);
    for (i = 0; i < a->count; ++i) {
        if (!language_contains(b, a->words[i])) {
            if (!language_add(&tmp, a->words[i]))
                return false;
        }
    }
    *result = tmp;
    return true;
}

bool language_concat(struct Language *result, const struct Language *a, const struct Language *b) {
    struct Language tmp;
    char buffer[WORDS_MAX_WORD + 1];
    int i;
    int j;

    assert(result != NULL && a != NULL && b != NULL);
    language_init(&tmp);
    for (i = 0; i < a->count; ++i) {
        for (j = 0; j < b->count; ++j) {
            /* Слишком длинные произведения отбрасываются, а не обрывают
               работу: язык всё равно ограничен длиной слова. */
            if (!word_concat(buffer, sizeof(buffer), a->words[i], b->words[j]))
                continue;
            if (!language_add(&tmp, buffer))
                return false;
        }
    }
    *result = tmp;
    return true;
}

bool language_power(struct Language *result, const struct Language *language, int n) {
    struct Language accumulator;
    struct Language next;
    int i;

    assert(result != NULL && language != NULL);
    if (n < 0)
        return false;

    language_init(&accumulator);
    if (!language_add(&accumulator, "")) /* L^0 = { ε } */
        return false;

    for (i = 0; i < n; ++i) {
        if (!language_concat(&next, &accumulator, language))
            return false;
        accumulator = next;
    }
    *result = accumulator;
    return true;
}

bool language_star(struct Language *result, const struct Language *language, size_t max_length) {
    struct Language accumulator;
    struct Language layer;
    struct Language next;
    int i;
    bool grew = true;

    assert(result != NULL && language != NULL);
    language_init(&accumulator);
    if (!language_add(&accumulator, "")) /* ε принадлежит L* всегда */
        return false;

    language_init(&layer);
    if (!language_add(&layer, ""))
        return false;

    /* Слои L^0, L^1, L^2, … добавляются, пока появляются слова не длиннее
       max_length. Без ограничения цикл не закончился бы. */
    while (grew) {
        if (!language_concat(&next, &layer, language))
            return false;
        grew = false;
        language_init(&layer);
        for (i = 0; i < next.count; ++i) {
            if (strlen(next.words[i]) > max_length)
                continue;
            if (!language_add(&layer, next.words[i]))
                return false;
            if (!language_contains(&accumulator, next.words[i])) {
                if (!language_add(&accumulator, next.words[i]))
                    return false;
                grew = true;
            }
        }
    }
    *result = accumulator;
    return true;
}

void language_print(const struct Language *language, FILE *out) {
    int i;

    assert(language != NULL && out != NULL);
    fprintf(out, "{ ");
    for (i = 0; i < language->count; ++i) {
        if (i > 0)
            fprintf(out, ", ");
        if (language->words[i][0] == '\0') {
            fprintf(out, "eps");
        } else {
            fprintf(out, "%s", language->words[i]);
        }
    }
    fprintf(out, " }  (слов: %d)\n", language->count);
}
