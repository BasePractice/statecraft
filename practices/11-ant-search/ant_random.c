#include "ant_random.h"

/*
 * Линейный конгруэнтный датчик из ISO/IEC 9899 (пример к rand()). Взят
 * намеренно: он умещается в несколько строк и воспроизводится на любой
 * платформе.
 */

#define ANT_RANDOM_SCALE 32768

void ant_random_init(struct AntRandom *r, uint32_t seed) {
    r->state = seed;
}

static uint32_t next_raw(struct AntRandom *r) {
    r->state = r->state * 1103515245UL + 12345UL;
    return (r->state >> 16) & 0x7fffUL;
}

int ant_random_below(struct AntRandom *r, int bound) {
    return (int)(next_raw(r) % (uint32_t)bound);
}

bool ant_random_chance(struct AntRandom *r, int percent) {
    return ant_random_below(r, 100) < percent;
}

double ant_random_normal(struct AntRandom *r) {
    double sum = 0.0;
    int i;

    for (i = 0; i < 12; ++i) {
        sum += (double)next_raw(r) / (double)ANT_RANDOM_SCALE;
    }
    return sum - 6.0;
}
