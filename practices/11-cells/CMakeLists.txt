# Лекция 11 — клеточные автоматы: правила Вольфрама, «Жизнь», муравей Лэнгтона.
project(11-cells)

statecraft_library(cells
        SOURCES cells.c cells.h
                ant_trail.c ant_trail.h
                render.c render.h)

# Векторный вывод (render.c) считает координаты через cos, sin и sqrt. На
# macOS они лежат в libc, а на Linux — в отдельной libm, и без явной
# линковки сборка падает на этапе компоновки. Нашлось в CI: на машине
# разработчика этого не видно.
find_library(STATECRAFT_M_LIBRARY m)
if (STATECRAFT_M_LIBRARY)
    target_link_libraries(cells PUBLIC "${STATECRAFT_M_LIBRARY}")
endif ()

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS cells)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES cells_tests.cpp
        LIBS cells)
