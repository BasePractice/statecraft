#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "thompson.h"

/*
 * Разбор идёт рекурсивным спуском по грамматике
 *
 *     union       := concat ('|' concat)*
 *     concat      := repeat*
 *     repeat      := atom ('*' | '+' | '?')*
 *     atom        := символ | '(' union ')'
 *
 * и сразу строит автомат: каждому правилу соответствует одна конструкция
 * Томпсона. Промежуточного дерева нет — оно здесь не нужно, а его отсутствие
 * делает связь «правило грамматики — фрагмент автомата» буквальной.
 *
 * Фрагмент автомата — пара (вход, выход): у конструкции Томпсона ровно один
 * вход и ровно один выход, и это её главное свойство, позволяющее собирать
 * фрагменты друг из друга.
 */

struct Fragment {
    int in;
    int out;
};

struct Parser {
    const char *p;
    struct Nfa *nfa;
    bool failed;
};

static bool parse_union(struct Parser *parser, struct Fragment *result);

static void fail(struct Parser *parser, const char *message) {
    if (!parser->failed) {
        fprintf(stderr, "thompson: %s (позиция %ld)\n", message, (long)(parser->p - parser->p));
        parser->failed = true;
    }
}

static int new_state(struct Parser *parser) {
    int state = nfa_add_state(parser->nfa);
    if (state == FSM_NO_STATE)
        fail(parser, "автомат не помещается в предел состояний");
    return state;
}

static bool is_atom_start(char c) {
    return c != '\0' && c != '|' && c != ')' && c != '*' && c != '+' && c != '?';
}

/* atom := символ | '(' union ')' */
static bool parse_atom(struct Parser *parser, struct Fragment *result) {
    if (*parser->p == '(') {
        ++parser->p;
        if (!parse_union(parser, result))
            return false;
        if (*parser->p != ')') {
            fail(parser, "не хватает закрывающей скобки");
            return false;
        }
        ++parser->p;
        return true;
    }

    if (!is_atom_start(*parser->p)) {
        fail(parser, "ожидался символ или открывающая скобка");
        return false;
    }

    {
        char symbol = *parser->p++;
        int in = new_state(parser);
        int out = new_state(parser);
        if (parser->failed)
            return false;
        nfa_add_transition(parser->nfa, in, symbol, out);
        result->in = in;
        result->out = out;
    }
    return true;
}

/* repeat := atom ('*' | '+' | '?')* */
static bool parse_repeat(struct Parser *parser, struct Fragment *result) {
    if (!parse_atom(parser, result))
        return false;

    while (*parser->p == '*' || *parser->p == '+' || *parser->p == '?') {
        char op = *parser->p++;
        int in = new_state(parser);
        int out = new_state(parser);
        if (parser->failed)
            return false;

        nfa_add_epsilon(parser->nfa, in, result->in);
        nfa_add_epsilon(parser->nfa, result->out, out);
        if (op == '*' || op == '?')
            nfa_add_epsilon(parser->nfa, in, out); /* ноль вхождений */
        if (op == '*' || op == '+')
            nfa_add_epsilon(parser->nfa, result->out, result->in); /* повтор */

        result->in = in;
        result->out = out;
    }
    return true;
}

/* concat := repeat* */
static bool parse_concat(struct Parser *parser, struct Fragment *result) {
    struct Fragment right;

    if (!parse_repeat(parser, result))
        return false;

    while (is_atom_start(*parser->p) || *parser->p == '(') {
        if (!parse_repeat(parser, &right))
            return false;
        nfa_add_epsilon(parser->nfa, result->out, right.in);
        result->out = right.out;
    }
    return true;
}

/* union := concat ('|' concat)* */
static bool parse_union(struct Parser *parser, struct Fragment *result) {
    struct Fragment left;
    struct Fragment right;

    if (!parse_concat(parser, &left))
        return false;
    *result = left;

    while (*parser->p == '|') {
        int in;
        int out;

        ++parser->p;
        if (!parse_concat(parser, &right))
            return false;

        in = new_state(parser);
        out = new_state(parser);
        if (parser->failed)
            return false;

        nfa_add_epsilon(parser->nfa, in, result->in);
        nfa_add_epsilon(parser->nfa, in, right.in);
        nfa_add_epsilon(parser->nfa, result->out, out);
        nfa_add_epsilon(parser->nfa, right.out, out);

        result->in = in;
        result->out = out;
    }
    return true;
}

bool thompson_build(struct Nfa *nfa, const char *pattern) {
    struct Parser parser;
    struct Fragment fragment;

    assert(nfa != NULL && pattern != NULL);
    nfa_init(nfa);
    parser.p = pattern;
    parser.nfa = nfa;
    parser.failed = false;

    if (!parse_union(&parser, &fragment))
        return false;
    if (*parser.p != '\0') {
        fprintf(stderr, "thompson: лишний символ «%c» после выражения\n", *parser.p);
        return false;
    }

    nfa->start = fragment.in;
    nfa_set_final(nfa, fragment.out, true);
    return true;
}
