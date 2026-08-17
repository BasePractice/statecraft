/**
 * @file
 * Графическая составляющая погрузчика: наблюдение за прогоном системы
 * управления в реальном времени.
 *
 * Приложение ничего не решает само. Оно берёт готовый прогон
 * (`loader_runner.h`) — тот же самый, который в консольном драйвере
 * проверяется тестом, — и рисует его состояние. Это требование, а не стиль:
 * картинка, у которой своя копия логики, рано или поздно начнёт показывать не
 * то, что происходит на самом деле, и такая картинка хуже отсутствующей.
 *
 * Приложение необязательное: собирается при -DSTATECRAFT_BUILD_GUI=ON и
 * найденной raylib. Курс собирается в CI, где нет ни графических библиотек,
 * ни дисплея, поэтому по умолчанию цель выключена.
 *
 * Управление:
 *   Пробел     пуск/пауза
 *   S          один такт (в паузе)
 *   ↑ / ↓      быстрее/медленнее
 *   R          квитировать аварию
 *   N          новое задание: следующая метка по кругу
 *   Esc        выход
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <raylib.h>

#include "loader_runner.h"

namespace {

constexpr int TILE = 16;
constexpr int PANEL_WIDTH = 300;
constexpr int PORTS_WIDTH = 200;
constexpr int MARGIN = 8;

/* Подписи. Кириллических глифов во встроенном шрифте raylib нет, поэтому при
   отсутствии внешнего шрифта надписи выводятся латиницей: лучше читаемая
   латиница, чем ряд пустых квадратов. */
struct Label {
    const char *ru;
    const char *en;
};

const Font *g_font = nullptr;

const char *pick(const Label &label) {
    return g_font != nullptr ? label.ru : label.en;
}

void draw_text(const char *text, int x, int y, int size, Color color) {
    if (g_font != nullptr) {
        DrawTextEx(*g_font, text, Vector2{(float)x, (float)y}, (float)size, 1.0f, color);
    } else {
        DrawText(text, x, y, size, color);
    }
}

/*
 * Внешний шрифт нужен только ради кириллицы. Путь можно задать ключом --font;
 * иначе перебираются типовые места. Ни один из шрифтов в курс не входит —
 * распространять чужой шрифт вместе с примером нельзя (см. ТД-1 и ТД-7).
 */
/*
 * Первым делом ищется Fira Code: этим шрифтом набран весь курс (см.
 * lectures/template/theme.typ), он моноширинный, и таблица портов в нём
 * выстраивается по колонкам без всяких ухищрений. Остальные — запасные, на
 * случай машины, где Fira Code не установлена.
 */
const char *const FONT_CANDIDATES[] = {
    "/Library/Fonts/FiraCode-Regular.ttf",
    "/usr/share/fonts/truetype/firacode/FiraCode-Regular.ttf",
    "/usr/local/share/fonts/FiraCode-Regular.ttf",
    "C:/Windows/Fonts/FiraCode-Regular.ttf",
    "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "C:/Windows/Fonts/arial.ttf",
};

/** Домашний каталог шрифтов пользователя: там Fira Code оказывается чаще всего. */
const char *const FONT_HOME_CANDIDATES[] = {
    "/Library/Fonts/FiraCode-Regular.ttf",
    "/.local/share/fonts/FiraCode-Regular.ttf",
    "/.fonts/FiraCode-Regular.ttf",
};

Font load_font(const char *requested, bool *loaded) {
    /* Латиница, кириллица и знаки препинания. */
    static int codepoints[512];
    int count = 0;

    for (int c = 32; c < 127; ++c)
        codepoints[count++] = c;
    for (int c = 0x0410; c <= 0x044F; ++c)
        codepoints[count++] = c;
    codepoints[count++] = 0x0401;
    codepoints[count++] = 0x0451;
    codepoints[count++] = 0x2192; /* стрелка в подписи маршрута */

    if (requested != nullptr) {
        Font font = LoadFontEx(requested, 18, codepoints, count);

        if (font.texture.id != 0) {
            *loaded = true;
            return font;
        }
        fprintf(stderr, "шрифт %s не загружен, подписи будут латиницей\n", requested);
    }
    {
        const char *home = getenv("HOME");

        for (const char *tail : FONT_HOME_CANDIDATES) {
            char path[512];

            if (home == nullptr)
                break;
            snprintf(path, sizeof(path), "%s%s", home, tail);
            if (!FileExists(path))
                continue;
            {
                Font font = LoadFontEx(path, 18, codepoints, count);

                if (font.texture.id != 0) {
                    *loaded = true;
                    return font;
                }
            }
        }
    }
    for (const char *candidate : FONT_CANDIDATES) {
        if (!FileExists(candidate))
            continue;
        {
            Font font = LoadFontEx(candidate, 18, codepoints, count);

            if (font.texture.id != 0) {
                *loaded = true;
                return font;
            }
        }
    }
    *loaded = false;
    return Font{};
}

/*
 * Значение 13 в слое map — покрытие проезда, а не тело стеллажа: по нему и
 * проложена разметка. Ошибиться здесь легко, и ошибка сразу видна на картинке —
 * «стеллажи» оказываются ровно теми проходами, по которым ездит погрузчик.
 */
Color cell_color(const FactoryMap *map, int row, int col) {
    int tile = factory_cell(map, FACTORY_LAYER_MAP, row, col);

    if (tile == 13)
        return Color{214, 210, 200, 255}; /* покрытие проезда */
    if (tile != 0)
        return Color{120, 96, 68, 255};   /* стены и стеллажи */
    return Color{245, 245, 242, 255};     /* всё прочее */
}

void draw_map(const FactoryMap *map, const Route &route) {
    for (int row = 0; row < map->rows; ++row) {
        for (int col = 0; col < map->cols; ++col) {
            int x = MARGIN + col * TILE;
            int y = MARGIN + row * TILE;

            DrawRectangle(x, y, TILE, TILE, cell_color(map, row, col));

            /* Слой things — оборудование и паллеты: погрузчику они безразличны
               (его дело — линия), но без них схема цеха выглядит пустой. */
            if (factory_cell(map, FACTORY_LAYER_THINGS, row, col) != 0)
                DrawRectangle(x + 3, y + 3, TILE - 6, TILE - 6, Color{96, 108, 128, 255});

            int path = factory_cell(map, FACTORY_LAYER_PATHS, row, col);
            if (path == FACTORY_PATH_H) {
                DrawRectangle(x, y + TILE / 2 - 2, TILE, 4, Color{240, 200, 60, 255});
            } else if (path == FACTORY_PATH_V) {
                DrawRectangle(x + TILE / 2 - 2, y, 4, TILE, Color{240, 200, 60, 255});
            }

            int point = factory_point_at(map, row, col);
            if (point != 0) {
                bool on_route = false;

                for (int i = 0; i < route.count && !on_route; ++i)
                    on_route = route.step[i].point == point;
                DrawCircle(x + TILE / 2, y + TILE / 2, (float)TILE / 2 - 1,
                           on_route ? Color{60, 140, 240, 255} : Color{120, 130, 150, 255});
                draw_text(TextFormat("%d", point), x + 3, y + 1, 12, RAYWHITE);
            }
        }
    }
}

/* Погрузчик рисуется между клетками: доля пути берётся из счётчика установки,
   иначе он прыгал бы через клетку скачком. */
void draw_loader(const LoaderRunner &runner) {
    static const int DELTA_ROW[] = {-1, 0, 1, 0};
    static const int DELTA_COL[] = {0, 1, 0, -1};

    const LoaderPlant &plant = runner.plant;
    float part = (float)plant.travel_cm / (float)LOADER_CELL_CM;
    float col = (float)plant.col + (float)DELTA_COL[plant.angle] * part;
    float row = (float)plant.row + (float)DELTA_ROW[plant.angle] * part;
    float x = MARGIN + (col + 0.5f) * TILE;
    float y = MARGIN + (row + 0.5f) * TILE;
    float angle = (float)plant.angle * 90.0f;

    if (plant.turn_elapsed_ms > 0) {
        float turn = (float)plant.turn_elapsed_ms / (float)LOADER_TURN_MS * 90.0f;

        angle += runner.commands.turn_right ? turn : -turn;
    }

    Vector2 centre{x, y};
    float radius = (float)TILE * 0.7f;
    Color body = runner.fault ? Color{220, 60, 60, 255} : Color{240, 120, 20, 255};

    DrawPoly(centre, 3, radius, angle - 90.0f, body);
    DrawPolyLines(centre, 3, radius, angle - 90.0f, BLACK);
    if (runner.sensors.load)
        DrawCircleV(centre, radius * 0.35f, Color{40, 40, 40, 255});
}

/*
 * Панель портов. Показывает то же, что видит автомат: значения всех входных и
 * выходных портов на текущем такте. Изменившееся с прошлого такта значение
 * подсвечивается — так на глаз видно, какой датчик шевельнулся и какая команда
 * снялась; по неподвижной таблице это не читается.
 */
struct PortRow {
    Label name;
    int value;
    int output; /* 1 — выход автомата, 0 — вход */
};

int collect_ports(const LoaderRunner &runner, PortRow *rows, int capacity) {
    const PortRow source[] = {
        {{"cmd_valid", "cmd_valid"}, runner.cmd_valid, 0},
        {{"cmd_code", "cmd_code"}, runner.cmd_code, 0},
        {{"cmd_point", "cmd_point"}, runner.cmd_point, 0},
        {{"cmd_extra", "cmd_extra"}, runner.cmd_extra, 0},
        {{"cmd_timeout", "cmd_timeout"}, runner.cmd_timeout_ms, 0},
        {{"sense_line", "sense_line"}, runner.sensors.line, 0},
        {{"sense_point", "sense_point"}, runner.sensors.point, 0},
        {{"sense_angle", "sense_angle"}, runner.sensors.angle, 0},
        {{"sense_odometer", "sense_odometer"}, runner.sensors.odometer, 0},
        {{"sense_range", "sense_range"}, runner.sensors.range, 0},
        {{"sense_motion", "sense_motion"}, runner.sensors.motion, 0},
        {{"sense_stack", "sense_stack"}, runner.sensors.stack, 0},
        {{"sense_pallet", "sense_pallet"}, runner.sensors.pallet, 0},
        {{"sense_load", "sense_load"}, runner.sensors.load, 0},
        {{"reset", "reset"}, runner.reset, 0},
        {{"cmd_ack", "cmd_ack"}, runner.cmd_ack, 1},
        {{"cmd_done", "cmd_done"}, runner.cmd_done, 1},
        {{"drive_gas", "drive_gas"}, runner.commands.gas, 1},
        {{"turn_left", "turn_left"}, runner.commands.turn_left, 1},
        {{"turn_right", "turn_right"}, runner.commands.turn_right, 1},
        {{"fork_up", "fork_up"}, runner.commands.fork_up, 1},
        {{"fault", "fault"}, runner.fault, 1},
    };
    int count = (int)(sizeof(source) / sizeof(source[0]));

    if (count > capacity)
        count = capacity;
    for (int i = 0; i < count; ++i)
        rows[i] = source[i];
    return count;
}

PortRow g_previous[32];
int g_previous_count = 0;

/*
 * Снимок портов делается перед тактом автомата, а не в отрисовке: кадров в
 * секунду шестьдесят, а тактов бывает и три. Сравнивай мы соседние кадры,
 * подсветка мигала бы одно мгновение и большей частью показывала бы «ничего не
 * изменилось».
 */
void ports_snapshot(const LoaderRunner &runner) {
    g_previous_count = collect_ports(runner, g_previous, 32);
}

void draw_ports(const LoaderRunner &runner, int origin_x, int origin_y) {
    static const Label L_INPUTS = {"Входные порты", "Input ports"};
    static const Label L_OUTPUTS = {"Выходные порты", "Output ports"};

    PortRow rows[32];
    int count = collect_ports(runner, rows, 32);
    int y = origin_y;
    const int line = 17;
    int header_drawn = 0;

    draw_text(pick(L_INPUTS), origin_x, y, 16, BLACK);
    y += line + 2;
    for (int i = 0; i < count; ++i) {
        bool changed = i < g_previous_count && g_previous[i].value != rows[i].value;

        if (rows[i].output && !header_drawn) {
            y += 6;
            draw_text(pick(L_OUTPUTS), origin_x, y, 16, BLACK);
            y += line + 2;
            header_drawn = 1;
        }
        if (changed)
            DrawRectangle(origin_x - 3, y - 2, 190, line, Color{255, 232, 120, 255});
        draw_text(TextFormat("%-15s %6d", pick(rows[i].name), rows[i].value), origin_x, y, 14,
                  changed ? BLACK : DARKGRAY);
        y += line;
    }
}

/*
 * Кнопки и ползунок рисуются вручную: raygui в курс не входит, а трёх кнопок и
 * одного ползунка ради ещё одной зависимости не стоит.
 */
struct Button {
    Rectangle box;
    Label label;
    bool active; /* режим, в котором кнопка «утоплена» */
};

bool draw_button(const Button &button, bool enabled) {
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, button.box);
    Color fill = button.active ? Color{96, 140, 200, 255}
                              : (hovered ? Color{225, 228, 235, 255} : Color{242, 242, 240, 255});

    DrawRectangleRec(button.box, enabled ? fill : Color{236, 236, 236, 255});
    DrawRectangleLinesEx(button.box, 1.0f, Color{150, 152, 158, 255});
    draw_text(pick(button.label), (int)button.box.x + 10, (int)button.box.y + 6, 16,
              enabled ? (button.active ? RAYWHITE : BLACK) : GRAY);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Ползунок частоты тактирования: от 1 до 240 тактов в секунду. */
int draw_speed_slider(Rectangle box, int speed) {
    static const Label L_SPEED = {"Тактов в секунду", "Ticks per second"};
    const int min_speed = 1;
    const int max_speed = 240;
    float part = (float)(speed - min_speed) / (float)(max_speed - min_speed);
    Vector2 mouse = GetMousePosition();

    draw_text(TextFormat("%s: %d", pick(L_SPEED), speed), (int)box.x, (int)box.y - 20, 16,
              DARKGRAY);
    DrawRectangleRec(box, Color{232, 232, 230, 255});
    DrawRectangle((int)box.x, (int)box.y, (int)(box.width * part), (int)box.height,
                  Color{140, 180, 230, 255});
    DrawRectangleLinesEx(box, 1.0f, Color{150, 152, 158, 255});
    DrawRectangle((int)(box.x + box.width * part) - 3, (int)box.y - 3, 6, (int)box.height + 6,
                  Color{70, 100, 150, 255});

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, box)) {
        float value = (mouse.x - box.x) / box.width;

        if (value < 0.0f)
            value = 0.0f;
        if (value > 1.0f)
            value = 1.0f;
        return min_speed + (int)(value * (float)(max_speed - min_speed));
    }
    return speed;
}

void draw_panel(const LoaderRunner &runner, int origin_x, bool running, int speed) {
    static const Label L_TITLE = {"Погрузчик цеха", "Factory loader"};
    static const Label L_ROUTE = {"Маршрут", "Route"};
    static const Label L_COMMAND = {"Команда", "Command"};
    static const Label L_POINT = {"Метка", "Point"};
    static const Label L_ANGLE = {"Направление", "Heading"};
    static const Label L_TIME = {"Время, с", "Time, s"};
    static const Label L_PATH = {"Путь, см", "Path, cm"};
    static const Label L_STATE = {"Состояние", "State"};
    static const Label L_RUNNING = {"работа", "running"};
    static const Label L_PAUSED = {"пауза", "paused"};
    static const Label L_FAULT = {"АВАРИЯ", "FAULT"};
    static const Label L_DONE = {"задание выполнено", "task complete"};
    static const Label L_SPEED = {"Тактов в секунду", "Ticks per second"};

    int y = MARGIN;
    const int line = 22;

    draw_text(pick(L_TITLE), origin_x, y, 20, BLACK);
    y += line + 6;
    draw_text(TextFormat("%s: %d %s %d", pick(L_ROUTE), runner.route.from,
                         g_font != nullptr ? "→" : "->", runner.route.to),
              origin_x, y, 18, DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %s", pick(L_COMMAND), loader_runner_command_name(&runner)), origin_x,
              y, 18, DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %d", pick(L_POINT), runner.sensors.point), origin_x, y, 18, DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %s", pick(L_ANGLE), route_direction_name(runner.sensors.angle)),
              origin_x, y, 18, DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %.1f", pick(L_TIME), runner.plant.clock_ms / 1000.0),
              origin_x, y, 18, DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %d", pick(L_PATH), runner.sensors.odometer), origin_x, y, 18,
              DARKGRAY);
    y += line;
    draw_text(TextFormat("%s: %d/%d", pick(L_SPEED), speed, 240), origin_x, y, 18, DARKGRAY);
    y += line + 6;

    const char *state = pick(running ? L_RUNNING : L_PAUSED);
    Color state_color = DARKGREEN;

    if (runner.fault) {
        state = pick(L_FAULT);
        state_color = MAROON;
    } else if (loader_runner_done(&runner)) {
        state = pick(L_DONE);
        state_color = DARKBLUE;
    }
    draw_text(TextFormat("%s: %s", pick(L_STATE), state), origin_x, y, 18, state_color);
}

bool parse_direction(const char *text, int *direction) {
    if (strcmp(text, "up") == 0) {
        *direction = ROUTE_UP;
    } else if (strcmp(text, "right") == 0) {
        *direction = ROUTE_RIGHT;
    } else if (strcmp(text, "down") == 0) {
        *direction = ROUTE_DOWN;
    } else if (strcmp(text, "left") == 0) {
        *direction = ROUTE_LEFT;
    } else {
        return false;
    }
    return true;
}

/* Следующая существующая метка по кругу — так задание меняется без ввода. */
int next_point(const FactoryMap *map, int point) {
    for (int i = 1; i <= FACTORY_POINT_MAX; ++i) {
        int candidate = (point + i - 1) % FACTORY_POINT_MAX + 1;

        if (factory_point_cell(map, candidate, nullptr, nullptr))
            return candidate;
    }
    return point;
}

} /* namespace */

int main(int argc, char **argv) {
    const char *file_name = "factory.json";
    const char *font_name = nullptr;
    int from = 1;
    int to = 10;
    int direction = ROUTE_RIGHT;
    bool lift = false;
    /* Кадровый предел и снимок нужны не пользователю, а проверке: так
       приложение можно запустить и убедиться, что оно рисует, не оставляя
       окно висеть. */
    int frames_limit = 0;
    const char *shot_name = nullptr;
    const char *scenario_name = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            file_name = argv[++i];
        } else if (strcmp(argv[i], "--font") == 0 && i + 1 < argc) {
            font_name = argv[++i];
        } else if (strcmp(argv[i], "--from") == 0 && i + 1 < argc) {
            from = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) {
            to = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            if (!parse_direction(argv[++i], &direction)) {
                fprintf(stderr, "неизвестное направление: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--lift") == 0) {
            lift = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames_limit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shot_name = argv[++i];
        } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            scenario_name = argv[++i];
        } else {
            printf("Использование: 21-additional-gui [--file ФАЙЛ] [--font ФАЙЛ]\n");
            printf("       [--from МЕТКА] [--to МЕТКА] [--dir up|right|down|left] [--lift]\n");
            printf("       [--frames N] [--shot ФАЙЛ] [--scenario ФАЙЛ]\n");
            return EXIT_FAILURE;
        }
    }

    FactoryMap map;
    char error[256];

    if (!factory_map_read_file(&map, file_name, error, sizeof(error))) {
        fprintf(stderr, "не удалось прочитать %s: %s\n", file_name, error);
        return EXIT_FAILURE;
    }

    LoaderRunner runner;
    PlanOptions options;
    plan_options_default(&options);
    if (lift) {
        options.lift_pallet = 101;
        options.lift_stack = 5001;
    }
    if (!loader_runner_init(&runner, &map, from, to, direction, &options)) {
        fprintf(stderr, "маршрут от метки %d до метки %d не построен\n", from, to);
        factory_map_destroy(&map);
        return EXIT_FAILURE;
    }

    const int width = MARGIN * 2 + map.cols * TILE + PANEL_WIDTH + PORTS_WIDTH;
    const int height = MARGIN * 2 + map.rows * TILE;

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, "Statecraft — погрузчик цеха");
    if (!IsWindowReady()) {
        /* Ни дисплея, ни графического контекста: так бывает в CI и на ssh без
           X11. Внятное сообщение вместо падения — требование ТД-9. */
        fprintf(stderr, "окно не создано: графический вывод недоступен\n");
        factory_map_destroy(&map);
        return EXIT_FAILURE;
    }
    SetTargetFPS(60);

    bool font_loaded = false;
    Font font = load_font(font_name, &font_loaded);
    if (font_loaded)
        g_font = &font;

    bool running = true;
    int speed = 30; /* тактов автомата в секунду */
    float accumulator = 0.0f;
    int frames = 0;

    Scenario scenario;
    bool scenario_loaded = false;
    char scenario_note[128] = {0};

    /* Сценарий можно задать ключом или бросить файл в окно: файлового диалога
       в raylib нет, а перетаскивание работает на всех трёх платформах. */
    if (scenario_name != nullptr) {
        if (scenario_read_file(&scenario, scenario_name, error, sizeof(error))) {
            scenario_loaded = true;
            loader_runner_set_scenario(&runner, &scenario);
            snprintf(scenario_note, sizeof(scenario_note), "%s: %d", GetFileName(scenario_name),
                     scenario.count);
        } else {
            fprintf(stderr, "сценарий %s не прочитан: %s\n", scenario_name, error);
        }
    }

    const int control_x = MARGIN * 2 + map.cols * TILE;
    const int control_y = height - MARGIN - 92;
    Button button_run = {{(float)control_x, (float)control_y, 84.0f, 28.0f},
                         {"Пуск", "Run"},
                         false};
    Button button_pause = {{(float)control_x + 92.0f, (float)control_y, 96.0f, 28.0f},
                           {"Пауза", "Pause"},
                           false};
    Button button_reset = {{(float)control_x + 196.0f, (float)control_y, 96.0f, 28.0f},
                           {"Сброс", "Reset"},
                           false};

    while (!WindowShouldClose()) {
        if (frames_limit > 0 && frames >= frames_limit)
            break;
        ++frames;
        if (IsKeyPressed(KEY_SPACE))
            running = !running;
        if (IsKeyPressed(KEY_UP) && speed < 240)
            speed *= 2;
        if (IsKeyPressed(KEY_DOWN) && speed > 1)
            speed /= 2;
        if (IsKeyPressed(KEY_R)) {
            loader_runner_reset_fault(&runner);
            ports_snapshot(runner);
            loader_runner_tick(&runner);
        }
        if (IsKeyPressed(KEY_N)) {
            int start = loader_plant_point(&runner.plant);
            int target = next_point(&map, runner.target);

            if (start == 0)
                start = runner.route.from;
            if (target == start)
                target = next_point(&map, target);
            if (!loader_runner_init(&runner, &map, start, target, runner.plant.angle, &options))
                fprintf(stderr, "маршрут от метки %d до метки %d не построен\n", start, target);
        }
        if (IsKeyPressed(KEY_S) && !running) {
            ports_snapshot(runner);
            loader_runner_tick(&runner);
        }

        /* Перетащенный в окно файл считается сценарием. */
        if (IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();

            if (dropped.count > 0) {
                Scenario loaded;

                if (scenario_read_file(&loaded, dropped.paths[0], error, sizeof(error))) {
                    if (scenario_loaded)
                        scenario_destroy(&scenario);
                    scenario = loaded;
                    scenario_loaded = true;
                    loader_runner_set_scenario(&runner, &scenario);
                    snprintf(scenario_note, sizeof(scenario_note), "%s: %d",
                             GetFileName(dropped.paths[0]), scenario.count);
                } else {
                    snprintf(scenario_note, sizeof(scenario_note), "%s", error);
                }
            }
            UnloadDroppedFiles(dropped);
        }

        bool finished = scenario_loaded ? loader_runner_scenario_done(&runner)
                                        : loader_runner_done(&runner);
        if (running && !runner.fault && !finished) {
            accumulator += GetFrameTime() * (float)speed;
            while (accumulator >= 1.0f) {
                ports_snapshot(runner);
                loader_runner_tick(&runner);
                accumulator -= 1.0f;
                if (runner.fault ||
                    (scenario_loaded ? loader_runner_scenario_done(&runner)
                                     : loader_runner_done(&runner)))
                    break;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        draw_map(&map, runner.route);
        draw_loader(runner);
        draw_panel(runner, control_x, running, speed);
        draw_ports(runner, control_x + PANEL_WIDTH, MARGIN);

        button_run.active = running;
        button_pause.active = !running;
        if (draw_button(button_run, true))
            running = true;
        if (draw_button(button_pause, true))
            running = false;
        if (draw_button(button_reset, true)) {
            /* Сброс — это и снятие аварии, и возврат прогона в начало. */
            int start = runner.route.from;
            int target = runner.route.to;

            if (!loader_runner_init(&runner, &map, start, target, direction, &options)) {
                fprintf(stderr, "прогон не перезапущен\n");
            } else if (lift) {
                loader_plant_place_pallet(&runner.plant, target, 5001, 101);
            }
            if (scenario_loaded)
                loader_runner_set_scenario(&runner, &scenario);
            accumulator = 0.0f;
        }
        speed = draw_speed_slider({(float)control_x, (float)(control_y + 60), 292.0f, 14.0f},
                                  speed);
        {
            static const Label L_SCENARIO = {"Сценарий (бросьте файл в окно)",
                                             "Scenario (drop a file here)"};

            static const Label L_KEYS = {"Пробел пуск   S шаг   R квитировать   N цель",
                                         "Space run   S step   R ack fault   N target"};

            draw_text(scenario_loaded ? scenario_note : pick(L_SCENARIO), control_x,
                      control_y + 82, 14, scenario_loaded ? DARKBLUE : GRAY);
            draw_text(pick(L_KEYS), control_x, control_y - 26, 13, GRAY);
        }
        EndDrawing();

        if (shot_name != nullptr && frames_limit > 0 && frames >= frames_limit) {
            TakeScreenshot(shot_name);
            shot_name = nullptr;
        }
    }

    if (scenario_loaded)
        scenario_destroy(&scenario);
    if (font_loaded)
        UnloadFont(font);
    CloseWindow();
    factory_map_destroy(&map);
    return EXIT_SUCCESS;
}
