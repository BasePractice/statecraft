#ifndef STATECRAFT_BARRIER_H
#define STATECRAFT_BARRIER_H

/**
 * @file
 * Лекция 7. Один и тот же управляющий автомат, реализованный тремя способами:
 * вложенный switch, таблица переходов и паттерн State.
 *
 * Объект управления — шлагбаум на въезде. Датчики и исполнительные механизмы
 * спрятаны за функциями barrier_out_*, поэтому все три реализации работают с
 * одним и тем же внешним миром и их трассы сравнимы буква в букву.
 *
 * Автомат:
 *
 *     состояние    card      opened    passed    closed    tick
 *     CLOSED       OPENING   —         —         —         —
 *     OPENING      —         OPEN      —         —         —
 *     OPEN         —         —         CLOSING   —         CLOSING по выдержке
 *     CLOSING      OPENING   —         —         CLOSED    —
 *
 * Прочерк означает, что событие в этом состоянии игнорируется: переход
 * остаётся в том же состоянии и не порождает действий. Игнорирование задано
 * явно — это и есть полнота входного алфавита из лекции.
 *
 * Клетка (OPEN, tick) — единственная со сторожевым условием: такт таймера
 * увеличивает счётчик выдержки и переводит автомат в CLOSING, только когда
 * счётчик дошёл до BARRIER_OPEN_TIMEOUT.
 *
 * Существенный переход — CLOSING по card: машина подъехала, пока створка
 * опускалась. Шлагбаум обязан пойти обратно вверх, а не «доопуститься сначала».
 */

#include <stdio.h>

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Состояния шлагбаума. */
enum BarrierState { BARRIER_CLOSED, BARRIER_OPENING, BARRIER_OPEN, BARRIER_CLOSING };

#define BARRIER_STATE_COUNT 4 /**< состояний: размер таблицы переходов */

/** Входной алфавит: события от датчиков и таймера. */
enum BarrierEvent {
    BARRIER_CARD,         /**< пропуск разрешён контроллером доступа */
    BARRIER_OPENED,       /**< сработал концевик «створка вверху» */
    BARRIER_PASSED,       /**< фотобарьер: машина освободила проезд */
    BARRIER_CLOSED_LIMIT, /**< сработал концевик «створка внизу» */
    BARRIER_TICK          /**< такт таймера: отсчёт выдержки в OPEN */
};

#define BARRIER_EVENT_COUNT 5 /**< событий: ширина таблицы переходов */

/** Действия автомата. Ими описывается всё, что автомат делает с миром. */
enum BarrierAction {
    BARRIER_MOTOR_UP,
    BARRIER_MOTOR_DOWN,
    BARRIER_MOTOR_STOP,
    BARRIER_LAMP_ON,
    BARRIER_LAMP_OFF
};

#define BARRIER_TRACE_MAX 64 /**< длина трассы; дальше поднимается overflow */

/**
 * Трасса выполнения: последовательность действий и состояний.
 *
 * Ради неё все три реализации и написаны — трассы сравниваются в тестах.
 * Расхождение означает, что реализации описывают разные автоматы, хотя
 * задумывались как один.
 */
struct BarrierTrace {
    enum BarrierAction actions[BARRIER_TRACE_MAX];
    size_t action_count;
    enum BarrierState states[BARRIER_TRACE_MAX];
    size_t state_count;
    bool overflow; /**< трасса переполнилась: сравнивать её уже нельзя */
};

void barrier_trace_init(struct BarrierTrace *trace);
const char *barrier_state_name(enum BarrierState state);
const char *barrier_event_name(enum BarrierEvent event);
const char *barrier_action_name(enum BarrierAction action);

/** Разбор имени события: "card", "opened", "passed", "closed", "tick".
    false — имя не распознано. */
bool barrier_event_by_name(const char *name, enum BarrierEvent *event);

/* --- реализация 1: вложенный switch --------------------------------------- */

/** Реализация вложенным switch: как автомат пишут чаще всего. */
struct BarrierSwitch {
    enum BarrierState state;
    int open_ticks; /**< сколько тактов створка уже открыта */
    struct BarrierTrace *trace;
};

void barrier_switch_init(struct BarrierSwitch *fsm, struct BarrierTrace *trace);
void barrier_switch_event(struct BarrierSwitch *fsm, enum BarrierEvent event);

/* --- реализация 2: таблица переходов -------------------------------------- */

/** Реализация таблицей переходов: автомат становится данными. */
struct BarrierTable {
    enum BarrierState state;
    int open_ticks;
    struct BarrierTrace *trace;
    /** Покрытие переходов: сколько раз сработала каждая клетка таблицы.
        Считать покрытие «бесплатно» получается только у этой реализации. */
    unsigned covered[BARRIER_STATE_COUNT][BARRIER_EVENT_COUNT];
};

void barrier_table_init(struct BarrierTable *fsm, struct BarrierTrace *trace);
void barrier_table_event(struct BarrierTable *fsm, enum BarrierEvent event);

/** Доля покрытых клеток таблицы переходов, 0..100 %. */
int barrier_table_coverage(const struct BarrierTable *fsm);

/** Печать непокрытых клеток: пары «состояние, событие». */
void barrier_table_print_uncovered(const struct BarrierTable *fsm, FILE *out);

/* --- реализация 3: паттерн State ------------------------------------------ */

struct BarrierObject;

/**
 * «Состояние-объект»: набор обработчиков. В C++ это был бы класс с
 * виртуальными методами (GoF, State), в C90 — структура указателей на функции.
 */
struct BarrierStateVtbl {
    enum BarrierState id;
    void (*on_enter)(struct BarrierObject *self);
    void (*on_event)(struct BarrierObject *self, enum BarrierEvent event);
};

/** Реализация паттерном State: текущее состояние — указатель на таблицу. */
struct BarrierObject {
    const struct BarrierStateVtbl *state;
    int open_ticks;
    struct BarrierTrace *trace;
};

void barrier_object_init(struct BarrierObject *fsm, struct BarrierTrace *trace);
void barrier_object_event(struct BarrierObject *fsm, enum BarrierEvent event);
enum BarrierState barrier_object_state(const struct BarrierObject *fsm);

/**
 * Набор сценариев, которым покрываются все двадцать клеток таблицы переходов.
 *
 * Один и тот же список используют пример и тесты: набор тестов — тоже часть
 * описания автомата, и держать его в двух местах нельзя.
 */
extern const char *const BARRIER_SCENARIOS[];
extern const int BARRIER_SCENARIO_COUNT;

/** Выдержка в состоянии OPEN: столько тактов до автоматического закрытия,
    если фотобарьер молчит. Общая для всех трёх реализаций. */
#define BARRIER_OPEN_TIMEOUT 3

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_BARRIER_H */
