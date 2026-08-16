# Лекция 3 — синтез автоматной схемы: таблица переходов → булевы функции → код.
project(03-synthesis)

statecraft_library(synthesis
        SOURCES boolean.c boolean.h synthesis.c synthesis.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS synthesis)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES synthesis_tests.cpp
        LIBS synthesis)
