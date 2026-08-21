/* Expected complexity is written next to each function and checked by run-tests.sh */

/* EXPECT: simple 1 */
int simple(int x) { return x + 1; }

/* EXPECT: one_if 2 */
int one_if(int x) {
    if (x > 0) return 1;
    return 0;
}

/* EXPECT: logical_ops 3 */
int logical_ops(int a, int b) {
    if (a > 0 && b > 0) return 1;   /* if=1, &&=1, base=1 */
    return 0;
}

/* EXPECT: loop_with_break 3 */
int loop_with_break(int n) {
    for (int i = 0; i < n; i++) {   /* for=1 */
        if (i == 5) break;          /* if=1 */
    }
    return 0;
}

/* EXPECT: switch_three_cases 4 */
int switch_three_cases(int x) {
    switch (x) {                    /* switch itself: 0 */
    case 1: return 10;              /* case=1 */
    case 2: return 20;              /* case=1 */
    case 3: return 30;              /* case=1 */
    default: return 0;              /* default: 0 */
    }
}

/* EXPECT: ternary 2 */
int ternary(int x) { return x > 0 ? 1 : -1; }

/* EXPECT: do_while 2 */
int do_while(int n) {
    int i = 0;
    do { i++; } while (i < n);
    return i;
}
