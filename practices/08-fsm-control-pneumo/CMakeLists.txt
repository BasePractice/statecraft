# Лекция 8 — циклограмма управления пневмоцилиндрами, кодогенерация SimInTech.
project(08-fsm-control-pneumo)

statecraft_library(pneumo_ctrl
        SOURCES pneumo_ctrl.c pneumo_ctrl.h
        DEFINES PNEUMO_DEBUG PNEUMO_SIMULATE)

statecraft_example(${PROJECT_NAME}-main
        SOURCES main.c
        LIBS pneumo_ctrl)

# Сценарий симуляции: шесть входов на такт и комментарий, поясняющий шаг.
# Перенесён из репозитория c_fsm при сверке поведения (ТД-8) — до этого
# практика собиралась, но запустить её было нечем: в каталоге лежат только
# двоичные протоколы SimInTech.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/simulate.pneumo.txt"
        "${CMAKE_CURRENT_BINARY_DIR}/simulate.pneumo.txt" COPYONLY)

if (STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}
            COMMAND ${PROJECT_NAME}-main
            "${CMAKE_CURRENT_SOURCE_DIR}/simulate.pneumo.txt")
endif ()
