#ifndef LITTLE_COURSE_PNEUMO_CTRL_H
#define LITTLE_COURSE_PNEUMO_CTRL_H

/**
 * @file
 * Лекция 8. Управление двумя пневмоцилиндрами по циклограмме.
 *
 * Цилиндры Y1 и Y2 ходят вверх-вниз в заданном порядке; каждый шаг
 * циклограммы — состояние автомата с собственной выдержкой и предельным
 * временем ожидания датчика. Не дождались — авария
 * (#PneumoState_FatalException), а не бесконечное ожидание.
 *
 * Рядом лежит модель SimInTech и порождённый ею код
 * (`generated/`), поэтому пример показывает и кодогенерацию: одна и та же
 * циклограмма нарисована в среде и написана руками.
 *
 * Порождённые файлы приведены как есть, кроме двух правок, сделанных
 * 18.08.2026: `PneumoAutomate.inc` и `.log` перекодированы из CP1251 в
 * UTF-8, а в `PneumoAutomate.h` абсолютные пути машины сборки
 * (`E:\GitHub\automata_programming\…`) заменены относительными — иначе
 * файл ссылается на каталог, которого нет ни у кого, включая автора.
 * В сборку `generated/` не входит: это иллюстрация, а не исходник.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Шаги циклограммы; последний — аварийное состояние. */
enum PneumoState {
    PneumoState_Init = 0,
    PneumoState_1,
    PneumoState_2,
    PneumoState_3,
    PneumoState_4,
    PneumoState_5,
    PneumoState_6,
    PneumoState_7,
    PneumoState_8,
    PneumoState_9,
    PneumoState_FatalException /**< датчик не ответил за отведённое время */
};

#define PNEUMO_CYLINDER_SIGNAL_UP 0   /**< датчик «шток вверху» */
#define PNEUMO_CYLINDER_SIGNAL_DOWN 1 /**< датчик «шток внизу» */

/** Пневмоцилиндр: два концевых датчика и команда управления. */
struct PneumoCylinder {
    int input_signal[2]; /**< показания датчиков, см. PNEUMO_CYLINDER_SIGNAL_* */
    int output_signal;   /**< команда: куда гнать шток */
};

#define PNEUMO_CYLINDER_Y1 0 /**< первый цилиндр */
#define PNEUMO_CYLINDER_Y2 1 /**< второй цилиндр */

/** Состояние управления: шаг циклограммы, таймеры и сами цилиндры. */
struct PneumoEngine {
    enum PneumoState state;
    int timeout; /**< тактов до аварии на текущем шаге */
    int delay;   /**< тактов выдержки на текущем шаге */
    /** Предельное время ожидания датчика для каждого шага. */
    int timeouts[PneumoState_FatalException];
    /** Выдержка для каждого шага. */
    int delays[PneumoState_FatalException];
    struct PneumoCylinder cylinders[2];
};

void pneumo_engine_init(struct PneumoEngine *engine);

/**
 * Один такт циклограммы.
 *
 * @return false, когда работа окончена: цикл исполнен или произошла авария.
 */
bool pneumo_engine_tick(struct PneumoEngine *engine);

void pneumo_engine_destroy(struct PneumoEngine *engine);

#if defined(__cplusplus)
}
#endif

#endif /* LITTLE_COURSE_PNEUMO_CTRL_H */
