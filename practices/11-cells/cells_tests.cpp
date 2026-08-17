#include <catch2/catch.hpp>
#include <cells.h>

#include <string>

/*
 * Проверяется не «программа не падает», а конкретные утверждения лекции:
 * период мигалки, сдвиг глайдера, треугольник Серпинского у правила 90,
 * шоссе муравья Лэнгтона. Каждый тест — проверяемая форма фразы из текста.
 */

namespace {

std::string row(const Elementary &ca) {
    std::string s;
    for (int i = 0; i < ca.width; ++i) {
        s += ca.cells[i] ? '#' : '.';
    }
    return s;
}

} /* namespace */

TEST_CASE("Правило 90 даёт треугольник Серпинского", "[11.Cells]") {
    Elementary ca;
    elementary_init(&ca, 90, 17);

    /* Из одной живой клетки правило 90 (XOR соседей) строит ряды
       треугольника Серпинского: 1, 101, 10001, 1010101, … */
    REQUIRE(row(ca) == "........#........");
    elementary_step(&ca);
    REQUIRE(row(ca) == ".......#.#.......");
    elementary_step(&ca);
    REQUIRE(row(ca) == "......#...#......");
    elementary_step(&ca);
    REQUIRE(row(ca) == ".....#.#.#.#.....");
}

TEST_CASE("Правило 110 несимметрично", "[11.Cells]") {
    Elementary ca;
    elementary_init(&ca, 110, 17);

    /* У правила 110 узор растёт влево: именно эта асимметрия и делает
       возможной передачу сигналов, на которой построено доказательство
       тьюринг-полноты (Кук, 2000). */
    elementary_step(&ca);
    REQUIRE(row(ca) == ".......##........");
    elementary_step(&ca);
    REQUIRE(row(ca) == "......###........");
    elementary_step(&ca);
    REQUIRE(row(ca) == ".....##.#........");
}

TEST_CASE("Правило 0 гасит всё, правило 255 заливает поле", "[11.Cells]") {
    Elementary dead;
    Elementary full;

    elementary_init(&dead, 0, 9);
    elementary_step(&dead);
    REQUIRE(row(dead) == ".........");

    elementary_init(&full, 255, 9);
    elementary_step(&full);
    REQUIRE(row(full) == "#########");
}

TEST_CASE("Мигалка имеет период 2", "[11.Cells]") {
    Life a;
    Life b;

    life_init(&a, 12, 12);
    REQUIRE(life_place(&a, "blinker", 4, 4));
    b = a;

    life_step(&a);
    REQUIRE_FALSE(life_equal_shifted(&a, &b, 0, 0));
    life_step(&a);
    REQUIRE(life_equal_shifted(&a, &b, 0, 0));
    REQUIRE(life_population(&a) == 3);
}

TEST_CASE("Блок устойчив", "[11.Cells]") {
    Life a;
    Life b;

    life_init(&a, 12, 12);
    REQUIRE(life_place(&a, "block", 3, 3));
    b = a;
    for (int i = 0; i < 5; ++i) {
        life_step(&a);
        REQUIRE(life_equal_shifted(&a, &b, 0, 0));
    }
}

TEST_CASE("Глайдер за четыре хода сдвигается по диагонали", "[11.Cells]") {
    Life a;
    Life start;

    life_init(&a, 24, 24);
    REQUIRE(life_place(&a, "glider", 4, 4));
    start = a;

    for (int i = 0; i < 4; ++i) {
        life_step(&a);
    }
    /* Скорость c/4: за четыре поколения — одна клетка вправо и одна вниз. */
    REQUIRE(life_equal_shifted(&a, &start, -1, -1));
    REQUIRE(life_population(&a) == 5);
}

TEST_CASE("Лёгкий корабль идёт по прямой со скоростью c/2", "[11.Cells]") {
    Life a;
    Life start;

    life_init(&a, 32, 16);
    REQUIRE(life_place(&a, "lwss", 2, 5));
    start = a;

    for (int i = 0; i < 4; ++i) {
        life_step(&a);
    }
    /* За четыре поколения — две клетки, то есть половина скорости света. */
    REQUIRE(life_equal_shifted(&a, &start, -2, 0));
}

TEST_CASE("R-пентамино долго не стабилизируется", "[11.Cells]") {
    Life a;
    Life previous;

    life_init(&a, 48, 48);
    REQUIRE(life_place(&a, "r-pentomino", 20, 20));

    /* Пять клеток порождают эволюцию длиной больше тысячи поколений —
       на конечном поле это видно как непрерывное изменение конфигурации. */
    bool changed_at_100 = false;
    for (int i = 0; i < 100; ++i) {
        previous = a;
        life_step(&a);
        changed_at_100 = !life_equal_shifted(&a, &previous, 0, 0);
    }
    REQUIRE(changed_at_100);
    REQUIRE(life_population(&a) > 5);
}

TEST_CASE("Неизвестная фигура отвергается", "[11.Cells]") {
    Life a;
    life_init(&a, 8, 8);
    REQUIRE_FALSE(life_place(&a, "нет такой фигуры", 0, 0));
    REQUIRE(life_population(&a) == 0);
}

TEST_CASE("Муравей Лэнгтона строит шоссе", "[11.Cells]") {
    Ant ant;
    ant_init(&ant, 128);

    /* Первые ~10 000 шагов узор выглядит хаотично, затем муравей уходит в
       периодическое «шоссе» с периодом 104 шага. Проверяем именно
       периодичность: смещение за 104 шага должно быть постоянным. */
    for (int i = 0; i < 10500; ++i) {
        REQUIRE(ant_step(&ant));
    }

    int x0 = ant.x;
    int y0 = ant.y;
    for (int i = 0; i < 104; ++i) {
        REQUIRE(ant_step(&ant));
    }
    int dx = ant.x - x0;
    int dy = ant.y - y0;
    REQUIRE((dx != 0 || dy != 0));

    for (int i = 0; i < 104; ++i) {
        REQUIRE(ant_step(&ant));
    }
    REQUIRE(ant.x - x0 == 2 * dx);
    REQUIRE(ant.y - y0 == 2 * dy);
}

TEST_CASE("Муравей помечает край и останавливается", "[11.Cells]") {
    Ant ant;
    ant_init(&ant, 16);

    long steps = 0;
    while (ant_step(&ant)) {
        ++steps;
        REQUIRE(steps < 100000);
    }
    REQUIRE(ant.escaped);
    REQUIRE(ant_black_count(&ant) > 0);
}
