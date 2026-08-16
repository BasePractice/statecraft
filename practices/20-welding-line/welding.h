#ifndef STATECRAFT_WELDING_H
#define STATECRAFT_WELDING_H

/*
 * Сквозной проект курса: управление сварочной линией.
 *
 * Линия работает так. По конвейеру приходит изделие; датчик наличия
 * сообщает о нём. Изделие фиксируется зажимом, сварочная головка
 * позиционируется к очередной точке и варит её с выдержкой. Точек у
 * изделия несколько; когда сварены все, зажим отпускается и изделие
 * уходит. Любая операция ограничена по времени: если концевик не
 * сработал за отведённое число тактов, линия уходит в аварию и ждёт
 * сброса оператором.
 *
 * Проект собирает вместе четыре лекции:
 *
 *   лекция 3 — модель задана таблицей переходов, её можно синтезировать
 *              практикой 03-synthesis;
 *   лекция 7 — реализация таблицей переходов с действиями, а не вложенными
 *              switch/if; ввод-вывод отделён от логики;
 *   лекция 8 — выдержки и таймауты как часть модели, а не как sleep;
 *   лекция 9 — учёт покрытия переходов и экспорт модели на Promela.
 *
 * Ввода-вывода здесь нет: датчики и приводы — это функции, которые
 * подставляет вызывающая сторона (структура WeldingHal). На стенде их
 * реализует драйвер, в тестах — эмулятор.
 */

#include <stdio.h>
#include "base_types.h"

#define WELDING_MAX_POINTS 8      /* точек сварки на изделии */
#define WELDING_CLAMP_TIMEOUT 5   /* тактов на срабатывание зажима */
#define WELDING_MOVE_TIMEOUT 10   /* тактов на позиционирование */
#define WELDING_WELD_DURATION 3   /* тактов на сварку одной точки */
#define WELDING_RELEASE_TIMEOUT 5 /* тактов на освобождение изделия */

#if defined(__cplusplus)
extern "C" {
#endif

enum WeldingState {
    WELDING_OFF,      /* питание снято, начальное состояние */
    WELDING_IDLE,     /* ждём изделие */
    WELDING_CLAMP,    /* зажим изделия */
    WELDING_POSITION, /* позиционирование головки к очередной точке */
    WELDING_WELD,     /* сварка точки, выдержка */
    WELDING_RELEASE,  /* освобождение изделия */
    WELDING_FAULT,    /* авария: операция не уложилась в таймаут */
    WELDING_STOPPED,  /* остановлено оператором, заключительное */
    WELDING_STATE_COUNT
};

enum WeldingEvent {
    WELDING_EV_TICK,     /* такт без внешнего события */
    WELDING_EV_POWER_ON, /* оператор включил линию */
    WELDING_EV_STOP,     /* оператор остановил линию */
    WELDING_EV_OBJECT,   /* датчик: изделие на позиции */
    WELDING_EV_CLAMPED,  /* концевик: зажим сомкнут */
    WELDING_EV_ARRIVED,  /* концевик: головка на точке */
    WELDING_EV_RELEASED, /* концевик: зажим разомкнут */
    WELDING_EV_RESET,    /* оператор сбросил аварию */
    WELDING_EV_COUNT
};

/* Приводы линии. Автомат управляет ими только через эти вызовы. */
enum WeldingActuator {
    WELDING_CONVEYOR,  /* лента конвейера */
    WELDING_CLAMP_DRV, /* привод зажима */
    WELDING_HEAD,      /* привод сварочной головки */
    WELDING_TORCH,     /* сварочная горелка */
    WELDING_ALARM,     /* сигнализация аварии */
    WELDING_ACTUATOR_COUNT
};

struct WeldingHal {
    void (*set)(enum WeldingActuator actuator, bool on, void *userdata);
    void (*log)(const char *message, void *userdata);
    void *userdata;
};

struct WeldingEngine {
    enum WeldingState state;
    int timer;        /* тактов в текущем состоянии */
    int point;        /* номер сваренной точки */
    int points_total; /* точек у изделия */
    int completed;    /* изделий обработано */
    int faults;       /* аварий за прогон */
    struct WeldingHal hal;
    char covered[WELDING_STATE_COUNT * WELDING_EV_COUNT];
};

/* Название состояния и события — для трасс, диаграмм и сообщений. */
const char *welding_state_name(enum WeldingState state);
const char *welding_event_name(enum WeldingEvent event);

void welding_init(struct WeldingEngine *engine, const struct WeldingHal *hal, int points_total);

/*
 * Один такт: обрабатывает событие, при необходимости — выдержки и
 * таймауты. WELDING_EV_TICK означает «внешних событий не было».
 * Возвращает состояние после такта.
 */
enum WeldingState welding_step(struct WeldingEngine *engine, enum WeldingEvent event);

bool welding_is_done(const struct WeldingEngine *engine);

/* --- покрытие переходов (лекция 9) ------------------------------------- */

/* Сколько переходов задано в таблице и сколько из них сработало. */
int welding_transition_count(void);
int welding_covered_count(const struct WeldingEngine *engine);

/* Печать несработавших переходов: то, что осталось не проверено тестами. */
void welding_print_uncovered(const struct WeldingEngine *engine, FILE *out);

/*
 * Объединение покрытия двух прогонов.
 *
 * Одним прогоном полного покрытия переходов не добиться: остановка
 * оператором заканчивает работу, поэтому каждый переход в STOPPED требует
 * своего прогона. Покрытие набирается по набору тестов, а не по одному —
 * именно поэтому в лекции 9 говорится о критерии покрытия для набора.
 */
void welding_merge_coverage(struct WeldingEngine *into, const struct WeldingEngine *from);

/* --- экспорт модели ---------------------------------------------------- */

/* Граф переходов в формате Graphviz. */
void welding_print_dot(FILE *out);

/*
 * Модель управления на Promela: состояния и переходы те же, события
 * выбираются недетерминированно. Проверяется на SPIN (лекция 9).
 */
void welding_print_promela(FILE *out);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_WELDING_H */
