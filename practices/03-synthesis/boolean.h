#ifndef STATECRAFT_BOOLEAN_H
#define STATECRAFT_BOOLEAN_H

/**
 * @file
 * Лекция 3. Булева функция, заданная таблицей, и её минимизация методом
 * Квайна — Мак-Класки.
 *
 * Функция хранится таблицей значений на всех 2^n наборах. Значение
 * #BOOL_DONT_CARE — «безразлично»: в лекции такие клетки помечены
 * звёздочками, и именно они позволяют получить формулу короче.
 *
 * Этим кодом проверены выкладки лекции: формулы синтеза разменного
 * аппарата, набранные в тексте, сверены с тем, что печатает
 * #dnf_print_c — раскрытием, а не на глаз.
 */

#include "base_types.h"
#include <stdio.h>

#define BOOL_MAX_VARS 8                    /**< переменных в функции */
#define BOOL_MAX_ROWS (1 << BOOL_MAX_VARS) /**< строк таблицы истинности */
#define BOOL_MAX_IMPLICANTS 256            /**< импликант на любом шаге */
#define BOOL_MAX_NAME 16                   /**< длина имени переменной */

#define BOOL_ZERO 0      /**< значение функции на наборе — ложь */
#define BOOL_ONE 1       /**< значение функции на наборе — истина */
#define BOOL_DONT_CARE 2 /**< набор невозможен: значение безразлично */

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Число строк таблицы истинности функции @p var_count переменных, то есть
 * 2 в степени @p var_count.
 *
 * Отдельная функция, а не выражение `1 << n` по месту: сдвиг знаковой единицы
 * — источник неопределённого поведения при большом сдвиге, и анализатор
 * справедливо указывает на него (MISRA 10.1). Здесь сдвиг беззнаковый, а
 * наружу отдаётся `int`: таблица истинности курса заведомо помещается в него.
 */
int bool_row_count(int var_count);

/** Булева функция n переменных, заданная таблицей истинности. */
struct BoolFunction {
    int var_count;                            /**< число переменных, n */
    char names[BOOL_MAX_VARS][BOOL_MAX_NAME]; /**< имена для печати формул */
    /** Значения на всех 2^n наборах: #BOOL_ZERO, #BOOL_ONE, #BOOL_DONT_CARE. */
    char values[BOOL_MAX_ROWS];
};

/**
 * Импликанта: конъюнкция, в которой часть переменных исключена.
 *
 * `bits[i]` — требуемое значение переменной i, `mask[i] = 0` означает, что
 * переменная в конъюнкцию не входит.
 *
 * @note Структура заполняется целиком, включая разряды сверх `var_count`:
 * на частичном заполнении курс уже обжигался — в Release там оставался
 * мусор со стека, и два теста падали при зелёном Debug.
 */
struct Implicant {
    char bits[BOOL_MAX_VARS];
    char mask[BOOL_MAX_VARS];
};

/** Дизъюнктивная нормальная форма — множество импликант. */
struct Dnf {
    struct Implicant terms[BOOL_MAX_IMPLICANTS];
    int count;
};

/** Функция n переменных, всюду равная нулю. Имена — `x1`, `x2`, ... */
void bool_init(struct BoolFunction *function, int var_count);

/** Имя переменной для печати формул; длиннее #BOOL_MAX_NAME обрезается. */
void bool_set_name(struct BoolFunction *function, int var, const char *name);

/**
 * Значение функции на наборе, заданном номером строки таблицы.
 *
 * Старший разряд номера — первая переменная, как в таблицах лекции.
 */
void bool_set(struct BoolFunction *function, int row, char value);

char bool_get(const struct BoolFunction *function, int row);

/**
 * Минимизация Квайна — Мак-Класки.
 *
 * Склеивание соседних наборов до простых импликант, затем покрытие. Наборы
 * #BOOL_DONT_CARE участвуют в склеивании, но покрывать их не нужно — на
 * этом и достигается сокращение формулы.
 *
 * @return false, если промежуточных импликант оказалось больше
 *         #BOOL_MAX_IMPLICANTS.
 */
bool bool_minimize(struct Dnf *dnf, const struct BoolFunction *function);

/** Значение ДНФ на наборе — для проверки, что минимизация ничего не потеряла. */
char dnf_eval(const struct Dnf *dnf, const struct BoolFunction *function, int row);

/** Совпадает ли ДНФ с функцией на всех наборах, кроме безразличных. */
bool dnf_equals(const struct Dnf *dnf, const struct BoolFunction *function);

/** Печать формулы: «x1 & !x2 | q1». Константы печатаются как 0 и 1. */
void dnf_print(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out);

/** Печать в синтаксисе Си: «(x1 && !x2) || q1». */
void dnf_print_c(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out);

/** Число литералов формулы — мера её сложности. */
int dnf_literals(const struct Dnf *dnf);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_BOOLEAN_H */
