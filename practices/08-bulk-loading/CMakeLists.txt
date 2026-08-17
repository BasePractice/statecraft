# Лекция 8 — загрузка сыпучих материалов: автомат управления промышленного
# размера и модель установки, на которой он проверяется.
project(08-bulk-loading)

statecraft_library(bulk
        SOURCES bulk_fsm.c bulk_fsm.h
                bulk_plant.c bulk_plant.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS bulk)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES bulk_tests.cpp
        LIBS bulk)
