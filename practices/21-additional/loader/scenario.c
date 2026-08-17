/**
 * @file
 * Разбор сценария входных портов: см. scenario.h.
 *
 * Разборщик отдельный от того, что читает конфигурацию цеха (factory_map.c), и
 * это не дублирование: там матрицы целых, здесь массив объектов «имя порта —
 * значение». Общего у них только подмножество JSON, а объединение ради него
 * стоило бы дороже, чем две сотни строк.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scenario.h"

static const char *const PORT_NAMES[LOADER_IN_COUNT]
        = {"cmd_valid",    "cmd_code",    "cmd_point",    "cmd_extra",      "cmd_timeout",
           "sense_line",   "sense_point", "sense_angle",  "sense_odometer", "sense_range",
           "sense_motion", "sense_stack", "sense_pallet", "sense_load",     "reset"};

const char *loader_in_port_name(int port) {
    if (port < 0 || port >= LOADER_IN_COUNT)
        return "?";
    return PORT_NAMES[port];
}

struct Reader {
    const char *text;
    size_t pos;
    char *error;
    size_t error_size;
};

static bool fail(struct Reader *reader, const char *message) {
    if (reader->error != NULL && reader->error_size > 0) {
        strncpy(reader->error, message, reader->error_size - 1);
        reader->error[reader->error_size - 1] = '\0';
    }
    return false;
}

static char peek(struct Reader *reader) {
    while (reader->text[reader->pos] == ' ' || reader->text[reader->pos] == '\t'
           || reader->text[reader->pos] == '\n' || reader->text[reader->pos] == '\r')
        ++reader->pos;
    return reader->text[reader->pos];
}

static bool expect(struct Reader *reader, char symbol) {
    if (peek(reader) != symbol)
        return fail(reader, "сценарий разобран не до конца: неожиданный символ");
    ++reader->pos;
    return true;
}

static bool read_string(struct Reader *reader, char *value, size_t value_size) {
    size_t length = 0;

    if (!expect(reader, '"'))
        return false;
    while (reader->text[reader->pos] != '"') {
        if (reader->text[reader->pos] == '\0')
            return fail(reader, "строка не закрыта");
        if (value != NULL && length + 1 < value_size)
            value[length++] = reader->text[reader->pos];
        ++reader->pos;
    }
    ++reader->pos;
    if (value != NULL && value_size > 0)
        value[length] = '\0';
    return true;
}

static bool read_int(struct Reader *reader, int *value) {
    int sign = 1;
    long result = 0;
    int digits = 0;

    peek(reader);
    if (reader->text[reader->pos] == '-') {
        sign = -1;
        ++reader->pos;
    }
    while (reader->text[reader->pos] >= '0' && reader->text[reader->pos] <= '9') {
        result = result * 10 + (reader->text[reader->pos] - '0');
        ++digits;
        ++reader->pos;
    }
    if (digits == 0)
        return fail(reader, "ожидалось число");
    *value = (int)(sign * result);
    return true;
}

static bool skip_value(struct Reader *reader);

static bool skip_composite(struct Reader *reader, char open, char close) {
    if (!expect(reader, open))
        return false;
    if (peek(reader) == close) {
        ++reader->pos;
        return true;
    }
    for (;;) {
        if (open == '{') {
            if (!read_string(reader, NULL, 0) || !expect(reader, ':'))
                return false;
        }
        if (!skip_value(reader))
            return false;
        if (peek(reader) != ',')
            break;
        ++reader->pos;
    }
    return expect(reader, close);
}

static bool skip_value(struct Reader *reader) {
    char c = peek(reader);

    if (c == '"')
        return read_string(reader, NULL, 0);
    if (c == '[')
        return skip_composite(reader, '[', ']');
    if (c == '{')
        return skip_composite(reader, '{', '}');
    if (c == 't' || c == 'f' || c == 'n') {
        while (reader->text[reader->pos] >= 'a' && reader->text[reader->pos] <= 'z')
            ++reader->pos;
        return true;
    }
    {
        int ignored;
        return read_int(reader, &ignored);
    }
}

static int port_by_name(const char *name) {
    int i;

    for (i = 0; i < LOADER_IN_COUNT; ++i) {
        if (strcmp(name, PORT_NAMES[i]) == 0)
            return i;
    }
    return -1;
}

/* Значения портов шага. Незнакомые имена пропускаются: сценарий может
   описывать модель, у которой портов больше. */
static bool read_in_ports(struct Reader *reader, struct ScenarioStep *step) {
    if (!expect(reader, '{'))
        return false;
    if (peek(reader) == '}') {
        ++reader->pos;
        return true;
    }
    for (;;) {
        char name[32];
        int port;

        if (!read_string(reader, name, sizeof(name)) || !expect(reader, ':'))
            return false;
        port = port_by_name(name);
        if (port >= 0) {
            int value;

            if (!read_int(reader, &value))
                return false;
            step->value[port] = value;
        } else if (!skip_value(reader)) {
            return false;
        }
        if (peek(reader) != ',')
            break;
        ++reader->pos;
    }
    return expect(reader, '}');
}

static bool read_step(struct Reader *reader, struct ScenarioStep *step) {
    memset(step, 0, sizeof(*step));
    if (!expect(reader, '{'))
        return false;
    if (peek(reader) == '}') {
        ++reader->pos;
        return true;
    }
    for (;;) {
        char name[32];

        if (!read_string(reader, name, sizeof(name)) || !expect(reader, ':'))
            return false;
        if (strcmp(name, "in_ports") == 0) {
            if (!read_in_ports(reader, step))
                return false;
        } else if (!skip_value(reader)) {
            return false;
        }
        if (peek(reader) != ',')
            break;
        ++reader->pos;
    }
    return expect(reader, '}');
}

static bool read_scenario(struct Reader *reader, struct Scenario *scenario) {
    int capacity = 0;

    if (!expect(reader, '['))
        return false;
    if (peek(reader) == ']') {
        ++reader->pos;
        return fail(reader, "сценарий пуст");
    }
    for (;;) {
        if (scenario->count == capacity) {
            int next = capacity == 0 ? 32 : capacity * 2;
            struct ScenarioStep *grown = (struct ScenarioStep *)realloc(
                    scenario->step, (size_t)next * sizeof(struct ScenarioStep));

            if (grown == NULL)
                return fail(reader, "не хватило памяти под сценарий");
            scenario->step = grown;
            capacity = next;
        }
        if (!read_step(reader, &scenario->step[scenario->count]))
            return false;
        ++scenario->count;
        if (peek(reader) != ',')
            break;
        ++reader->pos;
    }
    return expect(reader, ']');
}

bool scenario_read_file(struct Scenario *scenario, const char *file_name, char *error,
                        size_t error_size) {
    FILE *file;
    char *text;
    long size;
    size_t read;
    struct Reader reader;
    bool ok;

    if (scenario == NULL || file_name == NULL)
        return false;
    memset(scenario, 0, sizeof(*scenario));
    if (error != NULL && error_size > 0)
        error[0] = '\0';
    file = fopen(file_name, "rb");
    if (file == NULL) {
        if (error != NULL && error_size > 0) {
            strncpy(error, "сценарий не открывается", error_size - 1);
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

    reader.text = text;
    reader.pos = 0;
    reader.error = error;
    reader.error_size = error_size;
    ok = read_scenario(&reader, scenario);
    free(text);
    if (!ok)
        scenario_destroy(scenario);
    return ok;
}

void scenario_destroy(struct Scenario *scenario) {
    if (scenario == NULL)
        return;
    free(scenario->step);
    scenario->step = NULL;
    scenario->count = 0;
}
