#ifndef C_PROGRAMMING_PRACTICE_BASE_MACRO_H
#define C_PROGRAMMING_PRACTICE_BASE_MACRO_H

/**
 * @file
 * Связывание по правилам Си при сборке тестов.
 *
 * Тесты курса пишутся на C++ (Catch2), а сам практикум — на ISO C90.
 * Объявление, помеченное #EXTERN_C, получает связывание Си и в той, и в
 * другой трансляции, поэтому компоновщик находит одну и ту же функцию.
 */

/** `extern "C"` в C++ и пустота в Си. */
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#endif /* C_PROGRAMMING_PRACTICE_BASE_MACRO_H */
