# Integer Arithmetic Audit

## Cycle 64: ctz builtin and de Bruijn parity

- **Date:** 2026-07-28
- **Controller draw:** seed `1851309276`, eligible pool `52,53,72,74,77,81,82,84,87,89,95,97`, index `0`, goal `52`
- **Audit base:** `origin/master` `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`
- **Audit HEAD:** `60f8e5f78bb7290e57f1f339c2365a90647644a`
- **Branch:** `codex/fuzz-oracles`
- **Dirty state before and after:** clean
- **Protected worktrees:** Bitcoin Core and the user secp256k1 checkout were read-only for this cycle

### Hypothesis and scope

The distinct hypothesis was that `secp256k1_ctz32_var_debruijn`,
`secp256k1_ctz64_var_debruijn`, or the compiler-selection branches in
`src/util.h:384-443` could return the wrong trailing-zero count at an extreme
bit position or under a compiler where the selected builtin width differs.
These helpers have a nonzero-input contract, and the production callers in
`src/modinv32_impl.h:270,344` and `src/modinv64_impl.h:248,335` use sentinel
bits to preserve it. Modular inversion feeds libsecp ECDSA/Schnorr and public
key operations used by Bitcoin Core through `src/key.cpp`, `src/pubkey.cpp`,
and `src/script/interpreter.cpp`.

The source comment says the de Bruijn helpers are fallback implementations but
exposes them for separate testing. The existing `run_ctz_tests` covers four
fixed values per width shifted through the width, but does not independently
compute the result or exercise a large randomized set. `git blame` attributes
the implementation to `de0a643c` (2020-10-11); `git log -S` found no later ctz
fix. Searches of all existing journals found no ctz-specific finding. The
DER-length, scratch-size, scalar-inverse, and ecmult-count families were
excluded as already indexed cells.

### Verification

The disposable harness was `/tmp/secp256k1-goal52-ctz/ctz_oracle.c`, SHA-256
`07a7cc9534a6a997dcd9aa6dc81b80819e310a448a08938920a5c993690ffdeb`. Its
loop oracle counts zero bits without using any libsecp helper. It checked all
32 single-bit values, all 64 single-bit values, and 100,000 deterministic
Xorshift values at each width. It compared both the direct de Bruijn helper
and the compiler-selected helper and accumulated a digest over the inputs and
results.

Commands and key output:

```text
clang -std=c99 -O0 -Wall -Wextra -Werror -Wno-unused-function -I. ctz_oracle.c -o ctz-clang-o0
./ctz-clang-o0
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f

gcc -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -I. ctz_oracle.c -o ctz-gcc-o2
./ctz-gcc-o2
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f

clang -std=c99 -O2 -DFORCE_FALLBACK ...
./ctz-clang-fallback
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f

gcc -std=c99 -O2 -DFORCE_FALLBACK ...
./ctz-gcc-fallback
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f

clang -std=c99 -O2 -DVERIFY -fsanitize=address,undefined -DFORCE_FALLBACK ...
./ctz-clang-verify-asan
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f

gcc -std=c99 -O2 -DVERIFY -fsanitize=undefined -DFORCE_FALLBACK ...
./ctz-gcc-verify-ubsan
ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f
```

The forced-fallback builds intentionally redefine compiler feature macros in
the disposable harness; GCC emitted only the expected macro-redefinition
warnings. A `MUTATE_FALLBACK` negative control changed the result for input
`1` and failed immediately with:

```text
mismatch32 x=00000001 expected=0 fallback=1 selected=0
```

Project controls also passed:

```text
/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests --target=ctz_tests --target=modinv_tests --iterations=4 --seed=52 --log=1
Test ctz_tests PASSED
Test modinv_tests PASSED

/mnt/my_storage/secp256k1-build/current-full-int64-20260726/bin/tests --target=ctz_tests --target=modinv_tests --iterations=4 --seed=52 --log=1
Test ctz_tests PASSED
Test modinv_tests PASSED

/mnt/my_storage/secp256k1-build/oracles-next-msan/bin/tests --target=ctz_tests --target=modinv_tests --iterations=2 --seed=52 --log=1
Test ctz_tests PASSED
Test modinv_tests PASSED
```

### Verdict

**Dismissed: no clean-master integer defect.** Both width-specific fallback
tables and all selected builtin paths agreed with the independent oracle at
every tested bit position and randomized input. No sanitizer diagnostic,
modinv regression, out-of-range table index, or width-dependent result was
observed. The negative control proves the harness would report a changed
result. No production or test source change is justified, so this cycle has a
journal-only evidence commit.

Master-relative severity is none. The test does not execute on a big-endian
or non-GNU compiler, and it does not prove behavior for a zero input because
zero is outside the documented contract. Reopen if a new caller passes zero,
a new compiler builtin-selection branch appears, or an architecture build
reports a ctz/modinv divergence. The next queue retains the other cells in
`52,53,72,74,77,81,82,84,87,89,95,97` and excludes only this ctz cell.
