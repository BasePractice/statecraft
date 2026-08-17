/*
 * Прикладные распознаватели форматов: автомат как таблица, а не как лестница
 * switch. Устройство таблицы и её отличия от лестницы — в applied_dfa.h.
 */

#include "applied_dfa.h"

#include <string.h>

#define DIGITS "0123456789"
#define ANY "^"

/* --- прогон ------------------------------------------------------------------ */

/* Принадлежит ли символ множеству. Множество, начинающееся с '^', —
   дополнение: «любой символ, кроме перечисленных». */
static bool in_set(const char *set, char symbol) {
    if (set[0] == '^') {
        return strchr(set + 1, symbol) == NULL;
    }
    return strchr(set, symbol) != NULL;
}

static bool is_accepting(const struct Dfa *dfa, int state) {
    int i;

    for (i = 0; dfa->accepting[i] >= 0; ++i) {
        if (dfa->accepting[i] == state) {
            return true;
        }
    }
    return false;
}

/* Один такт: возвращает следующее состояние или -1, если перехода нет.
   Правила просматриваются по порядку — первое подходящее и срабатывает. */
static int dfa_step(const struct Dfa *dfa, int state, char symbol) {
    int i;

    for (i = 0; i < dfa->rule_count; ++i) {
        if (dfa->rules[i].from == state && in_set(dfa->rules[i].set, symbol)) {
            return dfa->rules[i].to;
        }
    }
    return -1;
}

int dfa_match_prefix(const struct Dfa *dfa, const char *text) {
    int state;
    int i;

    if (dfa == NULL || text == NULL) {
        return 0;
    }
    state = dfa->start;
    for (i = 0; text[i] != '\0'; ++i) {
        state = dfa_step(dfa, state, text[i]);
        if (state < 0) {
            return i;
        }
    }
    return i;
}

bool dfa_match(const struct Dfa *dfa, const char *text) {
    int state;
    int i;

    if (dfa == NULL || text == NULL) {
        return false;
    }
    state = dfa->start;
    for (i = 0; text[i] != '\0'; ++i) {
        state = dfa_step(dfa, state, text[i]);
        if (state < 0) {
            return false;
        }
    }
    return is_accepting(dfa, state);
}

int dfa_state_count(const struct Dfa *dfa) {
    int highest = dfa->start;
    int i;

    for (i = 0; i < dfa->rule_count; ++i) {
        if (dfa->rules[i].from > highest) {
            highest = dfa->rules[i].from;
        }
        if (dfa->rules[i].to > highest) {
            highest = dfa->rules[i].to;
        }
    }
    for (i = 0; dfa->accepting[i] >= 0; ++i) {
        if (dfa->accepting[i] > highest) {
            highest = dfa->accepting[i];
        }
    }
    return highest + 1;
}

/* --- ISBN --------------------------------------------------------------------- */

/*
 * (ISBN:?)?\d{9}[\dxX]
 *
 *   0        начало
 *   1..4     буквы I, S, B, N
 *   5        двоеточие
 *   6..14    прочитано от одной до девяти цифр номера
 *   15       прочитан контрольный знак — принимающее состояние
 *
 * Девять цифр приходится расписывать девятью состояниями: у автомата нет
 * счётчика, и «ровно девять раз» выражается только длиной цепочки. Это первое,
 * что видно по таблице, и первое, что удивляет на практике.
 */
/* clang-format off */
static const struct DfaRule ISBN_RULES[] = {
    {0,  "I",      1},  /* необязательный префикс ISBN */
    {0,  DIGITS,   6},  /* или сразу номер */
    {1,  "S",      2},
    {2,  "B",      3},
    {3,  "N",      4},
    {4,  ":",      5},  /* двоеточие тоже необязательно */
    {4,  DIGITS,   6},
    {5,  DIGITS,   6},
    {6,  DIGITS,   7},  /* прочитана 1-я цифра, ждём 2-ю */
    {7,  DIGITS,   8},
    {8,  DIGITS,   9},
    {9,  DIGITS,  10},
    {10, DIGITS,  11},
    {11, DIGITS,  12},
    {12, DIGITS,  13},
    {13, DIGITS,  14},  /* прочитано 8 цифр, ждём 9-ю */
    {14, "0123456789xX", 15} /* контрольный знак: цифра или X */
};
/* clang-format on */

static const int ISBN_ACCEPTING[] = {15, -1};

static const struct Dfa ISBN = {"isbn",     "(ISBN:?)?\\d{9}[\\dxX]",
                                0,          ISBN_ACCEPTING,
                                ISBN_RULES, (int)(sizeof(ISBN_RULES) / sizeof(ISBN_RULES[0]))};

const struct Dfa *dfa_isbn(void) {
    return &ISBN;
}

/* --- парный тег ---------------------------------------------------------------- */

/*
 * <kill>.*</kill> | <pop>.*</pop>
 *
 * Тело тега — «любые символы», после которых идёт закрывающий тег. Тонкость
 * здесь одна, зато существенная: при несовпадении в закрывающем теге нельзя
 * просто вернуться в состояние «читаю тело». Прочитанный символ '<' сам может
 * оказаться началом настоящего закрывающего тега — как во входе
 *
 *     <kill><</kill>
 *
 * Поэтому из каждого состояния закрывающего тега есть три исхода: символ
 * подошёл — идём дальше; пришёл '<' — начинаем закрывающий тег заново;
 * иначе — возвращаемся в тело. Это тот же приём, что в автомате Кнута —
 * Морриса — Пратта, и без него распознаватель ошибается.
 *
 * Состояния «kill»:
 *   0 '<' ; 1 'k' ; 2 'i' ; 3 'l' ; 4 'l' ; 5 '>' ;
 *   6 тело ; 7..12 закрывающий тег ; 13 принимающее.
 * Состояния «pop» устроены так же и занимают номера 20..31.
 */
/* clang-format off */
static const struct DfaRule TAG_RULES[] = {
    {0,  "<",  1},
    {1,  "k",  2},
    {1,  "p", 20},

    /* <kill> */
    {2,  "i",  3},
    {3,  "l",  4},
    {4,  "l",  5},
    {5,  ">",  6},

    /* тело: всё, кроме '<', остаётся в теле */
    {6,  "<",  7},
    {6,  ANY,  6},

    /* закрывающий тег </kill>: подошло — дальше, '<' — сначала, иначе — тело */
    {7,  "/",  8},   {7,  "<",  7},  {7,  ANY,  6},
    {8,  "k",  9},   {8,  "<",  7},  {8,  ANY,  6},
    {9,  "i", 10},   {9,  "<",  7},  {9,  ANY,  6},
    {10, "l", 11},   {10, "<",  7},  {10, ANY,  6},
    {11, "l", 12},   {11, "<",  7},  {11, ANY,  6},
    {12, ">", 13},   {12, "<",  7},  {12, ANY,  6},

    /* тег закрыт; если строка на этом не кончилась, всё прочитанное было
       телом, и закрывающий тег ещё впереди */
    {13, "<",  7},   {13, ANY,  6},

    /* <pop> */
    {20, "o", 21},
    {21, "p", 22},
    {22, ">", 23},

    {23, "<", 24},
    {23, ANY, 23},

    {24, "/", 25},   {24, "<", 24},  {24, ANY, 23},
    {25, "p", 26},   {25, "<", 24},  {25, ANY, 23},
    {26, "o", 27},   {26, "<", 24},  {26, ANY, 23},
    {27, "p", 28},   {27, "<", 24},  {27, ANY, 23},
    {28, ">", 29},   {28, "<", 24},  {28, ANY, 23},

    {29, "<", 24},   {29, ANY, 23}
};
/* clang-format on */

static const int TAG_ACCEPTING[] = {13, 29, -1};

static const struct Dfa TAG = {"tag",     "<(kill|pop)>.*</\\1>",
                               0,         TAG_ACCEPTING,
                               TAG_RULES, (int)(sizeof(TAG_RULES) / sizeof(TAG_RULES[0]))};

const struct Dfa *dfa_tag(void) {
    return &TAG;
}

/* --- число с плавающей точкой --------------------------------------------------- */

/*
 * [+-]?\d+(\.\d+)?([eE][+-]?\d+)?
 *
 *   0 начало ; 1 после знака ; 2 целая часть (принимающее) ;
 *   3 точка ; 4 дробная часть (принимающее) ;
 *   5 после e ; 6 знак порядка ; 7 порядок (принимающее).
 *
 * Здесь состояний ровно столько, сколько «мест» в записи числа, и это как раз
 * тот случай, когда таблица читается легче регулярного выражения.
 */
/* clang-format off */
static const struct DfaRule NUMBER_RULES[] = {
    {0, "+-",    1},
    {0, DIGITS,  2},
    {1, DIGITS,  2},
    {2, DIGITS,  2},
    {2, ".",     3},
    {2, "eE",    5},
    {3, DIGITS,  4},
    {4, DIGITS,  4},
    {4, "eE",    5},
    {5, "+-",    6},
    {5, DIGITS,  7},
    {6, DIGITS,  7},
    {7, DIGITS,  7}
};
/* clang-format on */

static const int NUMBER_ACCEPTING[] = {2, 4, 7, -1};

static const struct Dfa NUMBER = {"number",
                                  "[+-]?\\d+(\\.\\d+)?([eE][+-]?\\d+)?",
                                  0,
                                  NUMBER_ACCEPTING,
                                  NUMBER_RULES,
                                  (int)(sizeof(NUMBER_RULES) / sizeof(NUMBER_RULES[0]))};

const struct Dfa *dfa_number(void) {
    return &NUMBER;
}

/* --- список ---------------------------------------------------------------------- */

static const struct Dfa *const ALL[] = {&ISBN, &TAG, &NUMBER, NULL};

const struct Dfa *dfa_by_index(int index) {
    int i;

    if (index < 0) {
        return NULL;
    }
    for (i = 0; ALL[i] != NULL; ++i) {
        if (i == index) {
            return ALL[i];
        }
    }
    return NULL;
}

const struct Dfa *dfa_by_name(const char *name) {
    int i;

    if (name == NULL) {
        return NULL;
    }
    for (i = 0; ALL[i] != NULL; ++i) {
        if (strcmp(ALL[i]->name, name) == 0) {
            return ALL[i];
        }
    }
    return NULL;
}
