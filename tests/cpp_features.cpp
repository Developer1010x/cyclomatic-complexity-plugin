#include <vector>

/* EXPECT: try_catch 3 */
int try_catch(int x) {
    try {
        if (x < 0) throw 1;         /* if=1 */
    } catch (int) {                 /* catch=1 */
        return -1;
    }
    return x;
}

/* EXPECT: range_for 2 */
int range_for(const std::vector<int> &v) {
    int sum = 0;
    for (int i : v) sum += i;       /* range-for=1 */
    return sum;
}

/* Lambda complexity belongs to the lambda, not the enclosing function.
   EXPECT: with_lambda 1 */
int with_lambda(int x) {
    auto f = [](int v) { return v > 0 ? v : -v; };
    return f(x);
}
