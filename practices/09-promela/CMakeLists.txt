# Лекция 9 — модели Promela. Собственно сборки у них нет: модели проверяет
# внешний инструмент (SPIN), поэтому здесь регистрируются только тесты.
#
# SPIN ищется в PATH; каталог можно задать переменной SPIN — как переменной
# CMake, так и переменной окружения (её же понимает scripts/check-promela.sh).
# Если инструмента нет, подпроект пропускается: сборка курса не должна
# зависеть от наличия чужого пакета — то же правило, что у taktc.
project(09-promela)

if (NOT SPIN_HINT AND DEFINED ENV{SPIN})
    get_filename_component(SPIN_HINT "$ENV{SPIN}" DIRECTORY)
endif ()

find_program(SPIN_BIN NAMES spin HINTS "${SPIN_HINT}"
        DOC "Верификатор моделей Promela")

if (NOT SPIN_BIN)
    message(STATUS "statecraft: spin не найден — модели Promela пропущены "
            "(macOS: brew install spin, Debian/Ubuntu: apt install spin)")
    return()
endif ()

message(STATUS "statecraft: spin — ${SPIN_BIN}")

if (NOT STATECRAFT_BUILD_TESTS)
    return()
endif ()

# Проверка каждой модели — отдельный тест: так в отчёте ctest видно, какая
# именно модель разошлась с заявленным исходом, а не «упала проверка».
# Ожидаемый исход задаёт сама модель строкой «verify: ok|violation|skip»
# в шапке: часть примеров курса демонстрирует именно нарушение.
set(PROMELA_CHECK "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/check-promela.sh")

file(GLOB PROMELA_MODELS "${CMAKE_CURRENT_SOURCE_DIR}/*.pml")
list(SORT PROMELA_MODELS)

foreach (model IN LISTS PROMELA_MODELS)
    # NAME_WE обрезает по ПЕРВОЙ точке, а имена моделей начинаются с номера
    # («00.HelloWorld.pml»), поэтому нужен NAME_WLE — без последнего
    # расширения. Иначе все тесты назывались бы «09-promela-00».
    get_filename_component(model_name "${model}" NAME_WLE)
    string(TOLOWER "${model_name}" model_slug)
    string(REPLACE "." "-" model_slug "${model_slug}")
    add_test(NAME ${PROJECT_NAME}-${model_slug}
            COMMAND "${PROMELA_CHECK}" --verify "${model}")
    set_tests_properties(${PROJECT_NAME}-${model_slug} PROPERTIES
            ENVIRONMENT "SPIN=${SPIN_BIN}")
endforeach ()
