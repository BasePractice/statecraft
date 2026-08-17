/*
 * Векторный вывод: SVG без внешних зависимостей.
 *
 * Формат выбран из тех же соображений, что и всё остальное в практикуме:
 * файл пишется обычным fprintf, читается глазами, вставляется в typst одной
 * строкой и не тянет за собой библиотек. Цвета заданы серой шкалой — лекция
 * печатается на бумаге, и картинка обязана оставаться читаемой без цвета.
 */

#include "render.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define RENDER_CELL 10
#define RENDER_GAP 16
#define RENDER_MARGIN 6

/* Серая шкала: живая клетка, съеденная еда, пройденный путь, сетка. */
#define COLOR_ALIVE "#1a1a1a"
#define COLOR_FOOD "#1a1a1a"
#define COLOR_EATEN "#b0b0b0"
#define COLOR_VISITED "#e8e8e8"
#define COLOR_TRAIL "#f4f4f4"
#define COLOR_GRID "#cccccc"
#define COLOR_ANT "#000000"

void render_style_default(struct RenderStyle *style) {
    style->cell = RENDER_CELL;
    style->gap = RENDER_GAP;
    style->margin = RENDER_MARGIN;
    style->grid = true;
}

static struct RenderStyle effective(const struct RenderStyle *style) {
    struct RenderStyle result;

    render_style_default(&result);
    if (style != NULL) {
        if (style->cell > 0) {
            result.cell = style->cell;
        }
        if (style->gap > 0) {
            result.gap = style->gap;
        }
        if (style->margin > 0) {
            result.margin = style->margin;
        }
        result.grid = style->grid;
    }
    return result;
}

static void svg_open(FILE *out, int width, int height) {
    fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" ", width, height);
    fprintf(out, "width=\"%d\" height=\"%d\">\n", width, height);
    fprintf(out, "<rect width=\"%d\" height=\"%d\" fill=\"#ffffff\"/>\n", width, height);
}

static void svg_close(FILE *out) {
    fprintf(out, "</svg>\n");
}

static void svg_cell(FILE *out, int x, int y, int size, const char *color) {
    fprintf(out, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"%s\"/>\n", x, y, size,
            size, color);
}

/* Сетка поля: тонкие линии по границам клеток. */
static void svg_grid(FILE *out, int ox, int oy, int cols, int rows, int cell) {
    int i;

    fprintf(out, "<g stroke=\"%s\" stroke-width=\"0.5\">\n", COLOR_GRID);
    for (i = 0; i <= cols; ++i) {
        fprintf(out, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\"/>\n", ox + i * cell, oy,
                ox + i * cell, oy + rows * cell);
    }
    for (i = 0; i <= rows; ++i) {
        fprintf(out, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\"/>\n", ox, oy + i * cell,
                ox + cols * cell, oy + i * cell);
    }
    fprintf(out, "</g>\n");
}

/* Рамка кадра: без неё соседние кадры ленты сливаются. */
static void svg_frame(FILE *out, int ox, int oy, int cols, int rows, int cell) {
    fprintf(out,
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"none\" stroke=\"#666666\" "
            "stroke-width=\"1\"/>\n",
            ox, oy, cols * cell, rows * cell);
}

/* --- «Жизнь» ---------------------------------------------------------------- */

/* Границы области, в которой хоть в одном кадре есть живые клетки. Кадры
   обрезаются по ней одинаково: иначе фигура, движущаяся по полю, на каждом
   кадре оказывалась бы в середине, и движение по ленте не читалось бы. */
struct Bounds {
    int left;
    int top;
    int right;
    int bottom;
};

static bool life_bounds(const char *name, int width, int height, const int *generations, int count,
                        struct Bounds *bounds) {
    struct Life life;
    int generation = 0;
    int i;
    bool found = false;

    life_init(&life, width, height);
    if (!life_place(&life, name, 1, 1)) {
        return false;
    }
    bounds->left = life.width;
    bounds->top = life.height;
    bounds->right = -1;
    bounds->bottom = -1;

    for (i = 0; i < count; ++i) {
        int x;
        int y;

        while (generation < generations[i]) {
            life_step(&life);
            ++generation;
        }
        for (y = 0; y < life.height; ++y) {
            for (x = 0; x < life.width; ++x) {
                if (!life_get(&life, x, y)) {
                    continue;
                }
                found = true;
                if (x < bounds->left) {
                    bounds->left = x;
                }
                if (x > bounds->right) {
                    bounds->right = x;
                }
                if (y < bounds->top) {
                    bounds->top = y;
                }
                if (y > bounds->bottom) {
                    bounds->bottom = y;
                }
            }
        }
    }
    if (!found) {
        /* Всё вымерло: показываем угол поля, чтобы кадр не был пустым нулевого
           размера. */
        bounds->left = 0;
        bounds->top = 0;
        bounds->right = 3;
        bounds->bottom = 3;
    }
    /* Поле в две клетки вокруг: фигура не должна упираться в рамку. */
    bounds->left = (bounds->left > 2) ? bounds->left - 2 : 0;
    bounds->top = (bounds->top > 2) ? bounds->top - 2 : 0;
    bounds->right = (bounds->right + 2 < life.width) ? bounds->right + 2 : life.width - 1;
    bounds->bottom = (bounds->bottom + 2 < life.height) ? bounds->bottom + 2 : life.height - 1;
    return true;
}

bool render_life_svg(const char *name, int width, int height, const int *generations, int count,
                     const struct RenderStyle *style, FILE *out) {
    struct RenderStyle st = effective(style);
    struct Bounds bounds;
    struct Life life;
    int cols;
    int rows;
    int frame_width;
    int total_width;
    int total_height;
    int generation = 0;
    int i;

    if (count < 1 || generations == NULL || width < 1 || height < 1) {
        return false;
    }
    if (!life_bounds(name, width, height, generations, count, &bounds)) {
        return false;
    }
    life_init(&life, width, height);
    if (!life_place(&life, name, 1, 1)) {
        return false;
    }

    cols = bounds.right - bounds.left + 1;
    rows = bounds.bottom - bounds.top + 1;
    frame_width = cols * st.cell;
    total_width = 2 * st.margin + count * frame_width + (count - 1) * st.gap;
    total_height = 2 * st.margin + rows * st.cell;
    svg_open(out, total_width, total_height);

    for (i = 0; i < count; ++i) {
        int ox = st.margin + i * (frame_width + st.gap);
        int oy = st.margin;
        int x;
        int y;

        while (generation < generations[i]) {
            life_step(&life);
            ++generation;
        }
        if (st.grid) {
            svg_grid(out, ox, oy, cols, rows, st.cell);
        }
        for (y = 0; y < rows; ++y) {
            for (x = 0; x < cols; ++x) {
                if (life_get(&life, bounds.left + x, bounds.top + y)) {
                    svg_cell(out, ox + x * st.cell, oy + y * st.cell, st.cell, COLOR_ALIVE);
                }
            }
        }
        svg_frame(out, ox, oy, cols, rows, st.cell);
    }
    svg_close(out);
    return true;
}

/* --- сцены ------------------------------------------------------------------- */

struct ScenePart {
    const char *pattern;
    int x;
    int y;
};

struct Scene {
    const char *name;
    int side;
    struct ScenePart parts[4];
};

/*
 * Пожиратель и планер: планер летит по диагонали вниз-вправо и через
 * несколько ходов упирается в пожирателя. Тот проглатывает планер и
 * восстанавливает свою форму — конфигурация из семи клеток, которая чистит
 * поле от мусора (Госпер, 1971).
 */
static const struct Scene SCENES[] = {
        {"eater-vs-glider", 20, {{"eater", 11, 11}, {"glider", 4, 4}, {NULL, 0, 0}, {NULL, 0, 0}}},
        {NULL, 0, {{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}}}};

static const struct Scene *scene_by_name(const char *name) {
    int i;

    for (i = 0; SCENES[i].name != NULL; ++i) {
        if (strcmp(name, SCENES[i].name) == 0) {
            return &SCENES[i];
        }
    }
    return NULL;
}

static bool scene_setup(const struct Scene *scene, struct Life *life) {
    int i;

    life_init(life, scene->side, scene->side);
    for (i = 0; i < 4 && scene->parts[i].pattern != NULL; ++i) {
        if (!life_place(life, scene->parts[i].pattern, scene->parts[i].x, scene->parts[i].y)) {
            return false;
        }
    }
    return true;
}

bool render_scene_svg(const char *name, const int *generations, int count,
                      const struct RenderStyle *style, FILE *out) {
    struct RenderStyle st = effective(style);
    const struct Scene *scene = (name != NULL) ? scene_by_name(name) : NULL;
    struct Life life;
    int frame_width;
    int total_width;
    int total_height;
    int generation = 0;
    int i;

    if (scene == NULL || generations == NULL || count < 1) {
        return false;
    }
    if (!scene_setup(scene, &life)) {
        return false;
    }

    frame_width = life.width * st.cell;
    total_width = 2 * st.margin + count * frame_width + (count - 1) * st.gap;
    total_height = 2 * st.margin + life.height * st.cell;
    svg_open(out, total_width, total_height);

    for (i = 0; i < count; ++i) {
        int ox = st.margin + i * (frame_width + st.gap);
        int oy = st.margin;
        int x;
        int y;

        while (generation < generations[i]) {
            life_step(&life);
            ++generation;
        }
        if (st.grid) {
            svg_grid(out, ox, oy, life.width, life.height, st.cell);
        }
        for (y = 0; y < life.height; ++y) {
            for (x = 0; x < life.width; ++x) {
                if (life_get(&life, x, y)) {
                    svg_cell(out, ox + x * st.cell, oy + y * st.cell, st.cell, COLOR_ALIVE);
                }
            }
        }
        svg_frame(out, ox, oy, life.width, life.height, st.cell);
    }
    svg_close(out);
    return true;
}

/* --- тропа муравья ---------------------------------------------------------- */

/* Муравей рисуется треугольником, остриём по направлению взгляда: без этого
   по картинке нельзя понять, куда он сейчас смотрит, а вся стратегия — это
   повороты. */
static void svg_ant(FILE *out, int px, int py, int cell, int dir) {
    int a = cell / 6;
    int b = cell - a;
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;

    switch (dir) {
    case 0: /* восток */
        x1 = px + a;
        y1 = py + a;
        x2 = px + a;
        y2 = py + b;
        x3 = px + b;
        y3 = py + cell / 2;
        break;
    case 1: /* юг */
        x1 = px + a;
        y1 = py + a;
        x2 = px + b;
        y2 = py + a;
        x3 = px + cell / 2;
        y3 = py + b;
        break;
    case 2: /* запад */
        x1 = px + b;
        y1 = py + a;
        x2 = px + b;
        y2 = py + b;
        x3 = px + a;
        y3 = py + cell / 2;
        break;
    default: /* север */
        x1 = px + a;
        y1 = py + b;
        x2 = px + b;
        y2 = py + b;
        x3 = px + cell / 2;
        y3 = py + a;
        break;
    }
    /* Белая обводка: на съеденной клетке муравей иначе сливается с заливкой. */
    fprintf(out,
            "<polygon points=\"%d,%d %d,%d %d,%d\" fill=\"%s\" stroke=\"#ffffff\" "
            "stroke-width=\"0.8\"/>\n",
            x1, y1, x2, y2, x3, y3, COLOR_ANT);
}

bool render_trail_svg(const struct AntFsm *fsm, const int *steps, int count,
                      const struct RenderStyle *style, FILE *out) {
    struct RenderStyle st = effective(style);
    int frame_width;
    int total_width;
    int total_height;
    int i;

    if (fsm == NULL || steps == NULL || count < 1) {
        return false;
    }

    frame_width = TRAIL_SIDE * st.cell;
    total_width = 2 * st.margin + count * frame_width + (count - 1) * st.gap;
    total_height = 2 * st.margin + TRAIL_SIDE * st.cell;
    svg_open(out, total_width, total_height);

    for (i = 0; i < count; ++i) {
        struct TrailSnapshot snapshot;
        int ox = st.margin + i * (frame_width + st.gap);
        int oy = st.margin;
        int x;
        int y;

        /*
         * Каждый кадр — самостоятельный прогон с своим лимитом тактов. Так
         * проще и честнее: прогон детерминирован, поэтому кадр на такте N
         * всегда один и тот же, кто бы его ни строил.
         */
        ant_trail_snapshot(fsm, steps[i], &snapshot);
        if (st.grid) {
            svg_grid(out, ox, oy, TRAIL_SIDE, TRAIL_SIDE, st.cell);
        }
        for (y = 0; y < TRAIL_SIDE; ++y) {
            for (x = 0; x < TRAIL_SIDE; ++x) {
                int px = ox + x * st.cell;
                int py = oy + y * st.cell;

                if (snapshot.food[y][x]) {
                    svg_cell(out, px, py, st.cell, COLOR_FOOD);
                } else if (ant_trail_has_food(x, y)) {
                    svg_cell(out, px, py, st.cell, COLOR_EATEN);
                } else if (snapshot.visited[y][x]) {
                    svg_cell(out, px, py, st.cell, COLOR_VISITED);
                }
            }
        }
        /* Пройденный путь поверх клеток тропы — тонкой заливкой, чтобы не
           затирать еду. */
        for (y = 0; y < TRAIL_SIDE; ++y) {
            for (x = 0; x < TRAIL_SIDE; ++x) {
                if (snapshot.visited[y][x] && !snapshot.food[y][x] && !ant_trail_has_food(x, y)) {
                    svg_cell(out, ox + x * st.cell, oy + y * st.cell, st.cell, COLOR_TRAIL);
                }
            }
        }
        svg_ant(out, ox + snapshot.x * st.cell, oy + snapshot.y * st.cell, st.cell, snapshot.dir);
        svg_frame(out, ox, oy, TRAIL_SIDE, TRAIL_SIDE, st.cell);
    }
    svg_close(out);
    return true;
}

/* --- муравей Лэнгтона --------------------------------------------------------- */

bool render_langton_svg(int side, const long *steps, int count, const struct RenderStyle *style,
                        FILE *out) {
    struct RenderStyle st = effective(style);
    struct Ant ant;
    int frame_width;
    int total_width;
    int total_height;
    long done = 0;
    int i;

    if (steps == NULL || count < 1 || side < 4 || side > ANT_MAX_SIDE) {
        return false;
    }
    ant_init(&ant, side);

    frame_width = ant.side * st.cell;
    total_width = 2 * st.margin + count * frame_width + (count - 1) * st.gap;
    total_height = 2 * st.margin + ant.side * st.cell;
    svg_open(out, total_width, total_height);

    for (i = 0; i < count; ++i) {
        int ox = st.margin + i * (frame_width + st.gap);
        int oy = st.margin;
        int x;
        int y;

        while (done < steps[i] && ant_step(&ant)) {
            ++done;
        }
        /* Сетка при 96 клетках в стороне превратилась бы в серую заливку —
           у поля муравья её не рисуем, как ни просили бы настройки. */
        for (y = 0; y < ant.side; ++y) {
            for (x = 0; x < ant.side; ++x) {
                if (ant.cells[y][x]) {
                    svg_cell(out, ox + x * st.cell, oy + y * st.cell, st.cell, COLOR_ALIVE);
                }
            }
        }
        svg_ant(out, ox + ant.x * st.cell, oy + ant.y * st.cell, st.cell,
                (ant.dir == ANT_RIGHT)  ? 0
                : (ant.dir == ANT_DOWN) ? 1
                : (ant.dir == ANT_LEFT) ? 2
                                        : 3);
        svg_frame(out, ox, oy, ant.side, ant.side, st.cell);
    }
    svg_close(out);
    return true;
}

/* --- диаграмма автомата ------------------------------------------------------ */

/*
 * Раскладка круговая: состояния расставлены по окружности в порядке номеров.
 * Автоматических «красивых» раскладок здесь нет намеренно — круг предсказуем,
 * и два автомата с одинаковым числом состояний рисуются одинаково, поэтому
 * их можно сравнивать глазами.
 */

#define FSM_STATE_RADIUS 17
#define FSM_MIN_RADIUS 70
#define FSM_STEP_ARC 96 /* желаемое расстояние между соседними состояниями */
#define FSM_PADDING 46  /* поле под метки и петли */

/* Чертёжный шрифт по ГОСТ 2.304-81; запасные — на случай, если osifont в
   системе не установлен (см. lectures/scripts/fetch-fonts.sh). */
#define FSM_FONT "osifont, ISOCPEUR, sans-serif"

static const char *action_label(enum AntAction action) {
    switch (action) {
    case ANT_TURN_LEFT:
        return "Л";
    case ANT_TURN_RIGHT:
        return "П";
    default:
        return "Ш";
    }
}

static double fsm_layout_radius(int count) {
    double radius = (double)FSM_STEP_ARC * (double)count / 6.2831853;

    if (radius < (double)FSM_MIN_RADIUS) {
        radius = (double)FSM_MIN_RADIUS;
    }
    return radius;
}

/* Положение состояния на окружности. Угол отсчитывается от «девяти часов»,
   чтобы начальное состояние оказалось слева — там же, где входящая стрелка. */
static void fsm_state_center(int index, int count, double radius, double cx, double cy, double *x,
                             double *y) {
    double angle = 3.1415926 + 6.2831853 * (double)index / (double)count;

    *x = cx + radius * cos(angle);
    *y = cy + radius * sin(angle);
}

static void svg_text(FILE *out, double x, double y, int size, const char *text) {
    fprintf(out,
            "<text x=\"%.1f\" y=\"%.1f\" font-family=\"%s\" font-size=\"%d\" "
            "text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"#1a1a1a\">%s</text>\n",
            x, y, FSM_FONT, size, text);
}

/* Дуга перехода: квадратичная кривая с прогибом в сторону, чтобы переходы
   «туда» и «обратно» не ложились друг на друга. */
static void svg_arc(FILE *out, double x1, double y1, double x2, double y2, const char *label) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double length = sqrt(dx * dx + dy * dy);
    double nx;
    double ny;
    double bend;
    double sx;
    double sy;
    double ex;
    double ey;
    double mx;
    double my;
    double t = 0.3;
    double lx;
    double ly;

    if (length < 1.0) {
        return;
    }
    nx = -dy / length;
    ny = dx / length;
    bend = length / 5.0;
    if (bend > 34.0) {
        bend = 34.0;
    }

    /* Кривая начинается и кончается на границах кружков, а не в их центрах. */
    sx = x1 + dx / length * FSM_STATE_RADIUS;
    sy = y1 + dy / length * FSM_STATE_RADIUS;
    ex = x2 - dx / length * (FSM_STATE_RADIUS + 4.0);
    ey = y2 - dy / length * (FSM_STATE_RADIUS + 4.0);
    mx = (x1 + x2) / 2.0 + nx * bend * 2.0;
    my = (y1 + y2) / 2.0 + ny * bend * 2.0;

    fprintf(out,
            "<path d=\"M %.1f %.1f Q %.1f %.1f %.1f %.1f\" fill=\"none\" stroke=\"#1a1a1a\" "
            "stroke-width=\"1.2\" marker-end=\"url(#arrow)\"/>\n",
            sx, sy, mx, my, ex, ey);

    /*
     * Метка ставится не на середине дуги, а ближе к её началу: в середине
     * сходятся метки встречных переходов, и подписи налезают друг на друга.
     * Точка берётся с самой кривой Безье, поэтому подпись всегда рядом со
     * своей линией.
     */
    lx = (1.0 - t) * (1.0 - t) * sx + 2.0 * (1.0 - t) * t * mx + t * t * ex;
    ly = (1.0 - t) * (1.0 - t) * sy + 2.0 * (1.0 - t) * t * my + t * t * ey;
    svg_text(out, lx + nx * 11.0, ly + ny * 11.0, 11, label);
}

/* Петля: переход состояния в себя. Рисуется наружу от центра диаграммы. */
static void svg_loop(FILE *out, double x, double y, double cx, double cy, const char *label) {
    double dx = x - cx;
    double dy = y - cy;
    double length = sqrt(dx * dx + dy * dy);
    double ux;
    double uy;

    if (length < 1.0) {
        ux = 0.0;
        uy = -1.0;
    } else {
        ux = dx / length;
        uy = dy / length;
    }
    fprintf(out,
            "<path d=\"M %.1f %.1f C %.1f %.1f %.1f %.1f %.1f %.1f\" fill=\"none\" "
            "stroke=\"#1a1a1a\" stroke-width=\"1.2\" marker-end=\"url(#arrow)\"/>\n",
            x + ux * 12.0 - uy * 12.0, y + uy * 12.0 + ux * 12.0, x + ux * 46.0 - uy * 26.0,
            y + uy * 46.0 + ux * 26.0, x + ux * 46.0 + uy * 26.0, y + uy * 46.0 - ux * 26.0,
            x + ux * 14.0 + uy * 12.0, y + uy * 14.0 - ux * 12.0);
    svg_text(out, x + ux * 44.0, y + uy * 44.0, 11, label);
}

bool render_fsm_svg(const struct AntFsm *fsm, const struct RenderStyle *style, FILE *out) {
    double radius;
    double cx;
    double cy;
    int size;
    int i;
    int input;

    (void)style;
    if (fsm == NULL || fsm->state_count < 1 || fsm->state_count > ANT_FSM_MAX_STATES) {
        return false;
    }
    radius = fsm_layout_radius(fsm->state_count);
    size = (int)(2.0 * (radius + FSM_STATE_RADIUS + FSM_PADDING));
    cx = size / 2.0;
    cy = size / 2.0;

    svg_open(out, size, size);
    fprintf(out, "<defs><marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"9\" refY=\"5\" "
                 "markerWidth=\"7\" markerHeight=\"7\" orient=\"auto-start-reverse\">"
                 "<path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"#1a1a1a\"/></marker></defs>\n");

    /* Сначала дуги, потом кружки: так линии не перечёркивают номера. */
    for (i = 0; i < fsm->state_count; ++i) {
        double x;
        double y;

        fsm_state_center(i, fsm->state_count, radius, cx, cy, &x, &y);
        for (input = 1; input >= 0; --input) {
            char label[16];
            int target = fsm->next[i][input];
            double tx;
            double ty;

            sprintf(label, "%d/%s", input, action_label(fsm->action[i][input]));
            if (target == i) {
                svg_loop(out, x, y, cx, cy, label);
                continue;
            }
            fsm_state_center(target, fsm->state_count, radius, cx, cy, &tx, &ty);
            svg_arc(out, x, y, tx, ty, label);
        }
    }

    for (i = 0; i < fsm->state_count; ++i) {
        char number[8];
        double x;
        double y;

        fsm_state_center(i, fsm->state_count, radius, cx, cy, &x, &y);
        fprintf(out,
                "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%d\" fill=\"#ffffff\" stroke=\"#1a1a1a\" "
                "stroke-width=\"1.2\"/>\n",
                x, y, FSM_STATE_RADIUS);
        sprintf(number, "%d", i);
        svg_text(out, x, y, 13, number);
    }

    /*
     * Начальное состояние — входящая стрелка «ниоткуда». Она направлена по
     * касательной, а не по радиусу: наружу по радиусу уходит петля состояния,
     * и стрелки накладывались бы друг на друга.
     */
    {
        double x;
        double y;
        double ux = (cx - 0.0);
        double uy;

        fsm_state_center(0, fsm->state_count, radius, cx, cy, &x, &y);
        ux = x - cx;
        uy = y - cy;
        (void)ux;
        (void)uy;
        fprintf(out,
                "<path d=\"M %.1f %.1f L %.1f %.1f\" stroke=\"#1a1a1a\" stroke-width=\"1.2\" "
                "marker-end=\"url(#arrow)\"/>\n",
                x, y + FSM_STATE_RADIUS + 30.0, x, y + FSM_STATE_RADIUS + 5.0);
    }

    svg_close(out);
    return true;
}

/* --- одномерный автомат ------------------------------------------------------ */

bool render_elementary_svg(unsigned char rule, int width, int steps,
                           const struct RenderStyle *style, FILE *out) {
    struct RenderStyle st = effective(style);
    struct Elementary ca;
    int total_width;
    int total_height;
    int row;
    int i;

    if (width < 3 || width > CELLS_MAX_WIDTH || steps < 1) {
        return false;
    }
    elementary_init(&ca, rule, width);

    total_width = 2 * st.margin + width * st.cell;
    total_height = 2 * st.margin + (steps + 1) * st.cell;
    svg_open(out, total_width, total_height);
    if (st.grid) {
        svg_grid(out, st.margin, st.margin, width, steps + 1, st.cell);
    }
    for (row = 0; row <= steps; ++row) {
        for (i = 0; i < ca.width; ++i) {
            if (ca.cells[i]) {
                svg_cell(out, st.margin + i * st.cell, st.margin + row * st.cell, st.cell,
                         COLOR_ALIVE);
            }
        }
        if (row < steps) {
            elementary_step(&ca);
        }
    }
    svg_frame(out, st.margin, st.margin, width, steps + 1, st.cell);
    svg_close(out);
    return true;
}
