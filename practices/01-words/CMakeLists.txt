# Лекция 1 — слова, языки и операции над ними.
project(01-words)

statecraft_library(words SOURCES words.c words.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS words)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES words_tests.cpp
        LIBS words)
