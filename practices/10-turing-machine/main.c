#include <stdlib.h>
#include <stdio.h>
#include "turing_machine.h"

#define TAPE_TEXT "110010"

static void print_type(struct Engine *engine) {
    int i;

    for (i = 0; i < TAPE_LIMIT; ++i) {
        int ch = *(engine_type_get(engine, i));
        if (ch == EMPTY_SYMBOL)
            continue;
        fprintf(stdout, "%c", (char)ch);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

/**
 * Машина инвертирует биты входного слова: пример из лекции 10.
 * Программа задаётся таблицей переходов, состояние STOP_STATE — заключительное.
 */
int main(void) {
    struct Engine *engine;
    int offset;

    engine = engine_create(3, 2, 'A');

    engine_symbol_add(engine, '0');
    engine_symbol_add(engine, '1');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'A');
    engine_state_add(engine, 'B');

    engine_reference_add(engine, '0', 'A', '1', 'B', Right);
    engine_reference_add(engine, '1', 'A', '0', 'B', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'A', EMPTY_SYMBOL, 'A', Right);

    engine_reference_add(engine, '0', 'B', '1', 'B', Right);
    engine_reference_add(engine, '1', 'B', '0', 'B', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'B', EMPTY_SYMBOL, STOP_STATE, Right);

    offset = (TAPE_LIMIT / 2) + 2;
    engine_tape_copy(engine, offset, TAPE_TEXT);
    print_type(engine);
    machine(engine);
    print_type(engine);
    engine_destroy(&engine);

    return EXIT_SUCCESS;
}
