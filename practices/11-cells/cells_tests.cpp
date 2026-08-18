#include <catch2/catch.hpp>
#include <ant_trail.h>
#include <cells.h>
#include <render.h>

#include <algorithm>
#include <cstdio>
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

TEST_CASE("Ширина элементарного автомата приводится к допустимой", "[11.Cells]") {
    struct Elementary ca;

    /* Контракт публичной функции: ни одно значение ширины не должно
       приводить к порче памяти на шаге. Отрицательная ширина раньше давала
       memcpy на (size_t)(-1) байт — вызывающие её просто не передавали. */
    elementary_init(&ca, 90, -5);
    REQUIRE(ca.width == 1);
    elementary_step(&ca);
    REQUIRE(ca.width == 1);

    elementary_init(&ca, 90, 0);
    REQUIRE(ca.width == 1);

    elementary_init(&ca, 90, CELLS_MAX_WIDTH + 100);
    REQUIRE(ca.width == CELLS_MAX_WIDTH);
    elementary_step(&ca);
    REQUIRE(ca.width == CELLS_MAX_WIDTH);
}

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

TEST_CASE("Натюрморты не меняются", "[11.Cells]") {
    const char *names[] = {"block", "beehive", "loaf", "boat", "tub", "eater"};

    for (const char *name : names) {
        Life a;
        Life b;
        life_init(&a, 16, 16);
        REQUIRE(life_place(&a, name, 4, 4));
        b = a;
        for (int i = 0; i < 4; ++i) {
            life_step(&a);
            /* Фигура, взятая из литературы и не проверенная прогоном, рано
               или поздно оказывается записанной с ошибкой. */
            REQUIRE(life_equal_shifted(&a, &b, 0, 0));
        }
    }
}

TEST_CASE("Осцилляторы имеют заявленный период", "[11.Cells]") {
    struct Case {
        const char *name;
        int period;
        int side;
        int warmup; /* сколько ходов до выхода на цикл */
    };
    /* Пентадекатлон задан рядом из десяти клеток — это его предок, а не фаза:
       сам осциллятор устанавливается через два хода. Остальные фигуры
       записаны сразу в фазе цикла. */
    const Case cases[] = {{"blinker", 2, 16, 0}, {"toad", 2, 16, 0},   {"beacon", 2, 16, 0},
                          {"pulsar", 3, 24, 0},  {"pentadecathlon", 15, 24, 2}};

    for (const Case &c : cases) {
        Life a;
        Life start;
        life_init(&a, c.side, c.side);
        REQUIRE(life_place(&a, c.name, 4, 4));
        for (int i = 0; i < c.warmup; ++i) {
            life_step(&a);
        }
        start = a;
        for (int i = 1; i <= c.period; ++i) {
            life_step(&a);
            if (i < c.period) {
                REQUIRE_FALSE(life_equal_shifted(&a, &start, 0, 0));
            }
        }
        REQUIRE(life_equal_shifted(&a, &start, 0, 0));
    }
}

TEST_CASE("Средний и тяжёлый корабли идут со скоростью c/2", "[11.Cells]") {
    const char *names[] = {"mwss", "hwss"};

    for (const char *name : names) {
        Life a;
        Life start;
        life_init(&a, 32, 16);
        REQUIRE(life_place(&a, name, 2, 5));
        start = a;
        for (int i = 0; i < 4; ++i) {
            life_step(&a);
        }
        REQUIRE(life_equal_shifted(&a, &start, -2, 0));
    }
}

TEST_CASE("Ружьё Госпера растёт неограниченно", "[11.Cells]") {
    Life a;
    life_init(&a, 60, 60);
    REQUIRE(life_place(&a, "gosper-gun", 1, 1));

    int start_population = life_population(&a);
    REQUIRE(start_population == 36);
    for (int i = 0; i < 30; ++i) {
        life_step(&a);
    }
    /* За период ружьё выпускает планер — пять клеток, которые больше не
       возвращаются. Это и есть неограниченный рост: на бесконечном поле
       население растёт линейно. */
    REQUIRE(life_population(&a) == start_population + 5);
    for (int i = 0; i < 30; ++i) {
        life_step(&a);
    }
    REQUIRE(life_population(&a) == start_population + 10);
}

TEST_CASE("Долгие эволюции из горстки клеток", "[11.Cells]") {
    Life diehard;
    life_init(&diehard, 32, 32);
    REQUIRE(life_place(&diehard, "diehard", 8, 8));
    REQUIRE(life_population(&diehard) == 7);
    for (int i = 0; i < 130; ++i) {
        life_step(&diehard);
    }
    /* Название говорит само за себя: семь клеток живут 130 поколений и
       исчезают без следа. На торе поле конечно, поэтому проверяем сам факт
       вымирания. */
    REQUIRE(life_population(&diehard) == 0);

    Life acorn;
    life_init(&acorn, 48, 48);
    REQUIRE(life_place(&acorn, "acorn", 20, 20));
    REQUIRE(life_population(&acorn) == 7);
    for (int i = 0; i < 100; ++i) {
        life_step(&acorn);
    }
    REQUIRE(life_population(&acorn) > 7);
}

TEST_CASE("Чеширский кот оставляет улыбку, а затем отпечаток лапы", "[11.Cells]") {
    Life a;
    life_init(&a, 20, 20);
    REQUIRE(life_place(&a, "cheshire-cat", 6, 6));
    REQUIRE(life_population(&a) == 18);

    /* Конфигурация считана со скана книги М. Гарднера, и проверяется она
       ровно тем, что о ней там сказано: на шестом ходу от кота остаётся
       «улыбка» из четырёх клеток, на седьмом — «отпечаток лапы», блок. */
    for (int i = 0; i < 6; ++i) {
        life_step(&a);
    }
    REQUIRE(life_population(&a) == 4);

    Life smile = a;
    life_step(&a);
    REQUIRE(life_population(&a) == 4);
    REQUIRE_FALSE(life_equal_shifted(&a, &smile, 0, 0));

    Life block;
    life_init(&block, 20, 20);
    REQUIRE(life_place(&block, "block", 0, 0));
    /* Блок узнаётся по устойчивости: следующие ходы ничего не меняют. */
    Life before = a;
    for (int i = 0; i < 3; ++i) {
        life_step(&a);
        REQUIRE(life_equal_shifted(&a, &before, 0, 0));
    }
    REQUIRE(life_population(&block) == life_population(&a));
}

TEST_CASE("«Сад Эдема» — 226 клеток в прямоугольнике 33 на 9", "[11.Cells]") {
    Life a;
    life_init(&a, 40, 16);
    REQUIRE(life_place(&a, "garden-of-eden", 1, 1));

    /* Проверить отсутствие предшественника прогоном нельзя — это утверждение
       о несуществовании. Сверяются размеры и число клеток: если конфигурация
       будет случайно испорчена правкой, тест это заметит. */
    REQUIRE(life_population(&a) == 226);

    int left = 40;
    int right = -1;
    int top = 16;
    int bottom = -1;
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 40; ++x) {
            if (!life_get(&a, x, y)) {
                continue;
            }
            left = std::min(left, x);
            right = std::max(right, x);
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        }
    }
    REQUIRE(right - left + 1 == 33);
    REQUIRE(bottom - top + 1 == 9);
}

TEST_CASE("Пять триплетов ведут себя так, как в книге", "[11.Cells]") {
    const char *dying[] = {"triplet-step", "triplet-v", "triplet-diagonal"};

    for (const char *name : dying) {
        Life a;
        life_init(&a, 16, 16);
        REQUIRE(life_place(&a, name, 6, 6));
        REQUIRE(life_population(&a) == 3);
        for (int i = 0; i < 3; ++i) {
            life_step(&a);
        }
        REQUIRE(life_population(&a) == 0);
    }

    /* Четвёртый триплет — тримино — даёт блок и на этом останавливается. */
    Life corner;
    life_init(&corner, 16, 16);
    REQUIRE(life_place(&corner, "tromino", 6, 6));
    life_step(&corner);
    REQUIRE(life_population(&corner) == 4);
    Life block = corner;
    for (int i = 0; i < 3; ++i) {
        life_step(&corner);
        REQUIRE(life_equal_shifted(&corner, &block, 0, 0));
    }
}

TEST_CASE("Агар устойчив, а вирус решает всё", "[11.Cells]") {
    Life agar;
    life_init(&agar, 24, 24);
    REQUIRE(life_fill_agar(&agar));
    REQUIRE(life_population(&agar) == 256);

    Life a = agar;
    for (int i = 0; i < 5; ++i) {
        life_step(&a);
        REQUIRE(life_equal_shifted(&a, &agar, 0, 0));
    }

    /* Вирус в клетке, где сходятся углы четырёх блоков: агар уничтожает его и
       через два хода восстанавливает прежний вид. */
    Life corner = agar;
    life_set(&corner, 14, 14, true);
    life_step(&corner);
    REQUIRE_FALSE(life_equal_shifted(&corner, &agar, 0, 0));
    life_step(&corner);
    REQUIRE(life_equal_shifted(&corner, &agar, 0, 0));

    /* Вирус рядом с блоком — начинается разрушение, и оно только растёт. */
    Life edge = agar;
    life_set(&edge, 12, 14, true);
    int damage_before = 0;
    for (int step = 1; step <= 8; ++step) {
        life_step(&edge);
        int damage = 0;
        for (int y = 0; y < 24; ++y) {
            for (int x = 0; x < 24; ++x) {
                if (life_get(&edge, x, y) != life_get(&agar, x, y)) {
                    ++damage;
                }
            }
        }
        REQUIRE(damage > damage_before);
        damage_before = damage;
    }
}

TEST_CASE("Заливка агаром требует стороны, кратной трём", "[11.Cells]") {
    Life bad;
    life_init(&bad, 20, 24);
    /* Иначе на стыке через тор получится шов, и агар разрушится сам по себе,
       без всякого вируса, — а картинка соврёт о правиле. */
    REQUIRE_FALSE(life_fill_agar(&bad));
}

TEST_CASE("Правило чётности размножает тримино", "[11.Cells]") {
    Life a;
    life_init(&a, 48, 48);
    REQUIRE(life_place(&a, "tromino", 20, 20));
    REQUIRE(life_population(&a) == 3);

    /* Репликатор Фредкина: через четыре хода вместо одной фигуры на поле
       четыре её копии — 12 клеток. Именно это и описано в лекции: копии
       расходятся вправо, влево, вверх и вниз от опустевшего места. */
    for (int i = 0; i < 4; ++i) {
        life_step_parity(&a);
    }
    REQUIRE(life_population(&a) == 12);

    /* Копии — это именно тримино: три клетки в углу, восемь раз. */
    int corners = 0;
    for (int y = 0; y < 48; ++y) {
        for (int x = 0; x < 48; ++x) {
            if (life_get(&a, x, y) && life_get(&a, x + 1, y) && life_get(&a, x, y + 1)
                && !life_get(&a, x + 1, y + 1)) {
                ++corners;
            }
        }
    }
    REQUIRE(corners == 4);
}

TEST_CASE("Список фигур перебирается по индексу", "[11.Cells]") {
    Life a;
    int count = 0;

    for (int i = 0; life_pattern_name(i) != NULL; ++i) {
        life_init(&a, 60, 60);
        /* Каждое имя из списка обязано ставиться: список и таблица фигур —
           одно и то же место, разъехаться они не должны. */
        REQUIRE(life_place(&a, life_pattern_name(i), 1, 1));
        REQUIRE(life_population(&a) > 0);
        ++count;
    }
    REQUIRE(count >= 25);
    REQUIRE(life_pattern_name(count) == NULL);
    REQUIRE(life_pattern_name(-1) == NULL);
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

/* --- задача об умном муравье --------------------------------------------- */

TEST_CASE("Тропа — каноническая: 89 клеток еды", "[11.Cells]") {
    /* Число еды на тропе Санта-Фе — стандарт сравнения; если карту случайно
       поправят, разойдутся все числа тактов ниже, и тест скажет об этом
       раньше, чем расхождение попадёт в лекцию. */
    REQUIRE(ant_trail_food_total() == 89);
}

TEST_CASE("Осмотр четырёх направлений проходит тропу за 315 тактов", "[11.Cells]") {
    AntFsm fsm;
    ant_fsm_scan(&fsm);
    TrailRun r = ant_trail_run(&fsm, TRAIL_STEPS);

    REQUIRE(fsm.state_count == 5);
    REQUIRE(r.total == ant_trail_food_total());
    REQUIRE(r.eaten == r.total);
    REQUIRE(r.finished);
    REQUIRE(r.steps == 315);
}

TEST_CASE("Найденный поиском автомат проходит тропу за 181 такт", "[11.Cells]") {
    AntFsm fsm;
    ant_fsm_evolved(&fsm);
    TrailRun r = ant_trail_run(&fsm, TRAIL_STEPS);

    REQUIRE(fsm.state_count == 17);
    REQUIRE(r.eaten == r.total);
    REQUIRE(r.finished);
    REQUIRE(r.steps == 181);
    /* 17 состояний против 5 — цена 134 сэкономленных тактов. Именно этот
       размен и обсуждается в лекции. */
}

TEST_CASE("Осмотра вправо-влево со слепыми шагами на Санта-Фе не хватает", "[11.Cells]") {
    AntFsm fsm;
    ant_fsm_probe(&fsm);
    TrailRun r = ant_trail_run(&fsm, TRAIL_STEPS);

    /* Стратегия, достаточная для регулярной учебной тропы, на канонической
       съедает 59 из 89 и зацикливается: два слепых шага проносят муравья
       мимо поворота, а осмотр не заглядывает назад. */
    REQUIRE_FALSE(r.finished);
    REQUIRE(r.eaten == 59);
}

TEST_CASE("Осмотра без слепых шагов не хватает на разрыв в две клетки", "[11.Cells]") {
    AntFsm fsm;
    ant_fsm_lookaround(&fsm);
    TrailRun r = ant_trail_run(&fsm, TRAIL_STEPS);

    REQUIRE_FALSE(r.finished);
    REQUIRE(r.eaten == 42);
    REQUIRE(r.eaten < r.total / 2);
}

TEST_CASE("Прогон детерминирован", "[11.Cells]") {
    AntFsm fsm;
    ant_fsm_scan(&fsm);
    TrailRun a = ant_trail_run(&fsm, TRAIL_STEPS);
    TrailRun b = ant_trail_run(&fsm, TRAIL_STEPS);

    REQUIRE(a.eaten == b.eaten);
    REQUIRE(a.steps == b.steps);
}

TEST_CASE("Стратегия ищется по имени", "[11.Cells]") {
    AntFsm named;
    AntFsm direct;

    REQUIRE(ant_fsm_by_name("scan", &named));
    ant_fsm_scan(&direct);
    REQUIRE(named.state_count == direct.state_count);
    REQUIRE_FALSE(ant_fsm_by_name("нет такой стратегии", &named));
}

TEST_CASE("Запись автомата разбирается и печатается обратно", "[11.Cells]") {
    AntFsm fsm;
    AntFsm parsed;
    char spec[ANT_FSM_SPEC_SIZE];
    char again[ANT_FSM_SPEC_SIZE];

    ant_fsm_scan(&fsm);
    REQUIRE(ant_fsm_format(&fsm, spec, sizeof(spec)));
    /* Та же запись, что в журнале поиска из репозитория c_fsm. */
    REQUIRE(std::string(spec) == "3.0.0.1:3.0.0.2:3.0.0.3:3.0.0.4:3.0.2.0");

    REQUIRE(ant_fsm_parse(&parsed, spec));
    REQUIRE(parsed.state_count == fsm.state_count);
    REQUIRE(ant_fsm_format(&parsed, again, sizeof(again)));
    REQUIRE(std::string(again) == std::string(spec));

    TrailRun r = ant_trail_run(&parsed, TRAIL_STEPS);
    REQUIRE(r.steps == 315);
}

TEST_CASE("Испорченная запись автомата отвергается", "[11.Cells]") {
    AntFsm fsm;
    char small[8];

    REQUIRE_FALSE(ant_fsm_parse(&fsm, ""));
    REQUIRE_FALSE(ant_fsm_parse(&fsm, "3.0.0.1:"));
    REQUIRE_FALSE(ant_fsm_parse(&fsm, "3.0.0"));
    REQUIRE_FALSE(ant_fsm_parse(&fsm, "9.0.0.1"));       /* нет такого действия */
    REQUIRE_FALSE(ant_fsm_parse(&fsm, "3.0.0.7"));       /* переход в никуда */
    REQUIRE_FALSE(ant_fsm_parse(&fsm, "3.0.0.1 3.0.2.0"));

    /* Автомат при неудачном разборе остаётся пустым, а не наполовину
       заполненным: прогон такого автомата не делает ни шага. */
    TrailRun r = ant_trail_run(&fsm, TRAIL_STEPS);
    REQUIRE(r.eaten == 0);
    REQUIRE(r.steps == 0);

    ant_fsm_scan(&fsm);
    REQUIRE_FALSE(ant_fsm_format(&fsm, small, sizeof(small)));
}

/* --- векторный вывод ------------------------------------------------------ */

namespace {

/* SVG проверяется как текст: важно не «картинка красивая», а что файл
   получился разбираемым и содержит ровно столько кадров, сколько заказано. */
std::string render_to_string(bool (*draw)(FILE *), bool *ok) {
    std::string result;
    FILE *tmp = tmpfile();
    REQUIRE(tmp != NULL);
    *ok = draw(tmp);
    rewind(tmp);
    char buffer[4096];
    size_t got;
    while ((got = fread(buffer, 1, sizeof(buffer), tmp)) > 0) {
        result.append(buffer, got);
    }
    fclose(tmp);
    return result;
}

int count_substring(const std::string &text, const std::string &needle) {
    int count = 0;
    for (size_t pos = text.find(needle); pos != std::string::npos;
         pos = text.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}

} /* namespace */

TEST_CASE("Лента «Жизни» рисуется в SVG", "[11.Cells]") {
    bool ok = false;
    std::string svg = render_to_string(
        [](FILE *out) {
            static const int frames[] = {0, 1, 2, 3, 4};
            return render_life_svg("glider", 24, 24, frames, 5, NULL, out);
        },
        &ok);

    REQUIRE(ok);
    REQUIRE(svg.find("<svg") != std::string::npos);
    REQUIRE(svg.find("</svg>") != std::string::npos);
    /* Пять кадров — пять рамок. */
    REQUIRE(count_substring(svg, "stroke=\"#666666\"") == 5);
    /* Глайдер — пять клеток в каждом кадре. */
    REQUIRE(count_substring(svg, "fill=\"#1a1a1a\"") == 25);
}

TEST_CASE("Лента прогона муравья рисуется в SVG", "[11.Cells]") {
    bool ok = false;
    std::string svg = render_to_string(
        [](FILE *out) {
            static const int steps[] = {0, 20, 60};
            AntFsm fsm;
            ant_fsm_scan(&fsm);
            return render_trail_svg(&fsm, steps, 3, NULL, out);
        },
        &ok);

    REQUIRE(ok);
    REQUIRE(count_substring(svg, "stroke=\"#666666\"") == 3);
    /* Муравей — по треугольнику на кадр, и его видно на любой заливке. */
    REQUIRE(count_substring(svg, "<polygon") == 3);
}

TEST_CASE("Узор одномерного автомата рисуется в SVG", "[11.Cells]") {
    bool ok = false;
    std::string svg = render_to_string(
        [](FILE *out) { return render_elementary_svg(90, 21, 10, NULL, out); }, &ok);

    REQUIRE(ok);
    REQUIRE(svg.find("<svg") != std::string::npos);
    REQUIRE(count_substring(svg, "fill=\"#1a1a1a\"") > 10);
}

TEST_CASE("Негодные параметры рисовальщика отвергаются", "[11.Cells]") {
    bool ok = true;
    static const int frames[] = {0};

    render_to_string([](FILE *out) { return render_life_svg("нет такой", 24, 24, frames, 1, NULL, out); },
                     &ok);
    REQUIRE_FALSE(ok);

    ok = true;
    render_to_string([](FILE *out) { return render_life_svg("glider", 24, 24, frames, 0, NULL, out); },
                     &ok);
    REQUIRE_FALSE(ok);

    ok = true;
    render_to_string([](FILE *out) { return render_elementary_svg(90, 2, 5, NULL, out); }, &ok);
    REQUIRE_FALSE(ok);
}
