# Лекция 2 — автомат «задержка» в модели Мили + модель SimInTech.
project(02-fsm-delay)

statecraft_library(delay_fsm SOURCES delay_fsm.c delay_fsm.h)

statecraft_example(${PROJECT_NAME}-delay
        SOURCES delay_main.c
        LIBS delay_fsm)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES main_tests.cpp
        LIBS delay_fsm)
