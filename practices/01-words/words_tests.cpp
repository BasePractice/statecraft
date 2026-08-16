#include <cstring>
#include <catch2/catch.hpp>
#include <words.h>

TEST_CASE("Длина и пустое слово", "[01.Words]") {
    REQUIRE(word_length("") == 0);
    REQUIRE(word_length("abc") == 3);
}

TEST_CASE("Конкатенация", "[01.Words]") {
    char buffer[WORDS_MAX_WORD + 1];

    REQUIRE(word_concat(buffer, sizeof(buffer), "ab", "ba"));
    REQUIRE(std::strcmp(buffer, "abba") == 0);

    /* ε — нейтральный элемент конкатенации */
    REQUIRE(word_concat(buffer, sizeof(buffer), "", "abc"));
    REQUIRE(std::strcmp(buffer, "abc") == 0);
    REQUIRE(word_concat(buffer, sizeof(buffer), "abc", ""));
    REQUIRE(std::strcmp(buffer, "abc") == 0);

    /* переполнение обнаруживается, а не портит память */
    char small[4];
    REQUIRE_FALSE(word_concat(small, sizeof(small), "abc", "def"));
}

TEST_CASE("Степень слова", "[01.Words]") {
    char buffer[WORDS_MAX_WORD + 1];

    REQUIRE(word_power(buffer, sizeof(buffer), "ab", 0));
    REQUIRE(std::strcmp(buffer, "") == 0);
    REQUIRE(word_power(buffer, sizeof(buffer), "ab", 3));
    REQUIRE(std::strcmp(buffer, "ababab") == 0);
    REQUIRE_FALSE(word_power(buffer, sizeof(buffer), "ab", -1));
}

TEST_CASE("Обращение и префиксы", "[01.Words]") {
    char buffer[WORDS_MAX_WORD + 1];

    REQUIRE(word_reverse(buffer, sizeof(buffer), "abbc"));
    REQUIRE(std::strcmp(buffer, "cbba") == 0);

    REQUIRE(word_prefix(buffer, sizeof(buffer), "abbc", 2));
    REQUIRE(std::strcmp(buffer, "ab") == 0);
    /* префикс длиннее слова — это само слово */
    REQUIRE(word_prefix(buffer, sizeof(buffer), "ab", 10));
    REQUIRE(std::strcmp(buffer, "ab") == 0);
}

TEST_CASE("Префикс, суффикс, подслово", "[01.Words]") {
    REQUIRE(word_is_prefix("ab", "abbc"));
    REQUIRE_FALSE(word_is_prefix("bb", "abbc"));
    REQUIRE(word_is_suffix("bc", "abbc"));
    REQUIRE_FALSE(word_is_suffix("ab", "abbc"));
    REQUIRE(word_is_subword("bb", "abbc"));
    REQUIRE_FALSE(word_is_subword("ac", "abbc"));

    /* ε — префикс, суффикс и подслово любого слова */
    REQUIRE(word_is_prefix("", "abc"));
    REQUIRE(word_is_suffix("", "abc"));
    REQUIRE(word_is_subword("", "abc"));
}

TEST_CASE("Язык — множество, а не список", "[01.Words]") {
    struct Language language;

    language_init(&language);
    REQUIRE(language_add(&language, "a"));
    REQUIRE(language_add(&language, "a"));
    REQUIRE(language.count == 1);
    REQUIRE(language_contains(&language, "a"));
    REQUIRE_FALSE(language_contains(&language, "b"));
}

TEST_CASE("Теоретико-множественные операции", "[01.Words]") {
    struct Language a;
    struct Language b;
    struct Language result;

    language_init(&a);
    language_add(&a, "a");
    language_add(&a, "ab");
    language_init(&b);
    language_add(&b, "ab");
    language_add(&b, "b");

    REQUIRE(language_union(&result, &a, &b));
    REQUIRE(result.count == 3);

    REQUIRE(language_intersect(&result, &a, &b));
    REQUIRE(result.count == 1);
    REQUIRE(language_contains(&result, "ab"));

    REQUIRE(language_difference(&result, &a, &b));
    REQUIRE(result.count == 1);
    REQUIRE(language_contains(&result, "a"));
}

TEST_CASE("Произведение языков", "[01.Words]") {
    struct Language a;
    struct Language b;
    struct Language result;

    language_init(&a);
    language_add(&a, "a");
    language_add(&a, "b");
    language_init(&b);
    language_add(&b, "0");
    language_add(&b, "1");

    REQUIRE(language_concat(&result, &a, &b));
    REQUIRE(result.count == 4);
    REQUIRE(language_contains(&result, "a0"));
    REQUIRE(language_contains(&result, "b1"));

    /* язык с ε сохраняет исходные слова */
    struct Language with_eps;
    language_init(&with_eps);
    language_add(&with_eps, "");
    REQUIRE(language_concat(&result, &a, &with_eps));
    REQUIRE(result.count == 2);
    REQUIRE(language_contains(&result, "a"));
}

TEST_CASE("Степень языка", "[01.Words]") {
    struct Language language;
    struct Language result;

    language_init(&language);
    language_add(&language, "a");
    language_add(&language, "b");

    REQUIRE(language_power(&result, &language, 0));
    REQUIRE(result.count == 1);
    REQUIRE(language_contains(&result, ""));

    REQUIRE(language_power(&result, &language, 2));
    REQUIRE(result.count == 4); /* aa, ab, ba, bb */
    REQUIRE(language_contains(&result, "ba"));
}

TEST_CASE("Итерация ограничена длиной", "[01.Words]") {
    struct Language language;
    struct Language result;

    language_init(&language);
    language_add(&language, "ab");

    REQUIRE(language_star(&result, &language, 4));
    /* ε, ab, abab */
    REQUIRE(result.count == 3);
    REQUIRE(language_contains(&result, ""));
    REQUIRE(language_contains(&result, "ab"));
    REQUIRE(language_contains(&result, "abab"));
    REQUIRE_FALSE(language_contains(&result, "ababab"));
}

TEST_CASE("Итерация языка с пустым словом завершается", "[01.Words]") {
    struct Language language;
    struct Language result;

    /* Если бы алгоритм не проверял, что слово новое, {ε}* зациклился бы. */
    language_init(&language);
    language_add(&language, "");
    REQUIRE(language_star(&result, &language, 8));
    REQUIRE(result.count == 1);
    REQUIRE(language_contains(&result, ""));
}
