#ifndef STATECRAFT_BASE_TYPES_H
#define STATECRAFT_BASE_TYPES_H

/*
 * Курс требует ISO C90, в котором нет <stdbool.h> и <stdint.h>: оба
 * заголовка появились в C99. Этот файл даёт те же имена средствами C90 и
 * отходит в сторону, когда компилятор всё-таки C99/C++ — тогда берутся
 * штатные заголовки, и типы совпадают с ожиданиями компоновщика при сборке
 * тестов на C++.
 *
 * Подключать вместо <stdbool.h> и <stdint.h>:
 *     #include "base_types.h"
 */

#if defined(__cplusplus)
#include <cstddef>
#if __cplusplus >= 201103L
#include <cstdint>
#endif
#else
#include <stddef.h>
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#include <stdint.h>
#endif
#endif

/* --- bool ---------------------------------------------------------------- */
#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)

typedef int statecraft_bool;
#define bool statecraft_bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#endif

/* --- целые фиксированной разрядности -------------------------------------- */
#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)

#include <limits.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;

#if USHRT_MAX == 65535U
typedef short int16_t;
typedef unsigned short uint16_t;
#else
#error "нет 16-разрядного целого: перенесите base_types.h на эту платформу"
#endif

#if UINT_MAX == 4294967295U
typedef int int32_t;
typedef unsigned int uint32_t;
#elif ULONG_MAX == 4294967295UL
typedef long int32_t;
typedef unsigned long uint32_t;
#else
#error "нет 32-разрядного целого: перенесите base_types.h на эту платформу"
#endif

/*
 * 64-разрядного целого в C90 может не быть вовсе. Примеры курса им не
 * пользуются; если понадобится — включать под своей проверкой, а не
 * вводить long long, которого в C90 нет.
 */

#endif

/* --- inline --------------------------------------------------------------- */
/*
 * Ключевого слова inline в C90 нет. STATECRAFT_INLINE разворачивается в
 * static для C90 и в static inline там, где inline поддержан.
 */
#if defined(__cplusplus)
#define STATECRAFT_INLINE inline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define STATECRAFT_INLINE static inline
#else
#define STATECRAFT_INLINE static
#endif

#endif /* STATECRAFT_BASE_TYPES_H */
