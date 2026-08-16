# cmake/StatecraftStrictC.cmake
#
# Единые требования к коду курса: ISO C90 без расширений компилятора и
# без предупреждений. Курс требует того же от студенческих работ
# (см. _1.CodeStyle/-1.CodeStyle.md), поэтому примеры обязаны проходить
# ту же проверку, что и лабораторные.
#
#   statecraft_strict_c(<target>)   — навесить режим на цель
#
# Опция STATECRAFT_STRICT (по умолчанию ON) превращает предупреждения в
# ошибки. Выключается для разбора чужого кода: -DSTATECRAFT_STRICT=OFF.

function(statecraft_strict_c target)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)

    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if (STATECRAFT_STRICT)
            target_compile_options(${target} PRIVATE /WX)
        endif ()
        # MSVC не умеет C90 и ругается на стандартные функции C.
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else ()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -pedantic
            -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes
            -Wmissing-prototypes -Wold-style-definition -Wwrite-strings)
        if (STATECRAFT_STRICT)
            target_compile_options(${target} PRIVATE -Werror)
        endif ()
    endif ()
endfunction()

# Код, порождённый внешним компилятором (например, taktc), и склейка с ним.
#
# Требование C90 относится к коду, который пишут руками: студенческие работы и
# примеры курса. Выход чужого генератора им не связан — taktc порождает C99
# (uint8_t, bool, комментарии //), и его заголовок обязан включать
# драйвер. Поэтому такие цели собираются как C99, но предупреждения остаются:
# сгенерированный код тоже должен быть чистым.
function(statecraft_generated_c99 target)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)

    if (MSVC)
        target_compile_options(${target} PRIVATE /W3)
    else ()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif ()
endfunction()

# Тесты пишутся на C++ (Catch2) и под требования C90 не подпадают:
# студент C++-код не сдаёт. Здесь только базовая гигиена.
function(statecraft_test_cxx target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)
    if (NOT MSVC)
        target_compile_options(${target} PRIVATE -Wall)
    endif ()
endfunction()
