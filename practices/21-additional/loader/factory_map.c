/**
 * @file
 * Чтение конфигурации цеха: подмножество JSON без сторонних библиотек.
 *
 * Разбирается ровно то, что встречается в factory.json: объект верхнего
 * уровня, строковые значения, массивы массивов целых. Значения незнакомых
 * ключей пропускаются целиком, чтобы файл, дополненный чем-то ещё, не
 * перестал читаться.
 *
 * Оригинал (c_fsm, transport_loader.c) подключал для этого cJSON и требовал
 * квадратную матрицу: `matrix_allocate(&m, size)` выделяла size на size по
 * длине первой строки, а число строк не проверялось вовсе — файл с лишней
 * строкой читался за пределы выделенной памяти. Здесь карта может быть
 * прямоугольной, а несовпадение длин строк — ошибка с номером строки файла.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "factory_map.h"

struct Parser {
    const char *text;
    size_t pos;
    int line;
    char *error;
    size_t error_size;
};

/*
 * Сообщение об ошибке собирается вручную: `snprintf` появился только в C99, а
 * `sprintf` в буфер известного размера — как раз то, за что его и ругают.
 */
static void append_text(char *buffer, size_t size, const char *text) {
    size_t length = strlen(buffer);

    while (*text != '\0' && length + 1 < size)
        buffer[length++] = *text++;
    buffer[length] = '\0';
}

static void append_int(char *buffer, size_t size, int value) {
    char digits[16];
    int count = 0;

    if (value < 0) {
        append_text(buffer, size, "-");
        value = -value;
    }
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0 && count < (int)sizeof(digits) - 1);
    while (count-- > 0) {
        char one[2];

        one[0] = digits[count];
        one[1] = '\0';
        append_text(buffer, size, one);
    }
}

static bool fail(struct Parser *parser, const char *message) {
    if (parser->error != NULL && parser->error_size > 0) {
        parser->error[0] = '\0';
        append_text(parser->error, parser->error_size, "строка ");
        append_int(parser->error, parser->error_size, parser->line);
        append_text(parser->error, parser->error_size, ": ");
        append_text(parser->error, parser->error_size, message);
    }
    return false;
}

static void skip_spaces(struct Parser *parser) {
    while (parser->text[parser->pos] != '\0') {
        char c = parser->text[parser->pos];

        if (c == '\n') {
            ++parser->line;
            ++parser->pos;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            ++parser->pos;
        } else {
            break;
        }
    }
}

static char peek(struct Parser *parser) {
    skip_spaces(parser);
    return parser->text[parser->pos];
}

static bool expect(struct Parser *parser, char symbol) {
    if (peek(parser) != symbol) {
        char message[64];
        char quoted[4];

        quoted[0] = symbol;
        quoted[1] = '\'';
        quoted[2] = '\0';
        message[0] = '\0';
        append_text(message, sizeof(message), "ожидался символ '");
        append_text(message, sizeof(message), quoted);
        return fail(parser, message);
    }
    ++parser->pos;
    return true;
}

/* Строка JSON без экранирования: в конфигурации цеха его нет, а молчаливая
   поддержка половины escape-последовательностей хуже честного отказа. */
static bool parse_string(struct Parser *parser, char *value, size_t value_size) {
    size_t length = 0;

    if (!expect(parser, '"'))
        return false;
    while (parser->text[parser->pos] != '"') {
        char c = parser->text[parser->pos];

        if (c == '\0')
            return fail(parser, "строка не закрыта");
        if (c == '\\')
            return fail(parser, "экранирование в строках не поддержано");
        if (c == '\n')
            return fail(parser, "перевод строки внутри строки");
        if (value != NULL && length + 1 < value_size)
            value[length] = c;
        ++length;
        ++parser->pos;
    }
    ++parser->pos;
    if (value != NULL && value_size > 0)
        value[length < value_size ? length : value_size - 1] = '\0';
    return true;
}

static bool parse_int(struct Parser *parser, int *value) {
    int sign = 1;
    long result = 0;
    int digits = 0;

    skip_spaces(parser);
    if (parser->text[parser->pos] == '-') {
        sign = -1;
        ++parser->pos;
    }
    while (parser->text[parser->pos] >= '0' && parser->text[parser->pos] <= '9') {
        result = result * 10 + (parser->text[parser->pos] - '0');
        if (result > 1000000L)
            return fail(parser, "слишком большое число");
        ++digits;
        ++parser->pos;
    }
    if (digits == 0)
        return fail(parser, "ожидалось целое число");
    *value = (int)(sign * result);
    return true;
}

/*
 * Матрица: массив массивов целых. Длину строки задаёт первая строка, все
 * остальные обязаны ей соответствовать — иначе неясно, что считать картой.
 */
static bool parse_matrix(struct Parser *parser, int **data, int *rows, int *cols) {
    int *values = NULL;
    int capacity = 0;
    int count = 0;
    int row_count = 0;
    int col_count = -1;

    *data = NULL;
    *rows = 0;
    *cols = 0;
    if (!expect(parser, '['))
        return false;
    if (peek(parser) == ']') {
        ++parser->pos;
        return fail(parser, "пустая матрица");
    }
    for (;;) {
        int in_row = 0;

        if (!expect(parser, '[')) {
            free(values);
            return false;
        }
        for (;;) {
            int value;

            if (!parse_int(parser, &value)) {
                free(values);
                return false;
            }
            if (count == capacity) {
                int next = capacity == 0 ? 256 : capacity * 2;
                int *grown = (int *)realloc(values, (size_t)next * sizeof(int));

                if (grown == NULL) {
                    free(values);
                    return fail(parser, "не хватило памяти под матрицу");
                }
                values = grown;
                capacity = next;
            }
            values[count++] = value;
            ++in_row;
            if (peek(parser) != ',')
                break;
            ++parser->pos;
        }
        if (!expect(parser, ']')) {
            free(values);
            return false;
        }
        if (col_count < 0) {
            col_count = in_row;
            if (col_count > FACTORY_MAX_SIDE) {
                free(values);
                return fail(parser, "строка матрицы длиннее допустимого");
            }
        } else if (in_row != col_count) {
            free(values);
            return fail(parser, "строки матрицы разной длины");
        }
        ++row_count;
        if (row_count > FACTORY_MAX_SIDE) {
            free(values);
            return fail(parser, "матрица выше допустимого");
        }
        if (peek(parser) != ',')
            break;
        ++parser->pos;
    }
    if (!expect(parser, ']')) {
        free(values);
        return false;
    }
    *data = values;
    *rows = row_count;
    *cols = col_count;
    return true;
}

static bool skip_value(struct Parser *parser);

static bool skip_composite(struct Parser *parser, char open, char close) {
    if (!expect(parser, open))
        return false;
    if (peek(parser) == close) {
        ++parser->pos;
        return true;
    }
    for (;;) {
        if (open == '{') {
            if (!parse_string(parser, NULL, 0) || !expect(parser, ':'))
                return false;
        }
        if (!skip_value(parser))
            return false;
        if (peek(parser) != ',')
            break;
        ++parser->pos;
    }
    return expect(parser, close);
}

/* Пропуск значения незнакомого ключа: строка, число, массив, объект,
   true/false/null. */
static bool skip_value(struct Parser *parser) {
    char c = peek(parser);

    if (c == '"')
        return parse_string(parser, NULL, 0);
    if (c == '[')
        return skip_composite(parser, '[', ']');
    if (c == '{')
        return skip_composite(parser, '{', '}');
    if (c == 't' || c == 'f' || c == 'n') {
        while (parser->text[parser->pos] >= 'a' && parser->text[parser->pos] <= 'z')
            ++parser->pos;
        return true;
    }
    {
        int ignored;
        return parse_int(parser, &ignored);
    }
}

static int layer_by_name(const char *name) {
    if (strcmp(name, "map") == 0)
        return FACTORY_LAYER_MAP;
    if (strcmp(name, "things") == 0)
        return FACTORY_LAYER_THINGS;
    if (strcmp(name, "paths") == 0)
        return FACTORY_LAYER_PATHS;
    return -1;
}

static bool parse_document(struct Parser *parser, struct FactoryMap *map) {
    if (!expect(parser, '{'))
        return false;
    if (peek(parser) == '}') {
        ++parser->pos;
        return fail(parser, "конфигурация пуста");
    }
    for (;;) {
        char name[32];
        int layer;

        if (!parse_string(parser, name, sizeof(name)) || !expect(parser, ':'))
            return false;
        layer = layer_by_name(name);
        if (layer >= 0) {
            int *data;
            int rows;
            int cols;

            if (map->layer[layer] != NULL)
                return fail(parser, "слой задан дважды");
            if (!parse_matrix(parser, &data, &rows, &cols))
                return false;
            if (map->rows == 0) {
                map->rows = rows;
                map->cols = cols;
            } else if (map->rows != rows || map->cols != cols) {
                free(data);
                return fail(parser, "слои разного размера");
            }
            map->layer[layer] = data;
        } else if (strcmp(name, "version") == 0) {
            if (!parse_string(parser, map->version, sizeof(map->version)))
                return false;
        } else if (!skip_value(parser)) {
            return false;
        }
        if (peek(parser) != ',')
            break;
        ++parser->pos;
    }
    if (!expect(parser, '}'))
        return false;
    if (map->layer[FACTORY_LAYER_PATHS] == NULL)
        return fail(parser, "в конфигурации нет слоя paths");
    return true;
}

bool factory_map_read_memory(struct FactoryMap *map, const char *text, char *error,
                             size_t error_size) {
    struct Parser parser;
    int i;

    if (map == NULL || text == NULL)
        return false;
    memset(map, 0, sizeof(*map));
    strcpy(map->version, "unknown");
    parser.text = text;
    parser.pos = 0;
    parser.line = 1;
    parser.error = error;
    parser.error_size = error_size;
    if (error != NULL && error_size > 0)
        error[0] = '\0';
    if (!parse_document(&parser, map)) {
        factory_map_destroy(map);
        return false;
    }
    for (i = 0; i < FACTORY_LAYER_COUNT; ++i) {
        /* Слоя может не быть — тогда все его клетки пусты. Это законно:
           practices/21-additional/factory-grid.json обходится одним paths. */
        (void)i;
    }
    return true;
}

bool factory_map_read_file(struct FactoryMap *map, const char *file_name, char *error,
                           size_t error_size) {
    FILE *file;
    char *text;
    long size;
    size_t read;
    bool result;

    if (map == NULL || file_name == NULL)
        return false;
    file = fopen(file_name, "rb");
    if (file == NULL) {
        if (error != NULL && error_size > 0) {
            strncpy(error, "файл конфигурации не открывается", error_size - 1);
            error[error_size - 1] = '\0';
        }
        return false;
    }
    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    text = (char *)malloc((size_t)size + 1);
    if (text == NULL) {
        fclose(file);
        return false;
    }
    read = fread(text, 1, (size_t)size, file);
    text[read] = '\0';
    fclose(file);
    result = factory_map_read_memory(map, text, error, error_size);
    free(text);
    return result;
}

void factory_map_destroy(struct FactoryMap *map) {
    int i;

    if (map == NULL)
        return;
    for (i = 0; i < FACTORY_LAYER_COUNT; ++i) {
        free(map->layer[i]);
        map->layer[i] = NULL;
    }
    map->rows = 0;
    map->cols = 0;
}

int factory_cell(const struct FactoryMap *map, enum FactoryLayer layer, int row, int col) {
    if (map == NULL || layer < 0 || layer >= FACTORY_LAYER_COUNT)
        return 0;
    if (map->layer[layer] == NULL)
        return 0;
    if (row < 0 || row >= map->rows || col < 0 || col >= map->cols)
        return 0;
    return map->layer[layer][row * map->cols + col];
}

bool factory_is_line(const struct FactoryMap *map, int row, int col) {
    return factory_cell(map, FACTORY_LAYER_PATHS, row, col) != 0;
}

int factory_point_at(const struct FactoryMap *map, int row, int col) {
    int value = factory_cell(map, FACTORY_LAYER_PATHS, row, col);

    return (value >= 1 && value <= FACTORY_POINT_MAX) ? value : 0;
}

bool factory_point_cell(const struct FactoryMap *map, int point, int *row, int *col) {
    int r;
    int c;

    if (map == NULL || point < 1 || point > FACTORY_POINT_MAX)
        return false;
    for (r = 0; r < map->rows; ++r) {
        for (c = 0; c < map->cols; ++c) {
            if (factory_point_at(map, r, c) == point) {
                if (row != NULL)
                    *row = r;
                if (col != NULL)
                    *col = c;
                return true;
            }
        }
    }
    return false;
}

int factory_point_count(const struct FactoryMap *map) {
    int r;
    int c;
    int count = 0;

    if (map == NULL)
        return 0;
    for (r = 0; r < map->rows; ++r) {
        for (c = 0; c < map->cols; ++c) {
            if (factory_point_at(map, r, c) != 0)
                ++count;
        }
    }
    return count;
}

/*
 * Схема цеха. Слой map кодирует тайлы картинки, и разбирать их все незачем:
 * для печати важно лишь, где погрузчику есть куда ехать. Значение 13 — покрытие
 * проезда (по нему и проложена разметка), 1…12 — стены и контуры стеллажей,
 * 0 — пространство, до которого погрузчику дела нет.
 *
 * Значение 13 легко принять за тело стеллажа — так и было в первой версии
 * этой печати, пока картинка не показала, что «стеллажи» образуют ровно те
 * П-образные проходы, по которым идёт линия.
 */
void factory_map_print(const struct FactoryMap *map, FILE *out) {
    int r;
    int c;

    if (map == NULL || out == NULL)
        return;
    for (r = 0; r < map->rows; ++r) {
        for (c = 0; c < map->cols; ++c) {
            int point = factory_point_at(map, r, c);
            int path = factory_cell(map, FACTORY_LAYER_PATHS, r, c);
            int tile = factory_cell(map, FACTORY_LAYER_MAP, r, c);

            if (point != 0) {
                /* Метки печатаются по основанию 36: до 90 их всё равно не
                   бывает, а один знак держит колонку ровной. */
                fputc(point < 10 ? (char)('0' + point) : (char)('a' + point - 10), out);
            } else if (path == FACTORY_PATH_H) {
                fputc('-', out);
            } else if (path == FACTORY_PATH_V) {
                fputc('|', out);
            } else if (path != 0) {
                fputc('?', out);
            } else if (factory_cell(map, FACTORY_LAYER_THINGS, r, c) != 0) {
                fputc('o', out); /* что-то стоит на полу */
            } else if (tile == 13) {
                fputc('.', out); /* покрытие проезда */
            } else if (tile != 0) {
                fputc('#', out); /* стена или стеллаж */
            } else {
                fputc(' ', out);
            }
        }
        fputc('\n', out);
    }
}
