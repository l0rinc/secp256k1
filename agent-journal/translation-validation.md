# Compiler-Transformation Validation and Miscompile Isolation

## Selection

- Catalog goal: `78`
- Draw seed: `3923475549`
- Eligible slot: `77` of 98
- Selected on: `2026-07-27`
- Branch at selection: `codex/fuzz-oracles`
- HEAD at selection: `ebc658d0`
- Base at selection: `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`
- Status: active

## Cycle 2026-07-27: `secp256k1_int_cmov` translation check

### Hypothesis and contract

The bounded hypothesis was that compiler transformation of the inline
`secp256k1_int_cmov` helper in `src/util.h:330-347` could either select the
wrong operand or introduce a flag-dependent conditional branch after
optimization or LTO. Its documented domain is `flag` 0 or 1 and initialized,
non-negative `*r` and `*a`; the expected result is `a` for flag 1 and `r` for
flag 0. The helper is security-sensitive because its comment promises a
constant-time conditional move.

### Evidence

The environment was x86_64 Linux with Clang 22.1.7 and GCC 16.1.0. The
independent scratch harness was
`/tmp/secp256k1-translation-78/cmov_harness.c`, hash
`c18caa6ebdde05c7d34279159d7c468bb5d15528d3d00afa614e6ce579ad01f7`.
It generated one million deterministic vectors from a fixed 64-bit LCG,
masked both operands to the documented non-negative `int` domain, alternated
the valid flags 0 and 1, checked the result against a separate conditional
reference, and accumulated the output digest.

The exact build family was:

    clang/gcc -std=c99 -DSECP256K1_BUILD -O{0,2,3,s} -g -fno-omit-frame-pointer
    clang -O2 -flto -fuse-ld=lld and gcc -O2 -flto

All ten executables printed:

    ok vectors=1000000 digest=a8c75ce8c4009cc0

The output hash for every run was
`6a8afbff6de26bb2f37ede0ecae8890f45023fcbe7683d621570f634ffaa6a54`.
The extracted `probe` body was disassembled with
`objdump -d --no-show-raw-insn`; all ten bodies contained zero conditional or
loop branches. Clang O2 reduced to arithmetic/bitwise operations equivalent to
the reference, including the volatile flag load, and GCC emitted the same
branch-free shape. The source helper hash was
`07c07b7a9efab479e492cc48f6eedf4e0b6f19772e59d7b27fd60089bfe1cb9b`.

As an independent project-level check, the existing Clang ASan/UBSan Debug
builds ran `bin/tests -i=1 -j=2 -t=cmov_tests -log=1` and
`bin/tests -i=1 -j=2 -t=secp256k1_memczero_test -log=1`; both passed. The
same `cmov_tests` passed in the integrated recovery build. No compiler,
sanitizer, or test process remained after the checks.

The first scratch version generated signed negative operands and was discarded
before these results; the final harness stayed inside the documented domain.

### Verdict and limits

The hypothesis is **dismissed** for this kernel on the tested compilers,
optimization levels, LTO modes, and valid input domain. No production change,
regression test, or finding commit is justified. The result is not a proof of
all architectures, compilers, `VERIFY` diagnostics, inline-assembly paths, or
all constant-time helpers; Alive2 and cross-architecture execution were not
available. The next distinct hypotheses are `secp256k1_memzero_explicit`
dead-store preservation and a backend-specific constant-time helper check.

## Cycle 2026-07-27: `secp256k1_memzero_explicit` dead-store check

The second bounded hypothesis was that optimization or LTO could remove the
actual wipe of a local secret buffer because the buffer was otherwise dead.
The scratch harness
`/tmp/secp256k1-translation-78/wipe_harness.c` (hash
`d79e4dc674c4748319761f6ba9ee076cbb1efd87d2221ba4752c5c53f1d81550`)
initialized a 32-byte local buffer, called
`secp256k1_memzero_explicit`, then copied the first post-clear byte into a
volatile global. Every Clang/GCC `O0`, `O2`, `O3`, `Os`, and `O2+LTO` build
printed `ok post-clear=0`; every run output had hash
`21467fdbcb65fb0ce6ba2d9c0f14b8a73d7689ffb547b883fe66fd6edb0bfb6b`.

Assembly inspection found concrete clearing operations in every optimized
body. Clang O2/LTO emitted `xorps` followed by two `movaps` zero stores;
GCC O2/O3/LTO emitted `pxor` followed by two `movaps` zero stores; GCC Os
emitted `rep stos`. The O0 builds retained the explicit helper call. The
optimized compilers removed the immediately-dead secret initialization, which
is unrelated to clearing and does not weaken the observed wipe. No production
source change or regression test is justified.

This hypothesis is **dismissed** for the tested x86_64 compiler/toolchain
matrix. The combined goal remains active because cross-architecture code
generation, `VERIFY` instrumentation, and the backend-specific constant-time
helper queue remain untested. No compiler bug, secret disclosure, or
constant-time regression was demonstrated.

## Cycle 2026-07-27: portable versus forced-int64 backend cmov differential

### Hypothesis and contract

The next hypothesis was that compiler lowering of the backend-specific field
and scalar conditional moves could differ between the native x86_64
`5x52`/`4x64` representations and the forced-int64 `10x26`/`8x32`
representations. The relevant contracts require a flag of 0 or 1, preserve
the destination for 0, select the source for 1, and avoid a flag-dependent
branch in the production mask operation. The field and scalar fuzzers each
contain independent reference checks around these calls.

### Evidence

The native and forced builds used Clang 22.1.7, ASan/UBSan, `-DVERIFY`,
`-DVALGRIND`, `-fsanitize=fuzzer-no-link`, and the generated CMake `-O1 -g`
flags. The forced build added `-DUSE_FORCE_WIDEMUL_INT64=1`; native selected
`SECP256K1_WIDEMUL_INT128` from the platform. The exact target flag files were
`/tmp/secp256k1-oracles-next-build-native/src/CMakeFiles/fuzz_{field,scalar}.dir/flags.make`
and the corresponding forced-int64 files.

All 21 field corpus inputs and all 10 scalar corpus inputs passed individually
in both configurations with `-runs=1 -seed=78 -timeout=30 -rss_limit_mb=0`.
The four replay status logs contained no nonzero return code and no libFuzzer
artifact. The source hashes were:

    src/fuzz/field.c             2153ea88334cd47330c6f9e2308871e2834a23e70de13419fdab612aa1ac18f9
    src/fuzz/scalar.c            6dd32e857d391677503a63c21e44b1279bde646b7574c22f5f78e809863dd305
    src/field_5x52_impl.h        2c42559c21f23ca02848b76b29c5a2aca36d2fd932b902adfc9ba755ff850f6c
    src/field_10x26_impl.h       2c642c6c87d53e358c8254b0e3985c2917f999af362afe6dd8e5d0c1478f8289
    src/scalar_4x64_impl.h       fd094854ed4d3a752384ee164fddc991313244abdc95f5e21c14c30ba321fdd8
    src/scalar_8x32_impl.h       b37d6b65c1335fb014d58e4cdc0530232a30801f9ec583b2d037a8a44def093f

Short mutation-guided runs used the same fixed seed and a four-second budget.
Native field/scalar executed 66/55 units and added 12/37 units; forced-int64
field/scalar executed 63/40 units and added 16/22 units. Peak RSS was
41-45 MiB, every run returned zero, and no artifact was produced. Coverage
counts differ by representation and are not treated as a correctness metric.

For compiler-output evidence, the independent scratch wrapper
`/tmp/secp256k1-translation-78/backend-cmov-harness.c` (hash
`cc2841a9c8825e4a9658103bc6672f462a762a9b0faa5a0ab1b54712325e3df5`)
materialized `probe_fe` and `probe_scalar` without `VERIFY`, sanitizers, or
coverage. Clang and GCC `-O2` objects were built for native int128 and
forced-int64 backends. All eight inspected bodies had zero conditional or
loop branches and used mask arithmetic (`and`/`or`); Clang used scalar 64-bit
operations for native and 32-bit operations for forced-int64, while GCC also
used branch-free SSE mask operations. The object-only native builds emitted
warnings about unrelated unmaterialized int128 helpers from included headers;
the cmov wrapper objects themselves were emitted and inspected successfully.

### Verdict and limits

The hypothesis is **dismissed** for the tested x86_64 native and forced-int64
backends, compiler builds, sanitizer corpus oracles, and `-O2` production
cmov lowering. No production change, regression test, or finding commit is
justified. This does not prove all optimization levels, architecture-specific
assembly, cross builds, compiler versions, or secret-dependent behavior in
other helpers. The next distinct queue is a compiler/architecture matrix
around another small constant-time arithmetic helper, followed by a cross-
architecture or Alive2 reduction if the required toolchain is available.

## Handoff

Start by checking the worktree, compiler versions, supported optimization and
sanitizer builds, and any existing Alive2/IR/compiler evidence in
`src/fuzz/README.md` and `agent-journal/`. Select one bounded arithmetic,
aliasing, shift, overflow, or constant-time kernel with a deterministic
oracle. Separate source undefined behavior, test failure, inline-assembly
contract, and compiler defect. Keep any reduction and generated artifacts in
`/tmp`, record exact commands and hashes, and do not claim a compiler bug from
an optimization difference without a minimized reproducer and independent
verification. The next queue is a compiler/architecture matrix around another
small constant-time arithmetic helper, followed by a cross-architecture or
Alive2 reduction if the required toolchain is available. Do not repeat the
three dismissed hypotheses unless compiler, source, or architecture evidence
changes.
