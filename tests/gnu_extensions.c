/* GCC-specific syntax. Requires -std=gnu11 (the default for gcc, and what
   clang uses with -std=gnu11). This file is the reason the tool is useful
   on a GCC codebase rather than only a Clang one. */

/* EXPECT: elvis 2 */
int elvis(int x) {
    return x ?: -1;                 /* GNU binary conditional: 1 branch */
}

/* EXPECT: statement_expr 2 */
int statement_expr(int x) {
    int y = ({ int t = x; t > 0 ? t : -t; });   /* stmt expr + ternary */
    return y;
}

/* EXPECT: computed_goto 2 */
int computed_goto(int x) {
    void *target = &&done;          /* GNU label address */
    goto *target;                   /* indirect branch: 1 */
done:
    return x;
}

/* EXPECT: attributed 2 */
__attribute__((noinline, unused))
static int attributed(int x) {
    if (x) return 1;
    return 0;
}

/* EXPECT: typeof_and_range 3 */
int typeof_and_range(int x) {
    __typeof__(x) y = x;
    switch (y) {
    case 0 ... 5: return 1;         /* GNU case range: counted as one case */
    case 6 ... 9: return 2;
    }
    return 0;
}
