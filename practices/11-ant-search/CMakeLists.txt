# Лекция 11 — поиск автомата для задачи об умном муравье.
#
# Тропа, прогон и готовые стратегии берутся из практики 11-cells (библиотека
# cells): карта тропы обязана быть одна на весь курс, иначе числа тактов в
# лекции и в тестах разойдутся.
project(11-ant-search)

statecraft_library(ant_search
        SOURCES ant_search.c ant_search.h
                ant_ep93.c ant_ep93.h
                ant_random.c ant_random.h)
target_link_libraries(ant_search PUBLIC cells)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS ant_search)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES ant_search_tests.cpp
        LIBS ant_search)
