# Лекция 11 — клеточные автоматы: правила Вольфрама, «Жизнь», муравей Лэнгтона.
project(11-cells)

statecraft_library(cells SOURCES cells.c cells.h ant_trail.c ant_trail.h)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS cells)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES cells_tests.cpp
        LIBS cells)
