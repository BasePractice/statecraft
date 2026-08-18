#include <string>
#include <catch2/catch.hpp>
#include <mt_programs.h>

/*
  Таблицы этих машин напечатаны в приложении лекции 10. Прогон здесь нужен
  ровно затем, чтобы напечатанное не расходилось с работающим: автомат,
  разобранный вручную, проверяется программой, а не глазами.
 */

TEST_CASE("Распознавание a^n b^n", "[10.Turing_Machine]") {
    SECTION("слова языка принимаются") {
        REQUIRE(mt_recognize(mt_anbn_create, "ab") == 1);
        REQUIRE(mt_recognize(mt_anbn_create, "aabb") == 1);
        REQUIRE(mt_recognize(mt_anbn_create, "aaabbb") == 1);
        REQUIRE(mt_recognize(mt_anbn_create, "aaaabbbb") == 1);
    }

    SECTION("нарушенное равенство числа букв") {
        REQUIRE(mt_recognize(mt_anbn_create, "aab") == 0);
        REQUIRE(mt_recognize(mt_anbn_create, "abb") == 0);
        REQUIRE(mt_recognize(mt_anbn_create, "a") == 0);
        REQUIRE(mt_recognize(mt_anbn_create, "b") == 0);
    }

    SECTION("нарушенный порядок букв") {
        REQUIRE(mt_recognize(mt_anbn_create, "ba") == 0);
        REQUIRE(mt_recognize(mt_anbn_create, "abab") == 0);
        REQUIRE(mt_recognize(mt_anbn_create, "aabbab") == 0);
    }

    SECTION("пустое слово не принимается: в языке n >= 1") {
        REQUIRE(mt_recognize(mt_anbn_create, "") == 0);
    }
}

TEST_CASE("Распознавание палиндромов", "[10.Turing_Machine]") {
    SECTION("палиндромы принимаются") {
        REQUIRE(mt_recognize(mt_palindrome_create, "") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "a") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "aa") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "aba") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "abba") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "abaaba") == 1);
        REQUIRE(mt_recognize(mt_palindrome_create, "aabbbaa") == 1);
    }

    SECTION("непалиндромы не принимаются") {
        REQUIRE(mt_recognize(mt_palindrome_create, "ab") == 0);
        REQUIRE(mt_recognize(mt_palindrome_create, "abb") == 0);
        REQUIRE(mt_recognize(mt_palindrome_create, "abab") == 0);
        REQUIRE(mt_recognize(mt_palindrome_create, "aabaab") == 0);
    }
}

TEST_CASE("Палиндромы: перебор всех слов длиной до пяти", "[10.Turing_Machine]") {
    /* Перебор надёжнее выборочных примеров: он покрывает и чётные, и
       нечётные длины, и все места, где сверка может разойтись. */
    char word[8];
    int length;

    for (length = 0; length <= 5; ++length) {
        int mask;
        for (mask = 0; mask < (1 << length); ++mask) {
            int i;
            int expected = 1;
            for (i = 0; i < length; ++i)
                word[i] = ((mask >> i) & 1) ? 'b' : 'a';
            word[length] = '\0';
            for (i = 0; i < length / 2; ++i) {
                if (word[i] != word[length - 1 - i])
                    expected = 0;
            }
            INFO("слово: " << word);
            REQUIRE(mt_recognize(mt_palindrome_create, word) == expected);
        }
    }
}

TEST_CASE("Копирование w -> ww", "[10.Turing_Machine]") {
    char out[TAPE_LIMIT + 1];

    SECTION("слово удваивается") {
        REQUIRE(std::string(mt_transform(mt_copy_create, "a", out, sizeof(out))) == "aa");
        REQUIRE(std::string(mt_transform(mt_copy_create, "ab", out, sizeof(out))) == "abab");
        REQUIRE(std::string(mt_transform(mt_copy_create, "ba", out, sizeof(out))) == "baba");
        REQUIRE(std::string(mt_transform(mt_copy_create, "abb", out, sizeof(out))) == "abbabb");
        REQUIRE(std::string(mt_transform(mt_copy_create, "baab", out, sizeof(out))) == "baabbaab");
    }

    SECTION("пустое слово остаётся пустым") {
        REQUIRE(std::string(mt_transform(mt_copy_create, "", out, sizeof(out))).empty());
    }

    SECTION("разметки на ленте не остаётся") {
        std::string result(mt_transform(mt_copy_create, "abba", out, sizeof(out)));
        REQUIRE(result.find_first_of("XYcd") == std::string::npos);
    }
}
