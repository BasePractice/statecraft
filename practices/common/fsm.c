#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "fsm.h"

void nfa_init(struct Nfa *nfa) {
    assert(nfa != NULL);
    memset(nfa, 0, sizeof(*nfa));
    nfa->start = FSM_NO_STATE;
}

int nfa_add_state(struct Nfa *nfa) {
    assert(nfa != NULL);
    if (nfa->state_count >= FSM_MAX_STATES)
        return FSM_NO_STATE;
    return nfa->state_count++;
}

int nfa_symbol_index(struct Nfa *nfa, char symbol) {
    int i;

    assert(nfa != NULL);
    for (i = 0; i < nfa->symbol_count; ++i) {
        if (nfa->symbols[i] == symbol)
            return i;
    }
    if (nfa->symbol_count >= FSM_MAX_SYMBOLS)
        return FSM_NO_STATE;
    nfa->symbols[nfa->symbol_count] = symbol;
    return nfa->symbol_count++;
}

void nfa_add_transition(struct Nfa *nfa, int from, char symbol, int to) {
    int index = nfa_symbol_index(nfa, symbol);

    assert(nfa != NULL);
    assert(from >= 0 && from < nfa->state_count);
    assert(to >= 0 && to < nfa->state_count);
    assert(index != FSM_NO_STATE);
    nfa->trans[from][index][to] = 1;
}

void nfa_add_epsilon(struct Nfa *nfa, int from, int to) {
    assert(nfa != NULL);
    assert(from >= 0 && from < nfa->state_count);
    assert(to >= 0 && to < nfa->state_count);
    nfa->eps[from][to] = 1;
}

void nfa_set_final(struct Nfa *nfa, int state, bool value) {
    assert(nfa != NULL);
    assert(state >= 0 && state < nfa->state_count);
    nfa->final[state] = (char)(value ? 1 : 0);
}

void nfa_epsilon_closure(const struct Nfa *nfa, char *set) {
    int changed = 1;
    int from;
    int to;

    assert(nfa != NULL && set != NULL);
    /* Замыкание строится до неподвижной точки: пока на очередном проходе
       появляется хотя бы одно новое состояние. */
    while (changed) {
        changed = 0;
        for (from = 0; from < nfa->state_count; ++from) {
            if (!set[from])
                continue;
            for (to = 0; to < nfa->state_count; ++to) {
                if (nfa->eps[from][to] && !set[to]) {
                    set[to] = 1;
                    changed = 1;
                }
            }
        }
    }
}

bool nfa_accepts(const struct Nfa *nfa, const char *word) {
    char current[FSM_MAX_STATES];
    char next[FSM_MAX_STATES];
    const char *p;
    int from;
    int to;
    int index;
    int i;

    assert(nfa != NULL && word != NULL);
    if (nfa->start == FSM_NO_STATE)
        return false;

    memset(current, 0, sizeof(current));
    current[nfa->start] = 1;
    nfa_epsilon_closure(nfa, current);

    for (p = word; *p != '\0'; ++p) {
        index = FSM_NO_STATE;
        for (i = 0; i < nfa->symbol_count; ++i) {
            if (nfa->symbols[i] == *p)
                index = i;
        }
        if (index == FSM_NO_STATE)
            return false; /* символа нет во входном алфавите */

        memset(next, 0, sizeof(next));
        for (from = 0; from < nfa->state_count; ++from) {
            if (!current[from])
                continue;
            for (to = 0; to < nfa->state_count; ++to) {
                if (nfa->trans[from][index][to])
                    next[to] = 1;
            }
        }
        nfa_epsilon_closure(nfa, next);
        memcpy(current, next, sizeof(current));
    }

    for (i = 0; i < nfa->state_count; ++i) {
        if (current[i] && nfa->final[i])
            return true;
    }
    return false;
}

void nfa_print(const struct Nfa *nfa, FILE *out) {
    int from;
    int to;
    int s;

    assert(nfa != NULL && out != NULL);
    fprintf(out, "states %d\n", nfa->state_count);
    fprintf(out, "alphabet ");
    for (s = 0; s < nfa->symbol_count; ++s) {
        fputc(nfa->symbols[s], out);
    }
    fputc('\n', out);
    fprintf(out, "start %d\n", nfa->start);
    for (from = 0; from < nfa->state_count; ++from) {
        if (nfa->final[from])
            fprintf(out, "final %d\n", from);
    }
    for (from = 0; from < nfa->state_count; ++from) {
        for (to = 0; to < nfa->state_count; ++to) {
            if (nfa->eps[from][to])
                fprintf(out, "%d eps %d\n", from, to);
        }
        for (s = 0; s < nfa->symbol_count; ++s) {
            for (to = 0; to < nfa->state_count; ++to) {
                if (nfa->trans[from][s][to])
                    fprintf(out, "%d %c %d\n", from, nfa->symbols[s], to);
            }
        }
    }
}

void nfa_print_dot(const struct Nfa *nfa, FILE *out) {
    int from;
    int to;
    int s;

    assert(nfa != NULL && out != NULL);
    fprintf(out, "digraph nfa {\n");
    fprintf(out, "    rankdir=LR;\n");
    fprintf(out, "    node [shape=circle];\n");
    for (from = 0; from < nfa->state_count; ++from) {
        if (nfa->final[from])
            fprintf(out, "    %d [shape=doublecircle];\n", from);
    }
    fprintf(out, "    start [shape=point];\n");
    fprintf(out, "    start -> %d;\n", nfa->start);
    for (from = 0; from < nfa->state_count; ++from) {
        for (to = 0; to < nfa->state_count; ++to) {
            if (nfa->eps[from][to])
                fprintf(out, "    %d -> %d [label=\"eps\"];\n", from, to);
        }
        for (s = 0; s < nfa->symbol_count; ++s) {
            for (to = 0; to < nfa->state_count; ++to) {
                if (nfa->trans[from][s][to])
                    fprintf(out, "    %d -> %d [label=\"%c\"];\n", from, to, nfa->symbols[s]);
            }
        }
    }
    fprintf(out, "}\n");
}

bool nfa_read(struct Nfa *nfa, FILE *in) {
    char line[256];
    char word[64];
    int from;
    int to;
    int count;

    assert(nfa != NULL && in != NULL);
    nfa_init(nfa);

    while (fgets(line, (int)sizeof(line), in) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        if (sscanf(line, "states %d", &count) == 1) {
            if (count <= 0 || count > FSM_MAX_STATES) {
                fprintf(stderr, "fsm: число состояний вне диапазона 1..%d\n", FSM_MAX_STATES);
                return false;
            }
            nfa->state_count = count;
            continue;
        }
        if (sscanf(line, "alphabet %63s", word) == 1) {
            size_t i;
            for (i = 0; i < strlen(word); ++i) {
                if (nfa_symbol_index(nfa, word[i]) == FSM_NO_STATE) {
                    fprintf(stderr, "fsm: алфавит длиннее %d символов\n", FSM_MAX_SYMBOLS);
                    return false;
                }
            }
            continue;
        }
        if (sscanf(line, "start %d", &from) == 1) {
            nfa->start = from;
            continue;
        }
        if (sscanf(line, "final %d", &from) == 1) {
            if (from < 0 || from >= nfa->state_count) {
                fprintf(stderr, "fsm: заключительное состояние %d вне диапазона\n", from);
                return false;
            }
            nfa->final[from] = 1;
            continue;
        }
        if (sscanf(line, "%d %63s %d", &from, word, &to) == 3) {
            if (from < 0 || from >= nfa->state_count || to < 0 || to >= nfa->state_count) {
                fprintf(stderr, "fsm: переход %d -> %d вне диапазона состояний\n", from, to);
                return false;
            }
            if (strcmp(word, "eps") == 0) {
                nfa_add_epsilon(nfa, from, to);
            } else if (strlen(word) == 1) {
                nfa_add_transition(nfa, from, word[0], to);
            } else {
                fprintf(stderr, "fsm: метка перехода «%s» — не символ и не eps\n", word);
                return false;
            }
            continue;
        }
        fprintf(stderr, "fsm: непонятная строка: %s", line);
        return false;
    }

    if (nfa->state_count == 0) {
        fprintf(stderr, "fsm: не задано число состояний\n");
        return false;
    }
    if (nfa->start < 0 || nfa->start >= nfa->state_count) {
        fprintf(stderr, "fsm: начальное состояние не задано или вне диапазона\n");
        return false;
    }
    return true;
}

void dfa_init(struct Dfa *dfa) {
    int i;
    int s;

    assert(dfa != NULL);
    memset(dfa, 0, sizeof(*dfa));
    dfa->start = FSM_NO_STATE;
    for (i = 0; i < FSM_MAX_DFA_STATES; ++i) {
        for (s = 0; s < FSM_MAX_SYMBOLS; ++s) {
            dfa->trans[i][s] = FSM_NO_STATE;
        }
    }
}

bool dfa_accepts(const struct Dfa *dfa, const char *word) {
    int state;
    const char *p;
    int i;

    assert(dfa != NULL && word != NULL);
    if (dfa->start == FSM_NO_STATE)
        return false;

    state = dfa->start;
    for (p = word; *p != '\0'; ++p) {
        int index = FSM_NO_STATE;
        for (i = 0; i < dfa->symbol_count; ++i) {
            if (dfa->symbols[i] == *p)
                index = i;
        }
        if (index == FSM_NO_STATE)
            return false;
        state = dfa->trans[state][index];
        if (state == FSM_NO_STATE)
            return false; /* автомат не полный: перехода нет */
    }
    return dfa->final[state] ? true : false;
}

void dfa_print(const struct Dfa *dfa, FILE *out) {
    int i;
    int s;

    assert(dfa != NULL && out != NULL);
    fprintf(out, "states %d\n", dfa->state_count);
    fprintf(out, "alphabet ");
    for (s = 0; s < dfa->symbol_count; ++s) {
        fputc(dfa->symbols[s], out);
    }
    fputc('\n', out);
    fprintf(out, "start %d\n", dfa->start);
    for (i = 0; i < dfa->state_count; ++i) {
        if (dfa->final[i])
            fprintf(out, "final %d\n", i);
    }
    for (i = 0; i < dfa->state_count; ++i) {
        for (s = 0; s < dfa->symbol_count; ++s) {
            if (dfa->trans[i][s] != FSM_NO_STATE)
                fprintf(out, "%d %c %d\n", i, dfa->symbols[s], dfa->trans[i][s]);
        }
    }
}

void dfa_print_dot(const struct Dfa *dfa, FILE *out) {
    int i;
    int s;

    assert(dfa != NULL && out != NULL);
    fprintf(out, "digraph dfa {\n");
    fprintf(out, "    rankdir=LR;\n");
    fprintf(out, "    node [shape=circle];\n");
    for (i = 0; i < dfa->state_count; ++i) {
        if (dfa->final[i])
            fprintf(out, "    %d [shape=doublecircle];\n", i);
    }
    fprintf(out, "    start [shape=point];\n");
    fprintf(out, "    start -> %d;\n", dfa->start);
    for (i = 0; i < dfa->state_count; ++i) {
        for (s = 0; s < dfa->symbol_count; ++s) {
            if (dfa->trans[i][s] != FSM_NO_STATE)
                fprintf(out, "    %d -> %d [label=\"%c\"];\n", i, dfa->trans[i][s],
                        dfa->symbols[s]);
        }
    }
    fprintf(out, "}\n");
}
