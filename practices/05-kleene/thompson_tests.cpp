#include <catch2/catch.hpp>
#include <thompson.h>

/*
 * Проверяется не форма автомата, а язык, который он распознаёт: конструкций
 * Томпсона для одного выражения много, а язык у них один.
 */

static bool accepts(const char *pattern, const char *word) {
    struct Nfa nfa;
    REQUIRE(thompson_build(&nfa, pattern));
    return nfa_accepts(&nfa, word);
}

TEST_CASE("Один символ", "[05.Kleene]") {
    REQUIRE(accepts("a", "a"));
    REQUIRE_FALSE(accepts("a", "b"));
    REQUIRE_FALSE(accepts("a", ""));
    REQUIRE_FALSE(accepts("a", "aa"));
}

TEST_CASE("Конкатенация", "[05.Kleene]") {
    REQUIRE(accepts("abb", "abb"));
    REQUIRE_FALSE(accepts("abb", "ab"));
    REQUIRE_FALSE(accepts("abb", "abbb"));
}

TEST_CASE("Объединение", "[05.Kleene]") {
    REQUIRE(accepts("a|b", "a"));
    REQUIRE(accepts("a|b", "b"));
    REQUIRE_FALSE(accepts("a|b", "ab"));
}

TEST_CASE("Итерация Клини", "[05.Kleene]") {
    REQUIRE(accepts("a*", ""));
    REQUIRE(accepts("a*", "aaaa"));
    REQUIRE_FALSE(accepts("a*", "aab"));

    REQUIRE_FALSE(accepts("a+", ""));
    REQUIRE(accepts("a+", "a"));
    REQUIRE(accepts("a+", "aaa"));

    REQUIRE(accepts("a?", ""));
    REQUIRE(accepts("a?", "a"));
    REQUIRE_FALSE(accepts("a?", "aa"));
}

TEST_CASE("Сквозной пример курса: (a|b)*abb", "[05.Kleene]") {
    REQUIRE(accepts("(a|b)*abb", "abb"));
    REQUIRE(accepts("(a|b)*abb", "aabb"));
    REQUIRE(accepts("(a|b)*abb", "babaabb"));
    REQUIRE_FALSE(accepts("(a|b)*abb", ""));
    REQUIRE_FALSE(accepts("(a|b)*abb", "ab"));
    REQUIRE_FALSE(accepts("(a|b)*abb", "abba"));
}

TEST_CASE("Приоритет операций", "[05.Kleene]") {
    /* ab|c читается как (ab)|c, а не a(b|c) */
    REQUIRE(accepts("ab|c", "ab"));
    REQUIRE(accepts("ab|c", "c"));
    REQUIRE_FALSE(accepts("ab|c", "ac"));

    /* ab* читается как a(b*), а не (ab)* */
    REQUIRE(accepts("ab*", "a"));
    REQUIRE(accepts("ab*", "abbb"));
    REQUIRE_FALSE(accepts("ab*", "abab"));
}

TEST_CASE("Ошибки разбора", "[05.Kleene]") {
    struct Nfa nfa;
    REQUIRE_FALSE(thompson_build(&nfa, "(a"));
    REQUIRE_FALSE(thompson_build(&nfa, "a)"));
    REQUIRE_FALSE(thompson_build(&nfa, "*a"));
    REQUIRE_FALSE(thompson_build(&nfa, ""));
}

TEST_CASE("Размер автомата растёт линейно", "[05.Kleene]") {
    struct Nfa small;
    struct Nfa large;

    REQUIRE(thompson_build(&small, "abb"));
    REQUIRE(thompson_build(&large, "abbabb"));
    /* Конструкция Томпсона даёт не более 2 состояний на символ выражения —
       это и есть её ценность по сравнению с прямым построением ДКА. */
    REQUIRE(large.state_count == 2 * small.state_count);
}
