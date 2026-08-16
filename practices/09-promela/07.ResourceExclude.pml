/* verify: violation
 *
 * Наивная защита разделяемого ресурса переменной busy. Проверка и установка
 * не атомарны, поэтому оба процесса проходят (!busy) до того, как первый
 * успел выставить busy, и оказываются в критической секции одновременно —
 * assert(mutex <= 1) нарушается. Работающий алгоритм — в 08.MutexAlgorithm.
 */
bool busy;
byte mutex;

proctype P(bit i) {
    (!busy) -> busy = true;
    mutex++;
    printf("%d -> %d\n", i, mutex);
    mutex--;
    busy = false
}

active proctype invariant() {
    assert( mutex <= 1 )
}

init {
    atomic {
        run P(0); run P(1)
    }
}
