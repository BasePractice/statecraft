# Лекция 8 — циклограмма управления пневмоцилиндрами, кодогенерация SimInTech.
project(08-fsm-control-pneumo)

statecraft_library(pneumo_ctrl
        SOURCES pneumo_ctrl.c pneumo_ctrl.h
        DEFINES PNEUMO_DEBUG PNEUMO_SIMULATE)

statecraft_example(${PROJECT_NAME}-main
        SOURCES main.c
        LIBS pneumo_ctrl)
