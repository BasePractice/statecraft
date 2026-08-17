# Лекция 6 — учебный движок регулярных выражений и прикладные распознаватели
# форматов (ISBN, парный тег, число), записанные таблицей переходов.
project(06-regular-expression)

statecraft_library(simple_regexp
        SOURCES simple_regexp.c simple_regexp.h
                applied_dfa.c applied_dfa.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS simple_regexp)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES simple_regexp_tests.cpp applied_dfa_tests.cpp
        LIBS simple_regexp)
