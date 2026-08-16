#include <catch2/catch.hpp>
#include <barrier.h>

#include <cstring>
#include <string>
#include <vector>

/*
 * Главная проверка практики: три реализации одного автомата обязаны давать
 * одинаковую трассу на любой последовательности событий. Если расходятся —
 * какая-то из реализаций описывает другой автомат, и это ровно тот дефект,
 * который в коде без явной модели найти нечем.
 */

namespace {

std::vector<BarrierEvent> parse(const std::string &line) {
    std::vector<BarrierEvent> events;
    std::string word;
    std::string rest = line + " ";
    size_t pos = 0;

    while ((pos = rest.find(' ')) != std::string::npos) {
        word = rest.substr(0, pos);
        rest = rest.substr(pos + 1);
        if (word.empty()) {
            continue;
        }
        BarrierEvent event;
        REQUIRE(barrier_event_by_name(word.c_str(), &event));
        events.push_back(event);
    }
    return events;
}

struct Run {
    BarrierTrace sw;
    BarrierTrace table;
    BarrierTrace object;
    BarrierTable table_fsm;
};

void run(const std::string &line, Run &out) {
    BarrierSwitch sw;
    BarrierObject ob;
    std::vector<BarrierEvent> events = parse(line);

    barrier_trace_init(&out.sw);
    barrier_trace_init(&out.table);
    barrier_trace_init(&out.object);
    barrier_switch_init(&sw, &out.sw);
    barrier_table_init(&out.table_fsm, &out.table);
    barrier_object_init(&ob, &out.object);

    for (size_t i = 0; i < events.size(); ++i) {
        barrier_switch_event(&sw, events[i]);
        barrier_table_event(&out.table_fsm, events[i]);
        barrier_object_event(&ob, events[i]);
    }
    REQUIRE_FALSE(out.sw.overflow);
    REQUIRE(sw.state == barrier_object_state(&ob));
    REQUIRE(sw.state == out.table_fsm.state);
}

std::string actions(const BarrierTrace &trace) {
    std::string result;
    for (size_t i = 0; i < trace.action_count; ++i) {
        if (i != 0) {
            result += " ";
        }
        result += barrier_action_name(trace.actions[i]);
    }
    return result;
}

std::string states(const BarrierTrace &trace) {
    std::string result;
    for (size_t i = 0; i < trace.state_count; ++i) {
        if (i != 0) {
            result += " ";
        }
        result += barrier_state_name(trace.states[i]);
    }
    return result;
}

} /* namespace */

TEST_CASE("Три реализации дают одну трассу", "[07.ThreeWays]") {
    for (int i = 0; i < BARRIER_SCENARIO_COUNT; ++i) {
        Run r;
        run(BARRIER_SCENARIOS[i], r);
        INFO("сценарий: " << BARRIER_SCENARIOS[i]);
        REQUIRE(actions(r.sw) == actions(r.table));
        REQUIRE(actions(r.sw) == actions(r.object));
        REQUIRE(states(r.sw) == states(r.table));
        REQUIRE(states(r.sw) == states(r.object));
    }
}

TEST_CASE("Полный цикл пропуска машины", "[07.ThreeWays]") {
    Run r;
    run("card opened passed closed", r);
    REQUIRE(states(r.sw) == "CLOSED OPENING OPEN CLOSING CLOSED");
    REQUIRE(actions(r.sw) == "lamp_on motor_up motor_stop motor_down motor_stop lamp_off");
}

TEST_CASE("Створка закрывается по выдержке, если машина не проехала", "[07.ThreeWays]") {
    Run r;
    run("card opened tick tick", r);
    REQUIRE(states(r.sw) == "CLOSED OPENING OPEN OPEN OPEN");

    Run full;
    run("card opened tick tick tick", full);
    REQUIRE(states(full.sw) == "CLOSED OPENING OPEN OPEN OPEN CLOSING");
}

TEST_CASE("Карта во время закрытия разворачивает створку", "[07.ThreeWays]") {
    Run r;
    run("card opened passed card opened", r);
    REQUIRE(states(r.sw) == "CLOSED OPENING OPEN CLOSING OPENING OPEN");
    /* Реверс не зажигает лампу заново: она горит с момента открытия. */
    REQUIRE(actions(r.sw) == "lamp_on motor_up motor_stop motor_down motor_up motor_stop");
}

TEST_CASE("События вне своего состояния игнорируются", "[07.ThreeWays]") {
    Run r;
    run("opened passed closed tick tick tick", r);
    REQUIRE(states(r.sw) == "CLOSED CLOSED CLOSED CLOSED CLOSED CLOSED CLOSED");
    REQUIRE(actions(r.sw).empty());
}

TEST_CASE("Выдержка отсчитывается заново после каждого открытия", "[07.ThreeWays]") {
    Run r;
    run("card opened tick tick passed closed card opened tick tick", r);
    /* Два такта после второго открытия не должны закрыть створку: счётчик
       обнуляется при входе в OPEN. */
    REQUIRE(r.table_fsm.state == BARRIER_OPEN);
}

TEST_CASE("Набор сценариев покрывает все переходы", "[07.ThreeWays]") {
    unsigned total[BARRIER_STATE_COUNT][BARRIER_EVENT_COUNT] = {{0}};

    for (int i = 0; i < BARRIER_SCENARIO_COUNT; ++i) {
        Run r;
        run(BARRIER_SCENARIOS[i], r);
        for (int s = 0; s < BARRIER_STATE_COUNT; ++s) {
            for (int e = 0; e < BARRIER_EVENT_COUNT; ++e) {
                total[s][e] += r.table_fsm.covered[s][e];
            }
        }
    }

    for (int s = 0; s < BARRIER_STATE_COUNT; ++s) {
        for (int e = 0; e < BARRIER_EVENT_COUNT; ++e) {
            INFO("не покрыт переход " << barrier_state_name(static_cast<BarrierState>(s)) << " по "
                                      << barrier_event_name(static_cast<BarrierEvent>(e)));
            REQUIRE(total[s][e] > 0);
        }
    }
}

TEST_CASE("Одного сценария на полное покрытие не хватает", "[07.ThreeWays]") {
    Run r;
    run("card opened passed closed", r);
    /* Четыре события из двадцати клеток: покрытие переходов — критерий,
       который сам по себе ничего не гарантирует, но и достигается не даром. */
    REQUIRE(barrier_table_coverage(&r.table_fsm) < 50);
}
