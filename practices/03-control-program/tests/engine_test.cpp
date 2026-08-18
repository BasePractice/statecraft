#include <string>
#include <vector>
#include <catch2/catch.hpp>

extern "C" {
#include <engine.h>
}

/*
  Автомат линии сварки отделён от установки тремя интерфейсами, и это
  единственная причина, по которой его вообще можно проверить: здесь на месте
  железа стоит модель установки, а не запись показаний. Модель отвечает на
  команды — поехал влево, значит координата уменьшилась; опустил аппарат,
  значит сработал нижний концевик, — поэтому тест проверяет управление, а не
  совпадение с однажды снятой трассой.
 */

namespace {

/** Модель установки: рельс, подъёмник, конвейер. */
struct Plant {
    int x;              /* положение аппарата вдоль изделия, шагов от края */
    int y;              /* высота: 0 — вверху, DOWN_LIMIT — внизу */
    int ticks;          /* тактов с начала прогона */
    int power_off_tick; /* когда оператор снимет питание */
    int object_at;      /* такт, когда изделие приходит на конвейер */
    bool point_present; /* видит ли аппарат точку сварки под собой */
    bool line_caught;   /* конвейер заблокирован */
    bool object_gone;   /* изделие уехало */
    int free_tick;      /* такт, на котором конвейер освобождён */
    std::vector<std::string> log; /* протокол выданных команд */
    std::vector<int> welds;       /* координаты, где выполнена сварка */
};

const int DOWN_LIMIT = 3;

Plant plant;

void plant_reset(int power_off_tick, int object_at, bool point_present) {
    plant = Plant();
    plant.power_off_tick = power_off_tick;
    plant.object_at = object_at;
    plant.point_present = point_present;
}

int sensor_value(enum Sensor sensor) {
    switch (sensor) {
    case SENSOR_POWER_OFF:
        return plant.ticks >= plant.power_off_tick;
    case SENSOR_POINT_PRESENT:
        return plant.point_present;
    /* Изделие подъезжает: сначала виден его передний край (D1), затем
       только задний (D2) — по этой паре автомат и понимает, что изделие
       встало на место. */
    case SENSOR_D1:
        return !plant.object_gone && plant.ticks >= plant.object_at
                && plant.ticks < plant.object_at + 2;
    case SENSOR_D2:
        return !plant.object_gone && plant.ticks >= plant.object_at + 2;
    case SENSOR_M1:
        return plant.x <= 0; /* левый концевик */
    case SENSOR_M2:
        return 0;
    case SENSOR_M3:
        return plant.y <= 0; /* аппарат вверху */
    case SENSOR_M4:
        return plant.y >= DOWN_LIMIT; /* аппарат внизу */
    default:
        return 0;
    }
}

const char *sensor_name(enum Sensor sensor) {
    (void)sensor;
    return "sensor";
}

void plant_step(void) { ++plant.ticks; }

void device_step(enum Device device) {
    switch (device) {
    case DEVICE_WELDING_LEFT:
        --plant.x;
        break;
    case DEVICE_WELDING_RIGHT:
        ++plant.x;
        break;
    case DEVICE_WELDING_UP:
        --plant.y;
        break;
    case DEVICE_WELDING_DOWN:
        ++plant.y;
        break;
    case DEVICE_CATCH_LINE:
        plant.line_caught = true;
        plant.log.push_back("catch");
        break;
    case DEVICE_FREE_LINE:
        plant.line_caught = false;
        plant.object_gone = true;
        plant.free_tick = plant.ticks;
        plant.log.push_back("free");
        break;
    case DEVICE_WELDING:
        plant.welds.push_back(plant.x);
        plant.log.push_back("weld");
        break;
    case DEVICE_DROP_OBJECT:
        plant.object_gone = true;
        plant.log.push_back("drop");
        break;
    case DEVICE_POWER_OFF:
        plant.log.push_back("off");
        break;
    default:
        break;
    }
}

void device_print(const char *text) { (void)text; }

/** Прогон автомата на модели с заданными настройками изделия. */
void run(struct PropertyInterface *pi) {
    struct SensorInterface si;
    struct DeviceInterface di;

    si.get_value = sensor_value;
    si.get_name = sensor_name;
    si.simulate_update = plant_step;
    di.do_step = device_step;
    di.print = device_print;
    engine_execute(pi, &si, &di);
}

int steps_center = 10;
int steps_point[3] = {3, -3, -2};

int property_value(enum Property p) {
    switch (p) {
    case VerticalStepsCenter:
        return steps_center;
    case VerticalStepsPoint1:
        return steps_point[0];
    case VerticalStepsPoint2:
        return steps_point[1];
    case VerticalStepsPoint3:
        return steps_point[2];
    }
    return 0;
}

void run_with(int center, int p1, int p2, int p3) {
    struct PropertyInterface pi;
    steps_center = center;
    steps_point[0] = p1;
    steps_point[1] = p2;
    steps_point[2] = p3;
    pi.get_integer = property_value;
    run(&pi);
}

} /* namespace */

TEST_CASE("Снятие питания останавливает автомат", "[03.Control_Program]") {
    plant_reset(/* power_off_tick */ 2, /* object_at */ 100, /* point_present */ true);
    run_with(10, 3, -3, -2);

    REQUIRE(plant.log.back() == "off");
    REQUIRE(plant.welds.empty());
    /* Конвейер не захватывался: изделия не было. */
    REQUIRE(std::count(plant.log.begin(), plant.log.end(), std::string("catch")) == 0);
}

TEST_CASE("Полный цикл: три точки сварены, конвейер освобождён", "[03.Control_Program]") {
    plant_reset(/* power_off_tick */ 400, /* object_at */ 3, /* point_present */ true);
    run_with(10, 3, -3, -2);

    REQUIRE(plant.welds.size() == 3);
    /* Точки отсчитываются от того положения, из которого начат поиск:
       первая — от центра изделия, остальные — от исходного положения, куда
       аппарат возвращается после каждой сварки. */
    REQUIRE(plant.welds == std::vector<int>({13, -3, -5}));
    REQUIRE(plant.log.front() == "catch");
    REQUIRE(std::count(plant.log.begin(), plant.log.end(), std::string("free")) == 1);
    /* Цикл уложился в свои такты: дальше автомат ждёт следующего изделия,
       и прогон заканчивается только снятием питания — это управляющая
       программа, а не задача с ответом. */
    REQUIRE(plant.free_tick > 0);
    REQUIRE(plant.free_tick < 200);
}

TEST_CASE("Точка не найдена: изделие снимается", "[03.Control_Program]") {
    plant_reset(/* power_off_tick */ 200, /* object_at */ 3, /* point_present */ false);
    run_with(10, 3, -3, -2);

    REQUIRE(plant.welds.empty());
    REQUIRE(std::count(plant.log.begin(), plant.log.end(), std::string("drop")) >= 1);
}

TEST_CASE("Точка левее начала поиска находится", "[03.Control_Program]") {
    /* Регрессия: настройка со знаком минус задаёт сторону, а не число шагов.
       Пока расстояние сравнивалось со знаковым значением, счётчик шагов не
       совпадал с ним никогда, и аппарат уезжал влево без предела. */
    plant_reset(/* power_off_tick */ 400, /* object_at */ 3, /* point_present */ true);
    run_with(4, -2, -2, -2);

    REQUIRE(plant.welds.size() == 3);
    REQUIRE(plant.welds.front() == 2);
    /* Главное в этой проверке: поиск точки завершается. До исправления
       аппарат уезжал влево, пока оператор не снимал питание. */
    REQUIRE(plant.free_tick > 0);
    REQUIRE(plant.free_tick < 200);
}
