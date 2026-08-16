# Лекция 6 — учебный движок регулярных выражений (прямая трансляция определения).
project(06-regular-expression)

statecraft_library(simple_regexp SOURCES simple_regexp.c simple_regexp.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS simple_regexp)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES simple_regexp_tests.cpp
        LIBS simple_regexp)
