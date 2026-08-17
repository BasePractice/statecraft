#ifndef CONDITIONER_TURING_MACHINE_H
#define CONDITIONER_TURING_MACHINE_H

/**
 * @file
 * Лекция 10. Интерпретатор машины Тьюринга.
 *
 * Модель, которой конечный автомат отличается от универсального
 * вычислителя: та же таблица переходов, но добавлена лента, и головка
 * умеет по ней двигаться. Команда машины — пятёрка «читаемый символ,
 * состояние → записываемый символ, состояние, сдвиг».
 *
 * Лента здесь ограничена #TAPE_LIMIT ячейками, то есть строго это не
 * машина Тьюринга, а линейно-ограниченный автомат. Ограничение учебное —
 * снять его предлагается заданием.
 *
 * @todo Задания лекции: печатать имена состояний строкой и выводить
 * переходы в стандартный поток; читать ленту и таблицу переходов из файла;
 * снять ограничение на размер ленты; разрешить произвольный порядок
 * загрузки состояний и алфавита; при желании добавить порты ввода-вывода.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#define TAPE_LIMIT 30    /**< ячеек ленты: учебное ограничение */
#define EMPTY_SYMBOL 'E' /**< пустая ячейка ленты */
#define STOP_STATE '$'   /**< заключительное состояние: машина остановилась */

/** Сдвиг головки после выполнения команды. */
enum Direct { Left, Right, Stay };

/** Состояние машины: алфавит, таблица переходов, лента и положение головки. */
struct Engine;

/**
 * Заводит машину.
 *
 * @param symbols    сколько символов будет в алфавите
 * @param states     сколько состояний будет в таблице
 * @param init_state начальное состояние
 * @return машину; освобождать #engine_destroy. NULL — не хватило памяти.
 */
struct Engine *engine_create(int symbols, int states, char init_state);

void engine_symbol_add(struct Engine *engine, char symbol);

void engine_state_add(struct Engine *engine, char state);

/**
 * Команда машины: пятёрка из таблицы переходов.
 *
 * @param engine   машина
 * @param c_symbol символ под головкой
 * @param c_state  текущее состояние
 * @param symbol   что записать в ячейку
 * @param state    в какое состояние перейти
 * @param direct   куда сдвинуть головку
 */
void engine_reference_add(struct Engine *engine, char c_symbol, char c_state, char symbol,
                          char state, enum Direct direct);

/** Записывает на ленту строку, начиная с ячейки @p offset. */
void engine_tape_copy(struct Engine *engine, int offset, const char *tape);

void engine_tape_set(struct Engine *engine, int offset, char character);

/** Ставит головку в ячейку @p offset. */
void engine_offset_set(struct Engine *engine, int offset);

/** Содержимое ленты, начиная с ячейки @p offset; память принадлежит машине. */
char *engine_type_get(struct Engine *engine, int offset);

/** Освобождает машину и обнуляет указатель. */
void engine_destroy(struct Engine **engine);

/**
 * Прогон до остановки.
 *
 * Машина останавливается, попав в #STOP_STATE или не найдя команды для
 * текущей пары «символ, состояние». Останавливается не всякая машина —
 * это и есть неразрешимость проблемы остановки, о которой лекция.
 */
void machine(struct Engine *engine);

#if defined(__cplusplus)
}
#endif

#endif /* CONDITIONER_TURING_MACHINE_H */
