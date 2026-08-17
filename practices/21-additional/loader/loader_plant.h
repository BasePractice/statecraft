#ifndef STATECRAFT_LOADER_PLANT_H
#define STATECRAFT_LOADER_PLANT_H

/**
 * @file
 * Модель цеха и самого погрузчика: то, чем управляет автомат из
 * model/loader.takt.
 *
 * Автомат не проверяется сам по себе — он читает датчики и выдаёт команды
 * приводам, а верно ли он ими распорядился, видно только по тому, куда уехал
 * погрузчик. Поэтому здесь описана физика: погрузчик едет с известной
 * скоростью, поворот и подъём вил занимают известное время, дальномер видит
 * стену, энкодер считает сантиметры, сканеры читают коды.
 *
 * Разделение обязанностей строгое. Физические величины живут **здесь** и
 * попадают к автомату только показаниями датчиков (@ref LoaderSensors);
 * автомат отвечает командами приводам (@ref LoaderCommands). Ни одна
 * физическая величина не вычисляется автоматом счётом тактов: такт — шаг
 * логики, его частота — свойство платформы, и «двадцать тактов» на другом
 * устройстве означают другое расстояние и другое время.
 *
 * Период такта знает только стенд: он переводит миллисекунды и сантиметры в
 * свои шаги. Модель намеренно грубая — механика сведена к равномерному
 * движению; смысл не в точности, а в том, чтобы у команд были последствия, а
 * у датчиков — физический смысл.
 *
 * Поля структур объявлены целыми, а не bool: тесты практикума собираются
 * компилятором C++, где bool занимает байт, а код курса — как C90, где bool из
 * base_types.h это int. Структура с bool не последним полем получила бы в двух
 * трансляциях разную раскладку.
 */

#include "base_types.h"
#include "factory_map.h"
#include "route.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Период опроса: столько физического времени проходит за один вызов
 * loader_plant_tick(). Автомат этого числа не знает и знать не должен.
 */
#define LOADER_TICK_MS 50

/** Геометрия и скорости установки. */
#define LOADER_CELL_CM 50       /* клетка карты — полметра пола     */
#define LOADER_SPEED_CM_S 100   /* маршевая скорость, см/с          */
#define LOADER_TURN_MS 600      /* поворот на 90°                   */
#define LOADER_FORK_MS 800      /* подъём вил до захвата паллеты    */
#define LOADER_RANGE_MAX_CM 250 /* дальше дальномер не видит       */

/** Сколько мест хранения помнит стенд. */
#define LOADER_STACK_COUNT 4

/** Сколько раз надо тикнуть стендом, чтобы прошло @p ms миллисекунд. */
#define LOADER_TICKS_FOR(ms) ((ms) / LOADER_TICK_MS)

/** Команды приводам — то, что выдаёт автомат за такт. */
struct LoaderCommands {
    int gas;
    int turn_left;
    int turn_right;
    int fork_up;
    int fork_down;
};

/** Показания датчиков — то, что автомат читает на следующем такте. */
struct LoaderSensors {
    int line;     /* разметка под погрузчиком */
    int point;    /* метка RFID под считывателем, 0 — метки нет */
    int angle;    /* направление, enum RouteDirection */
    int odometer; /* энкодер: пройденный путь, см */
    int range;    /* дальномер: до препятствия впереди, см */
    int motion;   /* акселерометр: погрузчик действительно едет */
    int stack;    /* сканер места: код штабеля, 0 — не прочитан */
    int pallet;   /* сканер вил: код паллеты, 0 — не прочитан */
    int load;     /* тензодатчик вил: груз принят */
};

struct LoaderPlant {
    const struct FactoryMap *map;
    int row;
    int col;
    int angle;

    int travel_cm;       /* сколько пройдено в текущей клетке */
    int odometer_cm;     /* энкодер: путь с момента включения */
    int turn_elapsed_ms; /* сколько длится текущий поворот */
    int fork_elapsed_ms; /* сколько длится текущий подъём вил */
    int moved_this_tick; /* на этом такте погрузчик сдвинулся */

    /*
     * Места хранения: у каждого свой код, читаемый сканером, и паллета, если
     * она там стоит. Два места — этого хватает на «взять здесь, поставить
     * там»; больше в учебном примере не нужно.
     */
    int stack_point[LOADER_STACK_COUNT];
    int stack_code[LOADER_STACK_COUNT];
    int stack_pallet[LOADER_STACK_COUNT]; /* 0 — место свободно */

    int carried_pallet; /* код паллеты на вилах, 0 — вилы пусты */

    /*
     * Заклинивание привода: с этой клетки погрузчик перестаёт двигаться, хотя
     * газ подан. Датчики честно показывают, что движения нет, и отказ обязан
     * поймать сторожевой таймер команды. Значение -1 — исправная машина.
     */
    int jam_after_cells;

    /*
     * Метка, у которой проход перегорожен: чужой погрузчик, упавшая паллета,
     * ремонтная зона. Дальномер видит препятствие заранее, а въехать в него
     * нельзя — как и в жизни.
     */
    int blocked_point;

    /** Погрузчик потерял разметку: дальше ехать некуда. */
    int off_line;

    /** Автомат выдал взаимоисключающие команды (оба поворота сразу). */
    int conflicts;

    unsigned long ticks;    /* сколько раз стенд был протикан */
    unsigned long clock_ms; /* модельное время установки */
    unsigned long cells;    /* сколько клеток проехал погрузчик */
};

/**
 * Ставит погрузчик на метку @p point лицом в @p direction.
 * Возвращает false, если такой метки в конфигурации нет.
 */
bool loader_plant_init(struct LoaderPlant *plant, const struct FactoryMap *map, int point,
                       int direction);

/**
 * Ставит у метки @p point паллету: @p stack_code читает сканер места,
 * @p pallet_code — сканер на вилах. Без этого вилы поднимать не у чего, и
 * команда «взять паллету» закончится аварией — как и должна.
 */
void loader_plant_place_pallet(struct LoaderPlant *plant, int point, int stack_code,
                               int pallet_code);

/**
 * Заводит место хранения без паллеты: сканер прочитает код, но брать там
 * нечего. Сюда груз ставят командой «поставить».
 */
void loader_plant_add_stack(struct LoaderPlant *plant, int point, int stack_code);

/**
 * Привод заклинивает после @p cells пройденных клеток: газ подан, а погрузчик
 * стоит. Так проверяется сторожевой таймер команды — отказ, при котором все
 * прочие датчики молчат.
 */
void loader_plant_jam_after(struct LoaderPlant *plant, int cells);

/**
 * Перегораживает проход в клетке метки @p point. Дальномер начнёт видеть
 * препятствие за LOADER_RANGE_MAX_CM, а система управления обязана встать
 * раньше, чем упрётся: порог — STOP_RANGE в модели.
 */
void loader_plant_block(struct LoaderPlant *plant, int point);

/** Код паллеты на вилах или 0. */
int loader_plant_carried(const struct LoaderPlant *plant);

/** Один такт установки: команды исполняются, физика двигается. */
void loader_plant_tick(struct LoaderPlant *plant, const struct LoaderCommands *commands);

/** Показания датчиков после такта. */
void loader_plant_sensors(const struct LoaderPlant *plant, struct LoaderSensors *sensors);

/** Метка, на которой стоит погрузчик, или 0. */
int loader_plant_point(const struct LoaderPlant *plant);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_LOADER_PLANT_H */
