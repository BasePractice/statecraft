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

/* Чертёжный шрифт по ГОСТ 2.304-81; запасные — на случай, если osifont не
   установлен (ставится lectures/scripts/fetch-fonts.sh). */
#define FSM_FONT "osifont, ISOCPEUR, sans-serif"

/*
 * Автомат из полутора десятков состояний номерами не читается: цифры мелкие,
 * дуги пересекают их, и глаз всё равно не удерживает, где какая вершина.
 * Начиная с этого числа состояний вершины различаются заливкой, а номер не
 * печатается вовсе: рисунок в этом случае показывает не «какое состояние
 * куда», а плотность связей — ровно то, ради чего он в лекции и стоит.
 */
#define FSM_COLOR_THRESHOLD 8

/* Заливки вершин: светлые, чтобы дуги поверх оставались различимы. */
static const char *const FSM_STATE_COLORS[]
        = {"#f4f4f4", "#dfe7f2", "#e6dff2", "#f2dfe4", "#f2e8df", "#dff2e8",
           "#e4f2df", "#f2f0df", "#dfeef2", "#eadff2", "#f2dfdf", "#dfe2f2",
           "#e9f2df", "#f2e3df", "#dff2f0", "#efdff2", "#f2ecdf", "#e0e0e0"};
#define FSM_STATE_COLOR_COUNT ((int)(sizeof(FSM_STATE_COLORS) / sizeof(FSM_STATE_COLORS[0])))

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

/* Буквы, которыми помечаются кадры составного рисунка. Порядок по ГОСТ 7.32:
   Ё, З, Й, О, Ч, Ь, Ы, Ъ не используются. */
static const char *const PANEL_LETTERS[]
        = {"а", "б", "в", "г", "д", "е", "ж", "и", "к", "л", "м", "н"};
#define PANEL_LETTER_COUNT ((int)(sizeof(PANEL_LETTERS) / sizeof(PANEL_LETTERS[0])))

/*
 * Полоса под буквой кадра: составной рисунок без такой пометки нельзя
 * разобрать в тексте — «на втором кадре» читатель считает сам и ошибается.
 *
 * Размеры считаются от клетки поля, а не берутся числом: рисунок вписывается
 * в полосу набора целиком, поэтому чем больше в нём кадров, тем сильнее он
 * ужимается — буква фиксированного кегля становится нечитаемой.
 */
#define PANEL_MARK_BAND(cell) ((cell) * 7 / 2)
#define PANEL_MARK_SIZE(cell) ((cell) * 5 / 2)

static void svg_panel_mark(FILE *out, double cx, double cy, int index, int count, int cell) {
    const char *letter = (index >= 0 && index < PANEL_LETTER_COUNT) ? PANEL_LETTERS[index] : "?";

    /* У рисунка из одного кадра помечать нечего. */
    if (count < 2) {
        return;
    }

    fprintf(out,
            "<text x=\"%.1f\" y=\"%.1f\" font-family=\"%s\" font-size=\"%d\" "
            "text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"#1a1a1a\">%s)</text>\n",
            cx, cy, FSM_FONT, PANEL_MARK_SIZE(cell), letter);
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

/* Как делать ход: правило «Жизни» или правило чётности. Обе ленты рисуются
   одним кодом — различие ровно в одной функции. */
typedef void (*LifeStep)(struct Life *life);

static bool life_bounds(const char *name, int width, int height, const int *generations, int count,
                        LifeStep step, struct Bounds *bounds) {
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
            (*step)(&life);
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

static bool render_field_svg(const char *name, int width, int height, const int *generations,
                             int count, LifeStep step, const struct RenderStyle *style, FILE *out) {
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
    if (!life_bounds(name, width, height, generations, count, step, &bounds)) {
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
    total_height = 2 * st.margin + rows * st.cell + (count > 1 ? PANEL_MARK_BAND(st.cell) : 0);
    svg_open(out, total_width, total_height);

    for (i = 0; i < count; ++i) {
        int ox = st.margin + i * (frame_width + st.gap);
        int oy = st.margin;
        int x;
        int y;

        while (generation < generations[i]) {
            (*step)(&life);
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
        svg_panel_mark(out, (double)ox + (double)frame_width / 2.0,
                       (double)(oy + rows * st.cell) + (double)PANEL_MARK_BAND(st.cell) / 2.0, i,
                       count, st.cell);
    }
    svg_close(out);
    return true;
}

bool render_life_svg(const char *name, int width, int height, const int *generations, int count,
                     const struct RenderStyle *style, FILE *out) {
    return render_field_svg(name, width, height, generations, count, life_step, style, out);
}

bool render_parity_svg(const char *name, int side, const int *generations, int count,
                       const struct RenderStyle *style, FILE *out) {
    return render_field_svg(name, side, side, generations, count, life_step_parity, style, out);
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
    bool agar; /* залить поле агаром перед расстановкой фигур */
    struct ScenePart parts[4];
};

/*
 * Пожиратель и планер: планер летит по диагонали вниз-вправо и через
 * несколько ходов упирается в пожирателя. Тот проглатывает планер и
 * восстанавливает свою форму — конфигурация из семи клеток, которая чистит
 * поле от мусора (Госпер, 1971).
 *
 * Агар и «вирус»: одна лишняя живая клетка на регулярной решётке блоков.
 * Куда её поставить — решает всё. В углу, где сходятся четыре блока, агар
 * уничтожает вирус и через два хода восстанавливает прежний вид; рядом с
 * блоком — начинается разрушение, которое расходится по агару без предела
 * (Уэйнрайт). Обе сцены проверены тестами практики.
 *
 * Сторона поля сцен с агаром делится на 3: иначе на стыке через тор
 * получится шов, и агар разрушится сам по себе, без всякого вируса.
 */
/* clang-format off */
#define NO_PART {NULL, 0, 0}

static const struct Scene SCENES[] = {
    {"eater-vs-glider",   20, false, {{"eater", 11, 11}, {"glider", 4, 4}, NO_PART, NO_PART}},
    {"agar-virus",        24, true,  {{"tub-cell", 12, 14}, NO_PART, NO_PART, NO_PART}},
    {"agar-virus-corner", 24, true,  {{"tub-cell", 14, 14}, NO_PART, NO_PART, NO_PART}},
    {NULL, 0, false, {NO_PART, NO_PART, NO_PART, NO_PART}}
};
/* clang-format on */

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
    if (scene->agar && !life_fill_agar(life)) {
        return false;
    }
    for (i = 0; i < 4 && scene->parts[i].pattern != NULL; ++i) {
        /* «Вирус» — не фигура из таблицы, а одна живая клетка: заводить ради
           неё запись в списке фигур незачем. */
        if (strcmp(scene->parts[i].pattern, "tub-cell") == 0) {
            life_set(life, scene->parts[i].x, scene->parts[i].y, true);
            continue;
        }
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
    total_height
            = 2 * st.margin + TRAIL_SIDE * st.cell + (count > 1 ? PANEL_MARK_BAND(st.cell) : 0);
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
        svg_panel_mark(out, (double)ox + (double)frame_width / 2.0,
                       (double)(oy + TRAIL_SIDE * st.cell) + (double)PANEL_MARK_BAND(st.cell) / 2.0,
                       i, count, st.cell);
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
    total_height = 2 * st.margin + ant.side * st.cell + (count > 1 ? PANEL_MARK_BAND(st.cell) : 0);
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
        svg_panel_mark(out, (double)ox + (double)frame_width / 2.0,
                       (double)(oy + ant.side * st.cell) + (double)PANEL_MARK_BAND(st.cell) / 2.0,
                       i, count, st.cell);
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
#define FSM_MIN_RADIUS 78
#define FSM_STEP_ARC 104 /* желаемое расстояние между соседними состояниями */
#define FSM_PADDING 54   /* поле под метки и петли */
#define FSM_LABEL_SIZE 11
#define FSM_ARROW_GAP 3.0 /* зазор между наконечником стрелки и кружком */

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
   чтобы начальное состояние оказалось слева. */
static void fsm_state_center(int index, int count, double radius, double cx, double cy, double *x,
                             double *y) {
    double angle = 3.1415926 + 6.2831853 * (double)index / (double)count;

    *x = cx + radius * cos(angle);
    *y = cy + radius * sin(angle);
}

/*
 * Надпись с белой подложкой. Подложка обязательна: метки переходов ложатся
 * поверх других дуг, и без неё «1/Ш» на пересечении линий не разобрать.
 * Ширина считается по числу знаков — метки короткие и одного вида.
 */
static void svg_label(FILE *out, double x, double y, const char *text, int chars) {
    double w = (double)chars * (double)FSM_LABEL_SIZE * 0.62 + 3.0;
    double h = (double)FSM_LABEL_SIZE + 2.0;

    fprintf(out, "<rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" fill=\"#ffffff\"/>\n",
            x - w / 2.0, y - h / 2.0, w, h);
    fprintf(out,
            "<text x=\"%.1f\" y=\"%.1f\" font-family=\"%s\" font-size=\"%d\" "
            "text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"#1a1a1a\">%s</text>\n",
            x, y, FSM_FONT, FSM_LABEL_SIZE, text);
}

static void svg_state_number(FILE *out, double x, double y, const char *text) {
    fprintf(out,
            "<text x=\"%.1f\" y=\"%.1f\" font-family=\"%s\" font-size=\"13\" "
            "text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"#1a1a1a\">%s</text>\n",
            x, y, FSM_FONT, text);
}

/* --- квадратичная кривая ----------------------------------------------------- */

struct Quad {
    double x0;
    double y0;
    double cx; /* управляющая точка */
    double cy;
    double x1;
    double y1;
};

static void quad_point(const struct Quad *q, double t, double *x, double *y) {
    double u = 1.0 - t;

    *x = u * u * q->x0 + 2.0 * u * t * q->cx + t * t * q->x1;
    *y = u * u * q->y0 + 2.0 * u * t * q->cy + t * t * q->y1;
}

/*
 * Часть кривой между параметрами a и b (двойное деление по де Кастельжо).
 * Нужна, чтобы дуга начиналась и кончалась ровно на границах кружков:
 * стрелка обязана касаться состояния, иначе рисунок читается неверно.
 */
static struct Quad quad_segment(const struct Quad *q, double a, double b) {
    struct Quad right;
    struct Quad result;
    double s;
    double mx;
    double my;

    quad_point(q, a, &right.x0, &right.y0);
    right.cx = q->cx + (q->x1 - q->cx) * a;
    right.cy = q->cy + (q->y1 - q->cy) * a;
    right.x1 = q->x1;
    right.y1 = q->y1;

    s = (a < 1.0) ? (b - a) / (1.0 - a) : 0.0;
    mx = right.x0 + (right.cx - right.x0) * s;
    my = right.y0 + (right.cy - right.y0) * s;
    result.x0 = right.x0;
    result.y0 = right.y0;
    result.cx = mx;
    result.cy = my;
    quad_point(&right, s, &result.x1, &result.y1);
    return result;
}

/*
 * Параметр, при котором кривая выходит за окружность радиуса r вокруг точки.
 * Ищется перебором с мелким шагом: расстояние по кривой немонотонно, и
 * бинарный поиск нашёл бы не тот корень.
 */
static double quad_exit(const struct Quad *q, double px, double py, double r, bool from_start) {
    int i;

    for (i = 0; i <= 100; ++i) {
        double t = from_start ? (double)i / 100.0 : 1.0 - (double)i / 100.0;
        double x;
        double y;
        double dx;
        double dy;

        quad_point(q, t, &x, &y);
        dx = x - px;
        dy = y - py;
        if (sqrt(dx * dx + dy * dy) >= r) {
            return t;
        }
    }
    return from_start ? 0.0 : 1.0;
}

/*
 * Дуга перехода. Прогиб нужен, чтобы переходы «туда» и «обратно» не легли
 * друг на друга; направление прогиба одинаково для всех дуг, поэтому встречная
 * пара всегда расходится.
 */
static void svg_arc(FILE *out, double x1, double y1, double x2, double y2, const char *label,
                    double label_at) {
    struct Quad full;
    struct Quad arc;
    double dx = x2 - x1;
    double dy = y2 - y1;
    double length = sqrt(dx * dx + dy * dy);
    double nx;
    double ny;
    double bend;
    double t0;
    double t1;
    double lx;
    double ly;
    double ax;
    double ay;
    double tangent;

    if (length < 1.0) {
        return;
    }
    nx = -dy / length;
    ny = dx / length;
    bend = length / 3.0;
    if (bend > 62.0) {
        bend = 62.0;
    }

    full.x0 = x1;
    full.y0 = y1;
    full.x1 = x2;
    full.y1 = y2;
    full.cx = (x1 + x2) / 2.0 + nx * bend;
    full.cy = (y1 + y2) / 2.0 + ny * bend;

    t0 = quad_exit(&full, x1, y1, (double)FSM_STATE_RADIUS, true);
    t1 = quad_exit(&full, x2, y2, (double)FSM_STATE_RADIUS + FSM_ARROW_GAP, false);
    if (t1 <= t0) {
        return;
    }
    arc = quad_segment(&full, t0, t1);

    fprintf(out,
            "<path d=\"M %.1f %.1f Q %.1f %.1f %.1f %.1f\" fill=\"none\" stroke=\"#1a1a1a\" "
            "stroke-width=\"1.2\" marker-end=\"url(#arrow)\"/>\n",
            arc.x0, arc.y0, arc.cx, arc.cy, arc.x1, arc.y1);

    /*
     * Метка ставится не в середине дуги: там сходятся подписи встречных
     * переходов. Точка берётся с самой кривой, а два перехода одного
     * состояния подписываются в разных её местах.
     */
    quad_point(&arc, label_at, &lx, &ly);
    quad_point(&arc, label_at + 0.05, &ax, &ay);
    dx = ax - lx;
    dy = ay - ly;
    tangent = sqrt(dx * dx + dy * dy);
    if (tangent < 0.001) {
        dx = 1.0;
        dy = 0.0;
        tangent = 1.0;
    }
    svg_label(out, lx - dy / tangent * 10.0, ly + dx / tangent * 10.0, label, 3);
}

/* Петля: переход состояния в себя, наружу от центра диаграммы. */
static void svg_loop(FILE *out, double x, double y, double cx, double cy, const char *label) {
    double dx = x - cx;
    double dy = y - cy;
    double length = sqrt(dx * dx + dy * dy);
    double ux;
    double uy;
    double sx;
    double sy;
    double ex;
    double ey;

    if (length < 1.0) {
        ux = 0.0;
        uy = -1.0;
    } else {
        ux = dx / length;
        uy = dy / length;
    }
    /* Концы петли лежат на самой окружности состояния — по обе стороны от
       направления «наружу», примерно в 35° от него. */
    sx = x + (ux * 0.82 - uy * 0.57) * FSM_STATE_RADIUS;
    sy = y + (uy * 0.82 + ux * 0.57) * FSM_STATE_RADIUS;
    ex = x + (ux * 0.82 + uy * 0.57) * (FSM_STATE_RADIUS + FSM_ARROW_GAP);
    ey = y + (uy * 0.82 - ux * 0.57) * (FSM_STATE_RADIUS + FSM_ARROW_GAP);

    fprintf(out,
            "<path d=\"M %.1f %.1f C %.1f %.1f %.1f %.1f %.1f %.1f\" fill=\"none\" "
            "stroke=\"#1a1a1a\" stroke-width=\"1.2\" marker-end=\"url(#arrow)\"/>\n",
            sx, sy, x + ux * 50.0 - uy * 28.0, y + uy * 50.0 + ux * 28.0, x + ux * 50.0 + uy * 28.0,
            y + uy * 50.0 - ux * 28.0, ex, ey);
    svg_label(out, x + ux * 46.0, y + uy * 46.0, label, 3);
}

bool render_fsm_svg(const struct AntFsm *fsm, const struct RenderStyle *style, FILE *out) {
    double radius;
    double cx;
    double cy;
    int size;
    int i;
    int input;
    bool colored;

    (void)style;
    if (fsm == NULL || fsm->state_count < 1 || fsm->state_count > ANT_FSM_MAX_STATES) {
        return false;
    }
    colored = fsm->state_count >= FSM_COLOR_THRESHOLD;
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
            /* Метка дуги — «вход/действие»: число, косая черта и буква
               кириллицей (два байта в UTF-8). Считаем по тому же правилу,
               что и номер состояния: 11 знаков на число плюс запас. */
            char label[20];
            int target = fsm->next[i][input];
            double tx;
            double ty;

            sprintf(label, "%d/%s", input, action_label(fsm->action[i][input]));
            if (target == i) {
                svg_loop(out, x, y, cx, cy, label);
                continue;
            }
            fsm_state_center(target, fsm->state_count, radius, cx, cy, &tx, &ty);
            svg_arc(out, x, y, tx, ty, label, (input == 1) ? 0.24 : 0.46);
        }
    }

    for (i = 0; i < fsm->state_count; ++i) {
        /* Размер буфера — под любое значение int (11 знаков со знаком минус
           и завершающий нуль), а не под ожидаемое число состояний: snprintf
           в ISO C90 нет, и sprintf обрезать вывод не умеет. GCC такую
           подмену «здесь всегда мало» замечает: -Wformat-overflow. */
        char number[12];
        double x;
        double y;

        fsm_state_center(i, fsm->state_count, radius, cx, cy, &x, &y);
        fprintf(out,
                "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%d\" fill=\"%s\" stroke=\"#1a1a1a\" "
                "stroke-width=\"1.2\"/>\n",
                x, y, FSM_STATE_RADIUS,
                colored ? FSM_STATE_COLORS[i % FSM_STATE_COLOR_COUNT] : "#ffffff");
        if (!colored) {
            sprintf(number, "%d", i);
            svg_state_number(out, x, y, number);
        }
    }

    /*
     * Начальное состояние — входящая стрелка «ниоткуда». Она подходит снизу,
     * а не по радиусу: наружу по радиусу уходит петля состояния, и стрелки
     * накладывались бы друг на друга.
     */
    {
        double x;
        double y;

        fsm_state_center(0, fsm->state_count, radius, cx, cy, &x, &y);
        fprintf(out,
                "<path d=\"M %.1f %.1f L %.1f %.1f\" stroke=\"#1a1a1a\" stroke-width=\"1.2\" "
                "marker-end=\"url(#arrow)\"/>\n",
                x, y + FSM_STATE_RADIUS + 32.0, x, y + FSM_STATE_RADIUS + FSM_ARROW_GAP);
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
