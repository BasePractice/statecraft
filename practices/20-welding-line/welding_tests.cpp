#include <cstring>
#include <catch2/catch.hpp>
#include <welding.h>

/*
 * Тесты проверяют три вещи, каждая из которых обсуждается в лекции 9:
 * поведение на нормальном цикле, поведение на отказе и покрытие переходов.
 *
 * Покрытие состояний достигается коротким сценарием, покрытие переходов —
 * заметно более длинным; именно последнее вскрывает необработанные отказы.
 */

namespace {

struct Bench {
    int actuators[WELDING_ACTUATOR_COUNT];
    int calls;
};

void bench_set(enum WeldingActuator actuator, bool on, void *userdata) {
    Bench *bench = static_cast<Bench *>(userdata);
    bench->actuators[actuator] = on ? 1 : 0;
    ++bench->calls;
}

struct Fixture {
    struct WeldingEngine engine;
    struct WeldingHal hal;
    Bench bench;

    explicit Fixture(int points = 2) {
        std::memset(&bench, 0, sizeof(bench));
        hal.set = bench_set;
        hal.log = NULL;
        hal.userdata = &bench;
        welding_init(&engine, &hal, points);
    }

    enum WeldingState step(enum WeldingEvent event) { return welding_step(&engine, event); }

    void ticks(int count) {
        int i;
        for (i = 0; i < count; ++i) {
            welding_step(&engine, WELDING_EV_TICK);
        }
    }
};

} /* namespace */

TEST_CASE("Начальное состояние и включение", "[20.Welding]") {
    Fixture f;

    REQUIRE(f.engine.state == WELDING_OFF);
    /* До включения линия не реагирует на изделие */
    REQUIRE(f.step(WELDING_EV_OBJECT) == WELDING_OFF);
    REQUIRE(f.step(WELDING_EV_POWER_ON) == WELDING_IDLE);
    REQUIRE(f.bench.actuators[WELDING_CONVEYOR] == 1);
}

TEST_CASE("Полный цикл обработки изделия", "[20.Welding]") {
    Fixture f(2);

    REQUIRE(f.step(WELDING_EV_POWER_ON) == WELDING_IDLE);
    REQUIRE(f.step(WELDING_EV_OBJECT) == WELDING_CLAMP);
    /* конвейер остановлен на время обработки */
    REQUIRE(f.bench.actuators[WELDING_CONVEYOR] == 0);

    REQUIRE(f.step(WELDING_EV_CLAMPED) == WELDING_POSITION);
    REQUIRE(f.step(WELDING_EV_ARRIVED) == WELDING_WELD);
    REQUIRE(f.bench.actuators[WELDING_TORCH] == 1);

    /* выдержка сварки: до её истечения состояние удерживает управление */
    f.ticks(WELDING_WELD_DURATION - 1);
    REQUIRE(f.engine.state == WELDING_WELD);
    REQUIRE(f.step(WELDING_EV_TICK) == WELDING_POSITION);
    REQUIRE(f.engine.point == 1);
    REQUIRE(f.bench.actuators[WELDING_TORCH] == 0);

    /* вторая точка — последняя, после неё изделие освобождается */
    REQUIRE(f.step(WELDING_EV_ARRIVED) == WELDING_WELD);
    f.ticks(WELDING_WELD_DURATION - 1);
    REQUIRE(f.step(WELDING_EV_TICK) == WELDING_RELEASE);
    REQUIRE(f.bench.actuators[WELDING_CLAMP_DRV] == 0);

    REQUIRE(f.step(WELDING_EV_RELEASED) == WELDING_IDLE);
    REQUIRE(f.engine.completed == 1);
    REQUIRE(f.bench.actuators[WELDING_CONVEYOR] == 1);
}

TEST_CASE("Число точек задаётся при инициализации", "[20.Welding]") {
    Fixture f(4);
    int point;

    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    f.step(WELDING_EV_CLAMPED);

    for (point = 0; point < 3; ++point) {
        REQUIRE(f.step(WELDING_EV_ARRIVED) == WELDING_WELD);
        f.ticks(WELDING_WELD_DURATION - 1);
        REQUIRE(f.step(WELDING_EV_TICK) == WELDING_POSITION);
    }
    REQUIRE(f.step(WELDING_EV_ARRIVED) == WELDING_WELD);
    f.ticks(WELDING_WELD_DURATION - 1);
    REQUIRE(f.step(WELDING_EV_TICK) == WELDING_RELEASE);
}

TEST_CASE("Таймаут зажима уводит в аварию", "[20.Welding]") {
    Fixture f;

    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    REQUIRE(f.engine.state == WELDING_CLAMP);

    /* концевик не сработал: до истечения таймаута линия ждёт */
    f.ticks(WELDING_CLAMP_TIMEOUT - 1);
    REQUIRE(f.engine.state == WELDING_CLAMP);

    REQUIRE(f.step(WELDING_EV_TICK) == WELDING_FAULT);
    REQUIRE(f.bench.actuators[WELDING_ALARM] == 1);
    REQUIRE(f.bench.actuators[WELDING_TORCH] == 0);
    REQUIRE(f.engine.faults == 1);
}

TEST_CASE("Таймаут позиционирования уводит в аварию", "[20.Welding]") {
    Fixture f;

    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    f.step(WELDING_EV_CLAMPED);
    REQUIRE(f.engine.state == WELDING_POSITION);

    f.ticks(WELDING_MOVE_TIMEOUT - 1);
    REQUIRE(f.engine.state == WELDING_POSITION);
    REQUIRE(f.step(WELDING_EV_TICK) == WELDING_FAULT);
}

TEST_CASE("Сброс аварии возвращает линию в исходное", "[20.Welding]") {
    Fixture f;

    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    f.ticks(WELDING_CLAMP_TIMEOUT);
    REQUIRE(f.engine.state == WELDING_FAULT);

    REQUIRE(f.step(WELDING_EV_RESET) == WELDING_IDLE);
    REQUIRE(f.bench.actuators[WELDING_ALARM] == 0);
    REQUIRE(f.engine.point == 0);

    /* после сброса линия снова работает */
    REQUIRE(f.step(WELDING_EV_OBJECT) == WELDING_CLAMP);
}

TEST_CASE("Горелка не включается без зажима", "[20.Welding]") {
    Fixture f;

    /* Свойство безопасности: во всех состояниях, кроме сварки, горелка
       выключена. Проверяется прогоном аварийного сценария. */
    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    f.ticks(WELDING_CLAMP_TIMEOUT);
    REQUIRE(f.engine.state == WELDING_FAULT);
    REQUIRE(f.bench.actuators[WELDING_TORCH] == 0);
}

TEST_CASE("Остановка оператором доступна из рабочих состояний", "[20.Welding]") {
    enum WeldingState from[] = {WELDING_IDLE, WELDING_CLAMP, WELDING_POSITION, WELDING_WELD};
    size_t i;

    for (i = 0; i < sizeof(from) / sizeof(from[0]); ++i) {
        Fixture f;

        f.step(WELDING_EV_POWER_ON);
        if (from[i] != WELDING_IDLE)
            f.step(WELDING_EV_OBJECT);
        if (from[i] == WELDING_POSITION || from[i] == WELDING_WELD)
            f.step(WELDING_EV_CLAMPED);
        if (from[i] == WELDING_WELD)
            f.step(WELDING_EV_ARRIVED);

        REQUIRE(f.engine.state == from[i]);
        REQUIRE(f.step(WELDING_EV_STOP) == WELDING_STOPPED);
        REQUIRE(welding_is_done(&f.engine));
        REQUIRE(f.bench.actuators[WELDING_TORCH] == 0);
        REQUIRE(f.bench.actuators[WELDING_CONVEYOR] == 0);
    }
}

TEST_CASE("Покрытие состояний достигается коротким сценарием", "[20.Welding]") {
    Fixture f;

    /* Все состояния, кроме STOPPED, посещаются за один цикл с аварией —
       но переходов при этом сработала едва половина. */
    f.step(WELDING_EV_POWER_ON);
    f.step(WELDING_EV_OBJECT);
    f.step(WELDING_EV_CLAMPED);
    f.step(WELDING_EV_ARRIVED);
    f.ticks(WELDING_WELD_DURATION);
    f.step(WELDING_EV_ARRIVED);
    f.ticks(WELDING_WELD_DURATION);
    f.step(WELDING_EV_RELEASED);

    REQUIRE(welding_covered_count(&f.engine) < welding_transition_count());
}

TEST_CASE("Полное покрытие переходов набирается набором прогонов", "[20.Welding]") {
    Fixture total;

    /* Прогон 1: штатный цикл на два изделия. */
    {
        Fixture f;
        f.step(WELDING_EV_POWER_ON);
        f.step(WELDING_EV_OBJECT);
        f.step(WELDING_EV_CLAMPED);
        f.step(WELDING_EV_ARRIVED);
        f.ticks(WELDING_WELD_DURATION);
        f.step(WELDING_EV_ARRIVED);
        f.ticks(WELDING_WELD_DURATION);
        f.step(WELDING_EV_RELEASED);
        welding_merge_coverage(&total.engine, &f.engine);
    }

    /* Прогон 2: три таймаута — зажим, позиционирование, освобождение — и
       сброс аварии после каждого. */
    {
        Fixture f;
        f.step(WELDING_EV_POWER_ON);

        f.step(WELDING_EV_OBJECT);
        f.ticks(WELDING_CLAMP_TIMEOUT);
        REQUIRE(f.engine.state == WELDING_FAULT);
        f.step(WELDING_EV_RESET);

        f.step(WELDING_EV_OBJECT);
        f.step(WELDING_EV_CLAMPED);
        f.ticks(WELDING_MOVE_TIMEOUT);
        REQUIRE(f.engine.state == WELDING_FAULT);
        f.step(WELDING_EV_RESET);

        f.step(WELDING_EV_OBJECT);
        f.step(WELDING_EV_CLAMPED);
        f.step(WELDING_EV_ARRIVED);
        f.ticks(WELDING_WELD_DURATION);
        f.step(WELDING_EV_ARRIVED);
        f.ticks(WELDING_WELD_DURATION);
        REQUIRE(f.engine.state == WELDING_RELEASE);
        f.ticks(WELDING_RELEASE_TIMEOUT);
        REQUIRE(f.engine.state == WELDING_FAULT);
        welding_merge_coverage(&total.engine, &f.engine);
    }

    /*
     * Прогоны 3–8: остановка оператором из каждого состояния, где она
     * возможна. Каждый требует своего экземпляра: STOPPED — заключительное
     * состояние, и после него автомат больше не работает. Это и есть
     * причина, по которой покрытие переходов считают по набору тестов.
     */
    {
        enum WeldingState from[] = {WELDING_IDLE, WELDING_CLAMP, WELDING_POSITION, WELDING_WELD,
                                    WELDING_RELEASE, WELDING_FAULT};
        size_t i;

        for (i = 0; i < sizeof(from) / sizeof(from[0]); ++i) {
            Fixture f;

            f.step(WELDING_EV_POWER_ON);
            if (from[i] != WELDING_IDLE)
                f.step(WELDING_EV_OBJECT);
            if (from[i] == WELDING_FAULT) {
                f.ticks(WELDING_CLAMP_TIMEOUT);
            } else if (from[i] != WELDING_IDLE && from[i] != WELDING_CLAMP) {
                f.step(WELDING_EV_CLAMPED);
                if (from[i] != WELDING_POSITION) {
                    f.step(WELDING_EV_ARRIVED);
                    if (from[i] == WELDING_RELEASE) {
                        f.ticks(WELDING_WELD_DURATION);
                        f.step(WELDING_EV_ARRIVED);
                        f.ticks(WELDING_WELD_DURATION);
                    }
                }
            }
            REQUIRE(f.engine.state == from[i]);
            f.step(WELDING_EV_STOP);
            REQUIRE(f.engine.state == WELDING_STOPPED);
            welding_merge_coverage(&total.engine, &f.engine);
        }
    }

    INFO("непокрытые переходы печатает welding_print_uncovered");
    REQUIRE(welding_covered_count(&total.engine) == welding_transition_count());
}
