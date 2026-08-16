#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "turing_machine.h"

struct Reference {
    char symbol;
    char state;
    enum Direct direct;
};

struct CharList {
    char *list;
    int size;
};

struct Engine {
    struct CharList symbols;
    struct CharList states;
    struct Reference **references;
    char tape[TAPE_LIMIT];
    char state;
    int current_i;
    int state_c;
    int symbol_c;
    int symbol_n; /* размер таблицы references: нужен при освобождении */
};

static void char_list_new(struct CharList *list, int size) {
    assert(list != NULL);
    list->size = size;
    /* Массив символов: элемент — char, а не указатель на char. */
    list->list = (char *)malloc((size_t)size * sizeof(char));
    assert(list->list != NULL);
}

static void char_list_set(struct CharList *list, int i, char value) {
    assert(list != NULL);
    assert(list->list != NULL);
    assert(list->size > i && i >= 0);
    list->list[i] = value;
}

/* Возвращает индекс символа в алфавите или -1, если символа там нет.
   Раньше функция в этом случае не возвращала ничего: assert(0) исчезает
   при NDEBUG, и сборка Release получала неопределённое поведение. */
static int char_list_find(struct CharList *list, char ch) {
    int i;
    for (i = 0; i < list->size; ++i) {
        if (list->list[i] == ch)
            return i;
    }
    return -1;
}

struct Engine *engine_create(int symbols, int states, char init_state) {
    int i;
    struct Engine *engine;

    assert(symbols > 0 && states > 0);
    engine = (struct Engine *)calloc(1, sizeof(struct Engine));
    assert(engine != NULL);
    engine->symbol_n = symbols;
    engine->references = (struct Reference **)calloc((size_t)symbols, sizeof(struct Reference *));
    assert(engine->references != NULL);
    for (i = 0; i < symbols; ++i) {
        engine->references[i]
                = (struct Reference *)calloc((size_t)states, sizeof(struct Reference));
        assert(engine->references[i] != NULL);
    }
    engine->state = init_state;
    engine->current_i = TAPE_LIMIT / 2;
    char_list_new(&engine->states, states);
    char_list_new(&engine->symbols, symbols);
    memset(engine->tape, EMPTY_SYMBOL, sizeof(engine->tape));
    return engine;
}

void engine_symbol_add(struct Engine *engine, char symbol) {
    char_list_set(&engine->symbols, engine->symbol_c++, symbol);
}

void engine_state_add(struct Engine *engine, char state) {
    char_list_set(&engine->states, engine->state_c++, state);
}

void engine_reference_add(struct Engine *engine, char c_symbol, char c_state, char symbol,
                          char state, enum Direct direct) {
    int i_symbol = char_list_find(&engine->symbols, c_symbol);
    int i_state = char_list_find(&engine->states, c_state);

    /* Команда для символа или состояния, не объявленных заранее, — ошибка
       описания машины, а не молчаливая запись мимо таблицы. */
    assert(i_symbol >= 0 && i_state >= 0);
    if (i_symbol < 0 || i_state < 0)
        return;

    engine->references[i_symbol][i_state].symbol = symbol;
    engine->references[i_symbol][i_state].state = state;
    engine->references[i_symbol][i_state].direct = direct;
}

void engine_tape_copy(struct Engine *engine, int offset, const char *tape) {
    size_t length = strlen(tape);
    assert(offset >= 0 && offset + (int)length <= TAPE_LIMIT);
    memcpy(engine->tape + offset, tape, length);
}

void engine_tape_set(struct Engine *engine, int offset, const char character) {
    engine->tape[offset] = character;
}

char *engine_type_get(struct Engine *engine, int offset) {
    return engine->tape + offset;
}

void engine_offset_set(struct Engine *engine, int offset) {
    assert(offset >= 0 && offset < TAPE_LIMIT);
    engine->current_i = offset;
}

void engine_destroy(struct Engine **engine) {
    int i;

    if (engine != NULL && (*engine) != NULL) {
        /* Освобождаем всё, что выделил engine_create: раньше здесь
           терялись таблица переходов и оба алфавита. */
        for (i = 0; i < (*engine)->symbol_n; ++i) {
            free((*engine)->references[i]);
        }
        free((*engine)->references);
        free((*engine)->symbols.list);
        free((*engine)->states.list);
        free((*engine));
        (*engine) = NULL;
    }
}

void machine(struct Engine *engine) {
    while (1) {
        int i = engine->current_i;
        char tape_symbol;
        int i_symbol;
        int i_state;
        struct Reference *ref;

        /* Лента реализации конечна. Уход за её край — не «бесконечная
           лента», а выход за границы массива, поэтому машина
           останавливается. */
        if (i < 0 || i >= TAPE_LIMIT)
            break;

        tape_symbol = engine->tape[i];
        i_symbol = char_list_find(&engine->symbols, tape_symbol);
        i_state = char_list_find(&engine->states, engine->state);
        /* Символа нет в ленточном алфавите или состояния нет в множестве
           состояний: программа машины задана не полностью. */
        if (i_symbol < 0 || i_state < 0)
            break;

        ref = &(engine->references[i_symbol][i_state]);
        engine->tape[i] = ref->symbol;
        if (ref->state == STOP_STATE)
            break;
        engine->state = ref->state;
        if (ref->direct == Left) {
            engine->current_i--;
        } else if (ref->direct == Right) {
            engine->current_i++;
        } else {
            /* Stay: головка остаётся на месте */
        }
    }
}
