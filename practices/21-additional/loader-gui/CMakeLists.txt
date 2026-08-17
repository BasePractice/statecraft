# Графическая составляющая погрузчика: наблюдение за прогоном (C++ + raylib).
#
# Цель необязательная и по умолчанию выключена. Причины ровно две: raylib —
# внешняя зависимость, которой нет ни в CI, ни на машине без графики, и сам
# просмотр к учебному материалу ничего не добавляет — поведение автомата
# проверяется тестами и потактовой трассой.
#
#   cmake -S . -B build -DSTATECRAFT_BUILD_GUI=ON
#
# Приложению нужен и порождённый автомат: без `taktc` показывать нечего.

if (NOT STATECRAFT_BUILD_GUI)
    return()
endif ()

if (NOT TARGET loader_runtime)
    message(WARNING "statecraft: графика погрузчика пропущена — нет taktc, "
            "а значит и системы управления")
    return()
endif ()

find_package(raylib QUIET)
if (NOT raylib_FOUND)
    message(WARNING "statecraft: raylib не найдена — графика погрузчика пропущена "
            "(macOS: brew install raylib, Debian: apt install libraylib-dev)")
    return()
endif ()

add_executable(21-additional-gui loader_gui.cpp)
target_link_libraries(21-additional-gui PRIVATE loader_runtime raylib)
set_target_properties(21-additional-gui PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Конфигурации нужны рядом с исполняемым файлом.
foreach (config factory.json factory-grid.json)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../loader/${config}"
            "${CMAKE_CURRENT_BINARY_DIR}/${config}" COPYONLY)
endforeach ()

# Тестом графику не покрыть: она требует дисплея. Проверяется вручную —
# см. README практики.
