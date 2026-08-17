#include <catch2/catch.hpp>
#include <ant_search.h>

#include <string>
#include <vector>

/*
 * Проверяется не «поиск что-то нашёл», а свойства, без которых результатам
 * поиска нельзя верить: воспроизводимость по зерну, монотонность журнала
 * улучшений, отсутствие переходов в несуществующие состояния и то, что
 * найденный автомат действительно проходит тропу.
 */

namespace {

struct Improvements {
    std::vector<int> eaten;
    std::vector<int> steps;
};

void collect(const AntFsm *fsm, const TrailRun *run, long iteration, void *user) {
    (void)fsm;
    (void)iteration;
    Improvements *log = static_cast<Improvements *>(user);
    log->eaten.push_back(run->eaten);
    log->steps.push_back(run->steps);
}

AntSearch quick_config() {
    AntSearch cfg;
    ant_search_defaults(&cfg);
    cfg.iterations = 20000;
    return cfg;
}

} /* namespace */

TEST_CASE("Функция приспособленности сначала считает еду, потом такты", "[11.AntSearch]") {
    TrailRun few = {40, 89, 600, 400, false};
    TrailRun many = {89, 89, 600, 600, true};
    TrailRun fast = {89, 89, 315, 315, true};

    REQUIRE(ant_run_better(&many, &few));
    REQUIRE(ant_run_better(&fast, &many));
    REQUIRE_FALSE(ant_run_better(&many, &fast));
    REQUIRE_FALSE(ant_run_better(&few, &few));
}

TEST_CASE("Приспособленность по статье: еда плюс поправка меньше единицы", "[11.AntSearch]") {
    TrailRun few = {40, 89, 200, 150, false};
    TrailRun many = {41, 89, 200, 200, false};
    TrailRun fast = {41, 89, 120, 120, false};

    /* Формула (4) статьи: food + 0.01 * (1 - t / limit). Поправка не может
       перевесить разницу в одну съеденную клетку — иначе поиск начал бы
       предпочитать быстрые, но голодные автоматы. */
    REQUIRE(ant_fitness_ep93(&many, 200) > ant_fitness_ep93(&few, 200));
    REQUIRE(ant_fitness_ep93(&fast, 200) > ant_fitness_ep93(&many, 200));
    REQUIRE(ant_fitness_ep93(&fast, 200) - 41.0 < 1.0);
    REQUIRE(ant_fitness_ep93(&many, 200) == Approx(41.0));
}

TEST_CASE("Поиск воспроизводится по зерну", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult a;
    AntSearchResult b;
    char spec_a[ANT_FSM_SPEC_SIZE];
    char spec_b[ANT_FSM_SPEC_SIZE];

    REQUIRE(ant_search_run(&cfg, NULL, &a, NULL));
    REQUIRE(ant_search_run(&cfg, NULL, &b, NULL));

    /* Никакого глобального состояния: два поиска подряд в одном процессе
       дают один результат — ровно то, чего не позволял исходный код в
       репозитории c_fsm. */
    REQUIRE(ant_fsm_format(&a.best, spec_a, sizeof(spec_a)));
    REQUIRE(ant_fsm_format(&b.best, spec_b, sizeof(spec_b)));
    REQUIRE(std::string(spec_a) == std::string(spec_b));
    REQUIRE(a.run.eaten == b.run.eaten);
    REQUIRE(a.run.steps == b.run.steps);
    REQUIRE(a.iterations_done == b.iterations_done);
}

TEST_CASE("Разные зёрна дают разные пути поиска", "[11.AntSearch]") {
    AntSearch first = quick_config();
    AntSearch second = quick_config();
    AntSearchResult a;
    AntSearchResult b;

    second.seed = 12345;
    REQUIRE(ant_search_run(&first, NULL, &a, NULL));
    REQUIRE(ant_search_run(&second, NULL, &b, NULL));
    REQUIRE((a.improvements != b.improvements || a.run.steps != b.run.steps
             || a.iterations_done != b.iterations_done));
}

TEST_CASE("Журнал улучшений монотонен", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;
    Improvements collected;
    AntSearchLog log;

    log.improved = collect;
    log.user = &collected;
    REQUIRE(ant_search_run(&cfg, NULL, &result, &log));

    REQUIRE(collected.eaten.size() == static_cast<size_t>(result.improvements));
    for (size_t i = 1; i < collected.eaten.size(); ++i) {
        /* Каждая запись журнала строго лучше предыдущей: больше съедено либо
           столько же, но за меньшее число тактов. */
        bool better = collected.eaten[i] > collected.eaten[i - 1]
                      || (collected.eaten[i] == collected.eaten[i - 1]
                          && collected.steps[i] < collected.steps[i - 1]);
        REQUIRE(better);
    }
    if (!collected.eaten.empty()) {
        REQUIRE(collected.eaten.back() == result.run.eaten);
        REQUIRE(collected.steps.back() == result.run.steps);
    }
}

TEST_CASE("Найденный автомат корректен и проверяется прогоном", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;

    REQUIRE(ant_search_run(&cfg, NULL, &result, NULL));
    REQUIRE(result.best.state_count >= 1);
    REQUIRE(result.best.state_count <= cfg.max_states);
    for (int i = 0; i < result.best.state_count; ++i) {
        /* Переход в несуществующее состояние сделал бы прогон неопределённым;
           мутации обязаны держаться внутри таблицы. */
        REQUIRE(result.best.next[i][0] < result.best.state_count);
        REQUIRE(result.best.next[i][1] < result.best.state_count);
    }

    TrailRun again = ant_trail_run(&result.best, cfg.steps_limit);
    REQUIRE(again.eaten == result.run.eaten);
    REQUIRE(again.steps == result.run.steps);
    REQUIRE(again.finished == result.run.finished);
}

TEST_CASE("Поиск от готовой стратегии её не ухудшает", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;
    AntFsm start;

    ant_fsm_scan(&start);
    TrailRun before = ant_trail_run(&start, cfg.steps_limit);
    REQUIRE(before.steps == 315);

    /* Начальный автомат уже проходит тропу; поиск обязан её не потерять и
       дальше сокращать такты — на этом и построена история улучшений. */
    REQUIRE(ant_search_run(&cfg, &start, &result, NULL));
    REQUIRE(result.run.finished);
    REQUIRE(result.run.steps <= before.steps);
    REQUIRE(result.iterations_done == cfg.iterations);
}

TEST_CASE("Пройденная тропа поиск не останавливает", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;
    AntFsm start;

    cfg.max_states = 8;
    cfg.iterations = 100000;
    ant_fsm_scan(&start);

    /* 315 тактов — не предел: при пяти состояниях известен как раз этот
       результат, а лишние состояния позволяют его сократить. */
    REQUIRE(ant_search_run(&cfg, &start, &result, NULL));
    REQUIRE(result.run.finished);
    REQUIRE(result.run.steps < 315);
}

TEST_CASE("Поиск с ростом добавляет состояния и не выходит за потолок", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;
    AntFsm start;

    cfg.max_states = 8;
    cfg.stall_limit = 400;
    cfg.iterations = 20000;
    ant_fsm_lookaround(&start);   /* 4 состояния, тропу не проходит */

    REQUIRE(ant_search_run(&cfg, &start, &result, NULL));
    REQUIRE(result.best.state_count >= 4);
    REQUIRE(result.best.state_count <= cfg.max_states);
    /* Стартовая стратегия съедает 42 из 89; поиск обязан хотя бы не потерять
       этот результат. */
    REQUIRE(result.run.eaten >= 42);
}

/* --- алгоритм статьи Angeline и Pollack (1993) ---------------------------- */

namespace {

AntSearch ep93_config() {
    AntSearch cfg;
    ant_search_defaults_ep93(&cfg);
    cfg.population = 60;
    cfg.iterations = 60000;
    return cfg;
}

} /* namespace */

TEST_CASE("Постановка статьи: 200 тактов и 32 состояния", "[11.AntSearch]") {
    AntSearch cfg;
    AntFsm scan;

    ant_search_defaults_ep93(&cfg);
    REQUIRE(cfg.algorithm == ANT_SEARCH_EP93);
    REQUIRE(cfg.steps_limit == TRAIL_STEPS_EP93);
    REQUIRE(cfg.steps_limit == 200);
    REQUIRE(cfg.max_states == 32);
    REQUIRE(cfg.population == 300);
    REQUIRE(cfg.tournaments == 5);

    /* Именно этот лимит и делает задачу трудной: стратегия, придуманная
       вручную, тратит 315 тактов и в 200 не укладывается. */
    ant_fsm_scan(&scan);
    TrailRun r = ant_trail_run(&scan, cfg.steps_limit);
    REQUIRE_FALSE(r.finished);
}

TEST_CASE("Популяционный поиск воспроизводится по зерну", "[11.AntSearch]") {
    AntSearch cfg = ep93_config();
    AntSearchResult a;
    AntSearchResult b;
    char spec_a[ANT_FSM_SPEC_SIZE];
    char spec_b[ANT_FSM_SPEC_SIZE];

    REQUIRE(ant_search_run(&cfg, NULL, &a, NULL));
    REQUIRE(ant_search_run(&cfg, NULL, &b, NULL));

    REQUIRE(a.evaluations == b.evaluations);
    REQUIRE(a.generations == b.generations);
    REQUIRE(ant_fsm_format(&a.best, spec_a, sizeof(spec_a)));
    REQUIRE(ant_fsm_format(&b.best, spec_b, sizeof(spec_b)));
    REQUIRE(std::string(spec_a) == std::string(spec_b));
}

TEST_CASE("Популяционный поиск находит решение в постановке статьи", "[11.AntSearch]") {
    AntSearch cfg;
    AntSearchResult result;

    ant_search_defaults_ep93(&cfg);
    cfg.iterations = 200000;
    REQUIRE(ant_search_run(&cfg, NULL, &result, NULL));

    /* Автомат съедает всю еду за 200 тактов — то, чего ни одна из стратегий,
       придуманных вручную, не умеет. */
    REQUIRE(result.run.finished);
    REQUIRE(result.run.eaten == 89);
    REQUIRE(result.run.steps <= TRAIL_STEPS_EP93);
    REQUIRE(result.generations > 0);
    REQUIRE(result.evaluations >= result.generations);

    for (int i = 0; i < result.best.state_count; ++i) {
        REQUIRE(result.best.next[i][0] < result.best.state_count);
        REQUIRE(result.best.next[i][1] < result.best.state_count);
    }
    /* Результат перепроверяется независимым прогоном. */
    TrailRun again = ant_trail_run(&result.best, cfg.steps_limit);
    REQUIRE(again.steps == result.run.steps);
    REQUIRE(again.eaten == result.run.eaten);
}

TEST_CASE("Заморозка модулей включается и выключается", "[11.AntSearch]") {
    AntSearch with = ep93_config();
    AntSearch without = ep93_config();
    AntSearchResult a;
    AntSearchResult b;

    without.compression = false;
    REQUIRE(ant_search_run(&with, NULL, &a, NULL));
    REQUIRE(ant_search_run(&without, NULL, &b, NULL));

    /* Заморозка — единственное отличие двух прогонов, и она обязана быть
       наблюдаемой: без неё оператор сжатия не вызывается ни разу. */
    REQUIRE(a.compressions > 0);
    REQUIRE(b.compressions == 0);
}

TEST_CASE("Удаление состояний не оставляет переходов в никуда", "[11.AntSearch]") {
    AntSearch cfg = ep93_config();
    AntSearchResult result;

    /* Мутации по формуле (5) статьи удаляют состояния со сдвигом индексов —
       место, где легче всего оставить висящий переход. */
    cfg.max_states = 6;
    cfg.iterations = 30000;
    REQUIRE(ant_search_run(&cfg, NULL, &result, NULL));
    REQUIRE(result.best.state_count >= 1);
    REQUIRE(result.best.state_count <= cfg.max_states);
    for (int i = 0; i < result.best.state_count; ++i) {
        REQUIRE(result.best.next[i][0] < result.best.state_count);
        REQUIRE(result.best.next[i][1] < result.best.state_count);
    }
}

TEST_CASE("Негодная популяция отвергается", "[11.AntSearch]") {
    AntSearch cfg = ep93_config();
    AntSearchResult result;

    cfg.population = 1;
    REQUIRE_FALSE(ant_search_run(&cfg, NULL, &result, NULL));

    cfg = ep93_config();
    cfg.tournaments = 0;
    REQUIRE_FALSE(ant_search_run(&cfg, NULL, &result, NULL));
}

TEST_CASE("Негодные настройки отвергаются", "[11.AntSearch]") {
    AntSearch cfg = quick_config();
    AntSearchResult result;
    AntFsm start;

    cfg.max_states = 0;
    REQUIRE_FALSE(ant_search_run(&cfg, NULL, &result, NULL));

    cfg = quick_config();
    cfg.max_states = ANT_FSM_MAX_STATES + 1;
    REQUIRE_FALSE(ant_search_run(&cfg, NULL, &result, NULL));

    cfg = quick_config();
    cfg.steps_limit = 0;
    REQUIRE_FALSE(ant_search_run(&cfg, NULL, &result, NULL));

    /* Начальный автомат больше потолка: молча урезать его нельзя. */
    cfg = quick_config();
    cfg.max_states = 4;
    ant_fsm_evolved(&start);
    REQUIRE_FALSE(ant_search_run(&cfg, &start, &result, NULL));
}
