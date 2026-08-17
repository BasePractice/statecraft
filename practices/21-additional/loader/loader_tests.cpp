/*
 * Тесты планировщика и модели цеха. Система управления написана на Takt и
 * проверяется отдельно — сценарием симулятора (scenario/loader.json) и
 * прогоном драйвера (loader_main.c); здесь проверяется всё, что вокруг неё, и
 * эти тесты работают даже там, где компилятора Takt нет.
 */

#include "catch2/catch.hpp"

extern "C" {
#include "factory_map.h"
#include "loader_plant.h"
#include "route.h"
}

namespace {

struct Config {
    FactoryMap map;

    explicit Config(const char *file_name) {
        char error[256] = {0};
        REQUIRE(factory_map_read_file(&map, file_name, error, sizeof(error)));
    }
    ~Config() { factory_map_destroy(&map); }
};

} /* namespace */

TEST_CASE("конфигурация цеха читается целиком", "[factory]") {
    Config config("factory.json");

    CHECK(std::string(config.map.version) == "1.0.1.0");
    CHECK(config.map.rows == 34);
    CHECK(config.map.cols == 34);
    CHECK(factory_point_count(&config.map) == 18);

    int row = -1;
    int col = -1;
    REQUIRE(factory_point_cell(&config.map, 1, &row, &col));
    CHECK(row == 26);
    CHECK(col == 4);
    REQUIRE(factory_point_cell(&config.map, 10, &row, &col));
    CHECK(row == 7);
    CHECK(col == 8);
}

TEST_CASE("обращение за пределы карты не читает чужую память", "[factory]") {
    Config config("factory.json");

    /* Оригинал (PathLine::search) читал клетку до проверки границ и делал это
       на каждой клетке у края карты. */
    CHECK(factory_cell(&config.map, FACTORY_LAYER_PATHS, -1, 0) == 0);
    CHECK(factory_cell(&config.map, FACTORY_LAYER_PATHS, 0, -1) == 0);
    CHECK(factory_cell(&config.map, FACTORY_LAYER_PATHS, 34, 0) == 0);
    CHECK(factory_cell(&config.map, FACTORY_LAYER_PATHS, 0, 34) == 0);
    CHECK(factory_point_at(&config.map, -5, -5) == 0);
}

TEST_CASE("испорченная конфигурация отвергается с указанием строки", "[factory]") {
    FactoryMap map;
    char error[256];

    SECTION("строки матрицы разной длины") {
        const char *text = "{\n \"paths\": [\n  [1, 91, 2],\n  [0, 0]\n ]\n}";

        CHECK_FALSE(factory_map_read_memory(&map, text, error, sizeof(error)));
        CHECK(std::string(error).find("строка 4") == 0);
    }
    SECTION("нет обязательного слоя") {
        const char *text = "{ \"version\": \"1.0\" }";

        CHECK_FALSE(factory_map_read_memory(&map, text, error, sizeof(error)));
        CHECK(std::string(error).find("paths") != std::string::npos);
    }
    SECTION("незнакомый ключ пропускается целиком") {
        const char *text = "{ \"extra\": {\"a\": [1, 2, {\"b\": null}]},"
                           "  \"paths\": [[1, 91, 2]] }";

        REQUIRE(factory_map_read_memory(&map, text, error, sizeof(error)));
        CHECK(factory_point_count(&map) == 2);
        factory_map_destroy(&map);
    }
}

TEST_CASE("маршрут по цеху идёт от метки к метке", "[route]") {
    Config config("factory.json");
    Route route;

    REQUIRE(route_find(&config.map, 1, 10, ROUTE_RIGHT, 3, &route));
    REQUIRE(route.count == 5);
    const int expected[] = {2, 7, 8, 9, 10};
    for (int i = 0; i < route.count; ++i)
        CHECK(route.step[i].point == expected[i]);
    CHECK(route.step[0].direction == ROUTE_RIGHT);
    CHECK(route.step[1].direction == ROUTE_UP);
    CHECK(route.turns == 1);
    CHECK(route.cells == 23);
}

TEST_CASE("маршрута между метками может не быть", "[route]") {
    Config config("factory.json");
    Route route;

    CHECK_FALSE(route_find(&config.map, 1, 42, ROUTE_UP, 3, &route));
    CHECK_FALSE(route_find(&config.map, 77, 1, ROUTE_UP, 3, &route));

    REQUIRE(route_find(&config.map, 5, 5, ROUTE_UP, 3, &route));
    CHECK(route.count == 0);
    CHECK(route.cells == 0);
}

TEST_CASE("поворот имеет цену, и она меняет маршрут", "[route]") {
    Config config("factory-grid.json");
    Route cheap;
    Route counted;

    /* Решётка 3 на 3: из угла в угол ведут маршруты одной длины, но с разным
       числом поворотов. Поиск в ширину по клеткам, как в оригинале, между ними
       не различает. */
    REQUIRE(route_find(&config.map, 1, 9, ROUTE_RIGHT, 0, &cheap));
    REQUIRE(route_find(&config.map, 1, 9, ROUTE_RIGHT, 5, &counted));
    CHECK(cheap.cells == 16);
    CHECK(counted.cells == 16);
    CHECK(counted.turns == 1);
    CHECK(counted.turns <= cheap.turns);
}

TEST_CASE("план разворачивает маршрут в команды погрузчику", "[plan]") {
    Config config("factory.json");
    Route route;
    Plan plan;

    PlanOptions options;
    plan_options_default(&options);
    options.lift_pallet = 101;
    options.lift_stack = 5001;

    REQUIRE(route_find(&config.map, 1, 10, ROUTE_UP, 3, &route));
    REQUIRE(plan_build(&route, &options, &plan));

    /* Погрузчик стоит носом вверх, а ехать надо вправо: первый же шаг —
       поворот. */
    CHECK(plan.step[0].code == LOADER_CMD_TURN_RIGHT);
    CHECK(plan.step[1].code == LOADER_CMD_DRIVE);
    CHECK(plan.step[1].point == 2);
    CHECK(plan.step[2].code == LOADER_CMD_TURN_LEFT);
    CHECK(plan.step[plan.count - 1].code == LOADER_CMD_LIFT);
    CHECK(plan.step[plan.count - 1].point == 101);
    CHECK(plan.step[plan.count - 1].extra == 5001);

    /* Срок команды считает планировщик — по длине отрезка и паспортной
       скорости машины. Автомат его не вычисляет, а сторожит. */
    LoaderTiming timing;
    loader_timing_default(&timing);
    CHECK(plan.step[1].timeout_ms
          == route.step[0].cells * timing.cell_ms + timing.margin_ms);
    CHECK(plan.step[0].timeout_ms == timing.turn_ms + timing.margin_ms);

    int drives = 0;
    for (int i = 0; i < plan.count; ++i) {
        if (plan.step[i].code == LOADER_CMD_DRIVE)
            ++drives;
    }
    CHECK(drives == route.count);
}

TEST_CASE("разворот собирается из двух поворотов", "[plan]") {
    Config config("factory.json");
    Route route;
    Plan plan;

    REQUIRE(route_find(&config.map, 1, 2, ROUTE_LEFT, 3, &route));
    REQUIRE(plan_build(&route, nullptr, &plan));
    REQUIRE(plan.count == 3);
    CHECK(plan.step[0].code == LOADER_CMD_TURN_RIGHT);
    CHECK(plan.step[1].code == LOADER_CMD_TURN_RIGHT);
    CHECK(plan.step[2].code == LOADER_CMD_DRIVE);
}

TEST_CASE("установка отвечает на команды приводов", "[plant]") {
    Config config("factory.json");
    LoaderPlant plant;
    LoaderCommands commands = {0, 0, 0, 0};
    LoaderSensors sensors;

    REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_RIGHT));
    CHECK(loader_plant_point(&plant) == 1);

    SECTION("газ двигает погрузчик на клетку за время проезда клетки") {
        const int ticks = LOADER_TICKS_FOR(LOADER_CELL_CM * 1000 / LOADER_SPEED_CM_S);

        commands.gas = 1;
        for (int i = 0; i < ticks; ++i)
            loader_plant_tick(&plant, &commands);
        CHECK(plant.cells == 1);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.line == 1);
        CHECK(sensors.odometer == LOADER_CELL_CM);
        CHECK(sensors.motion == 1);
    }

    SECTION("поворот занимает LOADER_TURN_MS миллисекунд") {
        commands.turn_right = 1;
        for (int i = 0; i < LOADER_TICKS_FOR(LOADER_TURN_MS) - 1; ++i)
            loader_plant_tick(&plant, &commands);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.angle == ROUTE_RIGHT);
        loader_plant_tick(&plant, &commands);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.angle == ROUTE_DOWN);
    }

    SECTION("оба поворота сразу установка не исполняет") {
        commands.turn_left = 1;
        commands.turn_right = 1;
        loader_plant_tick(&plant, &commands);
        CHECK(plant.conflicts == 1);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.angle == ROUTE_RIGHT);
    }

    SECTION("сканеры и тензодатчик отзываются по мере подъёма вил") {
        loader_plant_place_pallet(&plant, 1, 5001, 101);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.stack == 5001); /* код места читается сразу */
        CHECK(sensors.pallet == 0);   /* вилы ещё не подошли к паллете */

        commands.fork_up = 1;
        for (int i = 0; i < LOADER_TICKS_FOR(LOADER_FORK_MS); ++i)
            loader_plant_tick(&plant, &commands);
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.pallet == 101);
        CHECK(sensors.load == 1);
    }

    SECTION("дальномер меряет свободный проезд, а не разметку") {
        /* Вверх от метки 1 разметки нет, но пол есть: до стены три клетки.
           Дальномер обязан показать именно это — он видит препятствия, а не
           линию. Вдоль магистрали свободного проезда больше, чем предел
           датчика, и показание упирается в него. */
        REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_UP));
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.range == 3 * LOADER_CELL_CM);

        REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_RIGHT));
        loader_plant_sensors(&plant, &sensors);
        CHECK(sensors.range == LOADER_RANGE_MAX_CM);
    }
}

TEST_CASE("съезд с разметки виден датчику линии", "[plant]") {
    Config config("factory.json");
    LoaderPlant plant;
    LoaderCommands commands = {0, 0, 0, 0};
    LoaderSensors sensors;

    /* Метка 1 стоит на магистрали; вверх от неё разметки нет. */
    REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_UP));
    commands.gas = 1;
    for (int i = 0; i < LOADER_TICKS_FOR(LOADER_CELL_CM * 1000 / LOADER_SPEED_CM_S); ++i)
        loader_plant_tick(&plant, &commands);
    loader_plant_sensors(&plant, &sensors);
    CHECK(sensors.line == 0);
    CHECK(plant.cells == 0);
}

TEST_CASE("план проводит погрузчик по цеху", "[plant][plan]") {
    Config config("factory.json");
    Route route;
    Plan plan;
    LoaderPlant plant;
    LoaderCommands commands = {0, 0, 0, 0};
    LoaderSensors sensors;

    REQUIRE(route_find(&config.map, 1, 10, ROUTE_RIGHT, 3, &route));
    REQUIRE(plan_build(&route, nullptr, &plan));
    REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_RIGHT));

    /*
     * Здесь план исполняет не автомат, а несколько строк прямо в тесте: это
     * проверка согласованности планировщика с физикой установки. Сам автомат
     * (model/loader.takt) проверяется симулятором Takt и драйвером.
     */
    const int LIMIT = 4000;
    int ticks = 0;
    for (int i = 0; i < plan.count; ++i) {
        const PlanStep &step = plan.step[i];

        commands.gas = 0;
        commands.turn_left = 0;
        commands.turn_right = 0;
        if (step.code == LOADER_CMD_TURN_LEFT || step.code == LOADER_CMD_TURN_RIGHT) {
            loader_plant_sensors(&plant, &sensors);
            int wanted = step.code == LOADER_CMD_TURN_RIGHT
                             ? (sensors.angle + 1) % ROUTE_DIRECTION_COUNT
                             : (sensors.angle + 3) % ROUTE_DIRECTION_COUNT;

            commands.turn_left = step.code == LOADER_CMD_TURN_LEFT;
            commands.turn_right = step.code == LOADER_CMD_TURN_RIGHT;
            while (sensors.angle != wanted && ++ticks < LIMIT) {
                loader_plant_tick(&plant, &commands);
                loader_plant_sensors(&plant, &sensors);
            }
            CHECK(sensors.angle == wanted);
        } else if (step.code == LOADER_CMD_DRIVE) {
            commands.gas = 1;
            /* сойти с метки, на которой стоим */
            loader_plant_sensors(&plant, &sensors);
            while (sensors.point != 0 && ++ticks < LIMIT) {
                loader_plant_tick(&plant, &commands);
                loader_plant_sensors(&plant, &sensors);
            }
            while (sensors.point == 0 && sensors.line == 1 && ++ticks < LIMIT) {
                loader_plant_tick(&plant, &commands);
                loader_plant_sensors(&plant, &sensors);
            }
            CHECK(sensors.point == step.point);
        }
    }
    CHECK(ticks < LIMIT);
    CHECK(plant.off_line == 0);
    CHECK(loader_plant_point(&plant) == 10);
    CHECK((int)plant.cells == route.cells);
}

/*
 * Задание — это уже не маршрут, а работа: доехать, взять паллету, отвезти,
 * поставить. Проверяется, что план собирается из двух перегонов и что стенд
 * действительно переносит груз с одного места на другое.
 */
TEST_CASE("задание разворачивается в план из двух перегонов", "[mission]") {
    Config config("factory.json");
    Mission mission;
    Plan plan;

    mission_default(&mission);
    mission.start_point = 1;
    mission.start_direction = ROUTE_RIGHT;
    mission.pick_point = 10;
    mission.pick_stack = 5001;
    mission.pick_pallet = 101;
    mission.place_point = 18;
    mission.place_stack = 5002;

    REQUIRE(mission_plan(&config.map, &mission, nullptr, &plan));

    int lifts = 0;
    int places = 0;
    for (int i = 0; i < plan.count; ++i) {
        if (plan.step[i].code == LOADER_CMD_LIFT)
            ++lifts;
        if (plan.step[i].code == LOADER_CMD_PLACE)
            ++places;
    }
    CHECK(lifts == 1);
    CHECK(places == 1);
    CHECK(plan.step[plan.count - 1].code == LOADER_CMD_PLACE);
    CHECK(plan.step[plan.count - 1].extra == 5002);
}

TEST_CASE("задание читается из файла", "[mission]") {
    Mission mission;
    char error[256] = {0};

    REQUIRE(mission_read_file(&mission, "mission/pick-and-place.json", error, sizeof(error)));
    CHECK(mission.pick_point == 10);
    CHECK(mission.pick_pallet == 101);
    CHECK(mission.place_point == 18);
    CHECK(mission.place_stack == 5002);
    CHECK(mission.jam_after_cells == -1);

    REQUIRE(mission_read_file(&mission, "mission/jam.json", error, sizeof(error)));
    CHECK(mission.jam_after_cells == 30);
}

TEST_CASE("вилы переносят паллету между местами", "[plant]") {
    Config config("factory.json");
    LoaderPlant plant;
    LoaderCommands commands = {0, 0, 0, 0, 0};
    LoaderSensors sensors;
    const int fork_ticks = LOADER_TICKS_FOR(LOADER_FORK_MS);

    REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_RIGHT));
    loader_plant_place_pallet(&plant, 1, 5001, 101);
    loader_plant_add_stack(&plant, 2, 5002);

    commands.fork_up = 1;
    for (int i = 0; i < fork_ticks; ++i)
        loader_plant_tick(&plant, &commands);
    loader_plant_sensors(&plant, &sensors);
    CHECK(loader_plant_carried(&plant) == 101);
    CHECK(sensors.load == 1);

    /* Поставить паллету можно только там, где место свободно: на метке 1 оно
       теперь пустое, но груз ставится там, где стоит погрузчик. */
    commands.fork_up = 0;
    commands.fork_down = 1;
    for (int i = 0; i < fork_ticks; ++i)
        loader_plant_tick(&plant, &commands);
    loader_plant_sensors(&plant, &sensors);
    CHECK(loader_plant_carried(&plant) == 0);
    CHECK(sensors.load == 0);
}

TEST_CASE("заклинивший привод останавливает погрузчик, а датчики это показывают", "[plant]") {
    Config config("factory.json");
    LoaderPlant plant;
    LoaderCommands commands = {0, 0, 0, 0, 0};
    LoaderSensors sensors;

    REQUIRE(loader_plant_init(&plant, &config.map, 1, ROUTE_RIGHT));
    loader_plant_jam_after(&plant, 1);
    commands.gas = 1;
    for (int i = 0; i < 100; ++i)
        loader_plant_tick(&plant, &commands);
    loader_plant_sensors(&plant, &sensors);

    CHECK(plant.cells == 1);      /* дальше одной клетки не уехал */
    CHECK(sensors.motion == 0);   /* акселерометр молчит          */
    CHECK(sensors.line == 1);     /* прочие датчики исправны      */
    CHECK(sensors.odometer == LOADER_CELL_CM);
}
