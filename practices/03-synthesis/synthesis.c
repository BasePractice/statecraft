#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "synthesis.h"

void machine_init(struct Machine *machine) {
    int s;
    int a;

    assert(machine != NULL);
    memset(machine, 0, sizeof(*machine));
    strcpy(machine->name, "machine");
    for (s = 0; s < SYN_MAX_STATES; ++s) {
        for (a = 0; a < SYN_MAX_SYMBOLS; ++a) {
            machine->next[s][a] = -1;
            machine->emit[s][a] = -1;
        }
    }
}

static int find_name(char table[][SYN_MAX_NAME], int count, const char *name) {
    int i;

    for (i = 0; i < count; ++i) {
        if (strcmp(table[i], name) == 0)
            return i;
    }
    return -1;
}

static int add_names(char table[][SYN_MAX_NAME], int *count, const char *line) {
    const char *p = line;
    char word[SYN_MAX_NAME];
    int taken;

    while (sscanf(p, "%31s%n", word, &taken) == 1) {
        p += taken;
        if (find_name(table, *count, word) >= 0)
            continue;
        if (*count >= SYN_MAX_SYMBOLS)
            return -1;
        strcpy(table[*count], word);
        ++(*count);
    }
    return *count;
}

bool machine_read(struct Machine *machine, FILE *in) {
    char line[256];
    char from[SYN_MAX_NAME];
    char symbol[SYN_MAX_NAME];
    char to[SYN_MAX_NAME];
    char out[SYN_MAX_NAME];

    assert(machine != NULL && in != NULL);
    machine_init(machine);

    while (fgets(line, (int)sizeof(line), in) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        if (strncmp(line, "name ", 5) == 0) {
            sscanf(line + 5, "%31s", machine->name);
            continue;
        }
        if (strncmp(line, "inputs ", 7) == 0) {
            if (add_names(machine->inputs, &machine->input_count, line + 7) < 0) {
                fprintf(stderr, "synthesis: входной алфавит больше %d символов\n", SYN_MAX_SYMBOLS);
                return false;
            }
            continue;
        }
        if (strncmp(line, "outputs ", 8) == 0) {
            if (add_names(machine->outputs, &machine->output_count, line + 8) < 0) {
                fprintf(stderr, "synthesis: выходной алфавит больше %d символов\n",
                        SYN_MAX_SYMBOLS);
                return false;
            }
            continue;
        }
        if (strncmp(line, "states ", 7) == 0) {
            if (add_names(machine->states, &machine->state_count, line + 7) < 0) {
                fprintf(stderr, "synthesis: состояний больше %d\n", SYN_MAX_STATES);
                return false;
            }
            continue;
        }
        if (sscanf(line, "%31s %31s -> %31s %31s", from, symbol, to, out) == 4) {
            int s = find_name(machine->states, machine->state_count, from);
            int a = find_name(machine->inputs, machine->input_count, symbol);
            int t = find_name(machine->states, machine->state_count, to);
            int y = find_name(machine->outputs, machine->output_count, out);

            if (s < 0 || a < 0 || t < 0 || y < 0) {
                fprintf(stderr, "synthesis: в строке «%s» есть необъявленное имя\n", line);
                return false;
            }
            machine->next[s][a] = t;
            machine->emit[s][a] = y;
            continue;
        }
        fprintf(stderr, "synthesis: непонятная строка: %s", line);
        return false;
    }

    if (machine->state_count == 0 || machine->input_count == 0 || machine->output_count == 0) {
        fprintf(stderr, "synthesis: не заданы состояния, входной или выходной алфавит\n");
        return false;
    }
    return true;
}

void machine_print_table(const struct Machine *machine, FILE *out) {
    int s;
    int a;

    assert(machine != NULL && out != NULL);
    fprintf(out, "%-10s", "");
    for (a = 0; a < machine->input_count; ++a) {
        fprintf(out, "%-10s", machine->inputs[a]);
    }
    fputc('\n', out);
    for (s = 0; s < machine->state_count; ++s) {
        fprintf(out, "%-10s", machine->states[s]);
        for (a = 0; a < machine->input_count; ++a) {
            char cell[SYN_MAX_NAME * 2];

            if (machine->next[s][a] < 0) {
                strcpy(cell, "*");
            } else {
                sprintf(cell, "%s/%s", machine->states[machine->next[s][a]],
                        machine->outputs[machine->emit[s][a]]);
            }
            fprintf(out, "%-10s", cell);
        }
        fputc('\n', out);
    }
}

int synthesis_bits(int count) {
    int bits = 1;
    int capacity = 2;

    assert(count > 0);
    while (capacity < count) {
        capacity *= 2;
        ++bits;
    }
    return bits;
}

/* Номер строки таблицы истинности для пары «состояние, входной символ».
   Старшие разряды — код входа, младшие — код состояния: тот же порядок,
   что в картах Карно лекции. */
static int row_of(const struct Synthesis *synthesis, int state, int input) {
    return (input << synthesis->state_bits) | state;
}

static int bit_of(int value, int bit_index, int bit_count) {
    return (value >> (bit_count - 1 - bit_index)) & 1;
}

bool synthesis_run(struct Synthesis *synthesis, const struct Machine *machine) {
    int var_count;
    int rows;
    int i;
    int s;
    int a;

    assert(synthesis != NULL && machine != NULL);
    synthesis->input_bits = synthesis_bits(machine->input_count);
    synthesis->state_bits = synthesis_bits(machine->state_count);
    synthesis->output_bits = synthesis_bits(machine->output_count);

    var_count = synthesis->input_bits + synthesis->state_bits;
    if (var_count > BOOL_MAX_VARS) {
        fprintf(stderr, "synthesis: нужно %d переменных, предел — %d\n", var_count, BOOL_MAX_VARS);
        return false;
    }
    rows = 1 << var_count;

    /* Имена переменных: x1..xk — разряды входа, q1..qm — разряды состояния. */
    for (i = 0; i < synthesis->state_bits; ++i) {
        char name[BOOL_MAX_NAME];

        bool_init(&synthesis->phi[i], var_count);
        sprintf(name, "phi%d", i + 1);
        (void)name;
    }
    for (i = 0; i < synthesis->output_bits; ++i) {
        bool_init(&synthesis->psi[i], var_count);
    }

    for (i = 0; i < synthesis->state_bits + synthesis->output_bits; ++i) {
        struct BoolFunction *function = (i < synthesis->state_bits)
                                                ? &synthesis->phi[i]
                                                : &synthesis->psi[i - synthesis->state_bits];
        int v;

        for (v = 0; v < synthesis->input_bits; ++v) {
            char name[BOOL_MAX_NAME];
            sprintf(name, "x%d", v + 1);
            bool_set_name(function, v, name);
        }
        for (v = 0; v < synthesis->state_bits; ++v) {
            char name[BOOL_MAX_NAME];
            sprintf(name, "q%d", v + 1);
            bool_set_name(function, synthesis->input_bits + v, name);
        }
    }

    /* Все наборы сначала безразличны: коды несуществующих состояний и
       входных символов так и останутся безразличными — это звёздочки
       в таблице лекции. */
    for (i = 0; i < synthesis->state_bits; ++i) {
        int row;
        for (row = 0; row < rows; ++row) {
            bool_set(&synthesis->phi[i], row, BOOL_DONT_CARE);
        }
    }
    for (i = 0; i < synthesis->output_bits; ++i) {
        int row;
        for (row = 0; row < rows; ++row) {
            bool_set(&synthesis->psi[i], row, BOOL_DONT_CARE);
        }
    }

    for (s = 0; s < machine->state_count; ++s) {
        for (a = 0; a < machine->input_count; ++a) {
            int row = row_of(synthesis, s, a);
            int next_state = machine->next[s][a];
            int output = machine->emit[s][a];

            if (next_state < 0)
                continue; /* переход не задан — набор остаётся безразличным */

            for (i = 0; i < synthesis->state_bits; ++i) {
                bool_set(&synthesis->phi[i], row,
                         (char)bit_of(next_state, i, synthesis->state_bits));
            }
            for (i = 0; i < synthesis->output_bits; ++i) {
                bool_set(&synthesis->psi[i], row, (char)bit_of(output, i, synthesis->output_bits));
            }
        }
    }

    for (i = 0; i < synthesis->state_bits; ++i) {
        if (!bool_minimize(&synthesis->phi_dnf[i], &synthesis->phi[i]))
            return false;
    }
    for (i = 0; i < synthesis->output_bits; ++i) {
        if (!bool_minimize(&synthesis->psi_dnf[i], &synthesis->psi[i]))
            return false;
    }
    return true;
}

bool synthesis_verify(const struct Synthesis *synthesis, const struct Machine *machine) {
    int i;
    int s;
    int a;

    assert(synthesis != NULL && machine != NULL);
    for (i = 0; i < synthesis->state_bits; ++i) {
        if (!dnf_equals(&synthesis->phi_dnf[i], &synthesis->phi[i]))
            return false;
    }
    for (i = 0; i < synthesis->output_bits; ++i) {
        if (!dnf_equals(&synthesis->psi_dnf[i], &synthesis->psi[i]))
            return false;
    }

    /* Сверх поразрядной проверки — прогон всей таблицы переходов:
       формулы должны воспроизводить состояние и выход целиком. */
    for (s = 0; s < machine->state_count; ++s) {
        for (a = 0; a < machine->input_count; ++a) {
            int row = row_of(synthesis, s, a);
            int next_state = 0;
            int output = 0;

            if (machine->next[s][a] < 0)
                continue;
            for (i = 0; i < synthesis->state_bits; ++i) {
                next_state = (next_state << 1)
                             | dnf_eval(&synthesis->phi_dnf[i], &synthesis->phi[i], row);
            }
            for (i = 0; i < synthesis->output_bits; ++i) {
                output = (output << 1) | dnf_eval(&synthesis->psi_dnf[i], &synthesis->psi[i], row);
            }
            if (next_state != machine->next[s][a] || output != machine->emit[s][a])
                return false;
        }
    }
    return true;
}

void synthesis_print(const struct Synthesis *synthesis, const struct Machine *machine, FILE *out) {
    int i;

    assert(synthesis != NULL && machine != NULL && out != NULL);
    fprintf(out,
            "Автомат «%s»: входов %d (%d разр.), состояний %d (%d разр.), "
            "выходов %d (%d разр.)\n",
            machine->name, machine->input_count, synthesis->input_bits, machine->state_count,
            synthesis->state_bits, machine->output_count, synthesis->output_bits);

    fprintf(out, "\nФункции переходов:\n");
    for (i = 0; i < synthesis->state_bits; ++i) {
        fprintf(out, "  q%d(t+1) = ", i + 1);
        dnf_print(&synthesis->phi_dnf[i], &synthesis->phi[i], out);
        fprintf(out, "   [литералов: %d]\n", dnf_literals(&synthesis->phi_dnf[i]));
    }

    fprintf(out, "\nФункции выходов:\n");
    for (i = 0; i < synthesis->output_bits; ++i) {
        fprintf(out, "  y%d(t)   = ", i + 1);
        dnf_print(&synthesis->psi_dnf[i], &synthesis->psi[i], out);
        fprintf(out, "   [литералов: %d]\n", dnf_literals(&synthesis->psi_dnf[i]));
    }
}

void synthesis_print_c(const struct Synthesis *synthesis, const struct Machine *machine,
                       FILE *out) {
    int i;

    assert(synthesis != NULL && machine != NULL && out != NULL);
    fprintf(out, "/* Порождено практикой лекции 3 по таблице переходов автомата «%s».\n",
            machine->name);
    fprintf(out, "   Состояние — %d разряд(а), выход — %d разряд(а). */\n\n", synthesis->state_bits,
            synthesis->output_bits);

    fprintf(out, "struct %sState {\n", machine->name);
    for (i = 0; i < synthesis->state_bits; ++i) {
        fprintf(out, "    int q%d;\n", i + 1);
    }
    fprintf(out, "};\n\n");

    fprintf(out, "void %s_step(struct %sState *state", machine->name, machine->name);
    for (i = 0; i < synthesis->input_bits; ++i) {
        fprintf(out, ", int x%d", i + 1);
    }
    for (i = 0; i < synthesis->output_bits; ++i) {
        fprintf(out, ", int *y%d", i + 1);
    }
    fprintf(out, ") {\n");

    for (i = 0; i < synthesis->state_bits; ++i) {
        fprintf(out, "    int q%d = state->q%d;\n", i + 1, i + 1);
    }
    fputc('\n', out);

    /* Выходы считаются до обновления состояния: функция выходов зависит от
       текущего состояния, а не от следующего. */
    for (i = 0; i < synthesis->output_bits; ++i) {
        fprintf(out, "    *y%d = ", i + 1);
        dnf_print_c(&synthesis->psi_dnf[i], &synthesis->psi[i], out);
        fprintf(out, ";\n");
    }
    fputc('\n', out);

    for (i = 0; i < synthesis->state_bits; ++i) {
        fprintf(out, "    state->q%d = ", i + 1);
        dnf_print_c(&synthesis->phi_dnf[i], &synthesis->phi[i], out);
        fprintf(out, ";\n");
    }
    fprintf(out, "}\n");
}
