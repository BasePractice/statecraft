# Сквозной проект курса: управление сварочной линией.
# Замыкает лекции 3 (модель таблицей), 7 (реализация), 8 (выдержки и
# таймауты) и 9 (покрытие переходов, модель для SPIN).
project(20-welding-line)

statecraft_library(welding SOURCES welding.c welding.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS welding)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES welding_tests.cpp
        LIBS welding)
