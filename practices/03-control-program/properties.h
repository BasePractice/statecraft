#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H

/**
 * @file
 * Лекция 3. Настройки установки, отделённые от логики.
 *
 * Автомат не содержит чисел, зависящих от изделия: он спрашивает их через
 * #PropertyInterface. Смена изделия — смена настроек, а не правка кода.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/** Настройки линии сварки: число шагов до точек сварки. */
enum Property {
    /** Шагов от левой крайней точки до правой — ширина изделия в шагах. */
    VerticalStepsCenter,

    VerticalStepsPoint1, /**< шагов до первой точки сварки */
    VerticalStepsPoint2, /**< шагов до второй точки сварки */
    VerticalStepsPoint3  /**< шагов до третьей точки сварки */
};

/** Доступ к настройкам: у эмулятора они зашиты, у установки читались бы с пульта. */
struct PropertyInterface {
    int (*get_integer)(enum Property p);
};

/** Подставляет настройки эмулятора — значения зашиты в код примера. */
void emulator_properties_init(struct PropertyInterface *pi);

#if defined(__cplusplus)
}
#endif

#endif /* C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H */
