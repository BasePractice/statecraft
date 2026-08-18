#include <catch2/catch.hpp>
#include <applied_dfa.h>

#include <string>

/*
 * Проверяется то, ради чего эти распознаватели перенесены в курс: что автомат
 * принимает ровно свой язык. Отрицательные примеры здесь важнее
 * положительных — распознаватель, который принимает всё подряд, проходит
 * любой набор «правильных» строк.
 */

TEST_CASE("ISBN распознаётся с префиксом и без", "[06.AppliedDfa]") {
    const Dfa *isbn = dfa_isbn();

    SECTION("Подходит") {
        REQUIRE(dfa_match(isbn, "0123456789"));
        REQUIRE(dfa_match(isbn, "012345678X"));
        REQUIRE(dfa_match(isbn, "012345678x"));
        REQUIRE(dfa_match(isbn, "ISBN0123456789"));
        REQUIRE(dfa_match(isbn, "ISBN:0123456789"));
        REQUIRE(dfa_match(isbn, "ISBN:012345678X"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(isbn, ""));
        REQUIRE_FALSE(dfa_match(isbn, "012345678"));   /* девять знаков */
        REQUIRE_FALSE(dfa_match(isbn, "01234567890")); /* одиннадцать */
        REQUIRE_FALSE(dfa_match(isbn, "X123456789")); /* X только в конце */
        REQUIRE_FALSE(dfa_match(isbn, "ISBN"));
        REQUIRE_FALSE(dfa_match(isbn, "ISBN:"));
        REQUIRE_FALSE(dfa_match(isbn, "isbn:0123456789")); /* префикс — прописными */
        REQUIRE_FALSE(dfa_match(isbn, "ISBN 0123456789"));
    }
}

TEST_CASE("Отказ показывает, на каком знаке разошлось", "[06.AppliedDfa]") {
    const Dfa *isbn = dfa_isbn();

    /* «ISBN» прочитано целиком, споткнулись на пробеле — пятый знак. */
    REQUIRE(dfa_match_prefix(isbn, "ISBN 0123456789") == 4);
    /* Здесь автомат дочитал строку до конца, но остановился не в
       принимающем состоянии: длины не хватило. */
    REQUIRE(dfa_match_prefix(isbn, "012345678") == 9);
    REQUIRE_FALSE(dfa_match(isbn, "012345678"));
}

TEST_CASE("Парный тег распознаётся вместе с телом", "[06.AppliedDfa]") {
    const Dfa *tag = dfa_tag();

    SECTION("Подходит") {
        REQUIRE(dfa_match(tag, "<kill></kill>"));
        REQUIRE(dfa_match(tag, "<kill>текст</kill>"));
        REQUIRE(dfa_match(tag, "<pop></pop>"));
        REQUIRE(dfa_match(tag, "<pop>что угодно</pop>"));
        /* Тело может содержать закрывающий тег: закрывает последний. */
        REQUIRE(dfa_match(tag, "<kill>a</kill>b</kill>"));
    }
    SECTION("Имена не смешиваются") {
        /* Ровно то, ради чего в исходном выражении стояла обратная ссылка. */
        REQUIRE_FALSE(dfa_match(tag, "<kill></pop>"));
        REQUIRE_FALSE(dfa_match(tag, "<pop></kill>"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(tag, ""));
        REQUIRE_FALSE(dfa_match(tag, "<kill>"));
        REQUIRE_FALSE(dfa_match(tag, "</kill>"));
        REQUIRE_FALSE(dfa_match(tag, "<kil></kil>"));
        REQUIRE_FALSE(dfa_match(tag, "<kill></kill"));
        REQUIRE_FALSE(dfa_match(tag, "x<kill></kill>"));
        /* Символ после открывающего имени обязан быть '>' — в исходном коде
           из c_fsm он не проверялся вовсе, и «<killZ» проходило. */
        REQUIRE_FALSE(dfa_match(tag, "<killZ</kill>"));
    }
}

TEST_CASE("Закрывающий тег ищется с возвратом, а не с начала", "[06.AppliedDfa]") {
    const Dfa *tag = dfa_tag();

    /*
     * Эти входы и отличают правильный автомат от наивного. Наивный при
     * несовпадении возвращается в состояние «читаю тело» и теряет уже
     * прочитанный '<', который сам был началом закрывающего тега.
     */
    REQUIRE(dfa_match(tag, "<kill><</kill>"));
    REQUIRE(dfa_match(tag, "<kill></</kill>"));
    REQUIRE(dfa_match(tag, "<kill></ki</kill>"));
    REQUIRE(dfa_match(tag, "<pop><</pop>"));
    REQUIRE(dfa_match(tag, "<pop></po</pop>"));
}

TEST_CASE("Число с плавающей точкой", "[06.AppliedDfa]") {
    const Dfa *number = dfa_number();

    SECTION("Подходит") {
        REQUIRE(dfa_match(number, "0"));
        REQUIRE(dfa_match(number, "42"));
        REQUIRE(dfa_match(number, "-42"));
        REQUIRE(dfa_match(number, "+42"));
        REQUIRE(dfa_match(number, "3.14"));
        REQUIRE(dfa_match(number, "-3.14"));
        REQUIRE(dfa_match(number, "1e9"));
        REQUIRE(dfa_match(number, "1E9"));
        REQUIRE(dfa_match(number, "1e-9"));
        REQUIRE(dfa_match(number, "-2.5e+10"));
    }
    SECTION("Не подходит") {
        REQUIRE_FALSE(dfa_match(number, ""));
        REQUIRE_FALSE(dfa_match(number, "."));
        REQUIRE_FALSE(dfa_match(number, "-"));
        REQUIRE_FALSE(dfa_match(number, ".5"));  /* без целой части */
        REQUIRE_FALSE(dfa_match(number, "5."));  /* без дробной */
        REQUIRE_FALSE(dfa_match(number, "1e"));  /* без порядка */
        REQUIRE_FALSE(dfa_match(number, "1e+")); /* тоже без порядка */
        REQUIRE_FALSE(dfa_match(number, "1.2.3"));
        REQUIRE_FALSE(dfa_match(number, "1e9e9"));
        REQUIRE_FALSE(dfa_match(number, " 42"));
        REQUIRE_FALSE(dfa_match(number, "42 "));
    }
}

TEST_CASE("Таблицы согласованы сами с собой", "[06.AppliedDfa]") {
    const Dfa *dfa;

    for (int i = 0; (dfa = dfa_by_index(i)) != NULL; ++i) {
        REQUIRE(dfa->name != NULL);
        REQUIRE(dfa->pattern != NULL);
        REQUIRE(dfa->rule_count > 0);
        REQUIRE(dfa->rule_count <= DFA_MAX_RULES);
        REQUIRE(dfa_state_count(dfa) <= DFA_MAX_STATES);
        REQUIRE(dfa_by_name(dfa->name) == dfa);

        /* Переход в несуществующее состояние или из него — молчаливая
           ошибка: строка просто перестанет распознаваться. */
        for (int r = 0; r < dfa->rule_count; ++r) {
            REQUIRE(dfa->rules[r].from >= 0);
            REQUIRE(dfa->rules[r].to >= 0);
            REQUIRE(dfa->rules[r].from < dfa_state_count(dfa));
            REQUIRE(dfa->rules[r].to < dfa_state_count(dfa));
            REQUIRE(dfa->rules[r].set != NULL);
        }
        /* Принимающее состояние, в которое нельзя попасть, — тоже ошибка. */
        for (int a = 0; dfa->accepting[a] >= 0; ++a) {
            bool reachable = dfa->accepting[a] == dfa->start;
            for (int r = 0; !reachable && r < dfa->rule_count; ++r) {
                reachable = dfa->rules[r].to == dfa->accepting[a];
            }
            REQUIRE(reachable);
        }
    }
    REQUIRE(dfa_by_name("нет такого") == NULL);
    REQUIRE(dfa_by_index(-1) == NULL);
}

TEST_CASE("Число состояний считается по таблице", "[06.AppliedDfa]") {
    /* Числа из лекции: девять цифр ISBN дают девять состояний, и обойтись
       меньшим числом нельзя — у автомата нет счётчика. */
    REQUIRE(dfa_state_count(dfa_number()) == 8);
    REQUIRE(dfa_state_count(dfa_isbn()) == 16);
    REQUIRE(dfa_state_count(dfa_tag()) == 30);
}

/*
 * UTF-8. Здесь важна не столько таблица, сколько то, что она отвергает:
 * наивный декодер («узнать длину по первому байту и проглотить остальное»)
 * принимает переполненную кодировку, суррогаты и коды за пределом U+10FFFF.
 */

namespace {

/* Независимая проверка по RFC 3629, написанная обычным разбором: длина по
   первому байту, продолжающие байты, диапазон полученного кода. Нужна как
   эталон — автомат сверяется с ней перебором, а не с самим собой. */
bool utf8_valid_ref(const unsigned char *bytes, size_t length) {
    size_t i = 0;

    while (i < length) {
        unsigned char lead = bytes[i];
        size_t extra;
        unsigned long code;
        size_t k;

        if (lead < 0x80u) {
            code = lead;
            extra = 0;
        } else if (lead >= 0xC2u && lead <= 0xDFu) {
            code = lead & 0x1Fu;
            extra = 1;
        } else if (lead >= 0xE0u && lead <= 0xEFu) {
            code = lead & 0x0Fu;
            extra = 2;
        } else if (lead >= 0xF0u && lead <= 0xF4u) {
            code = lead & 0x07u;
            extra = 3;
        } else {
            return false; /* C0, C1 и всё старше F4 недопустимы как ведущие */
        }
        if (extra > 0 && i + extra >= length) {
            return false; /* последовательность оборвана */
        }
        for (k = 1; k <= extra; ++k) {
            unsigned char cont = bytes[i + k];

            if (cont < 0x80u || cont > 0xBFu) {
                return false;
            }
            code = (code << 6) | (cont & 0x3Fu);
        }
        if (extra == 1 && code < 0x80u) {
            return false;
        }
        if (extra == 2 && code < 0x800u) {
            return false; /* переполненная кодировка */
        }
        if (extra == 3 && code < 0x10000u) {
            return false;
        }
        if (code >= 0xD800u && code <= 0xDFFFu) {
            return false; /* суррогаты в UTF-8 не кодируются */
        }
        if (code > 0x10FFFFu) {
            return false;
        }
        i += extra + 1;
    }
    return true;
}

/* Кодирование кодовой точки в UTF-8 — тоже своё, чтобы не зависеть от
   локали и от того, в какой кодировке сохранён этот файл. */
std::string utf8_encode(unsigned long code) {
    std::string out;

    if (code < 0x80u) {
        out += (char)code;
    } else if (code < 0x800u) {
        out += (char)(0xC0u | (code >> 6));
        out += (char)(0x80u | (code & 0x3Fu));
    } else if (code < 0x10000u) {
        out += (char)(0xE0u | (code >> 12));
        out += (char)(0x80u | ((code >> 6) & 0x3Fu));
        out += (char)(0x80u | (code & 0x3Fu));
    } else {
        out += (char)(0xF0u | (code >> 18));
        out += (char)(0x80u | ((code >> 12) & 0x3Fu));
        out += (char)(0x80u | ((code >> 6) & 0x3Fu));
        out += (char)(0x80u | (code & 0x3Fu));
    }
    return out;
}

} /* namespace */

TEST_CASE("UTF-8: принимается корректное, отвергается запрещённое", "[06.AppliedDfa]") {
    const Dfa *utf8 = dfa_utf8();

    REQUIRE(dfa_match(utf8, "ASCII text 123"));
    REQUIRE(dfa_match(utf8, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82")); /* Привет */
    REQUIRE(dfa_match(utf8, "\xE2\x82\xAC"));                                     /* U+20AC € */
    REQUIRE(dfa_match(utf8, "\xF0\x9F\x99\x82"));                                 /* U+1F642 */
    REQUIRE(dfa_match(utf8, "\xF4\x8F\xBF\xBF"));                                 /* U+10FFFF — предел */

    REQUIRE_FALSE(dfa_match(utf8, "\xC0\xAF"));         /* «/» переполненной кодировкой */
    REQUIRE_FALSE(dfa_match(utf8, "\xE0\x80\xAF"));     /* она же тремя байтами */
    REQUIRE_FALSE(dfa_match(utf8, "\xF0\x80\x80\xAF")); /* и четырьмя */
    REQUIRE_FALSE(dfa_match(utf8, "\xED\xA0\x80"));     /* суррогат U+D800 */
    REQUIRE_FALSE(dfa_match(utf8, "\xED\xBF\xBF"));     /* суррогат U+DFFF */
    REQUIRE_FALSE(dfa_match(utf8, "\xF4\x90\x80\x80")); /* U+110000 — за пределом */
    REQUIRE_FALSE(dfa_match(utf8, "\xF5\x80\x80\x80")); /* ведущий байт вне RFC 3629 */
    REQUIRE_FALSE(dfa_match(utf8, "\x80"));             /* продолжающий байт без ведущего */
    REQUIRE_FALSE(dfa_match(utf8, "\xD0"));             /* оборванная последовательность */
    REQUIRE_FALSE(dfa_match(utf8, "\xD0\x41"));         /* продолжение подменено буквой */
}

TEST_CASE("UTF-8: автомат принимает все кодовые точки", "[06.AppliedDfa]") {
    const Dfa *utf8 = dfa_utf8();
    unsigned long code;
    int accepted = 0;

    /* U+0000 в строке C неотличим от её конца, поэтому перебор с единицы. */
    for (code = 1; code <= 0x10FFFFu; ++code) {
        if (code >= 0xD800u && code <= 0xDFFFu) {
            continue;
        }
        if (!dfa_match(utf8, utf8_encode(code).c_str())) {
            FAIL("не принята кодовая точка U+" << std::hex << code);
        }
        ++accepted;
    }
    /* Точки 1..U+10FFFF без 2048 суррогатов D800..DFFF. */
    REQUIRE(accepted == 0x10FFFF - 2048);
}

TEST_CASE("UTF-8: автомат совпадает с прямым разбором по RFC", "[06.AppliedDfa]") {
    const Dfa *utf8 = dfa_utf8();
    unsigned first;
    unsigned second;
    unsigned third;

    /* Все одно- и двухбайтовые последовательности: 1 + 255 + 255 * 255. */
    for (first = 1; first < 256; ++first) {
        unsigned char one[1];
        std::string text;

        one[0] = (unsigned char)first;
        text.assign((const char *)one, 1);
        REQUIRE(dfa_match(utf8, text.c_str()) == utf8_valid_ref(one, 1));

        for (second = 1; second < 256; ++second) {
            unsigned char two[2];

            two[0] = (unsigned char)first;
            two[1] = (unsigned char)second;
            text.assign((const char *)two, 2);
            REQUIRE(dfa_match(utf8, text.c_str()) == utf8_valid_ref(two, 2));
        }
    }

    /* Трёхбайтовые с ведущим E0..EF: здесь живут суррогаты и переполнение. */
    for (first = 0xE0u; first <= 0xEFu; ++first) {
        for (second = 1; second < 256; ++second) {
            for (third = 1; third < 256; ++third) {
                unsigned char three[3];
                std::string text;

                three[0] = (unsigned char)first;
                three[1] = (unsigned char)second;
                three[2] = (unsigned char)third;
                text.assign((const char *)three, 3);
                REQUIRE(dfa_match(utf8, text.c_str()) == utf8_valid_ref(three, 3));
            }
        }
    }
}
