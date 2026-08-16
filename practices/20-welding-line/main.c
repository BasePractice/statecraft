#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "welding.h"

/*
 * Сквозной проект: прогон автомата сварочной линии по сценарию.
 *
 *     20-welding-line                      прогон встроенного сценария
 *     20-welding-line --scenario f.txt     прогон сценария из файла
 *     20-welding-line --dot                граф переходов для Graphviz
 *     20-welding-line --promela            модель для SPIN
 *
 * Сценарий — по событию на строку; пустая строка и '#' игнорируются.
 * Событие TICK означает «внешних событий на этом такте не было».
 */

struct Bench {
    int actuators[WELDING_ACTUATOR_COUNT];
    int tick;
    bool verbose;
};

static const char *ACTUATOR_NAMES[WELDING_ACTUATOR_COUNT]
        = {"конвейер", "зажим", "головка", "горелка", "авария"};

static void bench_set(enum WeldingActuator actuator, bool on, void *userdata) {
    struct Bench *bench = (struct Bench *)userdata;

    bench->actuators[actuator] = on ? 1 : 0;
    if (bench->verbose) {
        printf("        %-10s %s\n", ACTUATOR_NAMES[actuator], on ? "вкл" : "выкл");
    }
}

static void bench_log(const char *message, void *userdata) {
    struct Bench *bench = (struct Bench *)userdata;

    if (bench->verbose)
        printf("        %s\n", message);
}

static enum WeldingEvent parse_event(const char *name) {
    int i;

    for (i = 0; i < WELDING_EV_COUNT; ++i) {
        if (strcmp(welding_event_name((enum WeldingEvent)i), name) == 0)
            return (enum WeldingEvent)i;
    }
    return WELDING_EV_COUNT;
}

/* Встроенный сценарий: включение, два изделия подряд, авария зажима,
   сброс и остановка. Он же используется в тестах покрытия. */
static const char *DEFAULT_SCENARIO[]
        = {"POWER_ON", "OBJECT", "CLAMPED", "ARRIVED", "TICK", "TICK", "TICK",
           "ARRIVED",  "TICK",   "TICK",    "TICK",    "TICK", "TICK", "TICK",
           "RELEASED", "OBJECT", "TICK",    "TICK",    "TICK", "TICK", "TICK",
           "RESET",    "OBJECT", "CLAMPED", "ARRIVED", "TICK", "TICK", "TICK",
           "ARRIVED",  "TICK",   "TICK",    "TICK",    "TICK", "TICK", "TICK",
           "TICK",     "TICK",   "TICK",    "TICK",    "TICK", "TICK", "STOP"};

#define DEFAULT_SCENARIO_LEN ((int)(sizeof(DEFAULT_SCENARIO) / sizeof(DEFAULT_SCENARIO[0])))

static bool run_event(struct WeldingEngine *engine, struct Bench *bench, const char *name) {
    enum WeldingEvent event = parse_event(name);
    enum WeldingState before;
    enum WeldingState after;

    if (event == WELDING_EV_COUNT) {
        fprintf(stderr, "20-welding-line: неизвестное событие «%s»\n", name);
        return false;
    }

    before = engine->state;
    ++bench->tick;
    after = welding_step(engine, event);
    if (before != after) {
        printf("такт %3d: %-9s --%-9s--> %s\n", bench->tick, welding_state_name(before), name,
               welding_state_name(after));
    }
    return true;
}

static int run_scenario(FILE *input) {
    struct WeldingEngine engine;
    struct WeldingHal hal;
    struct Bench bench;
    char line[128];
    int i;

    memset(&bench, 0, sizeof(bench));
    bench.verbose = true;
    hal.set = bench_set;
    hal.log = bench_log;
    hal.userdata = &bench;

    welding_init(&engine, &hal, 2);

    if (input == NULL) {
        for (i = 0; i < DEFAULT_SCENARIO_LEN; ++i) {
            if (!run_event(&engine, &bench, DEFAULT_SCENARIO[i]))
                return EXIT_FAILURE;
        }
    } else {
        while (fgets(line, (int)sizeof(line), input) != NULL) {
            char *end;

            /* Строка длиннее буфера дочитывается до конца: иначе её хвост
               пришёл бы следующим вызовом и был бы принят за событие. */
            if (strchr(line, '\n') == NULL && !feof(input)) {
                int c;
                while ((c = fgetc(input)) != EOF && c != '\n') {
                    /* пропускаем остаток строки */
                }
            }

            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
                continue;
            end = strchr(line, '\n');
            if (end != NULL)
                *end = '\0';
            end = strchr(line, '\r');
            if (end != NULL)
                *end = '\0';
            if (line[0] == '\0')
                continue;
            if (!run_event(&engine, &bench, line))
                return EXIT_FAILURE;
        }
    }

    printf("\nИзделий обработано: %d, аварий: %d, состояние: %s\n", engine.completed, engine.faults,
           welding_state_name(engine.state));
    printf("Покрытие переходов: %d из %d\n", welding_covered_count(&engine),
           welding_transition_count());
    if (welding_covered_count(&engine) < welding_transition_count()) {
        printf("Не сработали:\n");
        welding_print_uncovered(&engine, stdout);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (argc == 1)
        return run_scenario(NULL);

    if (argc == 2 && strcmp(argv[1], "--dot") == 0) {
        welding_print_dot(stdout);
        return EXIT_SUCCESS;
    }
    if (argc == 2 && strcmp(argv[1], "--promela") == 0) {
        welding_print_promela(stdout);
        return EXIT_SUCCESS;
    }
    if (argc == 3 && strcmp(argv[1], "--scenario") == 0) {
        FILE *file = fopen(argv[2], "r");
        int code;

        if (file == NULL) {
            fprintf(stderr, "20-welding-line: не открывается «%s»\n", argv[2]);
            return EXIT_FAILURE;
        }
        code = run_scenario(file);
        fclose(file);
        return code;
    }

    fprintf(stderr, "Использование:\n");
    fprintf(stderr, "  20-welding-line                  встроенный сценарий\n");
    fprintf(stderr, "  20-welding-line --scenario ФАЙЛ  сценарий из файла\n");
    fprintf(stderr, "  20-welding-line --dot            граф переходов\n");
    fprintf(stderr, "  20-welding-line --promela        модель для SPIN\n");
    return EXIT_FAILURE;
}
