#ifndef STATECRAFT_FSM_H
#define STATECRAFT_FSM_H

/*
 * Представление автоматов и обмен ими между практиками лекций 4 и 5.
 *
 * Здесь только структуры данных и ввод-вывод: алгоритмы живут в самих
 * практиках, чтобы студент видел их целиком, а не по частям.
 *
 * Текстовый формат описания НКА (одна конструкция на строку, '#' — комментарий):
 *
 *     states 4          число состояний, нумерация с нуля
 *     alphabet ab       входной алфавит: по символу на позицию
 *     start 0           начальное состояние
 *     final 3           заключительное состояние (строк может быть несколько)
 *     0 a 1             переход по символу
 *     1 eps 2           переход по пустому слову
 *
 * Формат намеренно примитивен: его разбирает функция на сорок строк, а
 * файл можно написать руками.
 */

#include <stdio.h>
#include "base_types.h"

#define FSM_MAX_STATES 64      /* состояний в НКА */
#define FSM_MAX_DFA_STATES 128 /* состояний в ДКА: подмножества исходных */
#define FSM_MAX_SYMBOLS 16     /* мощность входного алфавита */
#define FSM_NO_STATE (-1)

#if defined(__cplusplus)
extern "C" {
#endif

struct Nfa {
    int state_count;
    int symbol_count;
    char symbols[FSM_MAX_SYMBOLS];
    int start;
    char final[FSM_MAX_STATES];
    char eps[FSM_MAX_STATES][FSM_MAX_STATES];
    char trans[FSM_MAX_STATES][FSM_MAX_SYMBOLS][FSM_MAX_STATES];
};

struct Dfa {
    int state_count;
    int symbol_count;
    char symbols[FSM_MAX_SYMBOLS];
    int start;
    char final[FSM_MAX_DFA_STATES];
    int trans[FSM_MAX_DFA_STATES][FSM_MAX_SYMBOLS];
};

/* --- НКА -------------------------------------------------------------- */

void nfa_init(struct Nfa *nfa);

/* Новое состояние; FSM_NO_STATE, если достигнут предел FSM_MAX_STATES. */
int nfa_add_state(struct Nfa *nfa);

/* Индекс символа в алфавите; символ добавляется, если его там ещё нет.
   FSM_NO_STATE, если алфавит переполнен. */
int nfa_symbol_index(struct Nfa *nfa, char symbol);

void nfa_add_transition(struct Nfa *nfa, int from, char symbol, int to);
void nfa_add_epsilon(struct Nfa *nfa, int from, int to);
void nfa_set_final(struct Nfa *nfa, int state, bool value);

/* eps-замыкание множества: на входе и выходе массив флагов длины
   state_count. */
void nfa_epsilon_closure(const struct Nfa *nfa, char *set);

/* Допускает ли НКА слово. Прямое моделирование по множеству состояний —
   то же, что делает детерминизация, но без запоминания подмножеств. */
bool nfa_accepts(const struct Nfa *nfa, const char *word);

void nfa_print(const struct Nfa *nfa, FILE *out);
void nfa_print_dot(const struct Nfa *nfa, FILE *out);

/* Чтение текстового формата. false — ошибка разбора, причина в stderr. */
bool nfa_read(struct Nfa *nfa, FILE *in);

/* --- ДКА -------------------------------------------------------------- */

void dfa_init(struct Dfa *dfa);
bool dfa_accepts(const struct Dfa *dfa, const char *word);
void dfa_print(const struct Dfa *dfa, FILE *out);
void dfa_print_dot(const struct Dfa *dfa, FILE *out);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_FSM_H */
