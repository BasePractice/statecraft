#include <cstring>
#include <catch2/catch.hpp>
#include "mathematics_lang.h"


TEST_CASE("Контекст", "[05.Lexical_Analyze]") {
    struct LexerContext *ctx;

    SECTION("Строковая. Нормальная.") {
        REQUIRE(lexer_init_string(&ctx, "()"));
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }
    SECTION("Строковая. Ошибочная.") {
        REQUIRE_FALSE(lexer_init_string(&ctx, nullptr));
    }
    SECTION("Файловая. Нормальная.") {
        REQUIRE(lexer_init_file(&ctx, "__lexer_source.txt"));
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }
    SECTION("Файловая. Ошибочная.") {
        REQUIRE_FALSE(lexer_init_file(&ctx, nullptr));
        REQUIRE_FALSE(lexer_init_file(&ctx, "UNKNOWN"));
    }
}

TEST_CASE("Разбор", "[05.Lexical_Analyze]") {
    SECTION("Empty") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_string(&ctx, ""));
        REQUIRE_FALSE(lexer_next(ctx, &token));
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }

    SECTION("2 ^ 6") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_string(&ctx, " 2 ^ 6"));
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenPol);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE_FALSE(lexer_next(ctx, &token));
        REQUIRE(token.type == End);
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }

    SECTION("()") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_string(&ctx, "()"));
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenLPar);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenRPar);
        REQUIRE_FALSE(lexer_next(ctx, &token));
        REQUIRE(token.type == End);
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }

    SECTION("   (  ) ") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_string(&ctx, "()"));
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenLPar);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenRPar);
        REQUIRE_FALSE(lexer_next(ctx, &token));
        REQUIRE(token.type == End);
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }

    SECTION("(3 * 8)/6 + 1 - sin(1.5667)") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_string(&ctx, "(3 * 8)/6 + 1 - sin(1.5667)"));
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenLPar);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenMul);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenRPar);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenDiv);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenPlus);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenMinus);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenId);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenLPar);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenReal);
        REQUIRE(strncmp("1.5667", token.p, token.it_end - token.it_start) == 0);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenRPar);
        REQUIRE_FALSE(lexer_next(ctx, &token));
        REQUIRE(token.type == End);
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }

    SECTION("5 * 7 + tg(3.0)") {
        struct LexerContext *ctx = nullptr;
        struct LexerToken token{};
        REQUIRE(lexer_init_file(&ctx, "__lexer_source.txt"));
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE('5' == *token.p);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenMul);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenInt);
        REQUIRE('7' == *token.p);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenPlus);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenId);
        REQUIRE(strncmp("tg", token.p, token.it_end - token.it_start) == 0);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenLPar);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenReal);
        REQUIRE(strncmp("3.0", token.p, token.it_end - token.it_start) == 0);

        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.type == TokenRPar);

        REQUIRE_FALSE(lexer_next(ctx, &token));
        REQUIRE(token.type == End);
        lexer_destroy(&ctx);
        REQUIRE(ctx == 0);
    }
}

TEST_CASE("Длина односимвольной лексемы", "[06.Lexical_Analyze]") {
    /* Регрессия: у односимвольных лексем `it_end` не сдвигался, и длина
       выходила нулевой. Дефект не был виден, пока у практики не появилась
       точка входа, печатающая текст лексемы. */
    struct LexerContext *ctx = 0;
    struct LexerToken token;
    const char *ops = "+-*/^()";
    size_t i;

    REQUIRE(lexer_init_string(&ctx, ops));
    for (i = 0; i < strlen(ops); ++i) {
        INFO("лексема: " << ops[i]);
        REQUIRE(lexer_next(ctx, &token));
        REQUIRE(token.it_end - token.it_start == 1);
        REQUIRE(*token.p == ops[i]);
    }
    REQUIRE_FALSE(lexer_next(ctx, &token));
    REQUIRE(token.type == End);
    lexer_destroy(&ctx);
}

TEST_CASE("Закрывающая скобка не путается с умножением", "[06.Lexical_Analyze]") {
    /* В разборе односимвольных лексем условие «закрывающая скобка» стояло
       дважды, и во второй раз ей приписывался вид TokenMul. Ветвь была
       недостижима, но проверка закрепляет, что вид у скобки один. */
    struct LexerContext *ctx = 0;
    struct LexerToken token;

    REQUIRE(lexer_init_string(&ctx, "(1)"));
    REQUIRE(lexer_next(ctx, &token));
    REQUIRE(token.type == TokenLPar);
    REQUIRE(lexer_next(ctx, &token));
    REQUIRE(token.type == TokenInt);
    REQUIRE(lexer_next(ctx, &token));
    REQUIRE(token.type == TokenRPar);
    lexer_destroy(&ctx);
}
