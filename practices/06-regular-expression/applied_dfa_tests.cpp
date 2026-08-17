#include <catch2/catch.hpp>
#include <applied_dfa.h>

#include <string>

/*
 * Проверяется то, ради чего эти распознаватели перенесены в курс: что автомат
 * принимает ровно свой язык. Отрицательные примеры здесь важнее
 * положительных — распознаватель, который принимает всё подряд, проходит
 * любой набор «правильных» строк.
 */

TEST_CASE("ISBN распознаётся с префиксом и без", "[06.AppliedDfa]") {
    const Dfa *isbn = dfa_isbn();

    SECTION("Подходит") {
        REQUIRE(dfa_match(isbn, "0123456789"));
        REQUIRE(dfa_match(isbn, "012345678X"));
        REQUIRE(dfa_match(isbn, "012345678x"));
        REQUIRE(dfa_match(isbn, "ISBN0123456789"));
        REQUIRE(dfa_match(isbn, "ISBN:0123456789"));
        REQUIRE(dfa_match(isbn, "ISBN:012345678X"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(isbn, ""));
        REQUIRE_FALSE(dfa_match(isbn, "012345678"));   /* девять знаков */
        REQUIRE_FALSE(dfa_match(isbn, "01234567890")); /* одиннадцать */
        REQUIRE_FALSE(dfa_match(isbn, "X123456789")); /* X только в конце */
        REQUIRE_FALSE(dfa_match(isbn, "ISBN"));
        REQUIRE_FALSE(dfa_match(isbn, "ISBN:"));
        REQUIRE_FALSE(dfa_match(isbn, "isbn:0123456789")); /* префикс — прописными */
        REQUIRE_FALSE(dfa_match(isbn, "ISBN 0123456789"));
    }
}

TEST_CASE("Отказ показывает, на каком знаке разошлось", "[06.AppliedDfa]") {
    const Dfa *isbn = dfa_isbn();

    /* «ISBN» прочитано целиком, споткнулись на пробеле — пятый знак. */
    REQUIRE(dfa_match_prefix(isbn, "ISBN 0123456789") == 4);
    /* Здесь автомат дочитал строку до конца, но остановился не в
       принимающем состоянии: длины не хватило. */
    REQUIRE(dfa_match_prefix(isbn, "012345678") == 9);
    REQUIRE_FALSE(dfa_match(isbn, "012345678"));
}

TEST_CASE("Парный тег распознаётся вместе с телом", "[06.AppliedDfa]") {
    const Dfa *tag = dfa_tag();

    SECTION("Подходит") {
        REQUIRE(dfa_match(tag, "<kill></kill>"));
        REQUIRE(dfa_match(tag, "<kill>текст</kill>"));
        REQUIRE(dfa_match(tag, "<pop></pop>"));
        REQUIRE(dfa_match(tag, "<pop>что угодно</pop>"));
        /* Тело может содержать закрывающий тег: закрывает последний. */
        REQUIRE(dfa_match(tag, "<kill>a</kill>b</kill>"));
    }
    SECTION("Имена не смешиваются") {
        /* Ровно то, ради чего в исходном выражении стояла обратная ссылка. */
        REQUIRE_FALSE(dfa_match(tag, "<kill></pop>"));
        REQUIRE_FALSE(dfa_match(tag, "<pop></kill>"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(tag, ""));
        REQUIRE_FALSE(dfa_match(tag, "<kill>"));
        REQUIRE_FALSE(dfa_match(tag, "</kill>"));
        REQUIRE_FALSE(dfa_match(tag, "<kil></kil>"));
        REQUIRE_FALSE(dfa_match(tag, "<kill></kill"));
        REQUIRE_FALSE(dfa_match(tag, "x<kill></kill>"));
        /* Символ после открывающего имени обязан быть '>' — в исходном коде
           из c_fsm он не проверялся вовсе, и «<killZ» проходило. */
        REQUIRE_FALSE(dfa_match(tag, "<killZ</kill>"));
    }
}

TEST_CASE("Закрывающий тег ищется с возвратом, а не с начала", "[06.AppliedDfa]") {
    const Dfa *tag = dfa_tag();

    /*
     * Эти входы и отличают правильный автомат от наивного. Наивный при
     * несовпадении возвращается в состояние «читаю тело» и теряет уже
     * прочитанный '<', который сам был началом закрывающего тега.
     */
    REQUIRE(dfa_match(tag, "<kill><</kill>"));
    REQUIRE(dfa_match(tag, "<kill></</kill>"));
    REQUIRE(dfa_match(tag, "<kill></ki</kill>"));
    REQUIRE(dfa_match(tag, "<pop><</pop>"));
    REQUIRE(dfa_match(tag, "<pop></po</pop>"));
}

TEST_CASE("Число с плавающей точкой", "[06.AppliedDfa]") {
    const Dfa *number = dfa_number();

    SECTION("Подходит") {
        REQUIRE(dfa_match(number, "0"));
        REQUIRE(dfa_match(number, "42"));
        REQUIRE(dfa_match(number, "-42"));
        REQUIRE(dfa_match(number, "+42"));
        REQUIRE(dfa_match(number, "3.14"));
        REQUIRE(dfa_match(number, "-3.14"));
        REQUIRE(dfa_match(number, "1e9"));
        REQUIRE(dfa_match(number, "1E9"));
        REQUIRE(dfa_match(number, "1e-9"));
        REQUIRE(dfa_match(number, "-2.5e+10"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(number, ""));
        REQUIRE_FALSE(dfa_match(number, "."));
        REQUIRE_FALSE(dfa_match(number, "-"));
        REQUIRE_FALSE(dfa_match(number, ".5"));  /* без целой части */
        REQUIRE_FALSE(dfa_match(number, "5."));  /* без дробной */
        REQUIRE_FALSE(dfa_match(number, "1e"));  /* без порядка */
        REQUIRE_FALSE(dfa_match(number, "1e+")); /* тоже без порядка */
        REQUIRE_FALSE(dfa_match(number, "1.2.3"));
        REQUIRE_FALSE(dfa_match(number, "1e9e9"));
        REQUIRE_FALSE(dfa_match(number, " 42"));
        REQUIRE_FALSE(dfa_match(number, "42 "));
    }
}

TEST_CASE("Таблицы согласованы сами с собой", "[06.AppliedDfa]") {
    const Dfa *dfa;

    for (int i = 0; (dfa = dfa_by_index(i)) != NULL; ++i) {
        REQUIRE(dfa->name != NULL);
        REQUIRE(dfa->pattern != NULL);
        REQUIRE(dfa->rule_count > 0);
        REQUIRE(dfa->rule_count <= DFA_MAX_RULES);
        REQUIRE(dfa_state_count(dfa) <= DFA_MAX_STATES);
        REQUIRE(dfa_by_name(dfa->name) == dfa);

        /* Переход в несуществующее состояние или из него — молчаливая
           ошибка: строка просто перестанет распознаваться. */
        for (int r = 0; r < dfa->rule_count; ++r) {
            REQUIRE(dfa->rules[r].from >= 0);
            REQUIRE(dfa->rules[r].to >= 0);
            REQUIRE(dfa->rules[r].from < dfa_state_count(dfa));
            REQUIRE(dfa->rules[r].to < dfa_state_count(dfa));
            REQUIRE(dfa->rules[r].set != NULL);
        }
        /* Принимающее состояние, в которое нельзя попасть, — тоже ошибка. */
        for (int a = 0; dfa->accepting[a] >= 0; ++a) {
            bool reachable = dfa->accepting[a] == dfa->start;
            for (int r = 0; !reachable && r < dfa->rule_count; ++r) {
                reachable = dfa->rules[r].to == dfa->accepting[a];
            }
            REQUIRE(reachable);
        }
    }
    REQUIRE(dfa_by_name("нет такого") == NULL);
    REQUIRE(dfa_by_index(-1) == NULL);
}

TEST_CASE("Число состояний считается по таблице", "[06.AppliedDfa]") {
    /* Числа из лекции: девять цифр ISBN дают девять состояний, и обойтись
       меньшим числом нельзя — у автомата нет счётчика. */
    REQUIRE(dfa_state_count(dfa_number()) == 8);
    REQUIRE(dfa_state_count(dfa_isbn()) == 16);
    REQUIRE(dfa_state_count(dfa_tag()) == 30);
}
