# Лекция 7 — автомат поиска вставки в последовательности (пример Кузнецова).
project(07-fsm-insert-element)

statecraft_library(inserting_fsm SOURCES inserting_fsm.c inserting_fsm.h)

statecraft_example(${PROJECT_NAME}-main
        SOURCES inserting_main.c
        LIBS inserting_fsm)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES inserting_tests.cpp
        LIBS inserting_fsm)
