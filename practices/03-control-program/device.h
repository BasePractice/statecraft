#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H

/**
 * @file
 * Лекция 3. Выходы установки: команды исполнительным механизмам.
 *
 * Парная к `sensor.h` половина: там автомат читает входы, здесь выдаёт
 * воздействия. Обе стороны — таблицы функций, поэтому автомат не знает,
 * работает он с установкой или с эмулятором.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/** Команды исполнительным механизмам линии сварки. */
enum Device {
    DEVICE_POWER_OFF,      /**< снять питание */
    DEVICE_CATCH_LINE,     /**< захват конвейера */
    DEVICE_FREE_LINE,      /**< освобождение конвейера */
    DEVICE_POINT_POSITION, /**< отметить положение точки сварки */

    DEVICE_WELDING_UP,    /**< поднять сварочный аппарат */
    DEVICE_WELDING_DOWN,  /**< опустить сварочный аппарат */
    DEVICE_WELDING_LEFT,  /**< сдвинуть влево */
    DEVICE_WELDING_RIGHT, /**< сдвинуть вправо */
    DEVICE_WELDING_NOP,   /**< остановить перемещение */
    DEVICE_WELDING,       /**< выполнить сварку */

    DEVICE_DROP_OBJECT, /**< снять изделие с конвейера */

    MT1, /**< шаговый двигатель конвейера */

    MT2, /**< шаг налево */
    MT3, /**< шаг направо */
    MT4, /**< шаг вверх */
    MT5  /**< шаг вниз */
};

/** Выдача воздействий; у настоящей установки и у эмулятора она разная. */
struct DeviceInterface {
    /** Выполнить команду. */
    void (*do_step)(enum Device device);
    /** Печать протокола прогона: что автомат сделал и почему. */
    void (*print)(const char *text);
};

/** Подставляет эмулятор: команды печатаются, а не выдаются в железо. */
void emulator_device_init(struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H */
