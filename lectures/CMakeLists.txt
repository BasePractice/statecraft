# lectures/CMakeLists.txt — сборка PDF лекций как часть общей сборки курса.
#
# Реестр лекций читается из course.typ тем же способом, что и в
# scripts/lib.sh: единственный источник правды — course.typ.
#
# Цели:
#   lectures            все PDF (входит в ALL)
#   lecture-<id>        одна лекция
#   lectures-clean      удалить собранные PDF
#
# PDF складываются в ${CMAKE_BINARY_DIR}/lectures/, каталог lectures/out/
# остаётся за скриптами scripts/build.sh для ручной работы.

find_program(TYPST_BIN typst)

if (NOT TYPST_BIN)
    message(WARNING
            "typst не найден — PDF лекций собираться не будут.\n"
            "  brew install typst   (или см. lectures/README.md)\n"
            "  отключить проверку: -DSTATECRAFT_BUILD_LECTURES=OFF")
    add_custom_target(lectures
            COMMAND ${CMAKE_COMMAND} -E echo
            "typst не установлен: лекции пропущены")
    return()
endif ()

set(LECTURES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LECTURES_OUT "${CMAKE_BINARY_DIR}/lectures")
file(MAKE_DIRECTORY "${LECTURES_OUT}")

# --- реестр лекций из course.typ -------------------------------------------
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

# --- общие зависимости: шаблон, общие файлы, библиография ------------------
file(GLOB_RECURSE LECTURE_COMMON_DEPS CONFIGURE_DEPENDS
        "${LECTURES_ROOT}/template/*.typ"
        "${LECTURES_ROOT}/shared/*.typ"
        "${LECTURES_ROOT}/bib/*.bib")
list(APPEND LECTURE_COMMON_DEPS "${LECTURES_ROOT}/course.typ")

set(TYPST_ARGS --root "${LECTURES_ROOT}")
if (IS_DIRECTORY "${LECTURES_ROOT}/fonts")
    list(APPEND TYPST_ARGS --font-path "${LECTURES_ROOT}/fonts")
endif ()
if (IS_DIRECTORY "${LECTURES_ROOT}/.typst-packages")
    list(APPEND TYPST_ARGS --package-cache-path "${LECTURES_ROOT}/.typst-packages")
endif ()

set(LECTURE_PDFS "")

foreach (id IN LISTS LECTURE_IDS)
    set(src "${LECTURES_ROOT}/src/${id}/main.typ")
    if (NOT EXISTS "${src}")
        message(WARNING "лекция ${id} объявлена в course.typ, но src/${id}/main.typ нет")
        continue()
    endif ()

    file(GLOB_RECURSE deps CONFIGURE_DEPENDS
            "${LECTURES_ROOT}/src/${id}/*.typ"
            "${LECTURES_ROOT}/src/${id}/code/*"
            "${LECTURES_ROOT}/src/${id}/images/*")

    set(pdf "${LECTURES_OUT}/${id}.pdf")
    add_custom_command(
            OUTPUT "${pdf}"
            COMMAND "${TYPST_BIN}" compile ${TYPST_ARGS} "${src}" "${pdf}"
            DEPENDS ${deps} ${LECTURE_COMMON_DEPS}
            WORKING_DIRECTORY "${LECTURES_ROOT}"
            COMMENT "typst: ${id}.pdf"
            VERBATIM)

    add_custom_target(lecture-${id} DEPENDS "${pdf}")
    list(APPEND LECTURE_PDFS "${pdf}")
endforeach ()

add_custom_target(lectures ALL DEPENDS ${LECTURE_PDFS})

add_custom_target(lectures-clean
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${LECTURES_OUT}"
        COMMENT "Удаление собранных PDF")

list(LENGTH LECTURE_PDFS n)
message(STATUS "statecraft: лекций в реестре — ${n}, PDF в ${LECTURES_OUT}")
