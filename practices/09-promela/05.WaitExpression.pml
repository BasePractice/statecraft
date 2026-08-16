/* verify: violation
 *
 * Блокировка на условии. Процесс A ждёт (x > 2 && y == 1), но увеличить x
 * некому: B не запущен. Верификатор находит invalid end state — процесс
 * навсегда остаётся в ожидании. Нарушение здесь и есть учебная цель.
 */
int x;

proctype B() {
    x = x + 1
}

proctype A() {
    int y = 1;

    skip;
    x = 2;
    (x > 2 && y == 1);
    printf("This is end\n");
}

init {
    run A()
}
