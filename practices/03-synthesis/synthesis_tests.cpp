#include <cstdio>
#include <cstring>
#include <catch2/catch.hpp>
#include <synthesis.h>

namespace {

/* Разменный аппарат из лекции: q(t+1) = (q + x) mod 3, y = (q + x) / 3,
   где x — номинал монеты, а состояние — долг в копейках. */
void build_coin(struct Machine *machine) {
    static const int coins[] = {1, 3, 5, 10};
    int s;
    int a;

    machine_init(machine);
    std::strcpy(machine->name, "coin");
    for (a = 0; a < 4; ++a) {
        std::snprintf(machine->inputs[a], SYN_MAX_NAME, "%d", coins[a]);
    }
    machine->input_count = 4;
    for (a = 0; a < 5; ++a) {
        std::snprintf(machine->outputs[a], SYN_MAX_NAME, "%d", a);
    }
    machine->output_count = 5;
    for (s = 0; s < 3; ++s) {
        std::snprintf(machine->states[s], SYN_MAX_NAME, "%d", s);
    }
    machine->state_count = 3;

    for (s = 0; s < 3; ++s) {
        for (a = 0; a < 4; ++a) {
            int sum = s + coins[a];
            machine->next[s][a] = sum % 3;
            machine->emit[s][a] = sum / 3;
        }
    }
}

} /* namespace */

TEST_CASE("Разрядность кода", "[03.Synthesis]") {
    REQUIRE(synthesis_bits(2) == 1);
    REQUIRE(synthesis_bits(3) == 2);
    REQUIRE(synthesis_bits(4) == 2);
    REQUIRE(synthesis_bits(5) == 3);
    REQUIRE(synthesis_bits(8) == 3);
    REQUIRE(synthesis_bits(9) == 4);
}

TEST_CASE("Минимизация: склеивание соседних наборов", "[03.Synthesis]") {
    struct BoolFunction function;
    struct Dnf dnf;

    /* f(x1, x2) = x1: единицы на наборах 10 и 11 склеиваются в одну
       импликанту без x2. */
    bool_init(&function, 2);
    bool_set(&function, 0, BOOL_ZERO);
    bool_set(&function, 1, BOOL_ZERO);
    bool_set(&function, 2, BOOL_ONE);
    bool_set(&function, 3, BOOL_ONE);

    REQUIRE(bool_minimize(&dnf, &function));
    REQUIRE(dnf.count == 1);
    REQUIRE(dnf_literals(&dnf) == 1);
    REQUIRE(dnf_equals(&dnf, &function));
}

TEST_CASE("Минимизация: безразличные наборы укорачивают формулу", "[03.Synthesis]") {
    struct BoolFunction strict;
    struct BoolFunction relaxed;
    struct Dnf strict_dnf;
    struct Dnf relaxed_dnf;

    /* Одна и та же функция: сначала нули на «лишних» наборах, потом
       безразличие. Во втором случае формула обязана быть не длиннее. */
    bool_init(&strict, 3);
    bool_set(&strict, 0, BOOL_ZERO);
    bool_set(&strict, 1, BOOL_ZERO);
    bool_set(&strict, 2, BOOL_ZERO);
    bool_set(&strict, 3, BOOL_ZERO);
    bool_set(&strict, 4, BOOL_ONE);
    bool_set(&strict, 5, BOOL_ONE);
    bool_set(&strict, 6, BOOL_ZERO);
    bool_set(&strict, 7, BOOL_ZERO);

    relaxed = strict;
    bool_set(&relaxed, 6, BOOL_DONT_CARE);
    bool_set(&relaxed, 7, BOOL_DONT_CARE);

    REQUIRE(bool_minimize(&strict_dnf, &strict));
    REQUIRE(bool_minimize(&relaxed_dnf, &relaxed));
    REQUIRE(dnf_equals(&strict_dnf, &strict));
    REQUIRE(dnf_equals(&relaxed_dnf, &relaxed));
    REQUIRE(dnf_literals(&relaxed_dnf) < dnf_literals(&strict_dnf));
}

TEST_CASE("Тождественно нулевая и тождественно единичная функции", "[03.Synthesis]") {
    struct BoolFunction function;
    struct Dnf dnf;
    int row;

    bool_init(&function, 3);
    for (row = 0; row < 8; ++row) {
        bool_set(&function, row, BOOL_ZERO);
    }
    REQUIRE(bool_minimize(&dnf, &function));
    REQUIRE(dnf.count == 0);
    REQUIRE(dnf_equals(&dnf, &function));

    for (row = 0; row < 8; ++row) {
        bool_set(&function, row, BOOL_ONE);
    }
    REQUIRE(bool_minimize(&dnf, &function));
    REQUIRE(dnf_literals(&dnf) == 0); /* константа 1 — импликанта без литералов */
    REQUIRE(dnf_equals(&dnf, &function));
}

TEST_CASE("Синтез разменного аппарата воспроизводит таблицу", "[03.Synthesis]") {
    struct Machine machine;
    struct Synthesis synthesis;

    build_coin(&machine);
    REQUIRE(synthesis_run(&synthesis, &machine));

    /* Четыре входных символа — 2 разряда, три состояния — 2 разряда,
       пять выходных символов — 3 разряда: ровно как на лекции. */
    REQUIRE(synthesis.input_bits == 2);
    REQUIRE(synthesis.state_bits == 2);
    REQUIRE(synthesis.output_bits == 3);

    REQUIRE(synthesis_verify(&synthesis, &machine));
}

TEST_CASE("Синтез детектора 1101 даёт формулы лекции", "[03.Synthesis]") {
    /* Тот же автомат, что в лекции 2, и та же таблица, что в
       machine/detector.fsm. Тест закрепляет числа, напечатанные в лекции 3:
       разойдутся формулы — разойдётся и лекция. */
    const char *text = "name detector\n"
                       "inputs 0 1\n"
                       "outputs 0 1\n"
                       "states q0 q1 q2 q3\n"
                       "q0 0 -> q0 0\n"
                       "q0 1 -> q1 0\n"
                       "q1 0 -> q0 0\n"
                       "q1 1 -> q2 0\n"
                       "q2 0 -> q3 0\n"
                       "q2 1 -> q2 0\n"
                       "q3 0 -> q0 0\n"
                       "q3 1 -> q1 1\n";
    struct Machine machine;
    struct Synthesis synthesis;
    FILE *in = std::tmpfile();

    REQUIRE(in != NULL);
    std::fputs(text, in);
    std::rewind(in);
    REQUIRE(machine_read(&machine, in));
    std::fclose(in);

    REQUIRE(synthesis_run(&synthesis, &machine));

    /* Вход двоичный, состояний четыре, выход двоичный: безразличных
       наборов у этого автомата нет — в отличие от разменного аппарата. */
    REQUIRE(synthesis.input_bits == 1);
    REQUIRE(synthesis.state_bits == 2);
    REQUIRE(synthesis.output_bits == 1);

    /* Числа из таблицы «Цена схемы детектора» в лекции 3. */
    REQUIRE(dnf_literals(&synthesis.phi_dnf[0]) == 5);
    REQUIRE(dnf_literals(&synthesis.phi_dnf[1]) == 9);
    REQUIRE(dnf_literals(&synthesis.psi_dnf[0]) == 3);

    REQUIRE(synthesis_verify(&synthesis, &machine));
}

TEST_CASE("Чтение описания автомата", "[03.Synthesis]") {
    const char *text = "name demo\n"
                       "inputs a b\n"
                       "outputs 0 1\n"
                       "states S T\n"
                       "S a -> T 1\n"
                       "S b -> S 0\n"
                       "T a -> T 0\n"
                       "T b -> S 1\n";
    FILE *file = std::tmpfile();
    struct Machine machine;
    struct Synthesis synthesis;

    REQUIRE(file != NULL);
    std::fputs(text, file);
    std::rewind(file);

    REQUIRE(machine_read(&machine, file));
    std::fclose(file);

    REQUIRE(machine.input_count == 2);
    REQUIRE(machine.state_count == 2);
    REQUIRE(machine.output_count == 2);
    REQUIRE(std::strcmp(machine.name, "demo") == 0);

    REQUIRE(synthesis_run(&synthesis, &machine));
    REQUIRE(synthesis.state_bits == 1);
    REQUIRE(synthesis.output_bits == 1);
    REQUIRE(synthesis_verify(&synthesis, &machine));
}

TEST_CASE("Незаданные переходы остаются безразличными", "[03.Synthesis]") {
    struct Machine machine;
    struct Synthesis synthesis;

    /* Три состояния кодируются двумя разрядами: код 11 не соответствует
       ни одному состоянию, и наборы с ним обязаны быть безразличными. */
    build_coin(&machine);
    REQUIRE(synthesis_run(&synthesis, &machine));

    {
        int unused_state_row = (0 << synthesis.state_bits) | 3; /* q = 11 */
        REQUIRE(bool_get(&synthesis.phi[0], unused_state_row) == BOOL_DONT_CARE);
        REQUIRE(bool_get(&synthesis.psi[0], unused_state_row) == BOOL_DONT_CARE);
    }
}
