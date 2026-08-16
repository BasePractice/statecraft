#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "welding.h"

/*
 * Автомат задан таблицей переходов: строка таблицы — ребро графа, действие
 * на переходе — функция. Это второй из трёх способов реализации, о которых
 * говорит лекция 7; в отличие от вложенных switch/if, здесь граф автомата
 * присутствует в программе явно и его можно распечатать, проверить и
 * посчитать по нему покрытие.
 */

struct Transition {
    enum WeldingState from;
    enum WeldingEvent event;
    enum WeldingState to;
    void (*action)(struct WeldingEngine *engine);
};

static const char *STATE_NAMES[WELDING_STATE_COUNT]
        = {"OFF", "IDLE", "CLAMP", "POSITION", "WELD", "RELEASE", "FAULT", "STOPPED"};

static const char *EVENT_NAMES[WELDING_EV_COUNT]
        = {"TICK", "POWER_ON", "STOP", "OBJECT", "CLAMPED", "ARRIVED", "RELEASED", "RESET"};

const char *welding_state_name(enum WeldingState state) {
    if (state < 0 || state >= WELDING_STATE_COUNT)
        return "?";
    return STATE_NAMES[state];
}

const char *welding_event_name(enum WeldingEvent event) {
    if (event < 0 || event >= WELDING_EV_COUNT)
        return "?";
    return EVENT_NAMES[event];
}

static void drive(struct WeldingEngine *engine, enum WeldingActuator actuator, bool on) {
    if (engine->hal.set != NULL)
        engine->hal.set(actuator, on, engine->hal.userdata);
}

static void say(struct WeldingEngine *engine, const char *message) {
    if (engine->hal.log != NULL)
        engine->hal.log(message, engine->hal.userdata);
}

/* --- действия на переходах --------------------------------------------- */

static void act_power_on(struct WeldingEngine *engine) {
    drive(engine, WELDING_ALARM, false);
    drive(engine, WELDING_CONVEYOR, true);
    say(engine, "линия включена, конвейер пущен");
}

static void act_stop(struct WeldingEngine *engine) {
    drive(engine, WELDING_CONVEYOR, false);
    drive(engine, WELDING_TORCH, false);
    drive(engine, WELDING_HEAD, false);
    drive(engine, WELDING_CLAMP_DRV, false);
    say(engine, "линия остановлена оператором");
}

static void act_clamp(struct WeldingEngine *engine) {
    drive(engine, WELDING_CONVEYOR, false);
    drive(engine, WELDING_CLAMP_DRV, true);
    engine->point = 0;
    say(engine, "изделие на позиции, зажим сомкнут");
}

static void act_move(struct WeldingEngine *engine) {
    drive(engine, WELDING_CLAMP_DRV, true);
    drive(engine, WELDING_HEAD, true);
    say(engine, "позиционирование головки");
}

static void act_weld(struct WeldingEngine *engine) {
    drive(engine, WELDING_HEAD, false);
    drive(engine, WELDING_TORCH, true);
    say(engine, "сварка точки");
}

static void act_point_done(struct WeldingEngine *engine) {
    drive(engine, WELDING_TORCH, false);
    ++engine->point;
    say(engine, "точка сварена");
}

static void act_release(struct WeldingEngine *engine) {
    drive(engine, WELDING_TORCH, false);
    drive(engine, WELDING_CLAMP_DRV, false);
    say(engine, "все точки сварены, зажим отпускается");
}

static void act_next_object(struct WeldingEngine *engine) {
    drive(engine, WELDING_CONVEYOR, true);
    ++engine->completed;
    say(engine, "изделие выдано, ждём следующее");
}

static void act_fault(struct WeldingEngine *engine) {
    drive(engine, WELDING_CONVEYOR, false);
    drive(engine, WELDING_TORCH, false);
    drive(engine, WELDING_HEAD, false);
    drive(engine, WELDING_ALARM, true);
    ++engine->faults;
    say(engine, "АВАРИЯ: операция не уложилась в отведённое время");
}

static void act_reset(struct WeldingEngine *engine) {
    drive(engine, WELDING_ALARM, false);
    drive(engine, WELDING_CLAMP_DRV, false);
    drive(engine, WELDING_CONVEYOR, true);
    engine->point = 0;
    say(engine, "авария сброшена, линия возвращена в исходное");
}

/*
 * Таблица переходов. WELDING_EV_TICK в строке означает переход, который
 * автомат делает сам: выдержка сварки истекла или сработал таймаут.
 * Порядок строк значим только для чтения — условия ортогональны, то есть
 * пара «состояние, событие» встречается не более одного раза.
 */
static const struct Transition TRANSITIONS[]
        = {{WELDING_OFF, WELDING_EV_POWER_ON, WELDING_IDLE, act_power_on},

           {WELDING_IDLE, WELDING_EV_OBJECT, WELDING_CLAMP, act_clamp},
           {WELDING_IDLE, WELDING_EV_STOP, WELDING_STOPPED, act_stop},

           {WELDING_CLAMP, WELDING_EV_CLAMPED, WELDING_POSITION, act_move},
           {WELDING_CLAMP, WELDING_EV_TICK, WELDING_FAULT, act_fault},
           {WELDING_CLAMP, WELDING_EV_STOP, WELDING_STOPPED, act_stop},

           {WELDING_POSITION, WELDING_EV_ARRIVED, WELDING_WELD, act_weld},
           {WELDING_POSITION, WELDING_EV_TICK, WELDING_FAULT, act_fault},
           {WELDING_POSITION, WELDING_EV_STOP, WELDING_STOPPED, act_stop},

           /* Из сварки автомат уходит сам: по истечении выдержки — к следующей
       точке либо на освобождение изделия. Оба перехода помечены TICK и
       разделены условием «остались ли точки», см. welding_step. */
           {WELDING_WELD, WELDING_EV_TICK, WELDING_POSITION, act_point_done},
           {WELDING_WELD, WELDING_EV_STOP, WELDING_STOPPED, act_stop},

           {WELDING_RELEASE, WELDING_EV_RELEASED, WELDING_IDLE, act_next_object},
           {WELDING_RELEASE, WELDING_EV_TICK, WELDING_FAULT, act_fault},
           {WELDING_RELEASE, WELDING_EV_STOP, WELDING_STOPPED, act_stop},

           {WELDING_FAULT, WELDING_EV_RESET, WELDING_IDLE, act_reset},
           {WELDING_FAULT, WELDING_EV_STOP, WELDING_STOPPED, act_stop}};

#define TRANSITION_COUNT ((int)(sizeof(TRANSITIONS) / sizeof(TRANSITIONS[0])))

int welding_transition_count(void) {
    return TRANSITION_COUNT;
}

void welding_init(struct WeldingEngine *engine, const struct WeldingHal *hal, int points_total) {
    assert(engine != NULL);
    assert(points_total > 0 && points_total <= WELDING_MAX_POINTS);

    memset(engine, 0, sizeof(*engine));
    engine->state = WELDING_OFF;
    engine->points_total = points_total;
    if (hal != NULL)
        engine->hal = *hal;
}

bool welding_is_done(const struct WeldingEngine *engine) {
    assert(engine != NULL);
    return engine->state == WELDING_STOPPED;
}

/* Таймаут или выдержка текущего состояния; 0 — состояние ждёт события
   сколь угодно долго. */
static int state_limit(enum WeldingState state) {
    switch (state) {
    case WELDING_CLAMP:
        return WELDING_CLAMP_TIMEOUT;
    case WELDING_POSITION:
        return WELDING_MOVE_TIMEOUT;
    case WELDING_WELD:
        return WELDING_WELD_DURATION;
    case WELDING_RELEASE:
        return WELDING_RELEASE_TIMEOUT;
    case WELDING_OFF:
    case WELDING_IDLE:
    case WELDING_FAULT:
    case WELDING_STOPPED:
    default:
        return 0;
    }
}

static int find_transition(enum WeldingState state, enum WeldingEvent event) {
    int i;

    for (i = 0; i < TRANSITION_COUNT; ++i) {
        if (TRANSITIONS[i].from == state && TRANSITIONS[i].event == event)
            return i;
    }
    return -1;
}

enum WeldingState welding_step(struct WeldingEngine *engine, enum WeldingEvent event) {
    int index;
    enum WeldingState target;

    assert(engine != NULL);
    ++engine->timer;

    /*
     * Внутреннее событие возникает только по истечении выдержки или
     * таймаута. Без этой проверки TICK срабатывал бы каждый такт, и
     * состояние не удерживало бы управление — ошибка, разобранная в
     * лекции 11 на примере охлаждения.
     */
    if (event == WELDING_EV_TICK) {
        int limit = state_limit(engine->state);
        if (limit == 0 || engine->timer < limit)
            return engine->state;
    }

    index = find_transition(engine->state, event);
    if (index < 0)
        return engine->state; /* событие в этом состоянии не обрабатывается */

    engine->covered[index] = 1;
    target = TRANSITIONS[index].to;

    /*
     * Единственная развилка, которой нет в таблице: после сварки точки
     * автомат идёт либо к следующей точке, либо на освобождение изделия.
     * Развилка вынесена сюда, потому что зависит от данных (счётчика
     * точек), а не от управления.
     */
    if (engine->state == WELDING_WELD && event == WELDING_EV_TICK) {
        if (engine->point + 1 >= engine->points_total)
            target = WELDING_RELEASE;
    }

    if (TRANSITIONS[index].action != NULL)
        TRANSITIONS[index].action(engine);

    if (engine->state == WELDING_WELD && target == WELDING_RELEASE)
        act_release(engine);

    engine->state = target;
    engine->timer = 0;
    return engine->state;
}

int welding_covered_count(const struct WeldingEngine *engine) {
    int i;
    int total = 0;

    assert(engine != NULL);
    for (i = 0; i < TRANSITION_COUNT; ++i) {
        if (engine->covered[i])
            ++total;
    }
    return total;
}

void welding_merge_coverage(struct WeldingEngine *into, const struct WeldingEngine *from) {
    int i;

    assert(into != NULL && from != NULL);
    for (i = 0; i < TRANSITION_COUNT; ++i) {
        if (from->covered[i])
            into->covered[i] = 1;
    }
}

void welding_print_uncovered(const struct WeldingEngine *engine, FILE *out) {
    int i;

    assert(engine != NULL && out != NULL);
    for (i = 0; i < TRANSITION_COUNT; ++i) {
        if (engine->covered[i])
            continue;
        fprintf(out, "  %s --%s--> %s\n", welding_state_name(TRANSITIONS[i].from),
                welding_event_name(TRANSITIONS[i].event), welding_state_name(TRANSITIONS[i].to));
    }
}

void welding_print_dot(FILE *out) {
    int i;

    assert(out != NULL);
    fprintf(out, "digraph welding {\n");
    fprintf(out, "    rankdir=LR;\n");
    fprintf(out, "    node [shape=box, style=rounded];\n");
    fprintf(out, "    FAULT [color=red];\n");
    fprintf(out, "    STOPPED [shape=doublecircle];\n");
    fprintf(out, "    start [shape=point];\n");
    fprintf(out, "    start -> OFF;\n");
    for (i = 0; i < TRANSITION_COUNT; ++i) {
        const char *style = "";

        if (TRANSITIONS[i].to == WELDING_FAULT)
            style = " [color=red, label=\"таймаут\"]";
        if (style[0] == '\0') {
            fprintf(out, "    %s -> %s [label=\"%s\"];\n", welding_state_name(TRANSITIONS[i].from),
                    welding_state_name(TRANSITIONS[i].to),
                    welding_event_name(TRANSITIONS[i].event));
        } else {
            fprintf(out, "    %s -> %s%s;\n", welding_state_name(TRANSITIONS[i].from),
                    welding_state_name(TRANSITIONS[i].to), style);
        }
    }
    /* Развилка по данным, которой нет в таблице переходов. */
    fprintf(out, "    WELD -> RELEASE [label=\"точки кончились\", style=dashed];\n");
    fprintf(out, "}\n");
}

void welding_print_promela(FILE *out) {
    int i;
    int s;

    assert(out != NULL);
    fprintf(out, "/* Модель управления сварочной линией для SPIN.\n");
    fprintf(out, "   Порождено практикой 20-welding-line по таблице переходов.\n\n");
    fprintf(out, "   Абстракция: события выбираются недетерминированно, выдержки и\n");
    fprintf(out, "   счётчик точек не моделируются — проверяются свойства управления,\n");
    fprintf(out, "   а не времени. Проверка:\n\n");
    fprintf(out, "       spin -a welding.pml && gcc -o pan pan.c && ./pan -a\n");
    fprintf(out, "*/\n\n");

    fprintf(out, "mtype = {");
    for (s = 0; s < WELDING_STATE_COUNT; ++s) {
        fprintf(out, "%s%s", (s > 0) ? ", " : "", STATE_NAMES[s]);
    }
    fprintf(out, "};\n\n");
    fprintf(out, "mtype state = OFF;\n\n");

    fprintf(out, "active proctype line() {\n");
    fprintf(out, "    do\n");
    for (i = 0; i < TRANSITION_COUNT; ++i) {
        fprintf(out, "    :: state == %s -> ", welding_state_name(TRANSITIONS[i].from));
        fprintf(out, "atomic { /* %s */ state = %s };\n", welding_event_name(TRANSITIONS[i].event),
                welding_state_name(TRANSITIONS[i].to));
    }
    fprintf(out, "    :: state == WELD -> atomic { /* точки кончились */ state = RELEASE };\n");
    fprintf(out, "    :: state == STOPPED -> break;\n");
    fprintf(out, "    od\n");
    fprintf(out, "}\n\n");

    fprintf(out, "/* После аварии линия обязана вернуться в рабочий режим. */\n");
    fprintf(out,
            "ltl recovery { [] (state == FAULT -> <> (state == IDLE || state == STOPPED)) }\n");
    fprintf(out, "/* Горелка не включается, пока изделие не зажато: свойство\n");
    fprintf(out, "   безопасности проверяется на полной модели с данными. */\n");
    fprintf(out, "ltl reachable_stop { <> (state == STOPPED) }\n");
}
