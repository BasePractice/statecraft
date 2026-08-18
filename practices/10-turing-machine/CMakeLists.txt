# Лекция 10 — машина Тьюринга: интерпретатор ленты, программы, тесты.
project(10-turing-machine)

statecraft_library(turing_machine
        SOURCES turing_machine.c turing_machine.h mt_programs.c mt_programs.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS turing_machine)

statecraft_example(${PROJECT_NAME}-algorithm
        SOURCES change_sharp_algorithm.c)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES turing_machine_test.cpp mt_programs_test.cpp
        LIBS turing_machine)
