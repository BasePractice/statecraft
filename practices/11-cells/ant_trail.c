/*
 * Учебная тропа и прогон автомата муравья по ней.
 *
 * Карта — 32 строки по 32 символа: '#' — еда, '.' — клетка тропы без еды
 * (разрыв), пробел — пустое поле. Менять её нельзя: результаты разных
 * стратегий сравнимы только на одной и той же тропе.
 *
 * Это учебная тропа, а не каноническая тропа Санта-Фе (89 клеток еды), —
 * см. комментарий в заголовке.
 */

#include "ant_trail.h"

#include <string.h>

/* Учебная тропа: ломаная с поворотами и разрывами возрастающей длины. */
static const char *const TRAIL[TRAIL_SIDE] = {
    " #########.######..#####        ",
    "                      ##        ",
    "     ######.############        ",
    "                       #        ",
    "                       #        ",
    "                       #        ",
    "                       #        ",
    "                       .        ",
    "                       #        ",
    "                       #        ",
    "                       #        ",
    "                       #        ",
    "  ####..######.#########        ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  .                             ",
    "  #                             ",
    "  #                             ",
    "  #                             ",
    "  ########.######..####         ",
    "                      #         ",
    "                      #         ",
    "                      #         ",
    "                      #         ",
    "                      #         ",
    "                      .         ",
    "                      #         ",
};

enum Direction { DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_NORTH };

struct World {
    char food[TRAIL_SIDE][TRAIL_SIDE];
    int x;
    int y;
    enum Direction dir;
};

static void world_init(struct World *w) {
    int y;
    int x;

    memset(w, 0, sizeof(*w));
    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            w->food[y][x] = (char)(TRAIL[y][x] == '#');
        }
    }
    w->x = 0;
    w->y = 0;
    w->dir = DIR_EAST;
}

/* Поле замкнуто в тор: муравей, ушедший за край, появляется с другой
   стороны. Так задача не зависит от того, где нарисована граница. */
static void ahead(const struct World *w, int *nx, int *ny) {
    int dx = 0;
    int dy = 0;

    switch (w->dir) {
    case DIR_EAST:
        dx = 1;
        break;
    case DIR_SOUTH:
        dy = 1;
        break;
    case DIR_WEST:
        dx = -1;
        break;
    case DIR_NORTH:
        dy = -1;
        break;
    default:
        break;
    }
    *nx = (w->x + dx + TRAIL_SIDE) % TRAIL_SIDE;
    *ny = (w->y + dy + TRAIL_SIDE) % TRAIL_SIDE;
}

static bool food_ahead(const struct World *w) {
    int nx;
    int ny;

    ahead(w, &nx, &ny);
    return w->food[ny][nx] != 0;
}

int ant_trail_food_total(void) {
    int y;
    int x;
    int count = 0;

    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            count += (TRAIL[y][x] == '#') ? 1 : 0;
        }
    }
    return count;
}

void ant_fsm_reference(struct AntFsm *fsm) {
    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = 6;

    /*
     * Стратегия: пока еда впереди — идти по ней; когда еда кончилась —
     * обшарить окрестность известной последовательностью поворотов, которая
     * находит продолжение тропы через любой из встречающихся на ней разрывов.
     *
     * Состояния 1..5 и есть эта последовательность: каждое помнит, сколько
     * шагов поиска уже сделано. Как только еда найдена, автомат из любого
     * состояния возвращается в 0 — «иду по тропе».
     */
    /* 0: движение по тропе */
    fsm->action[0][1] = ANT_STEP;       fsm->next[0][1] = 0;
    fsm->action[0][0] = ANT_TURN_RIGHT; fsm->next[0][0] = 1;
    /* 1: посмотреть направо */
    fsm->action[1][1] = ANT_STEP;       fsm->next[1][1] = 0;
    fsm->action[1][0] = ANT_TURN_LEFT;  fsm->next[1][0] = 2;
    /* 2: вернуться на курс */
    fsm->action[2][1] = ANT_STEP;       fsm->next[2][1] = 0;
    fsm->action[2][0] = ANT_TURN_LEFT;  fsm->next[2][0] = 3;
    /* 3: посмотреть налево */
    fsm->action[3][1] = ANT_STEP;       fsm->next[3][1] = 0;
    fsm->action[3][0] = ANT_TURN_RIGHT; fsm->next[3][0] = 4;
    /* 4: разрыв длиннее одной клетки — шагнуть вперёд вслепую */
    fsm->action[4][1] = ANT_STEP;       fsm->next[4][1] = 0;
    fsm->action[4][0] = ANT_STEP;       fsm->next[4][0] = 5;
    /* 5: второй слепой шаг, затем осмотр начинается заново */
    fsm->action[5][1] = ANT_STEP;       fsm->next[5][1] = 0;
    fsm->action[5][0] = ANT_STEP;       fsm->next[5][0] = 0;
}

/*
 * Стратегия без слепых шагов: только осмотр вправо-влево. Её достаточно,
 * пока разрывы тропы не длиннее одной клетки; на первом же разрыве в две
 * клетки муравей встаёт. Автомат нужен для сравнения в лекции и тестах.
 */
void ant_fsm_lookaround(struct AntFsm *fsm) {
    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = 4;
    fsm->action[0][1] = ANT_STEP;       fsm->next[0][1] = 0;
    fsm->action[0][0] = ANT_TURN_RIGHT; fsm->next[0][0] = 1;
    fsm->action[1][1] = ANT_STEP;       fsm->next[1][1] = 0;
    fsm->action[1][0] = ANT_TURN_LEFT;  fsm->next[1][0] = 2;
    fsm->action[2][1] = ANT_STEP;       fsm->next[2][1] = 0;
    fsm->action[2][0] = ANT_TURN_LEFT;  fsm->next[2][0] = 3;
    fsm->action[3][1] = ANT_STEP;       fsm->next[3][1] = 0;
    fsm->action[3][0] = ANT_TURN_RIGHT; fsm->next[3][0] = 0;
}

static struct TrailRun run(const struct AntFsm *fsm, int steps_limit, struct World *w) {
    struct TrailRun result;
    int state = 0;
    int step;

    world_init(w);
    result.eaten = 0;
    result.total = ant_trail_food_total();
    result.steps = 0;
    result.finished = false;

    for (step = 0; step < steps_limit; ++step) {
        int input = food_ahead(w) ? 1 : 0;
        enum AntAction action = fsm->action[state][input];
        int next_state = fsm->next[state][input];

        switch (action) {
        case ANT_STEP: {
            int nx;
            int ny;
            ahead(w, &nx, &ny);
            w->x = nx;
            w->y = ny;
            if (w->food[ny][nx]) {
                w->food[ny][nx] = 0;
                ++result.eaten;
            }
            break;
        }
        case ANT_TURN_LEFT:
            w->dir = (enum Direction)((w->dir + 3) % 4);
            break;
        case ANT_TURN_RIGHT:
            w->dir = (enum Direction)((w->dir + 1) % 4);
            break;
        default:
            break;
        }

        state = next_state;
        result.steps = step + 1;
        if (result.eaten == result.total) {
            result.finished = true;
            break;
        }
    }
    return result;
}

struct TrailRun ant_trail_run(const struct AntFsm *fsm, int steps_limit) {
    struct World w;
    return run(fsm, steps_limit, &w);
}

void ant_trail_print(const struct AntFsm *fsm, int steps_limit, FILE *out) {
    struct World w;
    struct TrailRun r = run(fsm, steps_limit, &w);
    int y;
    int x;

    fprintf(out, "Съедено %d из %d за %d тактов%s\n\n", r.eaten, r.total, r.steps,
            r.finished ? " (тропа пройдена)" : "");
    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            if (x == w.x && y == w.y) {
                fputc('@', out);
            } else if (w.food[y][x]) {
                fputc('#', out);
            } else if (TRAIL[y][x] == '#') {
                fputc('o', out);
            } else {
                fputc('.', out);
            }
        }
        fputc('\n', out);
    }
}
