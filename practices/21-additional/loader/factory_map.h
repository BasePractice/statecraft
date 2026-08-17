#ifndef STATECRAFT_FACTORY_MAP_H
#define STATECRAFT_FACTORY_MAP_H

/**
 * @file
 * Дополнительный пример: карта цеха, заданная внешней конфигурацией.
 *
 * Практика 21-additional переносит из репозитория c_fsm пример «погрузчик»:
 * автомат едет по разметке на полу цеха, читает метки RFID и по заданию
 * добирается до нужной точки. От остальных примеров курса он отличается тем,
 * что установка описана не кодом, а файлом: топология линий, метки и
 * расстановка стеллажей читаются из JSON.
 *
 * Конфигурация — три слоя одинакового размера:
 *
 *   map    — стены, стеллажи и пол: нужен только для печати схемы;
 *   things — расставленное оборудование: в этой практике не используется,
 *            читается, чтобы конфигурация принималась целиком;
 *   paths  — разметка, по которой едет погрузчик, и есть предмет примера.
 *
 * Кодировка слоя paths взята из c_fsm:
 *
 *   0                       — линии нет;
 *   1 … FACTORY_POINT_MAX   — метка RFID с этим номером;
 *   FACTORY_PATH_H          — горизонтальный участок линии;
 *   FACTORY_PATH_V          — вертикальный участок линии.
 *
 * Различие горизонтальных и вертикальных участков в оригинале объявлено, но
 * ни разу не использовано: поиск пути ходил по любой непустой клетке. Здесь
 * оно работает — по участку можно ехать только вдоль него, а повернуть
 * разрешено лишь на метке (см. route.h). Такое правило не выдумано под
 * реализацию: в конфигурации factory.json метки стоят ровно на пересечениях
 * магистрали с проходами, то есть именно там, где линия поворачивает.
 *
 * Разбор конфигурации — свой, на подмножестве JSON (объект верхнего уровня,
 * строки, массивы массивов целых). Оригинал подключал для этого cJSON (76 КБ
 * стороннего кода); здесь разбор занимает две сотни строк и, кстати, сам
 * является конечным автоматом — тем самым, о котором лекция 6.
 */

#include <stdio.h>

#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Наибольшая сторона карты: защита от заведомо испорченного файла. */
#define FACTORY_MAX_SIDE 256

/** Номера меток RFID занимают 1 … 90, дальше идут коды участков линии. */
#define FACTORY_POINT_MAX 90

/** Горизонтальный и вертикальный участки линии. */
#define FACTORY_PATH_H 91
#define FACTORY_PATH_V 92

/** Слои конфигурации. Порядок совпадает с порядком ключей в файле. */
enum FactoryLayer {
    FACTORY_LAYER_MAP,
    FACTORY_LAYER_THINGS,
    FACTORY_LAYER_PATHS,
    FACTORY_LAYER_COUNT
};

struct FactoryMap {
    char version[16];
    int rows;
    int cols;
    /**
     * Слой хранится построчно, rows * cols значений; NULL, если такого слоя в
     * файле не было. Обязателен только FACTORY_LAYER_PATHS.
     */
    int *layer[FACTORY_LAYER_COUNT];
};

/**
 * Читает конфигурацию из файла. При ошибке возвращает false и пишет в
 * @p error причину с номером строки; @p error допускает NULL.
 *
 * Карта, полученная удачно, освобождается factory_map_destroy().
 */
bool factory_map_read_file(struct FactoryMap *map, const char *file_name, char *error,
                           size_t error_size);

/** То же для конфигурации, уже находящейся в памяти (используется тестами). */
bool factory_map_read_memory(struct FactoryMap *map, const char *text, char *error,
                             size_t error_size);

void factory_map_destroy(struct FactoryMap *map);

/** Значение слоя в клетке; вне карты — 0, то есть «ничего». */
int factory_cell(const struct FactoryMap *map, enum FactoryLayer layer, int row, int col);

/** Есть ли в клетке линия (участок или метка). */
bool factory_is_line(const struct FactoryMap *map, int row, int col);

/**
 * Номер метки в клетке или 0, если метки там нет.
 * Клетки вне карты меток не содержат.
 */
int factory_point_at(const struct FactoryMap *map, int row, int col);

/**
 * Ищет клетку метки @p point. Возвращает false, если такой метки в
 * конфигурации нет; @p row и @p col тогда не меняются.
 */
bool factory_point_cell(const struct FactoryMap *map, int point, int *row, int *col);

/** Сколько меток в конфигурации. */
int factory_point_count(const struct FactoryMap *map);

/**
 * Печатает схему цеха: пол, стеллажи, линии и номера меток.
 * Нужна только человеку, автомат её не использует.
 */
void factory_map_print(const struct FactoryMap *map, FILE *out);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_FACTORY_MAP_H */
