# Лекция 7 — один автомат в трёх реализациях: switch, таблица, паттерн State.
project(07-three-ways)

statecraft_library(barrier
        SOURCES barrier_common.c barrier_switch.c barrier_table.c barrier_state.c barrier.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS barrier)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES barrier_tests.cpp
        LIBS barrier)
