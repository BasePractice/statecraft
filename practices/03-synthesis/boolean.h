#ifndef STATECRAFT_BOOLEAN_H
#define STATECRAFT_BOOLEAN_H

/*
 * Лекция 3. Булева функция, заданная таблицей, и её минимизация методом
 * Квайна — Мак-Класки.
 *
 * Функция хранится таблицей значений на всех 2^n наборах. Значение
 * BOOL_DONT_CARE — «безразлично»: в лекции такие клетки помечены
 * звёздочками, и именно они позволяют получить формулу короче.
 */

#include "base_types.h"
#include <stdio.h>

#define BOOL_MAX_VARS 8                    /* переменных в функции */
#define BOOL_MAX_ROWS (1 << BOOL_MAX_VARS) /* строк таблицы истинности */
#define BOOL_MAX_IMPLICANTS 256            /* импликант на любом шаге */
#define BOOL_MAX_NAME 16

#define BOOL_ZERO 0
#define BOOL_ONE 1
#define BOOL_DONT_CARE 2

#if defined(__cplusplus)
extern "C" {
#endif

struct BoolFunction {
    int var_count;
    char names[BOOL_MAX_VARS][BOOL_MAX_NAME];
    char values[BOOL_MAX_ROWS];
};

/*
 * Импликанта: конъюнкция, в которой часть переменных исключена.
 * bits[i] — требуемое значение переменной i, mask[i] = 0 означает, что
 * переменная в конъюнкцию не входит.
 */
struct Implicant {
    char bits[BOOL_MAX_VARS];
    char mask[BOOL_MAX_VARS];
};

struct Dnf {
    struct Implicant terms[BOOL_MAX_IMPLICANTS];
    int count;
};

void bool_init(struct BoolFunction *function, int var_count);
void bool_set_name(struct BoolFunction *function, int var, const char *name);

/* Значение функции на наборе, заданном номером строки таблицы. */
void bool_set(struct BoolFunction *function, int row, char value);
char bool_get(const struct BoolFunction *function, int row);

/*
 * Минимизация Квайна — Мак-Класки: склеивание соседних наборов до простых
 * импликант, затем покрытие. Наборы BOOL_DONT_CARE участвуют в склеивании,
 * но покрывать их не нужно.
 *
 * Возвращает false, если промежуточных импликант оказалось больше
 * BOOL_MAX_IMPLICANTS.
 */
bool bool_minimize(struct Dnf *dnf, const struct BoolFunction *function);

/* Значение ДНФ на наборе — для проверки, что минимизация ничего не потеряла. */
char dnf_eval(const struct Dnf *dnf, const struct BoolFunction *function, int row);

/* Совпадает ли ДНФ с функцией на всех наборах, кроме безразличных. */
bool dnf_equals(const struct Dnf *dnf, const struct BoolFunction *function);

/* Печать формулы: «x1 & !x2 | q1». Константы печатаются как 0 и 1. */
void dnf_print(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out);

/* Печать в синтаксисе C: «(x1 && !x2) || q1». */
void dnf_print_c(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out);

/* Число литералов формулы — мера её сложности. */
int dnf_literals(const struct Dnf *dnf);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_BOOLEAN_H */
