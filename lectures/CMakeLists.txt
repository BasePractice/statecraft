# lectures/CMakeLists.txt — сборка PDF лекций как часть общей сборки курса.
#
# Здесь нет логики сборки: она целиком в lectures/Makefile и scripts/.
# CMake вызывает те же цели make, что и человек руками, и передаёт каталог
# результата через OUT. Так порядок сборки, аргументы typst, шрифты и кэш
# пакетов описаны ровно в одном месте, а лекции собираются и без CMake:
#
#   cd lectures && make            # то же самое, результат в lectures/out
#
# Цели:
#   lectures            все PDF плюс копии под читаемыми именами (входит в ALL)
#   lecture-<id>        одна лекция
#   lectures-check      проверка окружения (make check)
#   lectures-sync-check листинги лекций не разошлись с practices/
#   lectures-clean      удалить собранные PDF
#
# PDF складываются в ${CMAKE_BINARY_DIR}/lectures/, каталог lectures/out/
# остаётся за ручной сборкой.

find_program(TYPST_BIN typst)
find_program(MAKE_BIN NAMES gmake make)

if (NOT TYPST_BIN OR NOT MAKE_BIN)
    if (NOT TYPST_BIN)
        message(WARNING
                "typst не найден — PDF лекций собираться не будут.\n"
                "  brew install typst   (или см. lectures/README.md)\n"
                "  отключить проверку: -DSTATECRAFT_BUILD_LECTURES=OFF")
    else ()
        message(WARNING "make не найден — PDF лекций собираться не будут")
    endif ()
    add_custom_target(lectures
            COMMAND ${CMAKE_COMMAND} -E echo
            "typst или make не установлены: лекции пропущены")
    return()
endif ()

set(LECTURES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LECTURES_OUT "${CMAKE_BINARY_DIR}/lectures")
file(MAKE_DIRECTORY "${LECTURES_OUT}")

# --- реестр лекций из course.typ -------------------------------------------
# Нужен не для сборки (её ведёт make), а для двух вещей: поштучных целей
# lecture-<id> и списка зависимостей, по которому CMake решает, звать ли make.
execute_process(
        COMMAND "${TYPST_BIN}" eval --root "${LECTURES_ROOT}"
        "{ import \"/course.typ\": lectures\n lectures.map(l => l.id).join(\" \") }"
        OUTPUT_VARIABLE LECTURE_IDS_RAW
        RESULT_VARIABLE LECTURE_IDS_RC
        ERROR_VARIABLE LECTURE_IDS_ERR
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT LECTURE_IDS_RC EQUAL 0)
    message(FATAL_ERROR "не удалось прочитать реестр лекций из course.typ:\n${LECTURE_IDS_ERR}")
endif ()

string(REPLACE "\"" "" LECTURE_IDS_RAW "${LECTURE_IDS_RAW}")
string(REPLACE " " ";" LECTURE_IDS "${LECTURE_IDS_RAW}")

# Всё, от чего зависит выхлоп: шаблон, общие файлы, библиография, сами лекции.
file(GLOB_RECURSE LECTURE_DEPS CONFIGURE_DEPENDS
        "${LECTURES_ROOT}/template/*.typ"
        "${LECTURES_ROOT}/shared/*.typ"
        "${LECTURES_ROOT}/bib/*.bib"
        "${LECTURES_ROOT}/src/*.typ"
        "${LECTURES_ROOT}/src/*/code/*"
        "${LECTURES_ROOT}/src/*/images/*")
list(APPEND LECTURE_DEPS "${LECTURES_ROOT}/course.typ" "${LECTURES_ROOT}/Makefile")

# Отметка о последней успешной сборке: без неё make звался бы на каждый
# `cmake --build`, даже когда в лекциях ничего не менялось.
set(LECTURES_STAMP "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/lectures.stamp")

add_custom_command(
        OUTPUT "${LECTURES_STAMP}"
        COMMAND "${MAKE_BIN}" --no-print-directory pretty "OUT=${LECTURES_OUT}"
        COMMAND ${CMAKE_COMMAND} -E touch "${LECTURES_STAMP}"
        DEPENDS ${LECTURE_DEPS}
        WORKING_DIRECTORY "${LECTURES_ROOT}"
        COMMENT "make pretty: PDF лекций в ${LECTURES_OUT}"
        VERBATIM)

add_custom_target(lectures ALL DEPENDS "${LECTURES_STAMP}")

foreach (id IN LISTS LECTURE_IDS)
    if (NOT EXISTS "${LECTURES_ROOT}/src/${id}/main.typ")
        message(WARNING "лекция ${id} объявлена в course.typ, но src/${id}/main.typ нет")
        continue()
    endif ()
    add_custom_target(lecture-${id}
            COMMAND "${MAKE_BIN}" --no-print-directory one "L=${id}" "OUT=${LECTURES_OUT}"
            WORKING_DIRECTORY "${LECTURES_ROOT}"
            COMMENT "make one L=${id}"
            VERBATIM)
endforeach ()

add_custom_target(lectures-check
        COMMAND "${MAKE_BIN}" --no-print-directory check
        WORKING_DIRECTORY "${LECTURES_ROOT}"
        COMMENT "make check: окружение сборки лекций"
        VERBATIM)

add_custom_target(lectures-sync-check
        COMMAND "${MAKE_BIN}" --no-print-directory sync-check
        WORKING_DIRECTORY "${LECTURES_ROOT}"
        COMMENT "make sync-check: листинги лекций против practices/"
        VERBATIM)

add_custom_target(lectures-clean
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${LECTURES_OUT}"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${LECTURES_STAMP}"
        COMMENT "Удаление собранных PDF")

list(LENGTH LECTURE_IDS n)
message(STATUS "statecraft: лекций в реестре — ${n}, PDF в ${LECTURES_OUT} (сборка через make)")
