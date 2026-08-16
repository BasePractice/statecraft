#ifndef STATECRAFT_WORDS_H
#define STATECRAFT_WORDS_H

/*
 * Лекция 1. Операции над словами и языками — те самые определения, которые
 * на лекции выглядят как формулы, а здесь работают.
 *
 * Слово — обычная строка C; пустое слово — пустая строка, а не NULL.
 * Язык — конечное множество слов, поэтому итерация всегда ограничена
 * длиной: L* бесконечен, и в память целиком не помещается.
 */

#include <stdio.h>
#include "base_types.h"

#define WORDS_MAX_WORD 64      /* длина слова */
#define WORDS_MAX_LANGUAGE 256 /* слов в языке */

#if defined(__cplusplus)
extern "C" {
#endif

struct Language {
    char words[WORDS_MAX_LANGUAGE][WORDS_MAX_WORD + 1];
    int count;
};

/* --- слова ------------------------------------------------------------ */

size_t word_length(const char *word);

/* Конкатенация: result := left right. false, если не помещается. */
bool word_concat(char *result, size_t capacity, const char *left, const char *right);

/* Степень слова: result := word^n. */
bool word_power(char *result, size_t capacity, const char *word, int n);

/* Обращение слова. */
bool word_reverse(char *result, size_t capacity, const char *word);

bool word_is_prefix(const char *prefix, const char *word);
bool word_is_suffix(const char *suffix, const char *word);
bool word_is_subword(const char *subword, const char *word);

/* Префикс длины n; при n больше длины слова возвращается всё слово. */
bool word_prefix(char *result, size_t capacity, const char *word, size_t n);

/* --- языки ------------------------------------------------------------ */

void language_init(struct Language *language);

/* Добавляет слово, если его там ещё нет (язык — множество).
   false, если язык переполнен или слово слишком длинное. */
bool language_add(struct Language *language, const char *word);

bool language_contains(const struct Language *language, const char *word);

/* Объединение, пересечение и разность языков. */
bool language_union(struct Language *result, const struct Language *a, const struct Language *b);
bool language_intersect(struct Language *result, const struct Language *a,
                        const struct Language *b);
bool language_difference(struct Language *result, const struct Language *a,
                         const struct Language *b);

/* Произведение языков: все слова вида (слово из a)(слово из b). */
bool language_concat(struct Language *result, const struct Language *a, const struct Language *b);

/* Степень языка: L^n. L^0 = { ε }. */
bool language_power(struct Language *result, const struct Language *language, int n);

/*
 * Итерация Клини, ограниченная длиной слова: все слова из L*, длина которых
 * не больше max_length. Ограничение обязательно: L* бесконечен, если L
 * содержит хотя бы одно непустое слово.
 */
bool language_star(struct Language *result, const struct Language *language, size_t max_length);

void language_print(const struct Language *language, FILE *out);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_WORDS_H */
