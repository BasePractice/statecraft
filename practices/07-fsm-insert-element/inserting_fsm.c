#include "inserting_fsm.h"

enum InsertingEvent inserting_engine(struct InsertingEngine *engine) {
    switch (engine->state) {
    case INSERTING_A:
        engine->s_1 = -1;
        engine->s_2 = -1;
        engine->state = INSERTING_B;
        return INSERTING_NEXT;
    case INSERTING_B:
        ++engine->s_1;
        ++engine->s_2;

        /*
         * Границы обеих последовательностей проверяются до чтения символов, и
         * три случая покрывают всё: кончились обе, кончилась только вторая,
         * кончилась только первая. В исходной версии (репозиторий c_fsm)
         * третьего случая не было — при m_1 короче m_2 автомат читал
         * m_1[s_1] за границей массива.
         */
        if (engine->s_1 >= engine->m_1_len && engine->s_2 >= engine->m_2_len) {
            engine->state = INSERTING_A;
            return INSERTING_OK_END;
        } else if (engine->s_2 >= engine->m_2_len) {
            /* вторая последовательность кончилась раньше: это не вставка */
            engine->state = INSERTING_A;
            return INSERTING_ERROR_END;
        } else if (engine->s_1 >= engine->m_1_len) {
            /* первая кончилась раньше: остаток второй и есть вставка */
            engine->c_1 = engine->s_1;
            engine->c_2 = engine->m_2_len - 1;
            engine->state = INSERTING_A;
            return INSERTING_DETECT_END;
        }

        if (engine->m_1[engine->s_1] == engine->m_2[engine->s_2]) {
            return INSERTING_NEXT;
        } else if (engine->m_1[engine->s_1] != engine->m_2[engine->s_2]) {
            engine->state = INSERTING_C;
            engine->c_1 = engine->s_1;
            engine->c_2 = engine->s_2;
            return INSERTING_NEXT;
        }
        return INSERTING_ERROR_END;
    case INSERTING_C:
        ++engine->c_2;
        /*
         * Условие самодостаточно: раньше к нему прибавлялось
         * `s_1 < m_1_len`, и при исчерпанной первой последовательности
         * автомат читал m_2[c_2] за границей.
         */
        if (engine->c_2 >= engine->m_2_len) {
            engine->state = INSERTING_A;
            return INSERTING_DETECT_END;
        } else if (engine->m_1[engine->c_1] == engine->m_2[engine->c_2]) {
            engine->state = INSERTING_B;
            engine->s_2 = engine->c_2;
            return INSERTING_DETECT;
        } else if (engine->m_1[engine->c_1] != engine->m_2[engine->c_2]) {
            return INSERTING_NEXT;
        }
        return INSERTING_ERROR_END;
    }
    return INSERTING_ERROR_END;
}

bool inserting_init(struct InsertingEngine *engine, char *m_1, size_t m_1_len, char *m_2,
                    size_t m_2_len) {
    if (0 == engine || 0 == m_1 || 0 == m_2)
        return false;
    engine->state = INSERTING_A;
    engine->c_1 = -1;
    engine->c_2 = -1;

    engine->s_1 = -1;
    engine->s_2 = -1;

    engine->m_1 = m_1;
    engine->m_1_len = m_1_len;
    engine->m_2 = m_2;
    engine->m_2_len = m_2_len;
    return true;
}
