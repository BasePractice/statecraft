#include <catch2/catch.hpp>
#include <bulk_plant.h>

#include <set>

/*
 * Проверяется поведение автомата на модели установки, а не совпадение с
 * записанной последовательностью входов. Разница существенна: запись
 * приходится переснимать после каждой правки, а модель отвечает на команды и
 * потому проверяет именно управление.
 */

namespace {

struct Run {
    BulkFsm fsm;
    BulkPlant plant;
    std::set<int> visited_states;
    bool done;
};

/* Прогон рецепта до конца или до исчерпания лимита тактов. */
Run run_recipe(unsigned long recipe, int limit = 400, bool feed_start = true) {
    Run run;
    run.done = false;
    REQUIRE(bulk_fsm_init(&run.fsm, recipe));
    bulk_plant_init(&run.plant);

    for (int i = 0; i < limit && !bulk_fsm_finished(&run.fsm); ++i) {
        if (feed_start) {
            bulk_input_set(&run.fsm, BULK_IN_START_CYCLE, run.fsm.state == BULK_CYCLE_IDLE);
        }
        bulk_fsm_tick(&run.fsm);
        bulk_plant_tick(&run.plant, &run.fsm);
        run.visited_states.insert(static_cast<int>(run.fsm.state));
    }
    run.done = bulk_fsm_finished(&run.fsm);
    return run;
}

} /* namespace */

TEST_CASE("Рецепт разбирается по цифрам", "[08.Bulk]") {
    BulkFsm fsm;

    REQUIRE(bulk_fsm_init(&fsm, 5151UL));
    /* Младшая цифра — первый цикл: 5151 читается как 1, 5, 1, 5. */
    REQUIRE(bulk_recipe_mask(&fsm, 0) == 1);
    REQUIRE(bulk_recipe_mask(&fsm, 1) == 5);
    REQUIRE(bulk_recipe_mask(&fsm, 2) == 1);
    REQUIRE(bulk_recipe_mask(&fsm, 3) == 5);
    REQUIRE(bulk_recipe_mask(&fsm, 4) == 0);
    REQUIRE(bulk_recipe_mask(&fsm, -1) == 0);
}

TEST_CASE("Негодный рецепт отвергается, а не исправляется молча", "[08.Bulk]") {
    BulkFsm fsm;

    /* Цифра 8 требует четвёртого резервуара, которого нет. */
    REQUIRE_FALSE(bulk_fsm_init(&fsm, 8UL));
    REQUIRE_FALSE(bulk_fsm_init(&fsm, 9UL));
    /* Пять цифр — пять малых циклов, а их только четыре. */
    REQUIRE_FALSE(bulk_fsm_init(&fsm, 11111UL));
    REQUIRE(bulk_fsm_init(&fsm, 7777UL));
}

TEST_CASE("Установка выходит в исходное положение сама", "[08.Bulk]") {
    BulkFsm fsm;
    BulkPlant plant;

    REQUIRE(bulk_fsm_init(&fsm, 1UL));
    bulk_plant_init(&plant);
    /* Модель начинает работу у последнего резервуара: где остановились в
       прошлый раз, автомат не знает и знать не должен. */
    REQUIRE(plant.position != BULK_HOME_POSITION);

    for (int i = 0; i < 40 && fsm.state != BULK_CYCLE_IDLE; ++i) {
        bulk_fsm_tick(&fsm);
        bulk_plant_tick(&plant, &fsm);
    }
    REQUIRE(fsm.state == BULK_CYCLE_IDLE);
    REQUIRE(plant.position == BULK_HOME_POSITION);
}

TEST_CASE("Рецепт исполняется целиком", "[08.Bulk]") {
    Run run = run_recipe(5151UL);

    REQUIRE(run.done);
    REQUIRE(run.fsm.state == BULK_FINISHED);
    REQUIRE(bulk_output(&run.fsm, BULK_OUT_DONE));

    /*
     * 5151 читается как циклы 1, 5, 1, 5: первый резервуар нужен во всех
     * четырёх циклах, третий — во втором и четвёртом, второй — ни разу.
     */
    REQUIRE(run.plant.filled[0] == 4);
    REQUIRE(run.plant.filled[1] == 0);
    REQUIRE(run.plant.filled[2] == 2);
}

TEST_CASE("Пустой цикл рецепта пропускается", "[08.Bulk]") {
    Run run = run_recipe(1001UL);

    REQUIRE(run.done);
    REQUIRE(run.fsm.state == BULK_FINISHED);
    /* Циклы 1, 0, 0, 1: первый резервуар дважды, остальные — ни разу. */
    REQUIRE(run.plant.filled[0] == 2);
    REQUIRE(run.plant.filled[1] == 0);
    REQUIRE(run.plant.filled[2] == 0);
}

TEST_CASE("Заслонка закрывается по команде, а не сама", "[08.Bulk]") {
    BulkFsm fsm;
    BulkPlant plant;
    bool close_seen = false;
    bool gate_was_open = false;

    REQUIRE(bulk_fsm_init(&fsm, 1UL));
    bulk_plant_init(&plant);
    for (int i = 0; i < 200 && !bulk_fsm_finished(&fsm); ++i) {
        bulk_input_set(&fsm, BULK_IN_START_CYCLE, fsm.state == BULK_CYCLE_IDLE);
        bulk_fsm_tick(&fsm);
        bulk_plant_tick(&plant, &fsm);
        if (bulk_output(&fsm, BULK_OUT_CLOSE_GATE)) {
            close_seen = true;
        }
        if (plant.gate[0] == BULK_GATE_OPEN) {
            gate_was_open = true;
        }
    }
    REQUIRE(gate_was_open);
    /* В оригинале из c_fsm по истечении выдержки подавалась команда «открыть»,
       а сигнал «закрыть» не использовался ни разу. */
    REQUIRE(close_seen);
    REQUIRE(plant.gate[0] == BULK_GATE_CLOSED);
}

TEST_CASE("Все состояния автомата достижимы", "[08.Bulk]") {
    Run run = run_recipe(5151UL);

    /* Кроме аварии: её на исправной установке быть не должно. */
    for (int state = 0; state < BULK_STATE_COUNT; ++state) {
        if (state == BULK_FAULT) {
            REQUIRE(run.visited_states.count(state) == 0);
            continue;
        }
        INFO("состояние: " << bulk_state_name(static_cast<BulkState>(state)));
        REQUIRE(run.visited_states.count(state) == 1);
    }
}

TEST_CASE("Снятие питания останавливает установку", "[08.Bulk]") {
    BulkFsm fsm;
    BulkPlant plant;

    REQUIRE(bulk_fsm_init(&fsm, 5151UL));
    bulk_plant_init(&plant);
    for (int i = 0; i < 30; ++i) {
        bulk_input_set(&fsm, BULK_IN_START_CYCLE, fsm.state == BULK_CYCLE_IDLE);
        bulk_fsm_tick(&fsm);
        bulk_plant_tick(&plant, &fsm);
    }
    REQUIRE_FALSE(bulk_fsm_finished(&fsm));
    REQUIRE(fsm.state != BULK_POWER_ON);

    /* Питание снято. Датчики автомат видит только через такт установки —
       поэтому сначала она, потом он. */
    plant.running = 0;
    bulk_plant_tick(&plant, &fsm);
    bulk_fsm_tick(&fsm);
    REQUIRE(fsm.state == BULK_FAULT);
    /* Все команды сняты: установка не должна продолжать движение. */
    for (int i = 0; i < BULK_OUTPUT_COUNT; ++i) {
        REQUIRE_FALSE(bulk_output(&fsm, static_cast<BulkOutput>(i)));
    }
    REQUIRE(bulk_fsm_finished(&fsm));
}

TEST_CASE("Авария установки переводит автомат в отказ", "[08.Bulk]") {
    BulkFsm fsm;
    BulkPlant plant;

    REQUIRE(bulk_fsm_init(&fsm, 1UL));
    bulk_plant_init(&plant);
    /* Пока автомат не покинул состояние включения, аварию он не
       рассматривает: питания ещё нет, и рассматривать нечего. */
    for (int i = 0; i < 4; ++i) {
        bulk_fsm_tick(&fsm);
        bulk_plant_tick(&plant, &fsm);
    }
    REQUIRE(fsm.state == BULK_HOMING);

    plant.fault = 1;
    bulk_plant_tick(&plant, &fsm);
    bulk_fsm_tick(&fsm);
    REQUIRE(fsm.state == BULK_FAULT);

    /* Из аварии автомат сам не выходит, даже если установка «починилась». */
    plant.fault = 0;
    bulk_plant_tick(&plant, &fsm);
    bulk_fsm_tick(&fsm);
    REQUIRE(fsm.state == BULK_FAULT);
}

TEST_CASE("Без команды оператора цикл не начинается", "[08.Bulk]") {
    Run run = run_recipe(5151UL, 100, false);

    REQUIRE_FALSE(run.done);
    REQUIRE(run.fsm.state == BULK_CYCLE_IDLE);
    REQUIRE(run.plant.filled[0] == 0);
}

TEST_CASE("Имена состояний, входов и выходов заданы", "[08.Bulk]") {
    for (int i = 0; i < BULK_STATE_COUNT; ++i) {
        REQUIRE(std::string(bulk_state_name(static_cast<BulkState>(i))) != "?");
    }
    for (int i = 0; i < BULK_INPUT_COUNT; ++i) {
        REQUIRE(std::string(bulk_input_name(static_cast<BulkInput>(i))) != "?");
    }
    for (int i = 0; i < BULK_OUTPUT_COUNT; ++i) {
        REQUIRE(std::string(bulk_output_name(static_cast<BulkOutput>(i))) != "?");
    }
}
