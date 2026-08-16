cmake_minimum_required(VERSION 3.16)
project(statecraft
        VERSION 1.0.0
        DESCRIPTION "Курс АПСУ: лекции на Typst и практикум на C90"
        LANGUAGES C CXX)

# Одна сборка даёт и PDF лекций, и собранные примеры:
#
#   cmake -S . -B build
#   cmake --build build           # лекции + примеры
#   ctest --test-dir build        # тесты примеров
#   cmake --build build --target style-check
#
# Отключить половину сборки:
#   -DSTATECRAFT_BUILD_LECTURES=OFF   только код
#   -DSTATECRAFT_BUILD_PRACTICE=OFF   только лекции

option(STATECRAFT_BUILD_LECTURES "Собирать PDF лекций (нужен typst)" ON)
option(STATECRAFT_BUILD_PRACTICE "Собирать примеры и проекты практикума" ON)
option(STATECRAFT_BUILD_TESTS    "Собирать тесты практикума (Catch2)" ON)
option(STATECRAFT_STRICT         "Предупреждения компилятора = ошибки" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(StatecraftStrictC)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif ()

if (STATECRAFT_BUILD_TESTS)
    enable_testing()
endif ()

if (STATECRAFT_BUILD_PRACTICE)
    add_subdirectory(practices)
endif ()

if (STATECRAFT_BUILD_LECTURES)
    add_subdirectory(lectures)
endif ()

# --- проверка оформления кода ----------------------------------------------
# Тот же скрипт запускается в CI; здесь — чтобы не выходить из сборки.
add_custom_target(style-check
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-style.sh"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Проверка оформления кода по .clang-format"
        USES_TERMINAL)

add_custom_target(style-fix
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-style.sh" --fix
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Переформатирование кода по .clang-format"
        USES_TERMINAL)

message(STATUS "statecraft: лекции=${STATECRAFT_BUILD_LECTURES} "
        "практикум=${STATECRAFT_BUILD_PRACTICE} "
        "тесты=${STATECRAFT_BUILD_TESTS} строгий режим=${STATECRAFT_STRICT}")
