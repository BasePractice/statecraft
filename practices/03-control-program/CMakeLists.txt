# Прикладной пример: управляющая программа — датчики, движок, устройство.
# Используется в лекциях 3, 7, 8 как источник реалистичных фрагментов.
project(03-control-program)

option(ENABLE_NETWORK_EMULATE "Эмуляция показаний датчиков по сети" OFF)
option(ENABLE_FILE_EMULATE "Эмуляция показаний датчиков через файл" OFF)

set(CONTROL_SOURCES sensor.c engine.c device.c properties.c
        sensor.h engine.h device.h properties.h)
set(CONTROL_DEFINES "")
set(CONTROL_SOURCES_EXTRA "")
set(CONTROL_LIBS "")

if (ENABLE_NETWORK_EMULATE)
    list(APPEND CONTROL_DEFINES NETWORK_EMULATE NETWOR_EMULATE_PORT="8097")
    list(APPEND CONTROL_SOURCES_EXTRA ../common/network/eth_server.c)
    if (WIN32)
        list(APPEND CONTROL_LIBS ws2_32)
    else ()
        list(APPEND CONTROL_LIBS pthread)
    endif ()

    statecraft_example(${PROJECT_NAME}-emulate-network-cli
            SOURCES emulate_network_cli.c ../common/network/eth_server.c
            INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../common/network
            DEFINES ${CONTROL_DEFINES}
            LIBS ${CONTROL_LIBS})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/registers.txt
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
elseif (ENABLE_FILE_EMULATE)
    list(APPEND CONTROL_DEFINES FILE_EMULATE)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/registers.txt
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()

statecraft_example(${PROJECT_NAME}
        SOURCES main.c ${CONTROL_SOURCES} ${CONTROL_SOURCES_EXTRA}
        INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../common/network
        DEFINES ${CONTROL_DEFINES}
        LIBS ${CONTROL_LIBS})
