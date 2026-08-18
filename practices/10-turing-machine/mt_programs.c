#include <stdlib.h>
#include <string.h>
#include "mt_programs.h"

/* Таблицы ниже набраны так же, как они напечатаны в приложении лекции 10:
   строка --- символ под головкой, столбец --- состояние. Порядок строк в
   коде соответствует порядку столбцов таблицы, чтобы расхождение между
   лекцией и программой было видно при чтении, а не только при прогоне. */

struct Engine *mt_anbn_create(void) {
    struct Engine *engine = engine_create(5, 4, 'A');

    engine_symbol_add(engine, 'a');
    engine_symbol_add(engine, 'b');
    engine_symbol_add(engine, 'X');
    engine_symbol_add(engine, 'Y');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'A');
    engine_state_add(engine, 'B');
    engine_state_add(engine, 'C');
    engine_state_add(engine, 'D');

    /* A --- очередная непомеченная a слева. */
    engine_reference_add(engine, 'a', 'A', 'X', 'B', Right);
    engine_reference_add(engine, 'Y', 'A', 'Y', 'D', Right);
    engine_reference_add(engine, 'b', 'A', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, EMPTY_SYMBOL, 'A', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, 'X', 'A', MT_REJECT, STOP_STATE, Stay);

    /* B --- поиск самой левой b справа. */
    engine_reference_add(engine, 'a', 'B', 'a', 'B', Right);
    engine_reference_add(engine, 'Y', 'B', 'Y', 'B', Right);
    engine_reference_add(engine, 'b', 'B', 'Y', 'C', Left);
    engine_reference_add(engine, EMPTY_SYMBOL, 'B', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, 'X', 'B', MT_REJECT, STOP_STATE, Stay);

    /* C --- возврат влево к границе разметки. */
    engine_reference_add(engine, 'a', 'C', 'a', 'C', Left);
    engine_reference_add(engine, 'Y', 'C', 'Y', 'C', Left);
    engine_reference_add(engine, 'X', 'C', 'X', 'A', Right);
    engine_reference_add(engine, 'b', 'C', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, EMPTY_SYMBOL, 'C', MT_REJECT, STOP_STATE, Stay);

    /* D --- хвост слова: до конца должны идти только Y. */
    engine_reference_add(engine, 'Y', 'D', 'Y', 'D', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'D', MT_ACCEPT, STOP_STATE, Stay);
    engine_reference_add(engine, 'a', 'D', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, 'b', 'D', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, 'X', 'D', MT_REJECT, STOP_STATE, Stay);

    return engine;
}

struct Engine *mt_palindrome_create(void) {
    struct Engine *engine = engine_create(3, 6, 'S');

    engine_symbol_add(engine, 'a');
    engine_symbol_add(engine, 'b');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'S');
    engine_state_add(engine, 'P');
    engine_state_add(engine, 'Q');
    engine_state_add(engine, 'R');
    engine_state_add(engine, 'T');
    engine_state_add(engine, 'K');

    /* S --- крайний левый символ: стереть и запомнить состоянием. */
    engine_reference_add(engine, 'a', 'S', EMPTY_SYMBOL, 'P', Right);
    engine_reference_add(engine, 'b', 'S', EMPTY_SYMBOL, 'Q', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'S', MT_ACCEPT, STOP_STATE, Stay);

    /* P, Q --- ход вправо до конца слова; различаются тем, что помнят. */
    engine_reference_add(engine, 'a', 'P', 'a', 'P', Right);
    engine_reference_add(engine, 'b', 'P', 'b', 'P', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'P', EMPTY_SYMBOL, 'R', Left);

    engine_reference_add(engine, 'a', 'Q', 'a', 'Q', Right);
    engine_reference_add(engine, 'b', 'Q', 'b', 'Q', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'Q', EMPTY_SYMBOL, 'T', Left);

    /* R, T --- сверка крайнего правого символа с запомненным. */
    engine_reference_add(engine, 'a', 'R', EMPTY_SYMBOL, 'K', Left);
    engine_reference_add(engine, 'b', 'R', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, EMPTY_SYMBOL, 'R', MT_ACCEPT, STOP_STATE, Stay);

    engine_reference_add(engine, 'b', 'T', EMPTY_SYMBOL, 'K', Left);
    engine_reference_add(engine, 'a', 'T', MT_REJECT, STOP_STATE, Stay);
    engine_reference_add(engine, EMPTY_SYMBOL, 'T', MT_ACCEPT, STOP_STATE, Stay);

    /* K --- возврат влево к остатку слова. */
    engine_reference_add(engine, 'a', 'K', 'a', 'K', Left);
    engine_reference_add(engine, 'b', 'K', 'b', 'K', Left);
    engine_reference_add(engine, EMPTY_SYMBOL, 'K', EMPTY_SYMBOL, 'S', Right);

    return engine;
}

struct Engine *mt_copy_create(void) {
    struct Engine *engine = engine_create(7, 6, 'S');

    engine_symbol_add(engine, 'a');
    engine_symbol_add(engine, 'b');
    engine_symbol_add(engine, 'X');
    engine_symbol_add(engine, 'Y');
    engine_symbol_add(engine, 'c');
    engine_symbol_add(engine, 'd');
    engine_symbol_add(engine, EMPTY_SYMBOL);

    engine_state_add(engine, 'S');
    engine_state_add(engine, 'P');
    engine_state_add(engine, 'Q');
    engine_state_add(engine, 'K');
    engine_state_add(engine, 'R');
    engine_state_add(engine, 'L');

    /* S --- пометить очередной символ оригинала. */
    engine_reference_add(engine, 'a', 'S', 'X', 'P', Right);
    engine_reference_add(engine, 'b', 'S', 'Y', 'Q', Right);
    engine_reference_add(engine, 'X', 'S', 'X', 'S', Right);
    engine_reference_add(engine, 'Y', 'S', 'Y', 'S', Right);
    engine_reference_add(engine, 'c', 'S', 'c', 'R', Stay);
    engine_reference_add(engine, 'd', 'S', 'd', 'R', Stay);
    engine_reference_add(engine, EMPTY_SYMBOL, 'S', EMPTY_SYMBOL, STOP_STATE, Stay);

    /* P, Q --- перенос символа в конец: состояние и есть та самая память,
       в которой символ едет по ленте. */
    engine_reference_add(engine, 'a', 'P', 'a', 'P', Right);
    engine_reference_add(engine, 'b', 'P', 'b', 'P', Right);
    engine_reference_add(engine, 'X', 'P', 'X', 'P', Right);
    engine_reference_add(engine, 'Y', 'P', 'Y', 'P', Right);
    engine_reference_add(engine, 'c', 'P', 'c', 'P', Right);
    engine_reference_add(engine, 'd', 'P', 'd', 'P', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'P', 'c', 'K', Left);

    engine_reference_add(engine, 'a', 'Q', 'a', 'Q', Right);
    engine_reference_add(engine, 'b', 'Q', 'b', 'Q', Right);
    engine_reference_add(engine, 'X', 'Q', 'X', 'Q', Right);
    engine_reference_add(engine, 'Y', 'Q', 'Y', 'Q', Right);
    engine_reference_add(engine, 'c', 'Q', 'c', 'Q', Right);
    engine_reference_add(engine, 'd', 'Q', 'd', 'Q', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'Q', 'd', 'K', Left);

    /* K --- возврат к границе разметки. */
    engine_reference_add(engine, 'a', 'K', 'a', 'K', Left);
    engine_reference_add(engine, 'b', 'K', 'b', 'K', Left);
    engine_reference_add(engine, 'c', 'K', 'c', 'K', Left);
    engine_reference_add(engine, 'd', 'K', 'd', 'K', Left);
    engine_reference_add(engine, 'X', 'K', 'X', 'S', Right);
    engine_reference_add(engine, 'Y', 'K', 'Y', 'S', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'K', EMPTY_SYMBOL, 'S', Right);

    /* R --- снятие разметки с копии, слева направо. */
    engine_reference_add(engine, 'c', 'R', 'a', 'R', Right);
    engine_reference_add(engine, 'd', 'R', 'b', 'R', Right);
    engine_reference_add(engine, 'a', 'R', 'a', 'R', Right);
    engine_reference_add(engine, 'b', 'R', 'b', 'R', Right);
    engine_reference_add(engine, EMPTY_SYMBOL, 'R', EMPTY_SYMBOL, 'L', Left);

    /* L --- снятие разметки с оригинала, справа налево. */
    engine_reference_add(engine, 'a', 'L', 'a', 'L', Left);
    engine_reference_add(engine, 'b', 'L', 'b', 'L', Left);
    engine_reference_add(engine, 'X', 'L', 'a', 'L', Left);
    engine_reference_add(engine, 'Y', 'L', 'b', 'L', Left);
    engine_reference_add(engine, EMPTY_SYMBOL, 'L', EMPTY_SYMBOL, STOP_STATE, Stay);

    return engine;
}

/* Кладёт слово на ленту и прогоняет машину до остановки. */
static struct Engine *mt_run(MtMachineBuilder build, const char *word) {
    struct Engine *engine = build();

    if (strlen(word) > 0)
        engine_tape_copy(engine, MT_TAPE_OFFSET, word);
    engine_offset_set(engine, MT_TAPE_OFFSET);
    machine(engine);
    return engine;
}

int mt_recognize(MtMachineBuilder build, const char *word) {
    struct Engine *engine = mt_run(build, word);
    int verdict = -1;
    int i;

    for (i = 0; i < TAPE_LIMIT; ++i) {
        char ch = *engine_type_get(engine, i);
        if (ch == MT_ACCEPT)
            verdict = 1;
        else if (ch == MT_REJECT)
            verdict = 0;
    }
    engine_destroy(&engine);
    return verdict;
}

const char *mt_transform(MtMachineBuilder build, const char *word, char *out, int size) {
    struct Engine *engine = mt_run(build, word);
    int length = 0;
    int i;

    for (i = 0; i < TAPE_LIMIT; ++i) {
        char ch = *engine_type_get(engine, i);
        if (ch == EMPTY_SYMBOL)
            continue;
        if (length + 1 >= size) {
            engine_destroy(&engine);
            return NULL;
        }
        out[length++] = ch;
    }
    out[length] = '\0';
    engine_destroy(&engine);
    return out;
}
