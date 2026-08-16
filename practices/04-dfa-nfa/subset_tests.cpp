#include <cstdio>
#include <cstring>
#include <catch2/catch.hpp>
#include <subset.h>

/*
 * Проверяется главное свойство обоих алгоритмов: язык не меняется.
 * Сравнение автоматов «по форме» бессмысленно — у одного языка много
 * представлений, а минимальный ДКА единствен лишь с точностью до
 * переименования состояний.
 */

namespace {

/* НКА для (a|b)*abb, собранный вручную: сквозной пример лекций 4, 5 и 6. */
void build_abb(struct Nfa *nfa) {
    int i;

    nfa_init(nfa);
    for (i = 0; i < 4; ++i) {
        REQUIRE(nfa_add_state(nfa) == i);
    }
    nfa->start = 0;
    nfa_set_final(nfa, 3, true);

    nfa_add_transition(nfa, 0, 'a', 0);
    nfa_add_transition(nfa, 0, 'b', 0);
    nfa_add_transition(nfa, 0, 'a', 1);
    nfa_add_transition(nfa, 1, 'b', 2);
    nfa_add_transition(nfa, 2, 'b', 3);
}

/* НКА, у которого n-й символ с конца равен 'a': классический пример
   экспоненциального взрыва при детерминизации. */
void build_nth_from_end(struct Nfa *nfa, int n) {
    int i;

    nfa_init(nfa);
    for (i = 0; i <= n; ++i) {
        REQUIRE(nfa_add_state(nfa) == i);
    }
    nfa->start = 0;
    nfa_set_final(nfa, n, true);

    nfa_add_transition(nfa, 0, 'a', 0);
    nfa_add_transition(nfa, 0, 'b', 0);
    nfa_add_transition(nfa, 0, 'a', 1);
    for (i = 1; i < n; ++i) {
        nfa_add_transition(nfa, i, 'a', i + 1);
        nfa_add_transition(nfa, i, 'b', i + 1);
    }
}

} /* namespace */

TEST_CASE("Детерминизация сохраняет язык", "[04.DFA_NFA]") {
    struct Nfa nfa;
    struct Dfa dfa;

    build_abb(&nfa);
    REQUIRE(subset_construction(&dfa, &nfa));

    const char *accepted[] = {"abb", "aabb", "babaabb", "bbabb"};
    const char *rejected[] = {"", "ab", "abba", "ba", "abbb"};
    size_t i;

    for (i = 0; i < sizeof(accepted) / sizeof(accepted[0]); ++i) {
        INFO("слово: " << accepted[i]);
        REQUIRE(nfa_accepts(&nfa, accepted[i]));
        REQUIRE(dfa_accepts(&dfa, accepted[i]));
    }
    for (i = 0; i < sizeof(rejected) / sizeof(rejected[0]); ++i) {
        INFO("слово: " << rejected[i]);
        REQUIRE_FALSE(nfa_accepts(&nfa, rejected[i]));
        REQUIRE_FALSE(dfa_accepts(&dfa, rejected[i]));
    }
}

TEST_CASE("Минимизация сохраняет язык и не увеличивает автомат", "[04.DFA_NFA]") {
    struct Nfa nfa;
    struct Dfa dfa;
    struct Dfa minimal;

    build_abb(&nfa);
    REQUIRE(subset_construction(&dfa, &nfa));
    REQUIRE(dfa_minimize(&minimal, &dfa));

    REQUIRE(minimal.state_count <= dfa.state_count);
    /* Минимальный ДКА для (a|b)*abb имеет ровно четыре состояния:
       «ничего», «видели a», «видели ab», «видели abb». */
    REQUIRE(minimal.state_count == 4);

    const char *words[] = {"", "a", "ab", "abb", "abba", "aabb", "babaabb", "bb", "abbb"};
    size_t i;
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        INFO("слово: " << words[i]);
        REQUIRE(dfa_accepts(&minimal, words[i]) == dfa_accepts(&dfa, words[i]));
    }
}

TEST_CASE("Минимизация склеивает эквивалентные состояния", "[04.DFA_NFA]") {
    struct Dfa dfa;
    struct Dfa minimal;

    /* Автомат для языка «слова из одной буквы a», записанный с лишним
       состоянием: 1 и 2 неотличимы. */
    dfa_init(&dfa);
    dfa.state_count = 3;
    dfa.symbol_count = 1;
    dfa.symbols[0] = 'a';
    dfa.start = 0;
    dfa.final[1] = 1;
    dfa.final[2] = 1;
    dfa.trans[0][0] = 1;
    dfa.trans[1][0] = 2;
    dfa.trans[2][0] = 2;

    REQUIRE(dfa_minimize(&minimal, &dfa));
    REQUIRE(minimal.state_count == 2);
    REQUIRE(dfa_accepts(&minimal, "a"));
    REQUIRE(dfa_accepts(&minimal, "aa"));
    REQUIRE_FALSE(dfa_accepts(&minimal, ""));
}

TEST_CASE("Недостижимые состояния удаляются", "[04.DFA_NFA]") {
    struct Dfa dfa;
    struct Dfa minimal;

    dfa_init(&dfa);
    dfa.state_count = 3;
    dfa.symbol_count = 1;
    dfa.symbols[0] = 'a';
    dfa.start = 0;
    dfa.final[1] = 1;
    dfa.trans[0][0] = 1;
    dfa.trans[2][0] = 1; /* состояние 2 недостижимо */

    REQUIRE(dfa_minimize(&minimal, &dfa));
    REQUIRE(minimal.state_count == 2);
}

TEST_CASE("Экспоненциальный взрыв при детерминизации", "[04.DFA_NFA]") {
    struct Nfa nfa;
    struct Dfa dfa;
    struct Dfa minimal;

    /* «Четвёртый символ с конца равен a»: НКА из 5 состояний,
       минимальный ДКА — 2^4 = 16. */
    build_nth_from_end(&nfa, 4);
    REQUIRE(nfa.state_count == 5);
    REQUIRE(subset_construction(&dfa, &nfa));
    REQUIRE(dfa_minimize(&minimal, &dfa));
    REQUIRE(minimal.state_count == 16);

    REQUIRE(dfa_accepts(&minimal, "abbb"));
    REQUIRE(dfa_accepts(&minimal, "bbabbb"));
    REQUIRE_FALSE(dfa_accepts(&minimal, "bbbb"));
    REQUIRE_FALSE(dfa_accepts(&minimal, "abb"));
}

TEST_CASE("Чтение текстового формата", "[04.DFA_NFA]") {
    const char *text = "# автомат для a(b|c)\n"
                       "states 3\n"
                       "alphabet abc\n"
                       "start 0\n"
                       "final 2\n"
                       "0 a 1\n"
                       "1 b 2\n"
                       "1 c 2\n";
    FILE *file = tmpfile();
    struct Nfa nfa;

    REQUIRE(file != NULL);
    fputs(text, file);
    rewind(file);

    REQUIRE(nfa_read(&nfa, file));
    fclose(file);

    REQUIRE(nfa.state_count == 3);
    REQUIRE(nfa.symbol_count == 3);
    REQUIRE(nfa_accepts(&nfa, "ab"));
    REQUIRE(nfa_accepts(&nfa, "ac"));
    REQUIRE_FALSE(nfa_accepts(&nfa, "a"));
    REQUIRE_FALSE(nfa_accepts(&nfa, "abc"));
}
