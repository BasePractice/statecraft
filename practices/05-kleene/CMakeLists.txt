# Лекция 5 — конструкция Томпсона: регулярное выражение в ε-НКА.
project(05-kleene)

statecraft_library(thompson SOURCES thompson.c thompson.h)
target_link_libraries(thompson PUBLIC fsm)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS thompson)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES thompson_tests.cpp
        LIBS thompson)
