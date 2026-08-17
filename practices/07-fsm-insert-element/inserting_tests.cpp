#include <cstring>
#include <catch2/catch.hpp>
#include "inserting_fsm.h"

TEST_CASE("Задержка", "[05.Lexical_Analyze]") {
    struct InsertingEngine engine = {};
    char m1[] = "24578912130";
    char m2[] = "24521232478912130";

    SECTION("Инициализация.") {
        REQUIRE_FALSE(inserting_init(nullptr, nullptr, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, nullptr, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, m1, 0, nullptr, 0));
        REQUIRE_FALSE(inserting_init(&engine, nullptr, 0, m2, 0));
        REQUIRE(inserting_init(&engine, m1, 0, m2, 0));
    }
    SECTION("Правильная последовательность.") {
        REQUIRE(inserting_init(&engine, m1, strlen(m1), m2, strlen(m2)));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_DETECT == inserting_engine(&engine));

        REQUIRE(3 == engine.c_1);
        REQUIRE(9 == engine.c_2);

        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_OK_END == inserting_engine(&engine));
    }
}

/*
 * Границы последовательностей. Массивы здесь намеренно без завершающего нуля:
 * автомат работает с длиной, а не со строкой, и именно так проявляется чтение
 * за границей. Исходная версия автомата (репозиторий c_fsm) на первом же из
 * этих случаев давала stack-buffer-overflow — проверку «первая кончилась, а
 * вторая нет» она не делала вовсе.
 */
TEST_CASE("Границы последовательностей", "[07-fsm-insert-element]") {
    struct InsertingEngine engine = {};

    SECTION("Вставка в конце: первая последовательность кончилась раньше") {
        char m1[3] = {'a', 'b', 'c'};
        char m2[5] = {'a', 'b', 'c', 'd', 'e'};

        REQUIRE(inserting_init(&engine, m1, sizeof(m1), m2, sizeof(m2)));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* A → B */
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* a = a */
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* b = b */
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* c = c */
        REQUIRE(INSERTING_DETECT_END == inserting_engine(&engine));
        REQUIRE(3 == engine.c_1);
        /* Граница исключающая: вставка занимает позиции [3, 5) второй
           последовательности. Тест закреплял 4 — то есть несогласованность
           с остальными путями автомата. */
        REQUIRE(5 == engine.c_2);
    }

    SECTION("Вторая последовательность короче: вставки нет") {
        char m1[4] = {'a', 'b', 'c', 'd'};
        char m2[3] = {'a', 'b', 'c'};

        REQUIRE(inserting_init(&engine, m1, sizeof(m1), m2, sizeof(m2)));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_ERROR_END == inserting_engine(&engine));
    }

    SECTION("Расхождение у самого конца: поиск продолжения упирается в границу") {
        char m1[3] = {'a', 'b', 'c'};
        char m2[3] = {'a', 'b', 'x'};

        REQUIRE(inserting_init(&engine, m1, sizeof(m1), m2, sizeof(m2)));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine));
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* a = a */
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* b = b */
        REQUIRE(INSERTING_NEXT == inserting_engine(&engine)); /* c ≠ x, B → C */
        REQUIRE(INSERTING_DETECT_END == inserting_engine(&engine));
    }
}
