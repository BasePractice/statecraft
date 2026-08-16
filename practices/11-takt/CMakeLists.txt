# Лекция 11 — язык Takt: модели, порождение C, драйвер на C90.
#
# Компилятор taktc внешний по отношению к курсу; ищется в PATH и по
# STATECRAFT_TAKTC (переменная CMake или окружения). Если не найден —
# подпроект пропускается с сообщением: остальная сборка курса не должна
# зависеть от наличия чужого репозитория.
#
#   cmake -S . -B build -DSTATECRAFT_TAKTC=/путь/к/taktc
project(11-takt)

find_program(TAKTC_BIN
        NAMES taktc
        HINTS "${STATECRAFT_TAKTC}" "$ENV{STATECRAFT_TAKTC}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../BuT/target/release"
        DOC "Компилятор языка Takt")

set(TAKT_MODELS traffic watchdog button_led pump cooling_bad cooling_good recovery)

if (NOT TAKTC_BIN)
    message(STATUS "statecraft: taktc не найден — примеры Takt пропущены "
            "(укажите -DSTATECRAFT_TAKTC=<каталог с taktc>)")
    return()
endif ()

message(STATUS "statecraft: taktc — ${TAKTC_BIN}")

set(TAKT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${TAKT_GEN_DIR}")

# Проверка синтаксиса и семантики всех моделей: цель ошибается, если хоть одна
# модель перестала компилироваться. Заодно порождается C для каждой из них.
set(TAKT_GENERATED "")
foreach (model IN LISTS TAKT_MODELS)
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/model/${model}.takt")
    set(out_c "${TAKT_GEN_DIR}/${model}.c")
    set(out_h "${TAKT_GEN_DIR}/${model}.h")
    add_custom_command(
            OUTPUT "${out_c}" "${out_h}"
            COMMAND "${TAKTC_BIN}" compile --target c "${src}" -o "${TAKT_GEN_DIR}/"
            DEPENDS "${src}"
            COMMENT "taktc: ${model}.takt → C"
            VERBATIM)
    list(APPEND TAKT_GENERATED "${out_c}" "${out_h}")
endforeach ()

add_custom_target(${PROJECT_NAME}-models DEPENDS ${TAKT_GENERATED})

# Порождённый код — C99 (uint8_t, bool, комментарии //), поэтому общий режим
# C90 к нему неприменим: это выход чужого компилятора, а не учебный код.
add_library(takt_generated STATIC
        "${TAKT_GEN_DIR}/watchdog.c"
        "${TAKT_GEN_DIR}/traffic.c")
target_include_directories(takt_generated PUBLIC "${TAKT_GEN_DIR}")
statecraft_generated_c99(takt_generated)

# Драйвер включает порождённый заголовок, а значит тоже собирается как C99:
# C90 не знает ни bool, ни uint8_t. Это единственная цель курса вне общего
# режима, и причина у неё внешняя — интерфейс чужого генератора.
add_executable(${PROJECT_NAME}-watchdog main.c)
target_link_libraries(${PROJECT_NAME}-watchdog PRIVATE takt_generated)
statecraft_generated_c99(${PROJECT_NAME}-watchdog)

if (STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}-watchdog COMMAND ${PROJECT_NAME}-watchdog)
endif ()
