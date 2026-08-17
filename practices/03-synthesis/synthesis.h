#ifndef STATECRAFT_SYNTHESIS_H
#define STATECRAFT_SYNTHESIS_H

/**
 * @file
 * Лекция 3. Синтез автоматной схемы: от таблицы переходов к булевым
 * функциям и коду.
 *
 * Порядок ровно тот же, что на лекции:
 *
 *   1. кодирование входных символов, выходных символов и состояний
 *      булевыми векторами (естественный код — номер символа в двоичной записи);
 *   2. построение таблиц истинности функций переходов φ и выходов ψ;
 *   3. минимизация каждой функции (boolean.h);
 *   4. печать системы канонических уравнений и порождение кода на C.
 *
 * Наборы, которым не соответствует ни один код состояния или входного
 * символа, помечаются как безразличные: на лекции это звёздочки в таблице.
 *
 * Формат описания автомата (строка на конструкцию, '#' — комментарий):
 *
 *     name coin              имя автомата (необязательно)
 *     inputs 1 3 5 10        входной алфавит
 *     outputs 0 1 2 3 4      выходной алфавит
 *     states 0 1 2           множество состояний, первое — начальное
 *     0 1 -> 1 0             из состояния 0 по входу 1 в состояние 1, выход 0
 */

#include "base_types.h"
#include "boolean.h"

#define SYN_MAX_SYMBOLS 16         /**< символов во входном и выходном алфавите */
#define SYN_MAX_STATES 16          /**< состояний автомата */
#define SYN_MAX_NAME 32            /**< длина имени символа или состояния */
#define SYN_MAX_BITS BOOL_MAX_VARS /**< разрядов кода: предел общий с boolean.h */

#if defined(__cplusplus)
extern "C" {
#endif

/** Автомат Мили, заданный таблицей переходов и выходов. */
struct Machine {
    char name[SYN_MAX_NAME];
    char inputs[SYN_MAX_SYMBOLS][SYN_MAX_NAME];
    int input_count;
    char outputs[SYN_MAX_SYMBOLS][SYN_MAX_NAME];
    int output_count;
    /** Состояния; первое из них — начальное. */
    char states[SYN_MAX_STATES][SYN_MAX_NAME];
    int state_count;
    /** Функция переходов; -1 — переход не задан (набор безразличен). */
    int next[SYN_MAX_STATES][SYN_MAX_SYMBOLS];
    /** Функция выходов; -1 — выход не задан (набор безразличен). */
    int emit[SYN_MAX_STATES][SYN_MAX_SYMBOLS];
};

/**
 * Результат синтеза: система канонических уравнений автомата.
 *
 * Для каждого разряда кода состояния хранится функция переходов φ, для
 * каждого разряда кода выхода — функция выходов ψ; рядом с каждой — её
 * минимальная ДНФ.
 */
struct Synthesis {
    int input_bits;  /**< разрядов кода входного символа */
    int state_bits;  /**< разрядов кода состояния */
    int output_bits; /**< разрядов кода выходного символа */
    struct BoolFunction phi[SYN_MAX_BITS];
    struct Dnf phi_dnf[SYN_MAX_BITS];
    struct BoolFunction psi[SYN_MAX_BITS];
    struct Dnf psi_dnf[SYN_MAX_BITS];
};

/** Автомат без состояний и символов. */
void machine_init(struct Machine *machine);

/**
 * Чтение описания автомата в формате, приведённом в начале файла.
 *
 * @return false при ошибке разбора; причина печатается в stderr.
 */
bool machine_read(struct Machine *machine, FILE *in);

void machine_print_table(const struct Machine *machine, FILE *out);

/** Число разрядов, нужное для кодирования @p count символов. */
int synthesis_bits(int count);

/**
 * Шаги 1–3: кодирование, таблицы истинности, минимизация.
 *
 * @return false, если кода не хватило разрядности (#SYN_MAX_BITS) или
 *         минимизация упёрлась в предел импликант.
 */
bool synthesis_run(struct Synthesis *synthesis, const struct Machine *machine);

/** Печать системы канонических уравнений в том же виде, что на лекции. */
void synthesis_print(const struct Synthesis *synthesis, const struct Machine *machine, FILE *out);

/** Порождение реализации автомата на Си по полученным формулам. */
void synthesis_print_c(const struct Synthesis *synthesis, const struct Machine *machine, FILE *out);

/**
 * Проверка: формулы воспроизводят исходную таблицу переходов.
 *
 * Синтез — преобразование, и оно обязано сохранять поведение. Именно этой
 * функцией в лекции 3 сверены формулы разменного аппарата.
 */
bool synthesis_verify(const struct Synthesis *synthesis, const struct Machine *machine);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_SYNTHESIS_H */
