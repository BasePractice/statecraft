#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H

/**
 * @file
 * Лекция 3. Входы установки: датчики линии сварки.
 *
 * Автомат управления не читает датчики сам, а обращается к ним через
 * #SensorInterface. Благодаря этому одна и та же логика работает и с
 * настоящей установкой, и с эмулятором — из файла (`FILE_EMULATE`) или по
 * сети (`NETWORK_EMULATE`). Это же требование повторено в поздних
 * практиках: логика отделена от ввода-вывода, иначе её нечем проверять.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Дискретные входы установки; значение ненулевое — датчик сработал. */
enum Sensor {
    SENSOR_POWER_OFF,     /**< тумблер «включено/выключено» */
    SENSOR_POINT_PRESENT, /**< деталь присутствует */

    SENSOR_D1, /**< начало изделия на конвейере */
    SENSOR_D2, /**< окончание изделия на конвейере */
    SENSOR_D3, /**< начало ширины изделия */
    SENSOR_D4, /**< окончание ширины изделия */
    SENSOR_D5, /**< высота изделия */

    SENSOR_M1, /**< концевой выключатель слева */
    SENSOR_M2, /**< концевой выключатель справа */
    SENSOR_M3, /**< концевой выключатель сверху */
    SENSOR_M4, /**< концевой выключатель снизу */

    SENSOR_S1, /**< положение шага сварки */

    LAST_SENSOR /**< число датчиков; в эмуляторе — ещё и выдержка такта */
};

/** Доступ к входам: у настоящей установки и у эмулятора он разный. */
struct SensorInterface {
    /** Текущее значение датчика. */
    int (*get_value)(enum Sensor sensor);

    /** Имя датчика для протокола прогона. */
    const char *(*get_name)(enum Sensor sensor);

    /** Шаг эмуляции: считать следующий набор входов. У железа — пустая. */
    void (*simulate_update)(void);
};

/** Подставляет эмулятор датчиков: чтение из файла или приём по сети. */
void emulator_sensor_init(struct SensorInterface *);

#if defined(__cplusplus)
}
#endif

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H */
