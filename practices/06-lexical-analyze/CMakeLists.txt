# Лекция 6 — лексический анализатор математических выражений.
project(06-lexical-analyze)

statecraft_library(mathematics_lang SOURCES mathematics_lang.c mathematics_lang.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS mathematics_lang)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES mathematics_lang_tests.cpp
        LIBS mathematics_lang
        WORKDIR ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests_data/__lexer_source.txt
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Инструмент командной строки — тоже часть практики, и проверяется он так же:
# на правильном выражении завершается успехом, на недопустимом символе —
# ошибкой. Без этого CLI мог сломаться незаметно (пока он был заглушкой, так и
# было: два дефекта лексера жили в коде именно потому, что смотреть на его
# работу глазами было нечем).
if (STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}-cli
            COMMAND ${PROJECT_NAME} "5 * 7 + tg(3.0) - 2^10")
    add_test(NAME ${PROJECT_NAME}-cli-error
            COMMAND ${PROJECT_NAME} "2 + %")
    set_tests_properties(${PROJECT_NAME}-cli-error PROPERTIES WILL_FAIL TRUE)
endif ()
