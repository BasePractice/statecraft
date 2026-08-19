#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mathematics_lang.h"

/**
 * @file
 * Лекция 6. Разбор выражения на лексемы: инструмент командной строки.
 *
 * Печатает поток лексем — то, что видит синтаксический анализатор, когда
 * работа конечного автомата закончена. Пока у практики не было точки входа,
 * лексер жил только внутри тестов, и посмотреть на его работу глазами
 * было нечем.
 */

static const char *token_name(enum LexerTokenType type) {
    switch (type) {
    case TokenLPar:
        return "LPAR";
    case TokenRPar:
        return "RPAR";
    case TokenInt:
        return "INT";
    case TokenReal:
        return "REAL";
    case TokenId:
        return "ID";
    case TokenDiv:
        return "DIV";
    case TokenMul:
        return "MUL";
    case TokenMinus:
        return "MINUS";
    case TokenPlus:
        return "PLUS";
    case TokenPol:
        return "POW";
    case End:
        return "END";
    }
    return "?";
}

static void usage(void) {
    fprintf(stderr, "Использование: 06-lexical-analyze [--file ФАЙЛ] [ВЫРАЖЕНИЕ ...]\n");
    fprintf(stderr, "  без аргументов разбирается пример: 5 * 7 + tg(3.0)\n");
}

/* Печать одной лексемы: место, вид и сам текст. */
static void print_token(const struct LexerToken *token) {
    size_t length = token->it_end - token->it_start;

    /* token->p указывает на начало самой лексемы, а не на начало текста:
       смещение it_start в него уже включено. */
    printf("%3lu:%-4lu %-6s «%.*s»\n", (unsigned long)token->line_no,
           (unsigned long)token->it_start, token_name(token->type), (int)length, token->p);
}

/* Склейка аргументов в одну строку: выражение удобнее писать без кавычек. */
static bool join_arguments(char *buffer, size_t size, int argc, char **argv) {
    int i;
    size_t used = 0;

    for (i = 0; i < argc; ++i) {
        size_t length = strlen(argv[i]);
        /* Нужно место под пробел, слово и завершающий ноль. Записано
           вычитанием: `used + length + 2` при большом length переполняется,
           и проверка перестаёт защищать (MISRA 10.4 указывает сюда же). */
        if (size < 2U || length > (size - 2U) - used)
            return false;
        if (used > 0U)
            buffer[used++] = ' ';
        memcpy(buffer + used, argv[i], length);
        used += length;
    }
    buffer[used] = '\0';
    return true;
}

int main(int argc, char **argv) {
    struct LexerContext *ctx = NULL;
    struct LexerToken token;
    char expression[1024];
    int count = 0;
    bool ok;

    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        usage();
        return EXIT_SUCCESS;
    }

    if (argc >= 3 && strcmp(argv[1], "--file") == 0) {
        ok = lexer_init_file(&ctx, argv[2]);
        if (!ok) {
            fprintf(stderr, "не удалось прочитать файл «%s»\n", argv[2]);
            return EXIT_FAILURE;
        }
    } else {
        const char *text = "5 * 7 + tg(3.0)";
        if (argc > 1) {
            if (!join_arguments(expression, sizeof(expression), argc - 1, argv + 1)) {
                fprintf(stderr, "выражение длиннее %lu знаков\n",
                        (unsigned long)sizeof(expression) - 1);
                return EXIT_FAILURE;
            }
            text = expression;
        }
        printf("Выражение: %s\n", text);
        ok = lexer_init_string(&ctx, text);
        if (!ok) {
            fprintf(stderr, "не хватило памяти\n");
            return EXIT_FAILURE;
        }
    }

    while (lexer_next(ctx, &token)) {
        print_token(&token);
        ++count;
    }

    /* Разбор прекращается и по концу входа, и по недопустимому символу:
       различает их поле error последней лексемы. */
    if (token.error != NULL) {
        printf("Ошибка в строке %lu, знак %lu: %s\n", (unsigned long)token.line_no,
               (unsigned long)token.it_start, token.error);
        lexer_destroy(&ctx);
        return EXIT_FAILURE;
    }

    printf("Лексем: %d\n", count);
    lexer_destroy(&ctx);
    return EXIT_SUCCESS;
}
