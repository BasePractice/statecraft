#include "base_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pneumo_ctrl.h"

int main(int argc, char **argv) {
    FILE *fd;
    if (argc < 2) {
        fprintf(stderr, "Не передан файл симуляции\n");
        return EXIT_FAILURE;
    }
    fd = fopen(argv[1], "rt");
    if (0 != fd) {
        struct PneumoEngine engine;
        bool running = true;

        memset(&engine, 0, sizeof(engine));
        pneumo_engine_init(&engine);
        while (running) {
            int eq_output[2];
            if (feof(fd)) {
                fprintf(stdout, "Завершение файла симуляции\n");
                running = false;
                continue;
            }
            {
                struct PneumoCylinder *y1 = &engine.cylinders[PNEUMO_CYLINDER_Y1];
                struct PneumoCylinder *y2 = &engine.cylinders[PNEUMO_CYLINDER_Y2];
                char line[256];
                char *hash;
                int parsed = 0;

                /*
                 * Строка читается целиком, а не сразу шестью числами: в
                 * сценарии за числами идёт комментарий, поясняющий такт
                 * («# Y1 пошёл вверх»). Без этого разбор спотыкался о решётку
                 * и останавливал прогон.
                 *
                 * Прочитаны должны быть все шесть значений: при неполной
                 * строке сигналы остались бы от предыдущего такта, и автомат
                 * пошёл бы по чужим входам — молча.
                 */
                while (parsed == 0 && fgets(line, (int)sizeof(line), fd) != NULL) {
                    hash = strchr(line, '#');
                    if (hash != NULL)
                        *hash = '\0';
                    parsed = sscanf(line, "%d %d %d %d %d %d",
                                    (int *)&y1->input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
                                    (int *)&y1->input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
                                    (int *)&y2->input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
                                    (int *)&y2->input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
                                    (int *)&eq_output[PNEUMO_CYLINDER_Y1],
                                    (int *)&eq_output[PNEUMO_CYLINDER_Y2]);
                    /* Пустая строка или строка из одного комментария — не
                       ошибка: читаем следующую. */
                    if (parsed == 0 || parsed == EOF)
                        parsed = 0;
                    else if (parsed != 6) {
                        fprintf(stderr, "Строка файла симуляции прочитана не полностью\n");
                        return EXIT_FAILURE;
                    }
                }
                if (parsed != 6) {
                    fprintf(stdout, "Завершение файла симуляции\n");
                    running = false;
                    continue;
                }
            }
            running = pneumo_engine_tick(&engine);
            if (!running) {
                fprintf(stderr, "Остановка из-за критической ошибки автомата\n");
            }
        }
        pneumo_engine_destroy(&engine);
    }
    if (0 != fd) {
        fclose(fd);
    }
    return EXIT_SUCCESS;
}
