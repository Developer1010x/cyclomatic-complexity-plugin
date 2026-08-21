# cyclomatic-complexity

A Clang plugin that reports the [cyclomatic complexity](https://en.wikipedia.org/wiki/Cyclomatic_complexity)
of every function defined in the file being compiled. It runs as part of a normal
compile, so it sees exactly the code the compiler sees — after preprocessing, with
your real include paths and feature flags.

It analyses C and C++ written for **either GCC or Clang**, including the GNU
extensions GCC codebases rely on: statement expressions, `?:` with an omitted
middle operand, computed `goto`, case ranges, `__typeof__` and `__attribute__`.

## What it measures

Cyclomatic complexity is the number of linearly independent paths through a
function. On a control-flow graph it is:

```
M = E - N + 2P
```

`E` edges, `N` nodes, `P` connected components. For a single function `P = 1`,
and this reduces to an equivalent that is far cheaper to compute:

```
M = (number of decision points) + 1
```

That equivalence is why this plugin walks the AST instead of building a CFG.

**Counted as decision points:** `if`, `for`, range-`for`, `while`, `do`, each
`case` label, `catch`, the ternary `?:`, the GNU `?:` binary form, `&&`, `||`,
and computed `goto`.

**Not counted:** the `switch` statement itself (its `case` labels are counted
instead), `default` (it is the already-existing fall-through path), `else`
(it adds no new path beyond its `if`), and non-logical binary operators.

Lambda bodies are scored as their own functions, not folded into the enclosing
one.

## Requirements

- LLVM/Clang 15 or newer, with development headers
- CMake 3.20+

On Debian/Ubuntu:

```sh
sudo apt install llvm-18-dev libclang-18-dev clang-18 cmake
```

## Build

```sh
./build.sh                  # or: LLVM_VERSION=17 ./build.sh
```

Produces `build/CyclomaticComplexity.so`.

## Use

```sh
clang -fsyntax-only -fplugin=build/CyclomaticComplexity.so yourfile.c
```

Output is `line`, `complexity`, `name`, tab-separated:

```
12	7	process_input
45	3	main
```

Write to a file instead of stdout:

```sh
clang -fsyntax-only -fplugin=build/CyclomaticComplexity.so \
      -Xclang -plugin-arg-cyclomatic-complexity -Xclang -o \
      -Xclang -plugin-arg-cyclomatic-complexity -Xclang report.txt \
      yourfile.c
```

### Analysing a GCC codebase

The plugin needs Clang to parse the file, but the file can be GCC-flavoured.
Pass the same flags the GCC build uses:

```sh
clang -fsyntax-only -std=gnu11 -D_GNU_SOURCE -I./include \
      -fplugin=build/CyclomaticComplexity.so \
      src/*.c
```

Clang accepts most GCC flags. A handful of GCC-only options (`-fanalyzer`,
some `-mabi=` values) have no Clang equivalent and must be dropped.

## Tests

```sh
./run-tests.sh
```

Each test file carries `EXPECT: <function> <complexity>` annotations derived by
hand. The annotation is the specification: if a test fails, work out which side
is wrong before changing either.

## Status and limits

- Only functions **defined in the main file** are reported. Functions from
  included headers are skipped, which is what makes the output readable.
- Template functions are reported per instantiation, so a template used with
  three types appears three times.
- The plugin requires Clang. It cannot load into GCC — the two have entirely
  different plugin APIs, and a GCC version would be a separate implementation
  against GIMPLE.

## History

This began as a college project in 2024, written to understand how compilers
work from the inside — specifically how a compiler represents code once it has
stopped being text and become a tree. Cyclomatic complexity was the excuse: it
is a small enough metric that the interesting part is not the arithmetic but
the AST walk needed to gather it, and the questions it forces are the real
ones. What counts as a branch? Why is a `switch` not one decision but several?
Where does a lambda's control flow belong?

The original is in `Sneha1m/llvm2024Projects` (PR #1). It mixed the libclang C
API and the Clang C++ AST API in one file, with the C half unreferenced and its
header never included, and hardcoded a macOS Homebrew path in CMake. It did not
build.

This is a rewrite of the working half, brought up to LLVM 18, corrected where
the original counted wrong, and given tests. The 2024 code is worth diffing
against `src/` — the gap between the two is most of what the project taught.

## Licence

MIT
