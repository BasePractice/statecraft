# Лекция 4 — детерминизация и минимизация.
project(04-dfa-nfa)

statecraft_library(subset SOURCES subset.c subset.h)
target_link_libraries(subset PUBLIC fsm)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS subset)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES subset_tests.cpp
        LIBS subset)
