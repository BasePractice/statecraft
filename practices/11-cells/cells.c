/*
 * Реализация трёх клеточных автоматов лекции 11.
 *
 * Общая черта всех трёх: новое состояние поля считается по старому целиком,
 * а не по мере обхода. Отсюда двойной буфер в каждом шаге — если обновлять
 * поле на месте, соседи справа увидят уже новые значения, и получится другой
 * автомат. Это самая частая ошибка в реализациях «Жизни».
 */

#include "cells.h"

#include <string.h>

/* --- одномерный автомат Вольфрама ---------------------------------------- */

void elementary_init(struct Elementary *ca, unsigned char rule, int width) {
    if (width > CELLS_MAX_WIDTH) {
        width = CELLS_MAX_WIDTH;
    }
    memset(ca, 0, sizeof(*ca));
    ca->rule = rule;
    ca->width = width;
    ca->cells[width / 2] = 1;
}

bool elementary_init_from(struct Elementary *ca, unsigned char rule, const char *pattern) {
    int i;
    int len = (int)strlen(pattern);

    if (len == 0 || len > CELLS_MAX_WIDTH) {
        return false;
    }
    memset(ca, 0, sizeof(*ca));
    ca->rule = rule;
    ca->width = len;
    for (i = 0; i < len; ++i) {
        if (pattern[i] != '0' && pattern[i] != '1') {
            return false;
        }
        ca->cells[i] = (char)(pattern[i] == '1');
    }
    return true;
}

void elementary_step(struct Elementary *ca) {
    char next[CELLS_MAX_WIDTH];
    int i;

    for (i = 0; i < ca->width; ++i) {
        int left = ca->cells[(i + ca->width - 1) % ca->width];
        int self = ca->cells[i];
        int right = ca->cells[(i + 1) % ca->width];
        /* Номер окрестности — трёхбитное число «левый центр правый»,
           состояние берётся как соответствующий бит номера правила. */
        int index = (left << 2) | (self << 1) | right;
        next[i] = (char)((ca->rule >> index) & 1);
    }
    memcpy(ca->cells, next, (size_t)ca->width);
}

void elementary_print(const struct Elementary *ca, FILE *out) {
    int i;
    for (i = 0; i < ca->width; ++i) {
        fputc(ca->cells[i] ? '#' : '.', out);
    }
    fputc('\n', out);
}

/* --- игра «Жизнь» --------------------------------------------------------- */

void life_init(struct Life *life, int width, int height) {
    memset(life, 0, sizeof(*life));
    life->width = (width > LIFE_MAX_SIDE) ? LIFE_MAX_SIDE : width;
    life->height = (height > LIFE_MAX_SIDE) ? LIFE_MAX_SIDE : height;
}

static int wrap(int value, int limit) {
    int result = value % limit;
    if (result < 0) {
        result += limit;
    }
    return result;
}

void life_set(struct Life *life, int x, int y, bool alive) {
    life->cells[wrap(y, life->height)][wrap(x, life->width)] = (char)(alive ? 1 : 0);
}

bool life_get(const struct Life *life, int x, int y) {
    return life->cells[wrap(y, life->height)][wrap(x, life->width)] != 0;
}

/* Фигуры записаны построчно: '#' — живая клетка, всё остальное — пустая.
   Свойство каждой фигуры (период, скорость, время жизни) проверяется тестом
   практики: фигура, взятая из литературы и не проверенная прогоном, рано или
   поздно оказывается записанной с ошибкой. */
/* clang-format off */

/* Натюрморты: не меняются вовсе. */
static const char *const BLOCK[] = {"##", "##", NULL};
static const char *const BEEHIVE[] = {".##.", "#..#", ".##.", NULL};
static const char *const LOAF[] = {".##.", "#..#", ".#.#", "..#.", NULL};
static const char *const BOAT[] = {"##.", "#.#", ".#.", NULL};
static const char *const TUB[] = {".#.", "#.#", ".#.", NULL};

/* Осцилляторы: повторяются через период. */
static const char *const BLINKER[] = {"###", NULL};
static const char *const TOAD[] = {".###", "###.", NULL};
static const char *const BEACON[] = {"##..", "##..", "..##", "..##", NULL};
static const char *const PULSAR[] = {
    "..###...###..",
    ".............",
    "#....#.#....#",
    "#....#.#....#",
    "#....#.#....#",
    "..###...###..",
    ".............",
    "..###...###..",
    "#....#.#....#",
    "#....#.#....#",
    "#....#.#....#",
    ".............",
    "..###...###..",
    NULL
};
/* Пентадекатлон задаётся рядом из десяти клеток: собственная его форма из
   этого ряда и получается на первых же ходах. */
static const char *const PENTADECATHLON[] = {"##########", NULL};

/* Корабли: воспроизводят себя со сдвигом. */
static const char *const GLIDER[] = {".#.", "..#", "###", NULL};
static const char *const LWSS[] = {"#..#.", "....#", "#...#", ".####", NULL};
static const char *const MWSS[] = {"..#...", "#...#.", ".....#", "#....#", ".#####", NULL};
static const char *const HWSS[] = {"..##...", "#....#.", "......#", "#.....#", ".######", NULL};

/* Долгая эволюция из горстки клеток. */
static const char *const R_PENTOMINO[] = {".##", "##.", ".#.", NULL};
static const char *const DIEHARD[] = {"......#.", "##......", ".#...###", NULL};
static const char *const ACORN[] = {".#.....", "...#...", "##..###", NULL};

/* Пожиратель: натюрморт, который уничтожает налетевший на него планер. */
static const char *const EATER[] = {"##..", "#.#.", "..#.", "..##", NULL};

/*
 * Ружьё Госпера — первая найденная конфигурация с неограниченным ростом
 * (Билл Госпер, 1970): каждые 30 поколений выпускает планер, поэтому
 * население растёт на пять клеток за период.
 */
static const char *const GOSPER_GUN[] = {
    "........................#...........",
    "......................#.#...........",
    "............##......##............##",
    "...........#...#....##............##",
    "##........#.....#...##..............",
    "##........#...#.##....#.#...........",
    "..........#.....#.......#...........",
    "...........#...#....................",
    "............##......................",
    NULL
};
/* clang-format on */

struct Pattern {
    const char *name;
    const char *const *rows;
};

static const struct Pattern PATTERNS[] = {{"block", BLOCK},
                                          {"beehive", BEEHIVE},
                                          {"loaf", LOAF},
                                          {"boat", BOAT},
                                          {"tub", TUB},
                                          {"blinker", BLINKER},
                                          {"toad", TOAD},
                                          {"beacon", BEACON},
                                          {"pulsar", PULSAR},
                                          {"pentadecathlon", PENTADECATHLON},
                                          {"glider", GLIDER},
                                          {"lwss", LWSS},
                                          {"mwss", MWSS},
                                          {"hwss", HWSS},
                                          {"r-pentomino", R_PENTOMINO},
                                          {"diehard", DIEHARD},
                                          {"acorn", ACORN},
                                          {"eater", EATER},
                                          {"gosper-gun", GOSPER_GUN},
                                          {NULL, NULL}};

static const char *const *pattern_by_name(const char *name) {
    int i;

    for (i = 0; PATTERNS[i].name != NULL; ++i) {
        if (strcmp(name, PATTERNS[i].name) == 0) {
            return PATTERNS[i].rows;
        }
    }
    return NULL;
}

const char *life_pattern_name(int index) {
    int i;

    for (i = 0; PATTERNS[i].name != NULL; ++i) {
        if (i == index) {
            return PATTERNS[i].name;
        }
    }
    return NULL;
}

bool life_place(struct Life *life, const char *name, int x, int y) {
    const char *const *rows = pattern_by_name(name);
    int row;

    if (rows == NULL) {
        return false;
    }
    for (row = 0; rows[row] != NULL; ++row) {
        int col;
        for (col = 0; rows[row][col] != '\0'; ++col) {
            if (rows[row][col] == '#') {
                life_set(life, x + col, y + row, true);
            }
        }
    }
    return true;
}

static int neighbours(const struct Life *life, int x, int y) {
    int dx;
    int dy;
    int count = 0;

    for (dy = -1; dy <= 1; ++dy) {
        for (dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            if (life_get(life, x + dx, y + dy)) {
                ++count;
            }
        }
    }
    return count;
}

void life_step(struct Life *life) {
    char next[LIFE_MAX_SIDE][LIFE_MAX_SIDE];
    int x;
    int y;

    memset(next, 0, sizeof(next));
    for (y = 0; y < life->height; ++y) {
        for (x = 0; x < life->width; ++x) {
            int n = neighbours(life, x, y);
            bool alive = life_get(life, x, y);
            /* Выживание — два или три соседа, рождение — ровно три. */
            next[y][x] = (char)((alive && (n == 2 || n == 3)) || (!alive && n == 3));
        }
    }
    memcpy(life->cells, next, sizeof(next));
}

int life_population(const struct Life *life) {
    int x;
    int y;
    int count = 0;

    for (y = 0; y < life->height; ++y) {
        for (x = 0; x < life->width; ++x) {
            count += life->cells[y][x] ? 1 : 0;
        }
    }
    return count;
}

bool life_equal_shifted(const struct Life *a, const struct Life *b, int dx, int dy) {
    int x;
    int y;

    if (a->width != b->width || a->height != b->height) {
        return false;
    }
    for (y = 0; y < a->height; ++y) {
        for (x = 0; x < a->width; ++x) {
            if (life_get(a, x, y) != life_get(b, x + dx, y + dy)) {
                return false;
            }
        }
    }
    return true;
}

void life_print(const struct Life *life, FILE *out) {
    int x;
    int y;

    for (y = 0; y < life->height; ++y) {
        for (x = 0; x < life->width; ++x) {
            fputc(life->cells[y][x] ? '#' : '.', out);
        }
        fputc('\n', out);
    }
}

/* --- муравей Лэнгтона ----------------------------------------------------- */

void ant_init(struct Ant *ant, int side) {
    memset(ant, 0, sizeof(*ant));
    ant->side = (side > ANT_MAX_SIDE) ? ANT_MAX_SIDE : side;
    ant->x = ant->side / 2;
    ant->y = ant->side / 2;
    ant->dir = ANT_UP;
}

bool ant_step(struct Ant *ant) {
    if (ant->escaped) {
        return false;
    }

    if (ant->cells[ant->y][ant->x]) {
        /* Чёрная клетка: поворот налево. */
        ant->dir = (enum AntDirection)((ant->dir + 3) % 4);
    } else {
        ant->dir = (enum AntDirection)((ant->dir + 1) % 4);
    }
    ant->cells[ant->y][ant->x] = (char)!ant->cells[ant->y][ant->x];

    switch (ant->dir) {
    case ANT_UP:
        --ant->y;
        break;
    case ANT_RIGHT:
        ++ant->x;
        break;
    case ANT_DOWN:
        ++ant->y;
        break;
    case ANT_LEFT:
        --ant->x;
        break;
    default:
        break;
    }
    ++ant->steps;

    if (ant->x < 0 || ant->y < 0 || ant->x >= ant->side || ant->y >= ant->side) {
        ant->escaped = true;
        return false;
    }
    return true;
}

int ant_black_count(const struct Ant *ant) {
    int x;
    int y;
    int count = 0;

    for (y = 0; y < ant->side; ++y) {
        for (x = 0; x < ant->side; ++x) {
            count += ant->cells[y][x] ? 1 : 0;
        }
    }
    return count;
}

void ant_print(const struct Ant *ant, FILE *out) {
    int x;
    int y;

    for (y = 0; y < ant->side; ++y) {
        for (x = 0; x < ant->side; ++x) {
            if (x == ant->x && y == ant->y && !ant->escaped) {
                fputc('@', out);
            } else {
                fputc(ant->cells[y][x] ? '#' : '.', out);
            }
        }
        fputc('\n', out);
    }
}
