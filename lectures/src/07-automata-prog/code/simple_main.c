#include "base_types.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Лекция 7, контрпример: управление, записанное вложенными switch/if.
 * Состояние здесь не выделено явно, поведение размазано по телу цикла —
 * с этим кодом сравнивается автоматная реализация.
 */

enum Direct { MoveLeft, MoveRight, MoveUp, MoveDown };

enum State { MoveCenter, NextState, PowerOff };

static void move_step(enum Direct direct) {
    fprintf(stdout, "Шаг в %d\n", direct);
}

static bool read_sensor_d1(void) {
    return false;
}

int main(void) {
    enum State state = MoveCenter;
    int step_it = 0;

    while (state != PowerOff) {
        bool d1 = read_sensor_d1();
        switch (state) {
        case MoveCenter:
            move_step(MoveLeft);
            ++step_it;
            if (d1 == true) {
                state = NextState;
                step_it = 0;
            } else if (step_it >= 3) {
                state = NextState;
                step_it = 0;
            }
            break;
        case NextState:
            state = PowerOff;
            break;
        case PowerOff:
        default:
            break;
        }
    }

    return EXIT_SUCCESS;
}
