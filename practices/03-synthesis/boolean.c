#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "boolean.h"

void bool_init(struct BoolFunction *function, int var_count) {
    int i;

    assert(function != NULL);
    assert(var_count > 0 && var_count <= BOOL_MAX_VARS);
    memset(function, 0, sizeof(*function));
    function->var_count = var_count;
    for (i = 0; i < var_count; ++i) {
        sprintf(function->names[i], "x%d", i + 1);
    }
}

void bool_set_name(struct BoolFunction *function, int var, const char *name) {
    assert(function != NULL && name != NULL);
    assert(var >= 0 && var < function->var_count);
    strncpy(function->names[var], name, BOOL_MAX_NAME - 1);
    function->names[var][BOOL_MAX_NAME - 1] = '\0';
}

void bool_set(struct BoolFunction *function, int row, char value) {
    assert(function != NULL);
    assert(row >= 0 && row < (1 << function->var_count));
    function->values[row] = value;
}

char bool_get(const struct BoolFunction *function, int row) {
    assert(function != NULL);
    assert(row >= 0 && row < (1 << function->var_count));
    return function->values[row];
}

/* Номер строки → набор значений переменных. Старший бит — первая переменная,
   как в таблицах лекции. */
static void row_to_bits(int row, int var_count, char *bits) {
    int i;

    /* Массив заполняется целиком, а не по var_count: все вызывающие держат
       буфер на BOOL_MAX_VARS, и разряды сверх используемых обязаны быть
       нулями. На частичном заполнении курс уже обжигался — импликанты
       получали мусор со стека (см. REPORT.md). GCC в Release видит здесь
       ровно это: -Wmaybe-uninitialized. */
    memset(bits, 0, BOOL_MAX_VARS);
    for (i = 0; i < var_count; ++i) {
        bits[i] = (char)((row >> (var_count - 1 - i)) & 1);
    }
}

static bool implicant_covers(const struct Implicant *implicant, const char *bits, int var_count) {
    int i;

    for (i = 0; i < var_count; ++i) {
        if (implicant->mask[i] && implicant->bits[i] != bits[i])
            return false;
    }
    return true;
}

static bool implicant_equal(const struct Implicant *a, const struct Implicant *b, int var_count) {
    int i;

    for (i = 0; i < var_count; ++i) {
        if (a->mask[i] != b->mask[i])
            return false;
        if (a->mask[i] && a->bits[i] != b->bits[i])
            return false;
    }
    return true;
}

static bool implicant_add(struct Dnf *dnf, const struct Implicant *implicant, int var_count) {
    int i;

    for (i = 0; i < dnf->count; ++i) {
        if (implicant_equal(&dnf->terms[i], implicant, var_count))
            return true;
    }
    if (dnf->count >= BOOL_MAX_IMPLICANTS)
        return false;
    dnf->terms[dnf->count++] = *implicant;
    return true;
}

/*
 * Шаг склеивания: две импликанты, различающиеся ровно в одной переменной
 * (при совпадающих масках), заменяются одной без этой переменной. Это и есть
 * тождество x·y ∨ x·!y = x, применяемое до тех пор, пока склеивать нечего.
 */
static bool merge_step(struct Dnf *out, const struct Dnf *in, char *used, int var_count) {
    int i;
    int j;
    int v;
    bool merged_any = false;

    out->count = 0;
    for (i = 0; i < in->count; ++i) {
        for (j = i + 1; j < in->count; ++j) {
            int diff = -1;
            bool same_mask = true;

            for (v = 0; v < var_count; ++v) {
                if (in->terms[i].mask[v] != in->terms[j].mask[v]) {
                    same_mask = false;
                    break;
                }
                if (in->terms[i].mask[v] && in->terms[i].bits[v] != in->terms[j].bits[v]) {
                    if (diff >= 0) {
                        diff = -2;
                        break;
                    }
                    diff = v;
                }
            }
            if (!same_mask || diff < 0)
                continue;

            {
                struct Implicant merged = in->terms[i];
                merged.mask[diff] = 0;
                merged.bits[diff] = 0;
                if (!implicant_add(out, &merged, var_count))
                    return false;
            }
            used[i] = 1;
            used[j] = 1;
            merged_any = true;
        }
    }
    (void)merged_any;
    return true;
}

bool bool_minimize(struct Dnf *dnf, const struct BoolFunction *function) {
    struct Dnf current;
    struct Dnf next;
    struct Dnf primes;
    char used[BOOL_MAX_IMPLICANTS];
    char covered[BOOL_MAX_ROWS];
    int rows;
    int row;
    int i;
    int v;

    assert(dnf != NULL && function != NULL);
    rows = 1 << function->var_count;
    dnf->count = 0;
    primes.count = 0;

    /* Начальные импликанты — все наборы, где функция равна 1 либо
       безразлична: безразличные помогают склеиванию. */
    current.count = 0;
    for (row = 0; row < rows; ++row) {
        char value = function->values[row];
        struct Implicant implicant;

        if (value != BOOL_ONE && value != BOOL_DONT_CARE)
            continue;
        /* Обнуление обязательно: разряды сверх var_count в сравнениях и
           подсчёте литералов участвуют наравне с остальными, а на стеке
           лежит мусор. В отладочной сборке он часто нулевой — и дефект
           виден только в Release. */
        memset(&implicant, 0, sizeof(implicant));
        row_to_bits(row, function->var_count, implicant.bits);
        for (v = 0; v < function->var_count; ++v) {
            implicant.mask[v] = 1;
        }
        if (!implicant_add(&current, &implicant, function->var_count))
            return false;
    }

    /* Склеиваем, пока склеивается. То, что не склеилось, — простая импликанта. */
    while (current.count > 0) {
        memset(used, 0, sizeof(used));
        if (!merge_step(&next, &current, used, function->var_count))
            return false;
        for (i = 0; i < current.count; ++i) {
            if (!used[i]) {
                if (!implicant_add(&primes, &current.terms[i], function->var_count))
                    return false;
            }
        }
        if (next.count == 0)
            break;
        current = next;
    }

    /* Покрытие. Сначала существенные импликанты: те, что единственными
       покрывают какой-нибудь набор. */
    memset(covered, 0, sizeof(covered));
    for (row = 0; row < rows; ++row) {
        char bits[BOOL_MAX_VARS];
        int cover_count = 0;
        int last = -1;

        if (function->values[row] != BOOL_ONE)
            continue;
        row_to_bits(row, function->var_count, bits);
        for (i = 0; i < primes.count; ++i) {
            if (implicant_covers(&primes.terms[i], bits, function->var_count)) {
                ++cover_count;
                last = i;
            }
        }
        if (cover_count == 1) {
            if (!implicant_add(dnf, &primes.terms[last], function->var_count))
                return false;
        }
    }

    /*
     * Затем — жадный добор: пока остались непокрытые наборы, берём импликанту,
     * покрывающую больше всего непокрытого.
     *
     * ВАЖНО: жадный выбор не гарантирует минимума. Точное решение — задача
     * о покрытии множества, она NP-полна (лекция 10). Для учебных примеров
     * жадный результат совпадает с ручным, но полагаться на это в общем
     * случае нельзя.
     */
    for (;;) {
        int best = -1;
        int best_gain = 0;

        for (row = 0; row < rows; ++row) {
            char bits[BOOL_MAX_VARS];

            if (function->values[row] != BOOL_ONE || covered[row])
                continue;
            row_to_bits(row, function->var_count, bits);
            for (i = 0; i < dnf->count; ++i) {
                if (implicant_covers(&dnf->terms[i], bits, function->var_count)) {
                    covered[row] = 1;
                    break;
                }
            }
        }

        for (i = 0; i < primes.count; ++i) {
            int gain = 0;

            for (row = 0; row < rows; ++row) {
                char bits[BOOL_MAX_VARS];

                if (function->values[row] != BOOL_ONE || covered[row])
                    continue;
                row_to_bits(row, function->var_count, bits);
                if (implicant_covers(&primes.terms[i], bits, function->var_count))
                    ++gain;
            }
            if (gain > best_gain) {
                best_gain = gain;
                best = i;
            }
        }

        if (best < 0)
            break;
        if (!implicant_add(dnf, &primes.terms[best], function->var_count))
            return false;
    }

    return true;
}

char dnf_eval(const struct Dnf *dnf, const struct BoolFunction *function, int row) {
    char bits[BOOL_MAX_VARS];
    int i;

    assert(dnf != NULL && function != NULL);
    row_to_bits(row, function->var_count, bits);
    for (i = 0; i < dnf->count; ++i) {
        if (implicant_covers(&dnf->terms[i], bits, function->var_count))
            return BOOL_ONE;
    }
    return BOOL_ZERO;
}

bool dnf_equals(const struct Dnf *dnf, const struct BoolFunction *function) {
    int rows;
    int row;

    assert(dnf != NULL && function != NULL);
    rows = 1 << function->var_count;
    for (row = 0; row < rows; ++row) {
        if (function->values[row] == BOOL_DONT_CARE)
            continue;
        if (dnf_eval(dnf, function, row) != function->values[row])
            return false;
    }
    return true;
}

static void print_formula(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out,
                          const char *and_op, const char *or_op, const char *not_op,
                          bool parentheses) {
    int i;
    int v;

    if (dnf->count == 0) {
        fprintf(out, "0");
        return;
    }
    for (i = 0; i < dnf->count; ++i) {
        int literals = 0;

        if (i > 0)
            fprintf(out, "%s", or_op);
        for (v = 0; v < function->var_count; ++v) {
            if (dnf->terms[i].mask[v])
                ++literals;
        }
        if (literals == 0) {
            fprintf(out, "1");
            continue;
        }
        if (parentheses && literals > 1 && dnf->count > 1)
            fprintf(out, "(");
        {
            int printed = 0;
            for (v = 0; v < function->var_count; ++v) {
                if (!dnf->terms[i].mask[v])
                    continue;
                if (printed > 0)
                    fprintf(out, "%s", and_op);
                if (!dnf->terms[i].bits[v])
                    fprintf(out, "%s", not_op);
                fprintf(out, "%s", function->names[v]);
                ++printed;
            }
        }
        if (parentheses && literals > 1 && dnf->count > 1)
            fprintf(out, ")");
    }
}

void dnf_print(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out) {
    print_formula(dnf, function, out, " & ", " | ", "!", false);
}

void dnf_print_c(const struct Dnf *dnf, const struct BoolFunction *function, FILE *out) {
    print_formula(dnf, function, out, " && ", " || ", "!", true);
}

int dnf_literals(const struct Dnf *dnf) {
    int total = 0;
    int i;
    int v;

    assert(dnf != NULL);
    for (i = 0; i < dnf->count; ++i) {
        for (v = 0; v < BOOL_MAX_VARS; ++v) {
            if (dnf->terms[i].mask[v])
                ++total;
        }
    }
    return total;
}
