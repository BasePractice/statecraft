#include <stdlib.h>
#include <stdio.h>

#define A_0 0
#define A_1 '#'
#define A_2 '$'
#define A_3 '0'
#define A_4 '1'

#define Q_S 0
#define Q_1 1
#define Q_2 2

static int type[] = {A_0, A_1, A_1, A_2, A_3, A_1, A_4, A_4, A_4, A_0};
#define TYPE_SIZE (sizeof(type) / sizeof(type[0]))

static void print_type(void);
static void machine(void);

int main(void) {
    print_type();
    machine();
    print_type();
    return 0;
}

static void machine(void) {
    int current_state = Q_1;
    int type_i = TYPE_SIZE / 2;

    for (;;) {
        if (current_state == Q_S) {
            break;
        }

        if (current_state == Q_1) {
            switch (type[type_i]) {
            case A_1:
            case A_2:
            case A_3:
            case A_4: {
                --type_i;
                break;
            }
            case A_0: {
                current_state = Q_2;
                ++type_i;
                break;
            }
            default:
                break;
            }
        }
        if (current_state == Q_2) {
            switch (type[type_i]) {
            case A_1:
            case A_2: {
                type[type_i] = A_4;
                ++type_i;
                break;
            }
            case A_3:
            case A_4: {
                ++type_i;
                break;
            }
            case A_0: {
                current_state = Q_S;
                break;
            }
            default:
                break;
            }
        }
    }
}

static void print_type(void) {
    int i;

    for (i = 0; i < (int)TYPE_SIZE; ++i) {
        if (type[i] == A_0)
            continue;
        fprintf(stdout, "%c", type[i]);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}
