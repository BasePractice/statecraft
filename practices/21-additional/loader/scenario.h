#ifndef STATECRAFT_LOADER_SCENARIO_H
#define STATECRAFT_LOADER_SCENARIO_H

/**
 * @file
 * Чтение сценария входных портов — того же файла, который прогоняет
 * `takt-sim` (scenario/loader.json).
 *
 * Зачем это графическому приложению. Обычный прогон подаёт автомату показания
 * модели цеха: погрузчик едет, датчики отвечают. Сценарий — противоположный
 * режим: значения портов заданы вручную по тактам, в том числе такие, которых
 * от исправной установки не дождёшься (чужая метка, пропавшая линия,
 * препятствие вплотную). Это отладочный стенд, и он должен читать ровно тот
 * файл, которым модель проверяется в тестах, — иначе разбор аварии в
 * приложении и проверка в CI разойдутся.
 *
 * Разбирается подмножество формата: массив шагов, у каждого объект `in_ports`
 * с числовыми значениями. Поля `guard`, `_step`, `time_ms` и прочие
 * пропускаются: их дело — симулятор.
 */

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Входные порты модели в том же порядке, в каком их показывает приложение. */
enum LoaderInPort {
    LOADER_IN_CMD_VALID,
    LOADER_IN_CMD_CODE,
    LOADER_IN_CMD_POINT,
    LOADER_IN_CMD_EXTRA,
    LOADER_IN_CMD_TIMEOUT,
    LOADER_IN_SENSE_LINE,
    LOADER_IN_SENSE_POINT,
    LOADER_IN_SENSE_ANGLE,
    LOADER_IN_SENSE_ODOMETER,
    LOADER_IN_SENSE_RANGE,
    LOADER_IN_SENSE_MOTION,
    LOADER_IN_SENSE_STACK,
    LOADER_IN_SENSE_PALLET,
    LOADER_IN_SENSE_LOAD,
    LOADER_IN_RESET,
    LOADER_IN_COUNT
};

struct ScenarioStep {
    int value[LOADER_IN_COUNT];
};

struct Scenario {
    struct ScenarioStep *step;
    int count;
};

/** Имя порта, как оно записано в сценарии и в модели. */
const char *loader_in_port_name(int port);

/**
 * Читает сценарий. При ошибке возвращает false и пишет причину в @p error
 * (допускает NULL). Удачно прочитанный сценарий освобождается
 * scenario_destroy().
 */
bool scenario_read_file(struct Scenario *scenario, const char *file_name, char *error,
                        size_t error_size);

void scenario_destroy(struct Scenario *scenario);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_LOADER_SCENARIO_H */
