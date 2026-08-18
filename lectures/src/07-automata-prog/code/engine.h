#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H

/**
 * @file
 * Лекция 3. Автомат управления линией точечной сварки.
 *
 * Прикладной пример к лекциям 3, 7 и 8: изделие приезжает на конвейере,
 * автомат его удерживает, находит точки сварки, сваривает и отпускает
 * ленту. Входы, выходы и настройки автомат получает интерфейсами
 * (`sensor.h`, `device.h`, `properties.h`) и потому не зависит от того,
 * настоящая перед ним установка или эмулятор.
 */

#include <properties.h>
#include <sensor.h>
#include <device.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Состояния автомата управления. */
enum EngineState {
    ENGINE_OFF,   /**< питание снято */
    ENGINE_ON,    /**< включено, идёт инициализация */
    ENGINE_ERROR, /**< авария: дальнейшая работа запрещена */

    /** Ожидание изделия: пока оно не спозиционировано, лента свободна. */
    ENGINE_WAIT_OBJECT,
    ENGINE_BEGIN_WORK,                   /**< лента заблокирована, работа начата */
    ENGINE_POSITION,                     /**< аппарат сводится к центру изделия */
    ENGINE_FIND_NEXT_POINT,              /**< выбор следующей точки сварки */
    ENGINE_FIND_PROCESS,                 /**< поиск точки идёт */
    ENGINE_POINT_NOT_FOUND,              /**< точка не найдена: изделие снимается */
    ENGINE_CONCRETE_NEXT_POINT_POSITION, /**< аппарат сводится над точкой */
    ENGINE_WELDING_DOWN,                 /**< аппарат опускается */
    ENGINE_WELDING_UP,                   /**< аппарат поднимается */
    ENGINE_WELDING,                      /**< идёт сварка */
    ENGINE_START_POSITION,               /**< возврат в исходное положение */
    ENGINE_FREE,                         /**< освобождение конвейера */
    ENGINE_WAIT_FREE_OBJECT,             /**< ожидание, пока изделие уедет */

    ENGINE_MOVE_WELDING_DEVICE /**< перемещение аппарата между точками */
};

/**
 * Прогон автомата до остановки.
 *
 * Функция сама тактирует эмуляцию: на каждом такте опрашивает входы через
 * @p si, принимает решение и выдаёт воздействия через @p di. Числа,
 * зависящие от изделия, берутся из @p pi.
 */
void engine_execute(struct PropertyInterface *pi, struct SensorInterface *si,
                    struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H */
