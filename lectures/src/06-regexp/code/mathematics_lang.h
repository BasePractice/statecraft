#ifndef MATHEMATICS_LANG_H
#define MATHEMATICS_LANG_H

/**
 * @file
 * Лекция 6. Лексический анализатор арифметических выражений.
 *
 * Показывает, чем занят конечный автомат в настоящем разборе текста:
 * лексер — это ДКА, у которого состояние определяется прочитанным
 * префиксом, а заключительные состояния выдают лексемы. Синтаксического
 * разбора здесь нет намеренно — он уже вне класса регулярных языков.
 *
 * @todo Задания лекции: добавить знак возведения в степень `^` и разбор
 * числа со знаком (`+10`, `-4`, `5 * 7 - tg(0) - -30`).
 */

#include "base_types.h"

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Разновидности лексем. */
enum LexerTokenType {
    TokenLPar,  /**< открывающая скобка */
    TokenRPar,  /**< закрывающая скобка */
    TokenInt,   /**< целое число */
    TokenReal,  /**< вещественное число */
    TokenId,    /**< имя: переменная или функция */
    TokenDiv,   /**< деление */
    TokenMul,   /**< умножение */
    TokenMinus, /**< вычитание */
    TokenPlus,  /**< сложение */
    TokenPol,   /**< возведение в степень */
    End         /**< вход исчерпан */
};

/**
 * Лексема: её вид и место во входном тексте.
 *
 * Текст лексемы не копируется — хранятся указатель на начало разбираемой
 * строки и границы отрезка. Пока жив контекст, жив и текст.
 */
struct LexerToken {
    const char *p;            /**< начало разбираемого текста */
    size_t it_start;          /**< смещение начала лексемы */
    size_t it_end;            /**< смещение за концом лексемы */
    size_t line_no;           /**< номер строки: для сообщений об ошибке */
    enum LexerTokenType type; /**< вид лексемы */
    const char *error;        /**< причина ошибки или NULL */
};

/** Состояние разбора; создаётся `lexer_init_*`, удаляется #lexer_destroy. */
struct LexerContext;

/**
 * Разбор строки в памяти.
 *
 * @param ctx  сюда записывается созданный контекст; освобождать
 *             #lexer_destroy
 * @param text разбираемый текст; он копируется, поэтому может быть временным
 * @return false, если не хватило памяти
 */
bool lexer_init_string(struct LexerContext **ctx, const char *text);

/**
 * Разбор файла: он читается целиком в память.
 *
 * @return false, если файл не открылся, не прочитался или не хватило памяти.
 */
bool lexer_init_file(struct LexerContext **ctx, const char *filename);

/** Освобождает контекст и обнуляет указатель. */
void lexer_destroy(struct LexerContext **ctx);

/**
 * Следующая лексема.
 *
 * @return false, когда вход исчерпан или встречен недопустимый символ; в
 *         последнем случае причина остаётся в поле `error` лексемы.
 */
bool lexer_next(struct LexerContext *ctx, struct LexerToken *token);

bool lexer_eof(struct LexerContext *ctx);

#if defined(__cplusplus)
}
#endif

#endif
