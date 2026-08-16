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
