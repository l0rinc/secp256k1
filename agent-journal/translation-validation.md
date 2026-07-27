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

## Cycle 2026-07-27: `secp256k1_scalar_cadd_bit` compiler lowering

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `ec2fedbbe52300916713323754b22ca746d7eb0e`, with base `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a` and no running fuzz, sanitizer, compiler, or profiling jobs. Clang 22.1.7, GCC 16.1.0, CMake, and Ninja were available; no Alive2 or QEMU executable was installed. The catalog, this journal, `src/fuzz/README.md`, scalar history, and prior journals were searched. Existing history and corpora already cover cadd-bit carry and no-op boundaries, but no prior journal entry verified compiler lowering of this helper.

### Hypothesis and contract

The bounded hypothesis was that optimization or LTO could make the flag in
`secp256k1_scalar_cadd_bit` influence control flow, or that the native
`4x64` and forced-int64 `8x32` implementations could produce different
results at limb carries and the order boundary. The public internal contract
at `src/scalar.h:52` requires `flag` 0 or 1, `bit < 256`, and a result that
does not overflow. The implementation uses a volatile flag and deliberately
maps flag zero to a no-op bit outside the represented limbs.

### Evidence

The independent scratch harness
`/tmp/secp256k1-translation-78/cadd-bit-harness.c` (SHA-256
`aa99d9ce69201d6a119ab354b40205a375d9c0f4e857bfa3d8eaad07dd2329c6`)
used a byte-level reference, not scalar arithmetic, for three families at all
256 bit positions: zero plus the selected bit, a carry chain of all lower
bits for positions below 255, and `order - 1 - 2^bit` plus the selected bit.
It checked both flags for 767 total cases and accumulated a deterministic
digest. The expected order boundary was encoded independently in bytes.

Clang and GCC native and `-DUSE_FORCE_WIDEMUL_INT64` builds all printed:

    ok cases=767 digest=4cf0c5e0e57450c3

That output was identical at `O0`, `O2`, `O3`, `Os`, and `O2+LTO` for all
eight compiler/backend combinations. Clang ASan/UBSan `-DVERIFY -DVALGRIND`
executions for native and forced-int64 also printed the same result, with no
diagnostic or nonzero exit. The implementation hashes were
`src/scalar_4x64_impl.h=fd094854ed4d3a752384ee164fddc991313244abdc95f5e21c14c30ba321fdd8`
and
`src/scalar_8x32_impl.h=b37d6b65c1335fb014d58e4cdc0530232a30801f9ec583b2d037a8a44def093f`.

The four non-LTO `O2` `probe` bodies and the four LTO `O2` bodies were
disassembled with `objdump -d --no-show-raw-insn --disassemble=probe`. Every
body contained zero conditional or loop jumps. Clang and GCC used `sete` to
form the per-limb masks, followed by shifts and add/adc carry propagation;
the native path used four 64-bit limbs and the forced path eight 32-bit limbs.
No compiler warning was emitted by the optimized or LTO builds.

### Verdict and limits

The hypothesis is **dismissed** for the tested x86_64 native and forced-int64
representations, Clang/GCC optimization and LTO matrix, independent byte
oracle, and ASan/UBSan/VERIFY execution. No production change, regression
test, or finding commit is justified. This is not evidence for other
architectures, assembly implementations, compilers, invalid flag/bit domains,
or unrelated scalar arithmetic. The next distinct queue is a compiler and
architecture check of another constant-time scalar/field helper; prioritize a
path with an independent algebraic or byte-level oracle and use cross-
architecture or Alive2 reduction if the missing tools become available.

## Cycle 2026-07-27: `secp256k1_scalar_cond_negate` compiler lowering

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `744637d3`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, with no running
fuzz, sanitizer, compiler, or profiling jobs. Clang 22.1.7 and GCC 16.1.0
were available; no QEMU or Alive2 executable was installed. The catalog,
prior goal journal, scalar implementation history, `src/fuzz/README.md`, and
existing finding notes were searched before selecting this distinct helper.

### Hypothesis and contract

The hypothesis was that optimization or LTO could make the flag in
`secp256k1_scalar_cond_negate` influence a branch, or that native `4x64` and
forced-int64 `8x32` carry paths could disagree for zero, nonzero, and order
boundary values. The contract at `src/scalar.h:81-83` requires flag 0 or 1,
promises constant-time conditional negation, and returns -1 when the flag is
one and 1 otherwise. Zero must remain zero when negated.

### Evidence

The independent scratch harness
`/tmp/secp256k1-translation-78/cond-negate-harness.c` (SHA-256
`6ff57b4b52da49e0599806954bb387ac433efa7151f8c4c1b011539b4d273991`)
implemented byte-level `order - input` reference arithmetic. It checked zero,
every power of two, `order - 2^bit` for every bit, and 256 deterministic
nonzero values, exercising both flags for 769 cases. It independently checked
the return value and serialized result; it did not call the production negate
helper for its expected values.

Clang and GCC native and `-DUSE_FORCE_WIDEMUL_INT64` executions all printed:

    ok cases=769 digest=607e49bc2d0edb6f

The same digest held at `O0`, `O2`, `O3`, `Os`, and `O2+LTO` for all eight
compiler/backend combinations. Clang ASan/UBSan `-DVERIFY -DVALGRIND` native
and forced-int64 builds also passed the complete harness with no diagnostics.
The implementation hashes were
`src/scalar_4x64_impl.h=fd094854ed4d3a752384ee164fddc991313244abdc95f5e21c14c30ba321fdd8`
and
`src/scalar_8x32_impl.h=b37d6b65c1335fb014d58e4cdc0530232a30801f9ec583b2d037a8a44def093f`.

The eight optimized `probe` bodies, including the four LTO outputs, were
disassembled with `objdump -d --no-show-raw-insn --disassemble=probe`. Each
had zero conditional or loop jumps. The generated code used flag-derived
mask arithmetic and `sete`/carry operations, not flag-dependent branches.
The focused existing ASan/UBSan `fuzz_scalar` binaries then executed the
`scalar zero one predicates\n` trigger once each with `-seed=78`; native and
forced-int64 both exited zero after 94 ms and 124 ms respectively, with no
artifact. The first attempt was rejected before target startup because the
specified scratch artifact directory did not exist; creating it and rerunning
produced the results above, so the setup error is not counted as target
evidence.

### Verdict and limits

The hypothesis is **dismissed** for the tested x86_64 native and forced-int64
representations, Clang/GCC optimization and LTO matrix, independent byte
oracle, sanitized `VERIFY` execution, and production fuzzer trigger. No
production change, regression test, or finding commit is justified. This
does not cover other architectures, assembly backends, compilers, invalid
flag domains, or unrelated scalar operations. The next distinct queue is a
compiler/architecture check of another constant-time field or scalar helper;
use cross-architecture or Alive2 reduction if the missing tools become
available.

## Cycle 2026-07-27: AArch64 `secp256k1_scalar_cond_negate` translation

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `f34c5d26`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, with no running
fuzz, sanitizer, compiler, or profiling jobs. The catalog, this journal,
`src/fuzz/README.md`, scalar history, and prior finding notes were searched.
Clang 22.1.7 and `aarch64-linux-gnu-objdump` were available. No QEMU runner,
AArch64 sysroot, Alive2, CBMC, or KLEE executable was available. A public
GitHub API search found no exact conditional-negation issue; the currently
visible matching PR concerned an unrelated test-result oracle and was not
used as evidence.

### Hypothesis and contract

The bounded hypothesis was that Clang's AArch64 lowering of the constant-time
conditional-negation helper could introduce a flag-dependent branch, or that
the native 4x64 and forced-int64 8x32 representations could produce visibly
different lowering in the helper's carry and zero-selection paths. The
contract at `src/scalar.h:81-83` requires flag 0 or 1, constant-time
conditional negation, and return value -1 when negated and 1 otherwise; zero
must remain zero when negated.

### Evidence

The independent byte-oracle harness
`/tmp/secp256k1-translation-78/cond-negate-harness.c` has SHA-256
`6ff57b4b52da49e0599806954bb387ac433efa7151f8c4c1b011539b4d273991`.
The AArch64 compile matrix used:

    clang --target=aarch64-linux-gnu -std=c99 -O{0,2,3,s} -g \
      -I/tmp/secp256k1-oracles-next/src [optional -DUSE_FORCE_WIDEMUL_INT64] \
      -c /tmp/secp256k1-translation-78/cond-negate-harness.c -o <object>

Native and forced-int64 objects were produced successfully at all four
optimization levels. `aarch64-linux-gnu-objdump -d --no-show-raw-insn
--disassemble=probe` found zero `b.cond`, `cbz`, `cbnz`, `tbz`, or `tbnz`
instructions in every probe output. The O2 native and forced-int64 probes
use AArch64 `csel`, `cneg`, and carry instructions for flag-derived data;
the O0 helper disassemblies likewise contain zero conditional or loop branch
mnemonics. The object hashes were:

    native:      O0 304b63e7e8d34c1a27772e8e39556f68964fba5f921c198fd47464a56d203045
                 O2 ae2d275c81009f4f7bf952f64f62e592b4ab591b47e42f5ca7eb81f534931d15
                 O3 f7c8ed17e4c303c81c27fe6bfd038774cc89cb224bfa7c9ad91f6c7825888510
                 Os aa1a22a2cf32d7eaac1ed9ea69fcf670c4ec00a65d9354345d870c41050d994d
    forced-int64: O0 44958373e6d3d546307a479600de520ef57c8bd5aa27fed5c7c12ddeff41998c
                 O2 ee297c490999d9f5552925dd8e43ce1c68c1861d0c122a3f4237c1f0abb34e28
                 O3 965b36d5aef58a1ababf59ca2bc3a8355f60dba679c1021095f6c7ec58588ad0
                 Os dc902ec6b41ed2ce894dc4e79ce859a472483b22c8e7f3df14be361a8c9995bb

Compile-only `-O1 -DVERIFY -DVALGRIND` builds also succeeded for both
representations with zero warning bytes; their object hashes were
`a68b4792aff0e0ccee73b7344b59e0981ff76a1992ad792f29f907d667274102` and
`c97c45c1b1dab53f994b652dfb913352be637f7debc4a786ba6d52d0fc79bde5`.
Attempts to compile ARMv7 and RISC-V variants stopped at the host's missing
`/usr/include/bits/libc-header-start.h` sysroot header, before any object was
created. No AArch64 executable was linked or run, so this cycle is codegen
evidence only, not a semantic execution result.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang AArch64 object lowering,
native and forced-int64 representations, optimization levels, and compile
diagnostics. No production change, regression test, or finding commit is
justified. This does not prove AArch64 runtime behavior, cover other
compilers, assembly paths, ARMv7/RISC-V, invalid flag domains, or establish
semantic equivalence without execution. The next distinct queue is another
small constant-time field or scalar helper; revisit AArch64 execution or the
ARMv7/RISC-V matrix if a runner or sysroot becomes available.

## Cycle 2026-07-27: `secp256k1_scalar_add` compiler lowering

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `63eba06e`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, with no running
fuzz, sanitizer, compiler, or profiling jobs. The catalog, this journal,
`src/fuzz/README.md`, `src/fuzz/scalar.c`, scalar implementation history,
and prior findings were searched. The public scalar contract is at
`src/scalar.h:48-49`; the native and forced-int64 implementations are at
`src/scalar_4x64_impl.h:96-120` and `src/scalar_8x32_impl.h:119-146`.
The existing fuzzer has an independent base-2^16 addition oracle and
alias checks at `src/fuzz/scalar.c:172-196` and `:568-575`, but no prior
journal cycle had checked this helper's compiler lowering directly.

### Hypothesis and contract

The bounded hypothesis was that optimization or LTO could introduce a
data-dependent jump in scalar addition's carry/overflow/reduction path, or
that the 4x64 and 8x32 implementations could disagree at modular-order
boundaries or under supported output aliasing. The contract is addition
modulo the group order `n`, returning whether the unreduced sum overflowed
the order. Inputs in this harness were canonical scalars below `n`.

### Evidence

The standalone byte-level harness
`/tmp/secp256k1-translation-78/scalar-add-harness.c` has SHA-256
`90ff74ca7b24bdab9e949286e5d256fdbfc7288981a961a2e00ace6455176efa`.
Its reference adds 33 bytes, compares the extended sum with `n`, subtracts
`n` across all 33 bytes when needed, and independently checks the serialized
result and overflow bit. It generated 773 values: zero, one, two, `n-1`,
`n-2`, every `2^k`, every `n-2^k` for `0 <= k < 256`, and 256 deterministic
values below `2^255`. It checked all 597,529 ordered pairs three ways:
distinct output, `r == a`, and `r == b`.

The native and `-DUSE_FORCE_WIDEMUL_INT64` Clang and GCC builds at `O0`,
`O2`, `O3`, and `Os` all printed exactly:

    ok values=773 pairs=597529 digest=4871192a2e4ff35b

The `-O2 -flto` Clang/GCC native and forced-int64 executables printed the
same result. `objdump -d --no-show-raw-insn --disassemble=probe` found zero
x86 jump mnemonics in all four non-LTO O2 probe objects and all four LTO
executables; the emitted code used `setcc`, `cmov`, and carry arithmetic.
Clang `-O1 -fsanitize=address,undefined -DVERIFY -DVALGRIND` native and
forced-int64 executions also printed the same result with no diagnostics.

The AArch64 compile matrix used:

    clang --target=aarch64-linux-gnu -std=c99 -g -O{0,2,3,s} \
      -I/tmp/secp256k1-oracles-next/src [optional -DUSE_FORCE_WIDEMUL_INT64] \
      -c /tmp/secp256k1-translation-78/scalar-add-harness.c -o <object>

All eight objects compiled successfully. `aarch64-linux-gnu-objdump -d
--no-show-raw-insn --disassemble=probe` found zero `b.cond`, `cbz`, `cbnz`,
`tbz`, or `tbnz` instructions for every representation and optimization
level. AArch64 `-O1 -DVERIFY -DVALGRIND` compile-only builds also succeeded
with zero warning bytes. The object hashes were:

    native:      O0 7c6f15c3fddce3d19df1fa04f8f3b4bb914573c0f9895bfcd2fff9dda5ebd9f6
                 O2 e61345bef1f6b7de41c76eb1bfcaa818f725a36a244a1ae4bcf1315198467b65
                 O3 2d76891d74db7628da735e62c70c2e0cf032d752b3c3478727301b90f1e9af40
                 Os 980e07430b96dea5102da57a168384d6955c21be4c17ca525728a341dc90c52f
                 VERIFY 3737733e684d9da563ec8b6803ae280f0528b2e83cd49654b3f8cd05b8cf9862
    forced-int64: O0 7134f024915107154d6d1bcdb42ae234d700c2e3469bce8e95f95bf6d134db2c
                 O2 ec6036946c5b0b890725546ed2471647e22eb91d1b952ed9777b62c1bed9696d
                 O3 183436996727d717373624b6d03ee73894b9a7cdf29cc2beab91c0efd25d8ab7
                 Os ee5d56e075f2bb77c073385382def71a903528774643a5a991fa9d5bdb6fbfe2
                 VERIFY 5aa2de193cf703d6b48e67541fb50ffe0df88412b4d75e7c72bc949cf230aecf

The oracle sensitivity control copied the source to `/tmp`, changed both
backend calls from `secp256k1_scalar_reduce(r, overflow)` to
`secp256k1_scalar_reduce(r, 0)`, and ran Clang O2 in both representations.
Both deliberately mutated binaries exited 1 at the same minimal boundary:

    mutated native exit=1
    distinct mismatch overflow=1/1
    case left=1 right=3
    mutated forced-int64 exit=1
    distinct mismatch overflow=1/1
    case left=1 right=3

The first run of the uncorrected scratch oracle printed its own
`reference subtraction underflow` diagnostics because it subtracted from
only the low 256 bits when the 257th carry was set. It was discarded; the
33-byte subtraction fix above was applied before any result was accepted.
The first mutation-overlay compile also failed because the copied `util.h`
needed the sibling `include/` directory; that scratch dependency was added
and the mutation was rerun. Neither setup failure is target evidence.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 backends, LTO, sanitized execution, and Clang
AArch64 code generation. The independent oracle is mutation-sensitive and
found no arithmetic, aliasing, or overflow mismatch. No production change,
regression test, or finding commit is justified. This does not provide
AArch64 runtime execution, GCC AArch64 coverage, ARMv7/RISC-V coverage, or a
proof for noncanonical inputs outside the helper contract. The next distinct
queue is another small constant-time or overflow-sensitive field/scalar
helper; revisit cross-target execution if a runner or sysroot appears.

## Cycle 2026-07-27: `secp256k1_scalar_half` compiler lowering

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `7d00f608`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, with no running
fuzz, sanitizer, compiler, or profiling jobs. The catalog, this journal,
`src/fuzz/README.md`, `src/fuzz/scalar.c`, `src/tests.c`, scalar history,
and prior findings were searched. The public contract is at
`src/scalar.h:66-67`; the native and forced-int64 implementations are at
`src/scalar_4x64_impl.h:197-235` and `src/scalar_8x32_impl.h:238-284`.
The existing fuzzer has a separate byte-level half reference at
`src/fuzz/scalar.c:308-331` and alias testing at `:586-590`, but no prior
journal cycle had checked this helper's compiler lowering directly. The
public API search returned only unrelated PR #1058, so it was not used as
evidence.

### Hypothesis and contract

The bounded hypothesis was that optimization or LTO could introduce a
parity-dependent branch in scalar halving, or that native 4x64 and forced
int64 8x32 carry chains could disagree for odd/even or order-boundary
inputs. The contract is multiplication by the modular inverse of 2. Since
the group order `n` is odd, the independent reference computes `a/2` for
even `a` and `(a+n)/2` for odd `a` using a 33-byte add followed by a full
33-byte right shift. Inputs in this harness were canonical scalars below
`n`.

### Evidence

The standalone byte-level harness
`/tmp/secp256k1-translation-78/scalar-half-harness.c` has SHA-256
`9c793ae99b1fbdb56748adf78100fa510d565e2048fad3fb226c5a5520d54d58`.
It generated 773 values: zero, one, two, `n-1`, `n-2`, every `2^k`, every
`n-2^k` for `0 <= k < 256`, and 256 deterministic values below `2^255`.
It independently checked the serialized result and both distinct and
in-place output forms for every value.

The native and `-DUSE_FORCE_WIDEMUL_INT64` Clang and GCC builds at `O0`,
`O2`, `O3`, and `Os` all printed exactly:

    ok values=773 cases=773 digest=3641e4d1aece7199

The `-O2 -flto` Clang/GCC native and forced-int64 executables printed the
same result. `objdump -d --no-show-raw-insn --disassemble=probe` found zero
x86 jump mnemonics in all four non-LTO O2 probe objects and all four LTO
executables. Clang `-O1 -fsanitize=address,undefined -DVERIFY -DVALGRIND`
native and forced-int64 executions also printed the same result with no
diagnostics.

The AArch64 compile matrix used:

    clang --target=aarch64-linux-gnu -std=c99 -g -O{0,2,3,s} \
      -I/tmp/secp256k1-oracles-next/src [optional -DUSE_FORCE_WIDEMUL_INT64] \
      -c /tmp/secp256k1-translation-78/scalar-half-harness.c -o <object>

All eight objects compiled successfully. `aarch64-linux-gnu-objdump -d
--no-show-raw-insn --disassemble=probe` found zero `b.cond`, `cbz`, `cbnz`,
`tbz`, or `tbnz` instructions for every representation and optimization
level. AArch64 `-O1 -DVERIFY -DVALGRIND` compile-only builds also succeeded
with zero warning bytes. The object hashes were:

    native:      O0 cd08a18a052085091efc6fadc90fddfc354263b1e72f46203890e07529e26d5e
                 O2 77fd129d3198e44439a6730f7f95b8420aa91d02a58901ab7deecf4e3555ea37
                 O3 8acb037ff89828502d439a574baac17d89ff6ce098bec115848e8cc8727b21ed
                 Os af85be614decf651a38e7a4a7f9017e268c191a2424a1f85341fbac4016a4bd7
                 VERIFY 7622dc34c435f8543fd3564f950c5eb8388e49f17943d3461d98ae7de72ea552
    forced-int64: O0 276aa1fdef154dd825e034dc1c23233e057bdc5e8cb82d3d4ad2e8433ec4de6a
                 O2 2818978ea370cbf110ca4a80567133fc4c8b3c6d125400e4abf48cc39851c144
                 O3 6a1cd2d1d89ff1a7d80adf17808fef1624cf9bbb4231a0a61ba1f0bf82faa70a
                 Os e89dd34d63e363de087d20e941a7bf43079734f01dd4801db05a0fb41e84f142
                 VERIFY 8d29686610be0e09d90001621531eebe68f89a09095b99080f7f1fc36b3e330c

The oracle sensitivity control copied the source to `/tmp`, changed both
backend parity masks to zero, and ran Clang O2 in both representations.
Both deliberately mutated binaries exited 1 at the first odd boundary:

    mutated native exit=1
    distinct mismatch
    case=1
    mutated forced-int64 exit=1
    distinct mismatch
    case=1

One parallel x86 disassembly command initially contained a shell-loop typo
and returned a syntax error before measurement; it was rerun correctly and
reported zero jumps for all four probes. No result from the failed command
was counted.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 backends, LTO, sanitized execution, and Clang
AArch64 code generation. The independent parity oracle is mutation-sensitive
and found no arithmetic, aliasing, or compiler-lowering mismatch. No
production change, regression test, or finding commit is justified. This
does not provide AArch64 runtime execution, GCC AArch64 coverage, ARMv7/
RISC-V coverage, or a proof for noncanonical inputs outside the contract.
The next distinct queue is another small constant-time or overflow-sensitive
field/scalar helper; revisit cross-target execution if a runner or sysroot
appears.

## Cycle 2026-07-27: `secp256k1_scalar_is_high` compiler lowering

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `0cade2f5`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, with no running
fuzz, sanitizer, compiler, or profiling jobs. The catalog, this journal,
`src/fuzz/README.md`, `src/fuzz/scalar.c`, `src/tests.c`, scalar history,
and prior findings were searched. The public contract is at
`src/scalar.h:78-79`; the native and forced-int64 implementations are at
`src/scalar_4x64_impl.h:244-256` and `src/scalar_8x32_impl.h:292-310`.
The existing fuzzer has an independent high-half boundary oracle, but no
prior journal cycle had checked this predicate's compiler lowering directly.
The public issue search for `scalar_is_high` returned no matching issue or
PR.

### Hypothesis and contract

The bounded hypothesis was that optimization or LTO could introduce a
secret-dependent branch in the high-half comparison, or that the native
4x64 and forced-int64 8x32 lexicographic comparisons could disagree at the
exact threshold. The contract is a predicate that returns true when a
canonical scalar is greater than `floor(n/2)`. The independent oracle derives
`floor(n/2)` by decrementing the byte-level order and shifting it right once;
it also checks that the predicate does not mutate its input.

### Evidence

The standalone byte-level harness
`/tmp/secp256k1-translation-78/scalar-high-harness.c` has SHA-256
`d8ffd68de4abb82e52030c5f669a9fe95a43e6bad94de6c253f7a437ba3d9e21`.
It tested the derived threshold, threshold plus one, zero, `n-1`, every
`2^k`, every `n-2^k` for `0 <= k < 256`, and 256 deterministic values below
`2^255`, for 772 canonical values total. All native and
`-DUSE_FORCE_WIDEMUL_INT64` Clang and GCC builds at `O0`, `O2`, `O3`, and
`Os` printed exactly:

    ok threshold=7fff values=772 digest=f67485d83805dd6f

The `-O2 -flto` Clang/GCC native and forced-int64 executables printed the
same result. `objdump -d --no-show-raw-insn --disassemble=probe` found zero
x86 jump mnemonics in all four non-LTO O2 probe objects and all four LTO
executables. Clang `-O1 -fsanitize=address,undefined -DVERIFY -DVALGRIND`
native and forced-int64 executions also printed the same result with no
diagnostics.

The AArch64 compile matrix used:

    clang --target=aarch64-linux-gnu -std=c99 -g -O{0,2,3,s} \
      -I/tmp/secp256k1-oracles-next/src [optional -DUSE_FORCE_WIDEMUL_INT64] \
      -c /tmp/secp256k1-translation-78/scalar-high-harness.c -o <object>

All eight objects compiled successfully. `aarch64-linux-gnu-objdump -d
--no-show-raw-insn --disassemble=probe` found zero `b.cond`, `cbz`, `cbnz`,
`tbz`, or `tbnz` instructions for every representation and optimization
level. AArch64 `-O1 -DVERIFY -DVALGRIND` compile-only builds also succeeded
with zero warning bytes. The object hashes were:

    native:      O0 cb878cef96f52308d43b7abe2039df9b0792a75f5c015f935d03f15b99beccdf
                 O2 27e4febcf895bf665e1556954db806fdcbeaad3256dc39b1330e3104744c7e5b
                 O3 a9d6d015238e001ac60c27b643fdd1c356d8398f32b0565355a1216029c72c48
                 Os 8e251eb866178195216bccd1536f18b4423203b4f2e6cc598459e05a23534241
                 VERIFY 819dc3497e289a528a6e05c57b22dc9e52170a687614375fb0a615862cabd5cd
    forced-int64: O0 2a410a9858b34cb610d64282ce7f9c7bf0e48a4bfc32f63e8cf308f5db63d07b
                 O2 24bda26c203a9e0b65334c9053a8842afd1ff1dfb1232266558a90758e59414e
                 O3 2e65fb2d2ee5af23c70d22e17a322957084c5a18bcd062f8ca387ba961c4d2e7
                 Os eb21adb35ef5a874e57b21a08673d4c077098340392feb2bba4fb4d2bf08d255
                 VERIFY 073d4f5dec0bf7d693fe559f4bdb80a33645597b4b58a0b04b462cba8c580acb

The oracle sensitivity control copied the source to `/tmp`, changed both
backend `return yes` statements in `secp256k1_scalar_is_high` to
`return yes ^ 1`, and ran Clang O2 in both representations. Both deliberately
mutated binaries exited 1 at the first case:

    mutated native exit=1
    predicate mismatch expected=0
    case=0
    mutated forced-int64 exit=1
    predicate mismatch expected=0
    case=0

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 backends, LTO, sanitized execution, and Clang
AArch64 code generation. The independent threshold oracle is
mutation-sensitive and found no predicate, input-mutation, or compiler-
lowering mismatch. No production change, regression test, or finding commit
is justified. This does not provide AArch64 runtime execution, GCC AArch64
coverage, ARMv7/RISC-V coverage, or a proof for noncanonical inputs outside
the contract. The next distinct queue is another small constant-time or
overflow-sensitive field/scalar helper; revisit cross-target execution if a
runner or sysroot appears.

## Cycle 2026-07-27: `secp256k1_scalar_mul_shift_var` translation validation

### Hypothesis and contract

The tenth bounded hypothesis was that compiler transformation or backend
selection could miscompile the rounded 512-bit scalar product near limb and
rounding boundaries, or disagree about the documented `shift > 512` zero
case. The contract in `src/scalar.h:95-96` requires `shift >= 256`, computes
`round((a*b) / 2**shift)` without modular reduction, and returns zero above
512. The historical `3f4b9d46` guard prevents an out-of-bounds rounding-bit
read at shifts above 512; this cycle validates the repaired lowering rather
than reporting that already-fixed defect again. The existing in-tree caller
uses the constant shift 384 in `secp256k1_scalar_split_lambda`.

### Evidence

The standalone harness
`/tmp/secp256k1-translation-78/scalar-mul-shift-harness.c` has SHA-256
`c31d80f0c502adb4de0a0c072d3d66933759615d691862cce598b26322d14930`.
It computes the full product independently in little-endian base-256 bytes,
extracts the shifted bits, and adds the independent rounding bit. It checks
input immutability and serialized output for 645 canonical values: zero,
one, two, `n-1`, `n-2`, every `2^k`, every `n-2^k` for `0 <= k < 256`, and
128 deterministic values below `2^255`. It covers 4,355 ordered pairs,
including every value paired with `n-1` and itself, selected boundary pairs,
and 256 deterministic random pairs. Each pair uses 23 shifts:

    256, 257, 258, 287, 288, 289, 319, 320, 321,
    383, 384, 385, 447, 448, 449, 479, 480, 481,
    511, 512, 513, 514, UINT_MAX

Clang 22.1.7 and GCC 16.1.0, native and `-DUSE_FORCE_WIDEMUL_INT64`, at
`O0`, `O2`, `O3`, and `Os`, all printed exactly:

    ok values=645 selected=53 pairs=4355 boundary-shifts=23 digest=9c673765989f3c14

Clang and GCC `O2 -flto` native and forced-int64 builds printed the same
digest. Clang `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined` native
and forced-int64 executions also printed the same digest with no
diagnostics. The final x86 Clang O2 probe object hashes were native
`e55753f5fa9ecd1cf4cce1fb130904aac69978dbc392ee956c8e7e85f9a67a32` and
forced-int64
`a3f341be75c2ec859d8d7494b9f209c67f258926ae0b461c3922445089a3d043`.
Their disassemblies begin with the expected `cmp $0x201`/`jb` public-shift
guard; the complete probes contained 12 and 24 jump mnemonics respectively,
all from shift-dependent selection and carry/control paths, so this
`_var` helper was not incorrectly judged against a branch-free contract.

The AArch64 compile-only matrix used Clang with
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. The conditional branch counts in `probe` were
respectively native `0, 11, 13, 11` and forced-int64 `0, 23, 29, 23`; the
optimized objects show the same `cmp w3, #0x201` guard and arithmetic
lowering. AArch64 `O1 -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function
-Werror` compile-only builds succeeded with zero diagnostics. Object hashes
were:

    native:      O0 5fbef5c6b08a3e8d3b2629d2354c01b10a356dd3b2d4b41c6720311e75ffa5f6
                 O2 f74e12d49838d9f9ad8456c0dfe714f1cb7eec6459e31b8350d628f891634f74
                 O3 182a8c48bd104f40596f84745429b6e5979b181735234095e4fe9cd5c3fbf284
                 Os 7ea8619beb160c6f655623041186a21295a16b4a20a650b21be6e506a924db32
                 VERIFY 6ae705099b1a9e921c2cc95be741752449adbca1e2cf6ce73e683707759fced9
    forced-int64: O0 295b24377ff854e2f585f5254a95c81feb4566cfa854e50b55b68de28165eba4
                 O2 282a0f17dc19ddd70321b55010b303f4cd024d5ae7f881fa485a7a327198a112
                 O3 cc265977a90d14b0eec9611959642c946d0db1302aff1a1ae64ae40f763fd4c3
                 Os db849e976869f0614edf46f2b087412622a29c17db67aae4024d939acf876d2a
                 VERIFY bd795aad07f1c0537fb01907375cc785c3597c3f8c2f27076574bc4b9c7b2fb6

The first strict AArch64 `-Werror` attempt also diagnosed unused static
functions from the standalone header translation unit; after removing one
unused scratch helper, the final warning policy suppressed only that known
header-level category and treated all remaining warnings as errors. No
AArch64 runtime was available because this host has neither an ARM sysroot
nor an emulator.

The mutation control copied `src/` to scratch, changed both backend guards
from `if (shift > 512)` to `if (shift >= 512)`, and ran the final Clang O2
harness in both representations. Both exited 1 at pair 6, `shift=512`, with
the expected rounded result differing from the mutated zero result. This
proves the independent oracle exercises the guard boundary rather than only
the above-512 zero path.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 scalar backends, LTO, ASan/UBSan/VERIFY/VALGRIND
execution, and Clang AArch64 code generation. The independent byte-level
oracle found no arithmetic, boundary, input-mutation, or compiler-lowering
mismatch. No production change, regression test, or finding commit is
justified. This does not provide AArch64 runtime execution, GCC AArch64,
ARMv7/RISC-V, Alive2 validation, or a proof for noncanonical inputs outside
the contract. The next distinct queue is another compiler/architecture
constant-time or overflow-sensitive helper; revisit cross-target execution
if a runner or sysroot appears.

## Cycle 2026-07-27: `secp256k1_scalar_split_128` translation validation

### Hypothesis and contract

The eleventh bounded hypothesis was that optimization or backend selection
could swap, truncate, or otherwise miscompile the exact low/high 128-bit
split used by endomorphism and WNAF callers. The contract in
`src/scalar.h:89` is `r1 + r2*2^128 = k`; for canonical `k`, `r1` contains
the low 128 bits and `r2` the high 128 bits, each with zero upper limbs.
The current callers in `src/ecmult_impl.h`, `src/tests.c`, and `src/fuzz/scalar.c`
use distinct output objects. The standalone check therefore tests the
documented value relation and input immutability, without asserting
unsupported output aliasing.

### Evidence

The independent harness
`/tmp/secp256k1-translation-78/scalar-split-128-harness.c` has SHA-256
`3a618d39fdd991cb69d25024b963cfc75a27196e25403d64776283db4dcc2d09`.
It derives expected serialized halves directly from the input bytes and
reconstructs the input by concatenating `r2`'s low 128 bits with `r1`'s low
128 bits. It tested zero, one, two, `n-1`, `n-2`, every `2^k`, every
`n-2^k` for `0 <= k < 256`, and 128 deterministic values below `2^255`,
645 canonical values total. Native and forced-int64 Clang and GCC builds at
`O0`, `O2`, `O3`, and `Os` all printed exactly:

    ok values=645 digest=8baa6752cf908402

Clang and GCC `O2 -flto` native and forced-int64 builds printed the same
digest. Clang `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined` native
and forced-int64 executions also printed the same digest with no
diagnostics.

The final Clang O2 x86 probe objects had hashes native
`ab9f7f4f028a069dd7537ec92af35979e1b9b1228ef215b5ba45a7ee8a12ab54` and
forced-int64
`cf173fb88b39f5fac3e1c2022d2af5877d216650f7845f904d91884b871eb8de`.
`objdump --disassemble=probe` found zero conditional or loop jumps in both.
The native output is direct 64-bit loads/stores plus zeroing of the upper
limbs; the forced output is the corresponding direct 32-bit sequence.

The AArch64 compile-only matrix used Clang with
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. Every probe had zero `b.cond`, `cbz`, `cbnz`,
`tbz`, or `tbnz` instructions. The O2 disassemblies are direct loads/stores
and `stp xzr, xzr` upper-limb clearing. AArch64
`O1 -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function -Werror`
compile-only builds succeeded with zero diagnostics. Object hashes were:

    native:      O0 99d9d6268f3fd1cb1a4a46c3b8ad645be2f2ccecc4ba8b9e9d5352973cba30d7
                 O2 16a877946c7d9ea1298226ad784b765853f71d8a6d3dc3141b375a63ae5fc596
                 O3 3a92cbd0de4fc7af8925e611ffe02bede2628ea96b43dd0545c429b611a6be73
                 Os fcb4c2dddf20de9a67439d5ceff5a7feaa64b42105a119f92c1a84551b52c400
                 VERIFY ebe2c1925f63d458f9e1cf3bf18b2a829ff48d6fd052bef4d6c5e8b54d9bdccb
    forced-int64: O0 83498ffcd5f3d9edf28a8b76c8e5a7727a585fd8ef76bf17c696e2103ba28378
                 O2 0cbebaa4bbdf9a72e049f0e95ecca8afe6eec890df2bf2991cb3186cfe0ab171
                 O3 81b273ceef0aa01368cbcebfc11b8e6c67e4851eb11e43ab033890e13017fdb8
                 Os ddb7ec8e89ef5171a4e15c5d6328ff6225818071ebe81c7e0260f187d1083614
                 VERIFY dac62cb272cb47c011fa69dde0496a2f07f9b465b53d2ef783c592eda80a5224

The mutation control copied `src/` to scratch and changed the first high-half
source limb from `k->d[2]` to `k->d[1]` in the 4x64 backend, and from
`k->d[4]` to `k->d[3]` in the 8x32 backend. The final Clang O2 harness exited
1 in both cases at value 3 (`n-1`):

    native: split mismatch case=3 ... mutated native exit=1
    forced: split mismatch case=3 ... mutated forced exit=1

This confirms the byte-level oracle detects a wrong backend limb mapping.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 scalar backends, LTO, ASan/UBSan/VERIFY/VALGRIND
execution, and Clang AArch64 code generation. The independent oracle found
no split, reconstruction, input-mutation, or compiler-lowering mismatch. No
production change, regression test, or finding commit is justified. This
does not exercise the intentionally simplified exhaustive-test scalar
backend, provide AArch64 runtime execution, GCC AArch64, ARMv7/RISC-V, or
prove unsupported output aliasing. The next distinct queue is another
compiler/architecture constant-time or overflow-sensitive helper; revisit
cross-target execution if a runner or sysroot appears.

## Cycle 2026-07-27: scalar bit-extraction translation validation

### Hypothesis and contract

The twelfth bounded hypothesis was that compiler transformation or native
versus forced-int64 representation could mishandle bit order, 32-bit limb
crossings, or the exact boundary checks in
`secp256k1_scalar_get_bits_var` and `secp256k1_scalar_get_bits_limb32`.
The contracts in `src/scalar.h:26-29` require `1 <= count <= 32` and
`offset + count <= 256`; the limb32 variant additionally requires every
requested bit to be in one 32-bit limb, while the variable variant must
handle crossings and is explicitly not constant-time in offset/count. The
historical `0cad3df5` check fix is relevant boundary precedent. The public
search found no matching get-bits issue or PR; its only lexical hit was the
unrelated historical signed-digit PR #693.

### Evidence

The standalone harness
`/tmp/secp256k1-translation-78/scalar-get-bits-harness.c` has SHA-256
`51405a9a753cf4d4b7f180e037ac004692d4d7c5c203cfef478d4211c6228bdb`.
Its reference reads each requested bit from the big-endian byte array and
places it in the corresponding little-endian result position. It checks
every valid offset and count pair for 645 canonical values: zero, one, two,
`n-1`, `n-2`, every `2^k`, every `n-2^k` for `0 <= k < 256`, and 128
deterministic values below `2^255`. The run covers 4,963,920
`get_bits_var` cases and 2,724,480 legal `get_bits_limb32` cases, and checks
input immutability. Native and forced-int64 Clang and GCC builds at `O0`,
`O2`, `O3`, and `Os` all printed exactly:

    ok values=645 var-cases=4963920 limb-cases=2724480 digest=7284ac614eb82cf4

Clang and GCC `O2 -flto` native and forced-int64 builds printed the same
digest. Clang `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined` native
and forced-int64 executions also printed the same digest with no
diagnostics.

The final Clang O2 x86 probe object hashes were native
`3924585c688c7956836bd6ef8c889f0c680235de2c4ab4ac2e489ac53b4e6e48` and
forced-int64
`99a32dd5f15af400f18a33b3e0251b58fc2c6b9ed372bec31fdf2196917470a8`.
Both `probe_var` bodies have the expected two-branch same-limb/cross-limb
selection, while both `probe_limb32` bodies have zero conditional or loop
jumps. The disassembly uses `shrd` for the x86 cross-limb extraction.

The AArch64 compile-only matrix used Clang with
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. `probe_var` had zero conditional branches at O0
and one at each optimized level in both representations; `probe_limb32` had
zero at every level. AArch64 O2 uses the expected conditional cross-limb
load and direct same-limb load. AArch64
`O1 -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function -Werror`
compile-only builds succeeded with zero diagnostics. Object hashes were:

    native:      O0 c3272e3b0d210fbc78eacf10497c186921abf1de45590e9947b240463b16dadd
                 O2 ee616f1a925f50ccdec5e0d91467ae58f0f2a7999f6a2178cdbaa118eb1beb22
                 O3 8f8a6b967038bb20d27af40feff37eca02b57d24e8f679322020672e0ef9de4d
                 Os 76c5b29502e8e7534f54b0b2d7147e53418aa21a0877e5c2e123e09aecab6edd
                 VERIFY 7179c27a6dca8002382e96b0b50b8725a9469a9cba0e06df5e4abbf3d76154fa
    forced-int64: O0 dd0fc28caafc28c694389dce68bd73d415a4aaad16deb0f4dbf307329a15917d
                 O2 0d7d899bcaf0b9079ccba1fd6d3f0ad82169edf736b0baa9397d0fe9434b3f23
                 O3 670845e9bcaef9a9bed7fd4782dc2596b0b53cd9996ba3e49aa906d6b2663760
                 Os 528639625cc07ed758203c670a861afd1144871d93df1f0a282d08052a38fbcf
                 VERIFY 1a449cbab77fd89b392d578731c0f2f6dc8d8b61a228d14e6293c61bda7235b3

The mutation control copied `src/` to scratch and changed the mask constant
`0xFFFFFFFF` to `0x7FFFFFFF` in each backend's bit-extraction helpers. Both
final Clang O2 harnesses exited 1 on the first nonzero high-bit value:

    var mismatch case=1 offset=0 count=1 expected=00000001 actual=00000000
    mutated native exit=1
    var mismatch case=1 offset=0 count=1 expected=00000001 actual=00000000
    mutated forced exit=1

The oracle therefore detects a real output-mask regression as well as the
cross-limb cases.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 scalar backends, LTO, ASan/UBSan/VERIFY/VALGRIND
execution, and Clang AArch64 code generation. The independent bit-order
oracle found no value, boundary, cross-limb, input-mutation, or
compiler-lowering mismatch. No production change, regression test, or
finding commit is justified. This does not exercise the intentionally
simplified exhaustive-test scalar backend, provide AArch64 runtime
execution, GCC AArch64, ARMv7/RISC-V, or invalid arguments that should trip
VERIFY checks. The next distinct queue is another compiler/architecture
constant-time or overflow-sensitive helper; revisit cross-target execution
if a runner or sysroot appears.

## Cycle 2026-07-27: `secp256k1_scalar_split_lambda` translation validation

### Hypothesis and contract

The thirteenth bounded hypothesis was that compiler transformation or
native versus forced-int64 arithmetic could miscompile the GLV scalar split,
including its rounded products, modular reconstruction, or signed 128-bit
bounds. The implementation contract in `src/scalar_impl.h:136-140` requires
`r1 + lambda*r2 == k (mod n)` and each output or its modular negation to fit
below `2^128`. It explicitly requires three distinct objects, which the
harness honors. The current callers are endomorphism/WNAF paths in
`src/ecmult_impl.h`; history and the public search found no current matching
defect. The search surfaced only historical endomorphism cleanup PR #830.

### Evidence

The standalone harness
`/tmp/secp256k1-translation-78/scalar-split-lambda-harness.c` has SHA-256
`a0f795909f6c13b48d407bd485ecfc3284accd1915b9f4587e5aa3b7895a6200`.
Its reference duplicates only the public constants as bytes, computes
products in base-256, reduces 512-bit products by independent bitwise long
division, performs modular add/subtract, and computes the fixed rounded
`2^-384` products before reconstructing both outputs. It then independently
checks the modular relation and signed 128-bit bounds. It tested 645
canonical values: zero, one, two, `n-1`, `n-2`, every `2^k`, every `n-2^k`
for `0 <= k < 256`, and 128 deterministic values below `2^255`.

Native and forced-int64 Clang and GCC builds at `O0`, `O2`, `O3`, and `Os`
all printed exactly:

    ok values=645 digest=14d092be313ccf6a

Clang and GCC `O2 -flto` native and forced-int64 builds printed the same
digest. Clang `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined` native
and forced-int64 executions also printed the same digest with no
diagnostics. The final Clang O2 x86 probe object hashes were native
`b3e5bedc63653c9f306cebe0ede36633644637ffa86c008d071bafa3c03c8c04` and
forced-int64
`dac07da20e4501ed60820ee9eae7ffb86e38820190522bc6a305412a466bf126`;
both probes had zero conditional or loop jumps.

The AArch64 compile-only matrix used Clang with
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. All eight probes had zero conditional branch
mnemonics. AArch64 `O1 -DVERIFY -DVALGRIND -Wall -Wextra
-Wno-unused-function` compile-only builds succeeded with zero diagnostics.
Object hashes were:

    native:      O0 17f91b88fdc1165246c7860e42a86f57a6814c91ec036ad3bc9a433891ffbaee
                 O2 1711cf913f4190be86f786c3b9cb231dc2e358acd47220937180a1a69ab15c08
                 O3 88457d1c50e71d1c675a5b9f37910a1b09fcc2826042f260c32f1c0f81d5afd3
                 Os ead0edf47add7eafec308e6779efaa44370e7e35282a5f158e1c6377c9551d16
                 VERIFY 3b3709263fa80f9be3fa1e1f0e3ed5e02a96c9b5345ae506f31c7fb18e06af28
    forced-int64: O0 05fe017f8f1475c71b5271e10facb35e081ff0441887b8c7a2eff51c5d02b08a
                 O2 3dd1972f57353654368ecaf429d00aef8d2c1386c1cc59ed45f395fa51a3add2
                 O3 508178ce44a9a0385d8467a0d3a0f54d05844815f897b4350da8c394d5718f74
                 Os d88ccb850907fce35b4f7e634730fca0996d4c96923b988264c189e5cf94c3e2
                 VERIFY 6c4673f602e0b9772c0f36ad271fe867cd6379e67ee5e362b2788777bb548930

The mutation control copied `src/` to scratch and first changed the
least-significant `g1` word `0x45DBB031` to `0x45DBB030`. Both backend
harnesses still passed: the perturbation is below the precision of the
fixed `2^-384` rounding for these 256-bit inputs, so it is not a useful
oracle-sensitivity mutant and is retained only as a documented control.
The stronger high-byte mutation changed `0x3086D221` to `0x3186D221`; both
final Clang O2 harnesses then exited 1 at `n-1`:

    split mismatch case=3 expected1=ff40 actual1=ff40 expected2=0000 actual2=0000
    mutated native exit=1
    split mismatch case=3 expected1=ff40 actual1=ff40 expected2=0000 actual2=0000
    mutated forced exit=1

The output prefixes are intentionally abbreviated by the scratch diagnostic;
the full-output comparison is what failed.

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 scalar backends, LTO, ASan/UBSan/VERIFY/VALGRIND
execution, and Clang AArch64 code generation. The independent base-256
oracle found no rounded-product, modular-reconstruction, bound,
input-mutation, or compiler-lowering mismatch. No production change,
regression test, or finding commit is justified. This does not exercise the
intentionally simplified exhaustive-test scalar backend, provide AArch64
runtime execution, GCC AArch64, ARMv7/RISC-V, or prove output aliasing that
the contract rejects. The next distinct queue is another
compiler/architecture constant-time or overflow-sensitive helper; revisit
cross-target execution if a runner or sysroot appears.

### Cycle 2026-07-27: scalar inverse translation

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `d3fe62a5`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, `aarch64-linux-gnu-objdump`, and the existing ASan/VERIFY test builds
were available. No AArch64 runtime, GCC AArch64 toolchain, Alive2, CBMC, or
KLEE executable was available. The catalog, this journal, the existing
fuzzer oracle, scalar history, and current public search results were read.
The relevant public evidence was historical: commit `dc1e87f3` added an
independent base-2^16 scalar-inverse fuzzer oracle; open issue
[#1216](https://github.com/bitcoin-core/secp256k1/issues/1216) discusses
verifying modinv metadata invariants; and closed PR
[#1031](https://github.com/bitcoin-core/secp256k1/pull/1031) emphasizes exact
divsteps invariants, signed-boundary cases, and measured constant-time
performance. None is a current report of an inverse result or compiler
mismatch. The review evidence favors explicit invariant lists, precise
boundary reasoning, and measured claims.

### Hypothesis and contract

The fourteenth bounded hypothesis was that compiler transformation or the
native 4x64 versus forced-int64 8x32 representation could miscompute
`secp256k1_scalar_inverse` or `secp256k1_scalar_inverse_var`, mishandle zero or
in-place operation, or introduce an unexpected secret-dependent branch in the
constant-time inverse. The contract at `src/scalar.h:58-63` requires the
inverse modulo the scalar group order and explicitly distinguishes the
constant-time and variable-time variants. The production implementations are
the 62-bit and 30-bit divsteps paths in
`src/scalar_4x64_impl.h:974-1004` and
`src/scalar_8x32_impl.h:789-819`; the exhaustive backend is intentionally a
separate small-order implementation and was not mixed into this full-order
claim. Existing callers include ECDSA signing and the scalar tests.

### Evidence

The standalone Boost generator
`/tmp/secp256k1-translation-78/scalar-inverse-vectors.cpp` has SHA-256
`c89c57dceb105dbc7ea94d6b41834d5d5351267cee71427071faf06f1b514fd9`.
It computes `a^(n-2) mod n` with a high-level `boost::multiprecision::cpp_int`
binary exponentiation model and emits 645 input/expected-output pairs. The
vector artifact has SHA-256
`f81eaacfd67fe3ebaed10892d59914d2a73d04bb4fff9b122ec538a559fbde76` and
contains zero, one, two, `n-1`, `n-2`, every `2^k`, every `n-2^k` for
`0 <= k < 256`, and 128 deterministic values below `2^255`. The C production
runner has SHA-256
`66316e4ad058c377a0eed2c339c105c8f5c0b1ad2054fbaf50a1a389697ca789`.
For every vector it checked canonical round-trip, both inverse variants,
variant equality, unchanged input, and distinct-output in-place operation.

Native and forced-int64 Clang and GCC builds at `O0`, `O2`, `O3`, and `Os`,
plus native and forced-int64 `O2 -flto` builds, all printed exactly:

    ok values=645 digest=609caf56698b92b5

Clang and GCC `O1 -DVERIFY` ASan/UBSan runs in both representations also
printed the same digest without diagnostics. The existing ASan and no-VERIFY
test binaries ran `inverse_tests`, `scalar_tests`, `modinv_tests`, and
`endomorphism_tests` for four iterations, two jobs, and fixed seed
`0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef`; both
completed without failures.

The final Clang O2 x86 probe object hashes were native
`9f83868b8b637666d7746d3788249e2f98777478232a6c431f58a569cb1a3b9d` and
forced-int64
`628366ef345280d1dd7e85177f3f73c42d9d2d2943f65254c1ac2ab7c9c97b37`.
The regular inverse probe contained only fixed-count loop `jne` branches;
the `inverse_var` probe contained the expected data-dependent divstep
branches. The distinction was preserved in both representations.

The AArch64 compile-only matrix used Clang with
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. The non-VERIFY object hashes were:

    native:      O0 40240d736e2bcf4d5346091fd766446d7162fea0770d3000962873543e519fd1
                 O2 bc19d726f239ca35d52edd53c3b3a4df31de734b9ec1a53ceeb3cb309720d040
                 O3 292be5db70b22e60105a1d344c6c585c9900f357106de54403351dfad4648277
                 Os b57dbfdafdeb0a2325db9a95aff921c3e8d5f144cdc409ae84d7f44d22b4fd4225
    forced-int64: O0 87f92847f8cbdde7701030d99271d024e2bde8d20d42ae484daff8c0911bf5c8
                 O2 4ff94f2dfd52d449a467b09c5c2b885c81c93302d34c8633e6f835ac85f3ebf8
                 O3 b55a32d5a01f6bda31de8ad8a9a76ea02355a825805c66885e6c42e9c44b2825
                 Os 414e58f5737bfb53cb5372d42d7ceebb47fee39f14c04c10f22b1069c189e583

At optimized levels, the regular inverse probe had 2/2/2 conditional branch
mnemonics for native `O2/O3/Os` and 2/2/3 for forced-int64 `O2/O3/Os`;
these were fixed loop branches. The variable-time probe had 12/12/9 in both
representations at `O2/O3/Os`, including data-dependent divstep branches.
`O0` leaves the implementation out of the
probe and therefore has no probe-local branch count. AArch64
`O0/O2/O3/Os -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function`
compile-only builds also succeeded with no diagnostics; their native hashes
were `da057a39f539d1acca3f3ccd715241a4620511bae34e5a7d3ebbbaf586fab096`,
`2c0ce78981ca61944b28010b6c03938840a8917c8e243b261978e022e366fea2`,
`4d8329c10d7b0e5d441455377dc48bd05b74a359d7651f4955be570c67ac1ab1`, and
`c21993410f98250c96c92f992c70aca3300fe76d85fafb8154bc9c4df1002466`; the
forced-int64 hashes were `e341a6e2b9f01f00e4c642d0c6a5ec7ee81b56898f0792968a8da3f8fcc88798`,
`deb31229469a5d17dbf6c63a6ba5d0a761a31578dddd5863b0cf52923ef6b785`,
`3a5fe42034f4973f0cc04f6272a77df1ebdafd85db4856ea0b920a73d05ae5fd`, and
`23c4fea68cab368dc113766b660a8edccd95e9d7ef0bd236dd0c6c477b2d50d1`.

The mutation control copied `src/` and `include/` to scratch and replaced
`secp256k1_scalar_from_signed62(r, &s)` and
`secp256k1_scalar_from_signed30(r, &s)` in both inverse functions with
`secp256k1_scalar_set_int(r, 0)`. Both clean Clang O2 runners rejected the
shared wrong result at case 1 and exited 1:

    inverse mismatch case=1
    mutated native exit=1
    inverse mismatch case=1
    mutated forced exit=1

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 scalar backends, LTO, ASan/UBSan/VERIFY execution,
focused existing tests, and Clang AArch64 code generation. The independent
high-level modular-inverse vectors found no result, aliasing, zero-input, or
compiler-lowering mismatch, and the mutation proves the oracle distinguishes
a shared inverse-output defect. No production change, regression test, or
finding commit is justified. This does not provide AArch64 runtime execution,
GCC AArch64, ARMv7/RISC-V, or a formal proof of constant-time behavior; the
branch inspection is compiler evidence only. The next queue is another
compiler/architecture constant-time or overflow-sensitive helper. Do not
repeat the fourteen dismissed hypotheses unless compiler, source, or
architecture evidence changes.

### Cycle 2026-07-27: field normalization translation

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `c8a24e97`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, `aarch64-linux-gnu-objdump`, and the existing ASan/VERIFY test builds
were available. No AArch64 runtime, GCC AArch64, ARMv7/RISC-V sysroot,
Alive2, CBMC, or KLEE executable was available. The catalog, this journal,
field contracts, recent field history, existing fuzz/test oracles, and public
review evidence were read before selecting the next distinct kernel.

The relevant history included `97727abd` (`field_10x26: fix magnitude-32
normalize overflow`), which replaced a too-narrow carry accumulator after an
in-contract magnitude-32 failure, and `512370b1` (`fuzz: catch 10x26
zero-predicate carry wrap`), which added an independent zero-predicate
regression oracle. Open PR
[#1028](https://github.com/bitcoin-core/secp256k1/pull/1028) discusses faster
constant-time normalization; its review explicitly challenges carry overflow
and the maximum caller magnitude, while the response distinguishes the
documented caller bound from the practical limit. These are useful seeds and
review constraints, not evidence of a current compiler mismatch.

### Hypothesis and contract

The fifteenth bounded hypothesis was that compiler transformation or the
native 5x52 versus forced-int64 10x26 field representation could miscompute
`secp256k1_fe_normalize`, `_weak`, or `_var` at the documented magnitude
bound, especially in the carry accumulator and final reduction, or alter the
constant-time branch shape of regular normalization. `src/field.h:20-31`
defines field magnitude and normalized-state contracts. The 5x52 implementation
uses 52-bit limbs and a 64-bit carry path at
`src/field_5x52_impl.h:43-78`; the 10x26 implementation uses 26-bit limbs and
an explicitly 64-bit first-pass accumulator at
`src/field_10x26_impl.h:53-106`. The public wrapper contract in
`src/field_impl.h` distinguishes regular, weak, and variable-time
normalization. The test must therefore cover canonical values, raw maximum
magnitude, carry combinations, weak normalization, final reduction, and
VERIFY metadata without treating implementation representation as the oracle.

### Evidence

The standalone C harness
`/tmp/secp256k1-translation-78/field-normalize-harness.c` has SHA-256
`87428fca4648a64401cbabcc9e626f140daab9b778dabbbd47d276223e40b223`.
It includes the project field implementation directly and generates 645
canonical values: zero, one, two, `p-1`, `p-2`, every `2^k`, every `p-2^k`
for `0 <= k < 256`, and 128 deterministic values below `2^255`. The
independent expected bytes are built from the field prime
`p = 2^256 - 2^32 - 977`, rather than from either field backend. Every value
is checked by regular normalization, variable-time normalization, and weak
normalization followed by variable-time normalization. The harness also
checks `secp256k1_fe_get_bounds(m)` for every `0 <= m <= 32`, all 31 sums of
complementary bounds, and 645 raised cases made by adding a canonical value to
a magnitude-31 zero representation. It verifies canonical bytes, expected
VERIFY metadata, unchanged inputs, and reports the digest.

Native and forced-int64 Clang and GCC builds at `O0`, `O2`, `O3`, and `Os`,
plus native and forced-int64 `O2 -flto` builds, all printed exactly:

    ok values=645 bound-cases=33 sum-cases=31 raised-cases=645 digest=f1bcc6afb297fe94

Clang and GCC native/forced-int64 `O1 -DVERIFY` ASan/UBSan runs printed the
same digest without diagnostics. The existing ASan and no-VERIFY binaries
also completed `field_half`, `fe_normalize_max_magnitude`, and `field_misc`
for four iterations, two jobs, with fixed seed
`abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd`.

The final Clang O2 x86 object hashes were native
`b18222ef5095a4173f86663b20171047a01768928e4f95ca617e76a7905371d1` and
forced-int64
`e13caf5aa255a221311df22b994cf4fd3dfdf998fa8dce3708e232f5ff9bdcca`.
The regular `normalize` probe had no conditional branch mnemonics in either
representation. The `normalize_var` and `normalize_weak_then_var` probes each
had one conditional branch in both representations, consistent with their
variable-time final-reduction contract rather than an unexpected branch in
regular normalization. A scratch native `-Wall -Wextra -Werror` build was
not used as evidence because included project headers expose unused static
helpers in this standalone harness; the cross-target VERIFY checks used
`-Wno-unused-function` and were diagnostic-free.

The non-VERIFY AArch64 compile-only matrix used Clang
`--target=aarch64-linux-gnu`, native and forced-int64 representations, and
`O0`, `O2`, `O3`, and `Os`. The object hashes were:

    native:      O0 357548415ba2e308e5376a9765f6d627a4d91988d4dc2733ebaa4a7027aac1ed
                 O2 409f4739eb3585efa75fda655af15cb2d54cddfc46b4d1fcb26569a34bed189e
                 O3 0f44e166142067dcd84573c6f4ef44153651e6473ff87ceb941f58514ecd431d
                 Os 1dfb907edd0ff29acdfcbf82dc53c9756ee169eb373d5f5e44b468119c52d3fa
    forced-int64: O0 75e8f22d1b3417596a7d7ec36ef7fa1575f296cf3adf2962f8607965e4f5906f
                 O2 b0916ede9452d1d79c57014da7d21aedcf8ca25135ff99b1992070ef651e3d0e
                 O3 ebaf129fb20f2c9749916760caf3624a9e5a373028c960e2b48bf0a71484396e
                 Os 620d6baeb30a37d437c0d1f823e6a1fee7dac338ae0ba4fb10e7570254e55d76

All three probe functions had zero conditional branch mnemonics at every
AArch64 optimization level; the compiler used conditional compare/select
instructions for the variable-time wrappers in this probe. AArch64
`O0/O2/O3/Os -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function`
compile-only builds also succeeded without diagnostics. Their native hashes
were `50baa1274c4cbb86f1b469ff51e40401ee48da628cb51e4b92d7546033725784`,
`427e9f1f9f3c1706cdb02395b52be41fbfd83458d43dd7643920932cb65fe301`,
`cfa17d8f94cec666b1a9d82dcfd5be1031a9e606efa2e00c34e6a6e503d24f45`, and
`76cd6cd9b34de343de826bb27131b7808c4fea68e60659de23aeab51607bf67b`; the
forced-int64 hashes were `4e79935e615fb0c0af09e53634969140de25d9b3ef8138214a8e347725ce58d7`,
`4594a28fadfcd4ecc74d33083075ea2129d3a90e0dffe3eeec0bab40e9a4706a`,
`689f3f21087e69846cf1177085e851ba5973ad8e86a3c3846659b1ab251a9fc0`, and
`2b3b46a8a879b9de3d2a692ddc8775fd3889d40c62d49fe19f3eea7d46a805c9`.

The mutation control copied `src/` and `include/` to scratch and changed the
field-prime reduction constant from `0x3D1` to `0x3D0` in the first full
`secp256k1_fe_impl_normalize` function of both field implementations. Clean
Clang O2 native and forced-int64 runners rejected the wrong result at the
first raw-bound case and exited 1:

    normalize mismatch kind=bound case=1
    mutated native exit=1
    normalize mismatch kind=bound case=1
    mutated forced exit=1

### Verdict and limits

The hypothesis is **dismissed** for the tested Clang/GCC x86_64 matrix,
native and forced-int64 field backends, LTO, ASan/UBSan/VERIFY execution,
focused field tests, and Clang AArch64 code generation. The independent
prime-based oracle found no normalization, carry, metadata, or compiler
lowering mismatch, and the mutation proves the boundary oracle detects a
shared reduction defect. No production change, regression test, or finding
commit is justified. This does not provide AArch64 runtime execution, GCC
AArch64, ARMv7/RISC-V, or a formal proof of constant-time behavior; branch
inspection remains compiler evidence only. The next queue is another
compiler/architecture constant-time or overflow-sensitive helper. Do not
repeat the fifteen dismissed hypotheses unless compiler, source, or
architecture evidence changes.

### Cycle 2026-07-27: field half translation and contract

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `88eae5c5`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, `aarch64-linux-gnu-objdump`, and the existing ASan/VERIFY test builds
were available. No AArch64 runtime, GCC AArch64, ARMv7/RISC-V sysroot,
Alive2, CBMC, or KLEE executable was available. The field-half source,
fuzzer oracle, tests, blame, and implementation history were read before
selecting this distinct arithmetic kernel.

The implementation history is unusually direct. Commit `925f78d5` added
`fe_half`; `d64bb5d4` added worst-case magnitude tests that assert
`normalized == 0`; and commit `89e324c6` abstracted the VERIFY wrapper. The
parent of `89e324c6` still documented that the output was not guaranteed to be
normalized, while the current wording incorrectly says it will be normalized.
This history, the implementation, and current tests independently identify a
stale source contract.

### Hypothesis and contract

The sixteenth bounded hypothesis was that compiler transformation or the
native 5x52 versus forced-int64 10x26 representation could miscompute
`secp256k1_fe_half` for odd residues, maximum valid input magnitude, carry
propagation, or the output magnitude contract, or introduce a branch into the
constant-time operation. The implementation proofs at
`src/field_5x52_impl.h:335-386` and `src/field_10x26_impl.h:1036-1099`
state output magnitude `floor(m/2)+1` for input magnitude `m`. The wrapper at
`src/field_impl.h:444-453` enforces input magnitude at most 31 and sets the
VERIFY normalized flag to zero. Existing tests at `src/tests.c:3296-3340`
check the same magnitude and non-normalized output behavior. The mathematical
oracle therefore computes `(x + p)` when an odd canonical residue requires it,
then divides by two, without using either field representation.

### Evidence

The standalone C harness
`/tmp/secp256k1-translation-78/field-half-harness.c` has SHA-256
`ae7ef47a3b7add09b1d53abe3b8b997f5ba4f60ff76f9fa32b883517ba632f9c`.
It covers the same 645 canonical boundary/random values used in the prior
normalization oracle, every valid `get_bounds(m)` input for `0 <= m <= 31`,
all 30 complementary sums whose total magnitude is 31, and 645 canonical
values raised through every magnitude from 1 to 31 by adding a zero residue.
For each case it checks the independent half result, the documented output
magnitude, and that doubling the normalized result returns the original
residue. The byte-level reference is the same carry-and-shift calculation as
the independent fuzzer reference in `src/fuzz/field.c:1016-1034`, reproduced
in the scratch harness and not called from production code.

A preliminary scratch pass used direct `SECP256K1_WIDEMUL_INT128` and
`SECP256K1_WIDEMUL_INT64` defines. It was discarded after `src/util.h:349-378`
showed that the repository selects backends with
`USE_FORCE_WIDEMUL_INT128` and `USE_FORCE_WIDEMUL_INT64`; those are the only
selectors used for the final evidence below.

Native and forced-int64 Clang and GCC builds using the repository selectors at
`O0`, `O2`, `O3`, and `Os`, plus native and forced-int64 `O2 -flto` builds,
all printed exactly:

    ok values=645 bound-cases=32 sum-cases=30 raised-cases=645 digest=19d79583eb93cd67

The four native/forced Clang/GCC `O1 -DVERIFY` ASan/UBSan runs also printed
the same digest without diagnostics. The existing ASan and no-VERIFY binaries
completed `field_half` and `field_misc` for four iterations, two jobs, with
fixed seed `abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd`.

The final Clang O2 x86 probe object hashes were native
`049d6f47e3d235cc31575c83bd8649b6cd1bf8793a7b733d8e95c867fb6e3673` and
forced-int64
`cfa89c48134b9562049d5f64df784edbe66f9850c8f239abefd6044c1017db3c`.
The `probe_half` body had zero conditional or loop jump mnemonics in both
representations. This is code-generation evidence for the non-VERIFY
constant-time path; VERIFY assertion branches are not production evidence.

The final non-VERIFY AArch64 compile-only matrix used Clang
`--target=aarch64-linux-gnu`, the repository's native and forced-int64
selectors, and `O0`, `O2`, `O3`, and `Os`. The object hashes were:

    native:      O0 b9768cc5a397877d90ccf555410b156197dc493c5d61dcd6f259034ed9bb4fc3
                 O2 d448b082c5efabbd21bbea7a2207bd3865be8bd5dabe822d8d26b5cc409d7bad
                 O3 5297598095a735ba09ed4926727b199f06e2c3eef2b073dc193fafb276fb48b1
                 Os 36c6e1f7123b119c6aa5ff006355ebdb7ab8dbef2436a87801ae717679b02442
    forced-int64: O0 f2b009d7c4ae67fe8f3ca4272127bd328bb4eae51e2c4cf05b22ea9132c3e735
                 O2 22b8ca747baec5d0eb2e70cda80ab371d5897824aa9713517fe9ac5a5a40a41d
                 O3 bd24e894cb0198c85332c1c5cfbfaf86cb9f338da2b7fd33ddc1f3c28b796edc
                 Os 13dee79594709b0466520c85e8c135c714a448a58ab759be2aad299921a3f478

All eight non-VERIFY `probe_half` bodies had zero conditional branch mnemonics.
The `O0/O2/O3/Os -DVERIFY -DVALGRIND -Wall -Wextra -Wno-unused-function`
cross-builds also succeeded without diagnostics. Their native hashes were
`6a397380eaa954d884a515e957e5a5c140177927749017ff3eea6bcfaedeece7`,
`07b6dab987d6cda5b078fc15efb35a8cd853272acfcd68a73a2a84c099d6a8cf`,
`b940a762975f271b65122fbce346a89c137ae87a056aeb9510cc4c597f8c3163`, and
`9df4b39740a63bbc2a98e4a93918a16f5e497d5bb51d1e99170a027de980ffa5`; the
forced-int64 hashes were
`778ce24f438c7ea339c1d7e361b67b8cef849f79dc1938aa539626ba89b1b374`,
`11e365ee9da1a75558984362aa8f45bc80802c209658c47c768e5accb2ce8d8f`,
`9c99b3f137312f78e0da7e95c22b4913f9dd418cf71d88f14118dcaeee65e3f4`, and
`3f3c23a2636340f19109dae151abb957afb05674ebdd20a756b4f73aa95eacaa`.

The mutation control copied `src/` and `include/` to scratch and changed the
low field-prime addend used when the input is odd from `0x...C2F` to
`0x...C2E` in both backend implementations. Clean Clang O2 runners using
the correct native and forced selectors rejected the wrong result at the
first nonzero value and exited 1:

    half mismatch kind=value case=1
    mutated native exit=1
    half mismatch kind=value case=1
    mutated forced exit=1

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native and forced-int64 field backends, LTO,
ASan/UBSan/VERIFY execution, focused field tests, and Clang AArch64 code
generation. The independent modular-halving oracle found no arithmetic,
magnitude, carry, or lowering mismatch, and the mutation proves that the
oracle distinguishes a shared odd-input defect. This does not provide
AArch64 runtime execution, GCC AArch64, ARMv7/RISC-V, or a formal proof of
constant-time behavior; branch inspection remains compiler evidence only.

Separately, the source-comment contract at `src/field.h:325-331` is a
confirmed documentation finding: it promised normalized output, contrary to
the `field_impl.h` wrapper, implementation bounds proof, tests, and the
pre-`89e324c6` contract. The smallest fix changed only the comment to state
that the output magnitude is `floor(m/2)+1` and normalization is not
guaranteed. Existing `field_half` and `field_misc` tests plus the corrected
harness cover the behavior; no production arithmetic changed. The source
comment fix and this journal/state update are committed together. The next
queue is another compiler/architecture constant-time or overflow-sensitive
helper. Do not repeat the sixteen dismissed compiler hypotheses unless
compiler, source, or architecture evidence changes.

### Cycle 2026-07-27: field negation translation

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `3bacc91c`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, `aarch64-linux-gnu-objdump`, and the existing ASan/VERIFY test builds
were available. No AArch64 runtime, GCC AArch64, ARMv7/RISC-V sysroot,
Alive2, CBMC, or KLEE executable was available. The field-negation contract,
both backend implementations, the independent field fuzzer oracle, blame,
and the historical static-assert change were read before selecting this
distinct arithmetic kernel.

### Hypothesis and contract

The seventeenth bounded hypothesis was that compiler transformation or the
native 5x52 versus forced-int64 10x26 representation could miscompute
`secp256k1_fe_negate` for zero, near-prime, maximum-magnitude, or aliasing
inputs, violate the output magnitude contract, or introduce a
data-dependent branch. The public contract at `src/field.h:204-217` accepts
an input of magnitude `m` with `0 <= m <= 31`, permits an uninitialized output,
and promises an output magnitude of `m+1` without normalization. The VERIFY
wrapper at `src/field_impl.h:297-305` checks the input bound and records
`m+1`/`normalized=0`. The 5x52 implementation at
`src/field_5x52_impl.h:278-291` and 10x26 implementation at
`src/field_10x26_impl.h:343-362` subtract each limb from `2*(m+1)` times
the corresponding representation of the field prime.

### Evidence

The standalone C harness
`/tmp/secp256k1-translation-78/field-negate-harness.c` has SHA-256
`bc4db3db512233ba6b545033dc8142f3c79054fddd7bdc2f1bfe7a7bf3859eac`. It
uses a byte-level `p-x` reference independent of either production field
representation. It covers 645 canonical values including zero, one, the
two values adjacent to the prime, every power of two and its prime
complement, and 128 deterministic values below `2^255`. Each canonical
value is raised to every input magnitude from 1 through 31 by adding a zero
residue, for 19,995 raised cases. It checks the residue, VERIFY metadata,
normalization state, cancellation after adding the original value, and
in-place aliasing for magnitudes 0, 1, 8, and 31. It also checks all 32
independent `get_bounds(m)` cases. Clean native and forced-backend O2 runs
printed exactly:

    ok values=645 bound-cases=32 raised-cases=19995 digest=0315022cfbcaee3d

Clang and GCC native/forced builds using the repository's actual
`USE_FORCE_WIDEMUL_INT128` and `USE_FORCE_WIDEMUL_INT64` selectors passed at
`O0`, `O2`, `O3`, and `Os`; native and forced O2 LTO builds also matched.
Clang/GCC ASan+UBSan VERIFY runs (with Clang also using VALGRIND checks)
matched without diagnostics. The existing ASan and recovery/no-VERIFY
binaries completed the focused `field_half`/`field_misc` tests with fixed
seeds, and both `fuzz_field` binaries accepted the
`src/fuzz/corpora/field/negation-byte-reference` input once without failure.

The final Clang O2 x86 probe object hashes were native
`e46131555e4fae480dbd9c6c8ca45eb8b9115e186499399d0e6bf3aa23e0adda` and
forced
`7ba0e6ef1d4a1ca3970bea2fd630bd406bab4da97633ef4e5d457fe2f31dbd68`.
Representative non-VERIFY `probe_negate_1`, `_8`, and `_31` bodies had no
conditional or loop jump mnemonics at O2 in either backend.

The Clang AArch64 compile-only matrix used the same native and forced
selectors at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16
builds completed. Non-VERIFY object hashes were:

    native: O0 2781984a89f4b88d4d3097b97300a062373c2257ecc16c70ff59a37b6f96f492
            O2 fc72cda8468530406f02b7407587691fb087cb0cebc233260a47512338a7926a
            O3 438adb4b4b900d205327b9737416eaf7a34fac5e13cf1bfe177542052e5319d2
            Os d665dc5d646ae75496bf4a1b5646395a33431152c0601085f3fcea0711b6934c
    forced: O0 2a8e4641340f6aef1421810865364f674655db8fceed9a26fc9aa1a8fbffd1a5
            O2 ab955fe23aaa5f200112152516757c394a5bcd01fd712862a4f86813aeb29036
            O3 77df9bdc6398ca538f51341c669537af768a9346ad05595c677c7633017fc4ff
            Os d235976003997cb59153fe9ed23eff22c1c1c2a297aa6abd52d94268a951cd4d

All eight non-VERIFY AArch64 probe bodies had no conditional branch
mnemonics. The AArch64 O0 unconditional branches are wrapper-call
scaffolding, not data-dependent production branches; VERIFY branches are
assertion/debug scaffolding and are not constant-time evidence.

The mutation controls copied `src/` and `include/` to scratch and changed
the first field-prime output constant from `0xFFFFEFFFFFC2FULL` to
`0xFFFFEFFFFFC2EULL` in the native 5x52 implementation and from
`0x3FFFC2FUL` to `0x3FFFC2EUL` in the forced 10x26 implementation. Both
clean Clang O2 runners rejected the mutation immediately:

    negate mismatch kind=raised case=0 m=1
    mutated native exit=1
    negate mismatch kind=raised case=0 m=1
    mutated forced exit=1

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native and forced-int64 field backends, LTO,
ASan/UBSan/VERIFY execution, focused field tests, and Clang AArch64 code
generation. The independent modular-negation oracle found no residue,
magnitude, aliasing, or lowering mismatch, and the mutation proves that the
oracle detects a shared field-prime defect. No production change,
regression test, or finding commit is justified. This does not provide
AArch64 runtime execution, GCC AArch64, ARMv7/RISC-V, or a formal proof of
constant-time behavior; branch inspection remains compiler evidence only.
The next queue is another compiler/architecture constant-time or
overflow-sensitive helper. Do not repeat the seventeen dismissed compiler
hypotheses unless compiler, source, or architecture evidence changes.

### Cycle 2026-07-27: scalar multiplication translation and Clang assembly performance

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `691c5be7`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, CMake/Ninja, `aarch64-linux-gnu-objdump`, and the existing ASan and
recovery/no-VERIFY builds were available. No AArch64 runtime or sysroot,
GCC AArch64, ARMv7/RISC-V runner, Alive2, CBMC, or KLEE was available.
The scalar multiplication contract, native and forced-width reducers and
products, the independent scalar fuzzer oracle, history, and related GitHub
issues were read before selecting this distinct kernel.

### Hypothesis and contract

The eighteenth bounded hypothesis was that compiler transformation, the
4x64 versus forced 8x32 scalar representation, or the x86_64 assembly
product path could miscompute `secp256k1_scalar_mul`, mishandle aliasing, or
introduce an unexpected data-dependent branch at product/reduction
boundaries. The contract at `src/scalar.h:55` is multiplication modulo the
group order. The native implementation uses the 4x64 reducer and product
paths in `src/scalar_4x64_impl.h:351-868`, including the optional x86_64
assembly product; the forced implementation uses the 8x32 paths in
`src/scalar_8x32_impl.h:407-653`. The existing independent fuzzer oracle in
`src/fuzz/scalar.c:30-51,381-435,517-536` computes a full base-2^16 product
and reduces it independently.

### Independent oracle evidence

The standalone C harness
`/tmp/secp256k1-translation-78/scalar-mul-harness.c` has SHA-256
`75fc7a299b70865e89a5c88dfc2a17cd8050902a4a6f8326f56178186b7b802a`. It
contains an independent base-2^16 full-product and binary long-division
reducer. It covers 645 canonical values including zero, one, `n-1`, `n-2`,
`n/2`, all powers of two and their order complements, and 128 deterministic
values. It checks 7,474 selected pairs, including boundary/self/reverse
pairs and 1,024 cross-boundary power/complement pairs. Every pair checks the
normal result and both left and right in-place aliases. Native assembly,
native portable C, and forced-int64 clean runs printed exactly:

    ok values=645 cases=7474 digest=739447fca5d13916

The second harness
`/tmp/secp256k1-translation-78/scalar-mul-cpp-harness.cpp` has SHA-256
`7f915b43c613343ac3b89711892bfa4bc3a9a5732f1d1e20f09a435a83a0bbf7`. It
uses Boost `cpp_int` only for the expected product modulo the order and
calls production code through the C shim
`scalar-mul-c-shim.c` (SHA-256
`d25dbe26cf07472fdd341a4011c1482b76698c5ee98fd9ece3a66dd62d971475`). The
C++ harness covers 581 values and 5,810 pairs, including both aliases. Clang
and GCC native-assembly and forced-int64 runs all printed:

    ok cpp-values=581 cpp-cases=5810 digest=79f3943e75212f57

The C shim was necessary because the internal C headers are not directly
C++-clean; this is a harness limitation, not a repository finding.

### Compiler, sanitizer, and project evidence

Clang and GCC built and ran the C harness for native assembly, native
portable C, and forced-int64 at `O0`, `O2`, `O3`, and `Os` (24 executions).
Clang and GCC native/forced `O2 -flto` builds (six executions) matched the
same digest. Clang and GCC native-assembly, native-portable-C, and
forced-int64 `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined`
executions (six executions) also matched without diagnostics. The existing
ASan and recovery/no-VERIFY binaries passed the focused scalar/field tests
with fixed seed `0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef`.
All ten files in `src/fuzz/corpora/scalar` ran once under both sanitized
scalar fuzzers with fixed seed `3923475549`, without diagnostics or
artifacts.

The six Clang/GCC O2 non-LTO probe bodies for native assembly, native C, and
forced-int64 had no conditional or loop jumps. The six corresponding O2 LTO
probe bodies also had none; the generated code used conditional moves,
`set` instructions, and carry operations. This is code-generation evidence,
not a proof of constant-time behavior.

The Clang AArch64 compile-only matrix used native and forced-int64 selectors
at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16 builds
completed. The non-VERIFY object hashes were:

    native: O0 817dea28c9933ac49036ff976704a97b5c965aaaf8a847d32716ec5b316b57b2
            O2 6c23665b19b935b4391a0b76e6e2ef147008a309d27746d95d0f6f9fa60dd8bd
            O3 bbdc83732734cafe3eb18e7ed26b102be44eeb0146f4b8c320a26e4fc908fa97
            Os 7409fc94b384cdff45a3545be29e3e1502919a6013b7ea97c5b39e0f4dcf216d
    forced: O0 48946eaf3d5b29c6f24738040ea5e46c1983ab6e29da8152448251fc5c208fd7
            O2 2e3458561bbd7c1364a8199cf95a23dc6215743a74b68bf0cb78239be49f8a57
            O3 a32f4edff5f5da6b49d909062be1cd6b4d711695c9d47ba31743f076f3a42a91
            Os e43da63afb1a907ac716511b4bad861ef1e3afb896e65e18ac4111b3dc632bf3

The VERIFY hashes, in native O0/O2/O3/Os then forced O0/O2/O3/Os order,
were:

    2c5d8e7b746eac93ae306e1b4906dea2e5948552245594527ad2d38e3a8c8f5b
    e0eefce580f2cc2b22093193859684b89aba8e16d1f5f312228da84638b67c07
    60ce276f511bb5463f5782208082a6b16f0fd275b13cb42d5d4ccb2faf71f8cd
    e8514c1b39e51c581fcba332533d0f0324566d6b3f44ba30371223b50cc5c6fe
    1ae661a5a37ce4c2099b2b82c9f447e2227b1d9a239ef9894ff0a4d587afe3c3
    c6363c1304c5598823ec6bb34dc7d91b7d36530545c66ac43f731d947ff6d711
    6ba1c64ddbfc006ea48a2ea553a950901e4e354e9be826bfd0c601929f7588bb
    e656162735a399133f26e26a3b6249319b7721557aabb90a892f928702e70044

No AArch64 runtime, GCC AArch64, ARMv7/RISC-V build, or formal translation
validation was possible.

### Mutation controls

Scratch copies of `src/` and `include/` changed
`SECP256K1_N_C_0` from `(~SECP256K1_N_0 + 1)` to
`(~SECP256K1_N_0 + 2)` in both scalar backend implementations. Clean Clang
O2 runners rejected the wrong result at `mul mismatch case=22` for native
assembly, native portable C, and forced-int64. The independent `cpp_int`
verifier also rejected both mutated native and forced builds at
`cpp mul mismatch case=22`. The oracle therefore distinguishes a shared
reduction-constant defect.

### Related performance lead

GitHub issue [#1682](https://github.com/bitcoin-core/secp256k1/issues/1682)
reports a current Clang x86_64 assembly `scalar_mul` slowdown relative to
the portable path and a GCC speedup. The issue is open and its discussion
already contains compiler/version comparisons. A local Release CMake
reproduction on Linux x86_64 (Intel Core i9-9900K, 16 CPUs, Linux
6.17.0-23-generic, Clang 22.1.7, GCC 16.1.0) used
`SECP256K1_BENCH_ITERS=300000 .../bin/bench_internal mul` with otherwise
matching builds:

    compiler  assembly scalar_mul min/avg/max (us)  noasm scalar_mul min/avg/max (us)
    Clang     0.0411/0.0411/0.0412                0.0336/0.0336/0.0337
    GCC       0.0376/0.0376/0.0377                0.0411/0.0412/0.0417

`field_mul` was 0.0190 us for both Clang variants and 0.0168/0.0168 us for
both GCC variants. A process-level `perf stat` run at 100,000 iterations
also showed Clang assembly at about 2.181 billion cycles and 0.6077 seconds
versus 1.897 billion cycles and 0.5289 seconds for noasm; it included the
whole benchmark process, so it is supporting evidence rather than a
function-isolated profile. The observation is reproducible performance
behavior, not a proven compiler defect or a new correctness finding. It is
handed to the pending compiler/optimization campaign (goal 70) with exact
builds and issue provenance; no automatic dispatcher change is justified
without a causal profile, portability policy, and broader benchmark data.

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native assembly/native C/forced-int64 paths, LTO,
ASan/UBSan/VERIFY execution, focused tests and scalar corpus, and Clang
AArch64 code generation. Both independent modular-product oracles found no
arithmetic, reduction, aliasing, or lowering mismatch, and the mutation
controls prove that they detect a shared scalar-order constant defect. No
production code or regression test change is justified. The separate Clang
assembly performance observation remains an open, reproducible lead for
goal 70, with no root cause established here. The next queue is another
compiler/architecture constant-time or overflow-sensitive helper; do not
repeat the eighteen dismissed compiler hypotheses unless compiler, source,
architecture, or performance evidence changes.

### Cycle 2026-07-27: scalar negation translation

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `e7916424`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, CMake/Ninja, `aarch64-linux-gnu-objdump`, and the existing ASan and
recovery/no-VERIFY builds were available. No AArch64 runtime or sysroot,
GCC AArch64, ARMv7/RISC-V runner, Alive2, CBMC, or KLEE was available.
The scalar negation contract, both backend implementations, the existing
fuzzer reference, tests, blame/history, and the prior conditional-negation
translation cycles were read before selecting this distinct helper.

### Hypothesis and contract

The nineteenth bounded hypothesis was that compiler lowering or the native
4x64 versus forced 8x32 representation could miscompute
`secp256k1_scalar_negate`, mishandle zero, break in-place aliasing, or emit a
data-dependent branch. The contract at `src/scalar.h:64` defines the result
as the complement modulo the group order. The native implementation at
`src/scalar_4x64_impl.h:176-197` uses a 128-bit carry accumulator and masks
all limbs with a zero-derived `nonzero` value; the forced implementation at
`src/scalar_8x32_impl.h:214-235` uses 32-bit limbs and a 64-bit carry
accumulator with the same zero masking. The implementation was introduced
with the constant-time scalar family in `1d52a8b1`. The existing fuzzer
reference at `src/fuzz/scalar.c:199-207` was treated as a seed rather than
the sole oracle.

### Independent oracle evidence

The standalone C harness
`/tmp/secp256k1-translation-78/scalar-negate-harness.c` has SHA-256
`7895c381fe35926cad7fcd479a1442fa12db714f73cdbbb380ba755d94b68c5f`. It
computes `n-a` with an independent big-endian byte subtraction and maps
zero to zero. It covers 645 canonical values: zero, one, `n-1`, `n-2`,
`n/2`, every power of two and its order complement, and 128 deterministic
values below `2^255`. Each value checks an out-of-place result, in-place
aliasing, and double negation back to the original bytes. Clang and GCC
native assembly, native portable C, and forced-int64 O2 runs all printed:

    ok values=645 cases=645 digest=3a235f391877b00c

The independent high-level verifier
`/tmp/secp256k1-translation-78/scalar-negate-cpp-harness.cpp` has SHA-256
`ba5598f7683491364dcfa8afdd066854ed45b669d5b27eef4104125aca363eed`. It
uses Boost `cpp_int` for `n-a` and calls production code through the C-only
shim `scalar-negate-c-shim.c` (SHA-256
`2de8a124970b518db10607af389c55800d55e0fe3642dc978050336db7bd0f91`). It
covers 581 values and checks both the normal and aliased calls. Clang and
GCC native-assembly and forced-int64 runs all printed:

    ok cpp-values=581 cpp-cases=581 digest=820b82c9f494768e

The shim was needed because the internal C headers are not directly C++
clean; this is a harness limitation, not a repository finding.

### Compiler, sanitizer, and project evidence

Clang and GCC built and ran the C oracle for native assembly, native
portable C, and forced-int64 at `O0`, `O2`, `O3`, and `Os` (24 executions).
Clang and GCC native/portable/forced `O2 -flto` builds (six executions)
matched the same digest. Clang and GCC native-assembly, native-portable-C,
and forced-int64 `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined`
executions (six executions) also matched without diagnostics. The existing
ASan and recovery/no-VERIFY binaries passed `scalar_tests`, `field_half`,
and `field_misc` for four iterations and two jobs with fixed seed
`123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef`. All ten
files in `src/fuzz/corpora/scalar` ran once under both sanitized scalar
fuzzers with fixed seed `3923475549`, without diagnostics or artifacts.

The six Clang/GCC O2 non-LTO `probe_negate` bodies for native assembly,
native C, and forced-int64 had zero conditional or loop jumps. The six
corresponding O2 LTO bodies also had zero. Representative x86 code used
`cmove` and `setb`; this is code-generation evidence and not a proof of
constant-time behavior.

The Clang AArch64 compile-only matrix used native and forced-int64 selectors
at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16 builds
completed, and all eight non-VERIFY probe bodies had zero conditional or
loop branch mnemonics. The non-VERIFY object hashes were:

    native: O0 cb172f0a60f0d59fe8bdd25e5ca076f6cc8a9f51ac1dba331a41bcd927568373
            O2 d6edd7f91de56ecf7d2f0b78c61d4ad59c4468ccb3bfbd31fb70b06bc9bab630
            O3 805aea193e8e3561c1b77ad016b4c5977b932928512fa54b159e0f3435a1949f
            Os ca1bc60910060e7fa9cc4d87b7de78de212c16056c99c57c8c9231a84109fc40
    forced: O0 49ac1508556066508215814c843571bc38b750f0dbe91467a6a5e23aafe6a2a1
            O2 169ea8c2015f44da9cad0c7493a2e34b186d974ae4d3215137131f689faf9e33
            O3 9253d0fe2d189569c9a7882491bf51d74591a418df7cdc8bf450148f1a16d609
            Os 09d3250834bf1de052fab35013ebc81f74a40c6b94b33609cc4b8d1cc2e477a6

The VERIFY hashes, in native O0/O2/O3/Os then forced O0/O2/O3/Os order,
were:

    9e7b65b6827b1a413eb0288aa60bae3e0533a735be33cd9db979fcbd3b47967a
    aff6323af2cdbd1f4bdef9242da84d548ad80562e30c1383bbe29c12c3e3226b
    135a98076ae10001329a6a4bdee490e2b12fd786971a906c38518e9f01e750e0
    309393cce4aa5ca7dce87d545f6ed92ce9882bcc269a2878ae36485beaace770
    42fa8ff9274328568215814c843571bc38b750f0dbe91467a6a5e23aafe6a2a1
    1193bac60d62c3a8bc4c4dff4f485ea3d8407bbea84ef4d72d30966ad88687f3
    0e75b469d75e6aa286325871ee072acb1630ed877b8d2c3c53ff50c86bf47684
    76ca204543edaa368cfd407d264e40e564c31cb5b512e3c7fbed6e08b23d87be

No AArch64 runtime, GCC AArch64, ARMv7/RISC-V build, or formal translation
validation was possible.

### Mutation controls

Scratch copies of `src/` and `include/` changed the first scalar-order
addend in `scalar_negate` from `SECP256K1_N_0 + 1` to `SECP256K1_N_0 + 2`
in both backend implementations. Clean Clang O2 runners rejected the
mutation immediately after the zero vector:

    negate mismatch case=1
    negate mismatch case=1

The independent `cpp_int` verifier likewise rejected both mutated native and
forced builds with `cpp negate mismatch case=1`. The controls show that the
oracle is sensitive to the scalar-order arithmetic rather than merely
checking the shared implementation shape.

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native assembly/native C/forced-int64 paths, LTO,
ASan/UBSan/VERIFY execution, focused tests and scalar corpus, and Clang
AArch64 code generation. Both independent negation oracles found no residue,
zero, aliasing, carry, or lowering mismatch, and the mutation controls failed
in both backend implementations. No production code, regression test, or
finding commit is justified. This does not provide AArch64 runtime
execution, GCC AArch64, ARMv7/RISC-V, or a formal constant-time proof. The
next queue is another compiler/architecture constant-time or
overflow-sensitive helper; do not repeat the nineteen dismissed compiler
hypotheses unless compiler, source, architecture, or performance evidence
changes.

### Cycle 2026-07-27: scalar byte decoding and overflow reduction

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `3529d3b3`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, CMake/Ninja, `aarch64-linux-gnu-objdump`, and the existing ASan and
recovery/no-VERIFY builds were available. No AArch64 runtime or sysroot,
GCC AArch64, ARMv7/RISC-V runner, Alive2, CBMC, or KLEE was available.
The `scalar_set_b32` contract, both representation-specific decoders and
reducers, the fuzzer reduction reference, boundary tests, blame/history,
and prior scalar translation cycles were read before selecting this distinct
representation boundary.

### Hypothesis and contract

The twentieth bounded hypothesis was that byte order, overflow detection, or
single-subtraction reduction could diverge between native 4x64 and forced
8x32 code, especially at `n-1`, `n`, `n+1`, `2^256-1`, carry boundaries,
or compiler optimization boundaries. The contract at `src/scalar.h:35-39`
loads a 32-byte big-endian integer, reduces it modulo the group order, and
reports whether the unreduced input was at least `n`; the output must be a
canonical scalar. The native decoder/reducer is at
`src/scalar_4x64_impl.h:64-155`, and the forced decoder/reducer is at
`src/scalar_8x32_impl.h:76-187`. The existing fuzzer reference at
`src/fuzz/scalar.c:129-140` independently relies on the fact that every
256-bit input is below twice the order and subtracts `n` at most once.
Historical edge hardening in `104f53ea` was also checked, including the
explicit carry-width casts and final reduction bounds.

### Independent oracle evidence

The standalone C harness
`/tmp/secp256k1-translation-78/scalar-set-b32-harness.c` has SHA-256
`9439124da8f90e78aa68b376cff7b5251e03959686dbf749239b17d1701dcc74`. It
computes the expected overflow bit with an independent big-endian compare
and performs the one permitted byte subtraction itself. It covers 395 raw
32-byte values: zero, one, `n-1`, `n`, `n+1`, `n+2`, `2^256-1`, the two
near-maximum values, `n/2`, `n/2+1`, all 256 powers of two, and 128 full-width
deterministic values. Each case checks both a non-NULL overflow output and
the NULL-output form. Clang and GCC native assembly, native portable C, and
forced-int64 O2 runs all printed:

    ok values=395 cases=395 digest=98dd70c0a3477278

The independent high-level verifier
`/tmp/secp256k1-translation-78/scalar-set-b32-cpp-harness.cpp` has SHA-256
`581440ca50f197ca74a0f6ac4e591e3b591c9206630e1865be3f5a544399e7de`. It
uses Boost `cpp_int` for the compare and subtraction and calls production
code through `scalar-set-b32-c-shim.c` (SHA-256
`0e8f3ad3952a00072bda06995e6e37bd52ab2fe0ce1a99463f5d03016e540738`). It
covers the same 395-value structure and both overflow-pointer forms. Clang
and GCC native-assembly and forced-int64 runs all printed:

    ok cpp-values=395 cpp-cases=395 digest=98dd70c0a3477278

### Compiler, sanitizer, and project evidence

Clang and GCC built and ran the C oracle for native assembly, native
portable C, and forced-int64 at `O0`, `O2`, `O3`, and `Os` (24 executions).
Clang and GCC native/portable/forced `O2 -flto` builds (six executions)
matched the same digest. Clang and GCC native-assembly, native-portable-C,
and forced-int64 `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined`
executions (six executions) also matched without diagnostics. The existing
ASan and recovery/no-VERIFY binaries passed `scalar_tests`, `field_half`,
and `field_misc` for four iterations and two jobs with fixed seed
`0fedcba9876543210fedcba9876543210fedcba9876543210fedcba987654321`. All ten
files in `src/fuzz/corpora/scalar` ran once under both sanitized scalar
fuzzers with fixed seed `3923475549`, without diagnostics or artifacts.

The production-function disassembly was checked separately from the
no-inline wrapper. Each Clang/GCC x86 O2 and O2-LTO native/portable/forced
`secp256k1_scalar_set_b32` body had exactly one conditional jump, the final
`overflow` output-pointer NULL check. The input comparisons lowered to
`set`/conditional-move operations; no branch depended on input bytes. The
same held for all eight Clang AArch64 O2-family production bodies: exactly
one `cbz` checked the optional output pointer, with no input-dependent
conditional or loop branch. This is code-generation evidence, not a formal
constant-time proof, and the pointer check is an intentional public API
shape rather than a secret-dependent branch.

The Clang AArch64 compile-only matrix used native and forced-int64 selectors
at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16 builds
completed. The non-VERIFY object hashes were:

    native: O0 a4fc0b7848032cd6d6fa73592409d9e880b26a34ded98730cc4e4368be4e3819
            O2 2705d2760f5c76320816f894c76382ba7ffc50f56018132ddf6c4beafab50829
            O3 b1b56927472f6571a9d871a3509628a47823e9402d0c99d6d1b748beed718b52
            Os 82d975ade349f7084fb3d5b204726db3f99fb51ba9ad3966a3acc974f7c3b4ac
    forced: O0 7932f530311fb994b1066f372ce52311b9d0672139da98eb421c579f3b593482
            O2 6a01d8f2588b45f732e078f3ca7e52e5fe4bfacec954a7a17194c385e29a9ed7
            O3 e28df14a7674422554840915661c9afb518a634db4418ffbe1343ff053c57c28
            Os 5c4ff18b775d042ca11da1bdc88d2e015fddf1f019cfa075e0434697bdec5549

The VERIFY hashes, in native O0/O2/O3/Os then forced O0/O2/O3/Os order,
were:

    ba77fb83db0bdd126a7c9cff18618467ec9af0fc902df78dbfe501a791c80029
    816fc13c3a5b5fb7d317a604647991b7f2266a55f8d53f677d4961c7c1d14abc
    5c7f452d9e9b9ad1321822508a25f61ad9a4e753d249d58c090070bfb101e3cb
    cf232902502251337bb7dc0323b3bded08103ebba1422c67e2d95f09eed9f9f0
    8e2437c508e52345dd09ea93f420873ba1529ce5153558cff43ca6a46c8ebee6
    fcb2b482e6f5114aae8d7945cbdec05a3106282aad691612aed223db284ee2a1
    8216396c4aa8476ccc7e2fb8f862ae96a029620cb372cd0dbc485371b77b4cb4
    c0521b60f33e723a80e4ad26bc6be571aa9c2c955b9c9999d45014eb49a641c6

No AArch64 runtime, GCC AArch64, ARMv7/RISC-V build, or formal translation
validation was possible.

### Mutation controls

Scratch copies of `src/` and `include/` changed
`SECP256K1_N_C_0` from `(~SECP256K1_N_0 + 1)` to
`(~SECP256K1_N_0 + 2)` in both backend implementations. Clean Clang O2
runners rejected the wrong reduction at the exact `n` input:

    set mismatch case=3
    set mismatch case=3

The independent `cpp_int` verifier likewise rejected both mutated native and
forced builds with `cpp set mismatch case=3`. The mutation proves that the
boundary oracle detects a wrong reduction constant rather than only
replaying the production comparison.

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native assembly/native C/forced-int64 paths, LTO,
ASan/UBSan/VERIFY execution, focused tests and scalar corpus, and Clang
AArch64 code generation. Both independent decoders found no byte-order,
overflow-bit, reduction, NULL-output, or representation mismatch. No
production code, regression test, or finding commit is justified. The one
observed branch is the explicit optional-output-pointer check and is not
input-dependent. This does not provide AArch64 runtime execution, GCC
AArch64, ARMv7/RISC-V, or a formal constant-time proof. The next queue is
another compiler/architecture constant-time or overflow-sensitive helper;
do not repeat the twenty dismissed compiler hypotheses unless compiler,
source, architecture, or performance evidence changes.

### Cycle 2026-07-27: secret-key validity wrapper

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `a51a299e`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, CMake/Ninja, `aarch64-linux-gnu-objdump`, and the existing ASan and
recovery/no-VERIFY builds were available. No AArch64 runtime or sysroot,
GCC AArch64, ARMv7/RISC-V runner, Alive2, CBMC, or KLEE was available.
The `scalar_set_b32_seckey` declaration, implementation, callers, fuzzer
checks, tests, blame/history, and the preceding raw decoder cycle were read
before selecting this adjacent but distinct validity wrapper.

### Hypothesis and contract

The twenty-first bounded hypothesis was that compiler lowering or backend
representation could misclassify zero, `n`, or high raw inputs as valid,
return inconsistent values around the order boundary, or leave a different
canonical output across native 4x64 and forced 8x32 paths. The API comment at
`src/scalar.h:38-40` promises a return value of one only for a valid secret
key. `src/scalar_impl.h:34-41` first calls `scalar_set_b32`, then returns
`(!overflow) & (!scalar_is_zero(r))`; history identifies the wrapper's
introduction in `9ab2cbe0`. The implementation necessarily writes the
canonical decoded scalar before returning, including for invalid inputs, but
that output-on-invalid behavior is an observed implementation invariant
rather than the short public comment's primary promise.

### Independent oracle evidence

The standalone C harness
`/tmp/secp256k1-translation-78/scalar-seckey-harness.c` has SHA-256
`434c02e7f93123ddc181a2a9ec0f6a1f116ec92f7799b836f19e04b6c9109d8f`. It
independently computes the canonical value and validity predicate
`0 < input < n` for 395 raw values: zero, one, `n-1`, `n`, `n+1`, `n+2`,
maximum 256-bit inputs, nearby maxima, half-order boundaries, every power of
two, and 128 full-width deterministic values. It checks the return value and
the canonical output for every input. Clang and GCC native assembly, native
portable C, and forced-int64 O2 runs all printed:

    ok values=395 cases=395 digest=c178fe0c22775966

The independent high-level verifier
`/tmp/secp256k1-translation-78/scalar-seckey-cpp-harness.cpp` has SHA-256
`943d065ad46bacf074b10ae213bf84ea6e8dae8282a0b70b26ec355f65490255`. It
uses Boost `cpp_int` for the validity comparison and reduction and calls
production code through `scalar-seckey-c-shim.c` (SHA-256
`8a9f45ead550655e227fab29943af3b21d3840ead218841a52187ed3303c071b`).
Clang and GCC native-assembly and forced-int64 runs produced the same
`c178fe0c22775966` digest.

### Compiler, sanitizer, and project evidence

Clang and GCC built and ran the C oracle for native assembly, native
portable C, and forced-int64 at `O0`, `O2`, `O3`, and `Os` (24 executions).
Clang and GCC native/portable/forced `O2 -flto` builds (six executions)
matched the same digest. Clang and GCC native-assembly, native-portable-C,
and forced-int64 `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined`
executions (six executions) also matched without diagnostics. The existing
ASan and recovery/no-VERIFY binaries passed `scalar_tests`, `field_half`,
and `field_misc` for four iterations and two jobs with fixed seed
`13579bdf2468ace013579bdf2468ace013579bdf2468ace013579bdf2468ace0`. All ten
files in `src/fuzz/corpora/scalar` ran once under both sanitized scalar
fuzzers with fixed seed `3923475549`, without diagnostics or artifacts.

The no-inline `probe_set_b32_seckey` bodies in all six Clang/GCC O2 native,
portable, and forced builds and all six O2-LTO builds had zero conditional or
loop jumps. Input comparisons and the zero test lowered to `set`/conditional
select operations. This is code-generation evidence, not a formal
constant-time proof.

The Clang AArch64 compile-only matrix used native and forced-int64 selectors
at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16 builds
completed, and all eight non-VERIFY probe bodies had zero conditional or
loop branch mnemonics. The non-VERIFY object hashes were:

    native: O0 7bf8b785a17408a15d60bd7990d7f72d0abe5c512f739cbce8bcaaadb717aadc
            O2 d4c478c405ad7dea9e2713164fd5de4b53eb3be2f8a97a108744a057c908def3
            O3 2a2eac75242e19b6c0dee01cfe3d5ceed213c6663e85c4034adf357018dc5b90
            Os 783414b8b507a00b6c631b64fe807544f406690b683e87f484b582193b5a996a
    forced: O0 6959be63cc12416bd8e62732929177de8d46fa2f621182c352b12ee38951670f
            O2 aae42a9f97c1aa558c38878986cedf82dcafde9c0210d4838f84eba85262ff24
            O3 fa8b0261c6f077598526bb903453898da09ef0df66843980c36652ed20991df6
            Os 7377726da1d84cac9e719fc6df19ef91760eb782023b58a0f3297d9bed7927b3

The VERIFY hashes, in native O0/O2/O3/Os then forced O0/O2/O3/Os order,
were:

    8a535797ebd488880cf0a942cba3e684bd7bd8ab63763df4f2391e825a5bd805
    fa75d9ce812b042befb0901631715793d156ec3265aa0cf7fe5a033495aae4ea
    0fde95b2cbf37e146097c292dc175c6437b8374f360cc291461f9ba256662ef3
    521afa84b94eae58cb2d353eb1ad231bfbdd45f8ddf22c8b3762c4d2bd0159a4
    0c0b6699ea59a0eb4d6a52c314638015cbebea8c104936a7b89aaf2078dfe0a0
    e3bc0eedc6cc47dd2c8f4da85e3ad69012d3fe2b3c93c0270209882ffbd90dc0
    a243e3d1ea5fbd8ce06d2b0e0adb0d9d3888175f3de6614dccee543f2d699f63
    69ab8076a45732748d90dbe20f3aee1f51e46b92f5b10b87eb0bdb90c226121f

No AArch64 runtime, GCC AArch64, ARMv7/RISC-V build, or formal translation
validation was possible.

### Mutation controls

Scratch copies changed the validity combination in `src/scalar_impl.h` from
`(!overflow) & (!secp256k1_scalar_is_zero(r))` to `|` in both native and
forced source trees. Clean Clang O2 runners rejected the mutation at the zero
vector:

    validity mismatch case=0 expected=0 actual=1
    validity mismatch case=0 expected=0 actual=1

The independent `cpp_int` verifier likewise rejected both mutated builds with
`cpp validity mismatch case=0`. The mutation proves the oracle observes the
validity contract and is not only checking the canonical decoded bytes.

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native assembly/native C/forced-int64 paths, LTO,
ASan/UBSan/VERIFY execution, focused tests and scalar corpus, and Clang
AArch64 code generation. Both independent oracles found no zero/order
boundary, validity-return, canonical-output, or lowering mismatch. No
production code, regression test, or finding commit is justified. This does
not provide AArch64 runtime execution, GCC AArch64, ARMv7/RISC-V, or a
formal constant-time proof. The next queue is another compiler/architecture
constant-time or overflow-sensitive helper; do not repeat the twenty-one
dismissed compiler hypotheses unless compiler, source, architecture, or
performance evidence changes.

### Cycle 2026-07-27: scalar equality comparison

### Pre-cycle audit

The worktree was clean on `codex/fuzz-oracles` at `e1578b0d`, based on
`origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. Clang 22.1.7, GCC
16.1.0, CMake/Ninja, `aarch64-linux-gnu-objdump`, and the existing ASan and
recovery/no-VERIFY builds were available. No AArch64 runtime or sysroot,
GCC AArch64, ARMv7/RISC-V runner, Alive2, CBMC, or KLEE was available.
The scalar equality declaration, both limbwise implementations, fuzzer
uses, tests, history, and previous scalar predicate cycles were read before
selecting this distinct comparison helper.

### Hypothesis and contract

The twenty-second bounded hypothesis was that native 4x64 and forced 8x32
limb comparison could disagree for zero, order-adjacent, power/complement,
or cross-limb differences, or that an optimizer could introduce an
input-dependent branch. The contract at `src/scalar.h:86` is equality of two
canonical scalar values. The native implementation at
`src/scalar_4x64_impl.h:886-892` and forced implementation at
`src/scalar_8x32_impl.h:679-685` XOR corresponding limbs, OR the differences,
and compare the aggregate to zero. Existing fuzzer checks at
`src/fuzz/scalar.c:598-603` were used as coverage/history seeds, not as the
independent oracle.

### Independent oracle evidence

The standalone C harness
`/tmp/secp256k1-translation-78/scalar-eq-harness.c` has SHA-256
`2ba1e8f4e437391e116cfd0e602a48a4305708290f9288f577dcdc88ed5b1c5c`. It
compares serialized canonical bytes with `memcmp`, then checks both argument
orders. It covers 645 values including zero, one, `n-1`, `n-2`, `n/2`, every
power of two and its order complement, and 128 deterministic values. Each
value is compared with itself, seven fixed boundary values, offset and
reverse values, producing 6,450 pair cases. Clang and GCC native assembly,
native portable C, and forced-int64 O2 runs all printed:

    ok values=645 cases=6450 digest=36554ee1f215b27b

The independent C++ verifier
`/tmp/secp256k1-translation-78/scalar-eq-cpp-harness.cpp` has SHA-256
`cb2a3dd4604779c1b8cf51df7be742d6498aa9d861d1a3ca5d1d8635b4790b2e`. It
uses serialized-byte equality and calls production code through
`scalar-eq-c-shim.c` (SHA-256
`1ffb7011558cf141ef5552a81e07d6fe234ad15a65a08fff5ee0aac3dc52013e`). It
covers 581 values and 5,810 symmetric pair cases. Clang and GCC
native-assembly and forced-int64 runs all printed:

    ok cpp-values=581 cpp-cases=5810 digest=022065fdd7629ffb

### Compiler, sanitizer, and project evidence

Clang and GCC built and ran the C oracle for native assembly, native
portable C, and forced-int64 at `O0`, `O2`, `O3`, and `Os` (24 executions).
Clang and GCC native/portable/forced `O2 -flto` builds (six executions)
matched the same digest. Clang and GCC native-assembly, native-portable-C,
and forced-int64 `O1 -DVERIFY -DVALGRIND -fsanitize=address,undefined`
executions (six executions) also matched without diagnostics. The existing
ASan and recovery/no-VERIFY binaries passed `scalar_tests`, `field_half`,
and `field_misc` for four iterations and two jobs with fixed seed
`2468ace013579bdf2468ace013579bdf2468ace013579bdf2468ace013579bdf`. All ten
files in `src/fuzz/corpora/scalar` ran once under both sanitized scalar
fuzzers with fixed seed `3923475549`, without diagnostics or artifacts.

The no-inline `probe_eq` bodies in all six Clang/GCC O2 native, portable, and
forced builds and all six O2-LTO builds had zero conditional or loop jumps.
Clang x86 lowered the final comparison to `sete`; Clang AArch64 lowered it to
vector compare/aggregate operations and a final bit calculation. This is
code-generation evidence, not a formal constant-time proof.

The Clang AArch64 compile-only matrix used native and forced-int64 selectors
at `O0`, `O2`, `O3`, and `Os`, with and without VERIFY. All 16 builds
completed, and all eight non-VERIFY probe bodies had zero conditional or
loop branch mnemonics. The non-VERIFY object hashes were:

    native: O0 e3ffabbaaaef6457a8f74279abc64961d02e72c5aa9b779ab853b372480799b4
            O2 ac9c2d2179cb039d63382d812592b29c44228d7a0251c56059d42a1955172c1b
            O3 a73d277888057d1614cbc658907c21bb416a9b0af34212d73309d0aad93e3c10
            Os e6c4bcb361260f0297e2ff7dd65994e226d7bec5f6796155b0afe9b4e1ed4213
    forced: O0 547342bfae0ab69691c539b8b1d15d494194b6f69d283c1dab8984764ee045ef
            O2 5187a522cfd1413832624a0d2cc6bed2ef1196ed966489e590e3cf36766355e9
            O3 79a811b63f54f7758852c2176ce087e45b9624bd031df596b42b175ad0812d08
            Os df28166dc3bf722aee35b3fe701e7cda4df240314f7bdcac58724fec87ff6eea

The VERIFY hashes, in native O0/O2/O3/Os then forced O0/O2/O3/Os order,
were:

    e65a749b775178ad9c12b9b0e321e0b76f1a4275126713f8b6a2f111a83cc2ea
    0b41e8a39ec053065fe8c916b79148f9960387910460ce6eb504691493472a
    b1e0054a22b1c2f342427cc53a3edac71407e372aa476ce4ea5decbf766fc87e
    d637c350d5d7d114341ca127fd539618d7fac13bf41be68e82466d60d0e4714b
    f6c65bfb8433b3f0ee81e836bb5cb7058d3beb6412415e0dc73689c8cfa57d98
    6d6cbad4dfa99f72ddcd86022daba36feaba1d2efce2e551e94de3fbb5e63940
    974582b8ec27823cb98bdeb44d25c42e2931202d51bc9b66792d852c2cec6113
    c5931cb5100b49f0d8198656643eb76bed5f70e410f80b06653bdbe6c1434038

No AArch64 runtime, GCC AArch64, ARMv7/RISC-V build, or formal translation
validation was possible.

### Mutation controls

Scratch copies changed the first XOR-aggregate operator from `|` to `&` in
both backend equality expressions. Clean Clang O2 runners rejected the
mutation at the first unequal fixed pair:

    eq mismatch case=2 expected=0 actual=1
    eq mismatch case=2 expected=0 actual=1

The independent C++ verifier likewise rejected both mutated native and forced
builds with `cpp eq mismatch case=2`. The mutation proves that the oracle
distinguishes unequal limb patterns rather than only testing equal inputs.

### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 matrix, native assembly/native C/forced-int64 paths, LTO,
ASan/UBSan/VERIFY execution, focused tests and scalar corpus, and Clang
AArch64 code generation. Both independent oracles found no equality,
symmetry, cross-limb, or lowering mismatch. No production code, regression
test, or finding commit is justified. This does not provide AArch64 runtime
execution, GCC AArch64, ARMv7/RISC-V, or a formal constant-time proof. The
next queue is another compiler/architecture constant-time or
overflow-sensitive helper; do not repeat the twenty-two dismissed compiler
hypotheses unless compiler, source, architecture, or performance evidence
changes.

### Cycle 2026-07-27: scalar zero predicate translation

#### Scope and hypothesis

The twenty-third goal-78 hypothesis targeted the direct
`secp256k1_scalar_is_zero` predicate in the native 4x64 backend
(`src/scalar_4x64_impl.h:170-174`) and forced 8x32 backend
(`src/scalar_8x32_impl.h:208-212`). Both implementations verify the scalar
then OR every limb and compare the aggregate with zero. The trust boundary is
the canonical scalar produced by `secp256k1_scalar_set_b32`; raw bytes are
untrusted until that conversion. The question was whether a compiler,
representation, optimization, or reduction-boundary mismatch could make a
nonzero canonical scalar appear zero, or make the two backends disagree.

Existing coverage was used only as a seed. The scalar fuzzer's
`scalar zero one predicates` trigger at `src/fuzz/scalar.c:1041-1060` checks
zero, one, `n-1`, and the overflowing `n -> 0` reduction, but does not provide
an independent oracle for the direct predicate. History search found the
implementation in `1d52a8b1`, with later constant-time and verification
changes in `44015000`, `1e0e885c`, and `a0fb68a2`; blame attributes the OR
reductions to `1d52a8b1`.

#### Independent oracle evidence

The C harness
`/tmp/secp256k1-translation-78/scalar-zero-harness.c` has SHA-256
`f4d2835412039baab6107000c080c0ee83cc5e897e84f63fd282e07f8390eba7`. It
constructs big-endian values independently, computes the expected reduction
with byte subtraction, checks the serialized result from production, and then
checks `is_zero`. It covers 648 values: zero, one, `n-1`, `n-2`, `n/2`, all
256 powers of two and their order complements, 128 deterministic values,
`n`, `n+1`, and the all-ones input. It printed the following for every
successful run:

    ok values=648 cases=648 digest=17cf173f2d144c3b

The C++ harness
`/tmp/secp256k1-translation-78/scalar-zero-cpp-harness.cpp` has SHA-256
`1d80091c4a5a6e7e61a180ad659d1443ce6ce2740f7233d898c06baf8e3838ee`, and its
C shim `/tmp/secp256k1-translation-78/scalar-zero-c-shim.c` has SHA-256
`3012f20ccad69c2742f73851da09d044646190b0bae5c1aba16de2aaa7f033f6`. It
decodes each input into an independent Boost `cpp_int`, computes `input mod
n == 0`, and calls production through the C shim. Its 327-value set includes
zero, one, `n-1`, `n-2`, all powers of two, 64 deterministic values, `n`,
`n+1`, and all ones. Every successful run printed:

    ok cpp-values=327 cpp-cases=327 digest=c1bacbbe8c75b942

#### Compiler and project evidence

The exact C matrix used
`-DSECP256K1_BUILD`, `-std=c99`, `-Wall -Wextra`, and `-I src -I include`
with Clang 22.1.7 and GCC 16.1.0. It covered `O0`, `O2`, `O3`, and `Os` for
each of x86_64 assembly (`-DUSE_ASM_X86_64=1`), portable native C, and forced
8x32 (`-DUSE_FORCE_WIDEMUL_INT64=1`): 24 executions, all with the C digest
above. Six additional `-O2 -flto` executions (both compilers and all three
selectors) matched. Six C++ executions (both compilers and all three
selectors) matched the C++ digest.

The sanitizer matrix used `-O1 -DVERIFY -DVALGRIND
-fsanitize=address,undefined -fno-sanitize-recover=all` and ran all six
compiler/backend combinations with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`; all six matched without diagnostics.

Fresh CMake/Ninja builds were made from this HEAD with Clang, RelWithDebInfo,
tests and non-libFuzzer corpus targets enabled. Native used
`-DSECP256K1_ASM=x86_64`; forced int64 used
`-DSECP256K1_ASM=OFF -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`. In each
build `ctest --test-dir <build> --output-on-failure -R 'scalar|fuzz.scalar'`
ran seven tests and reported `100% tests passed, 0 tests failed out of 7`,
including both `scalar_tests`, malformed scalar tests, ElligatorSwift bad
scalar tests, and `fuzz.scalar.seeds`.

For optimized x86 production probes, `objdump -d --no-show-raw-insn
--disassemble=probe_is_zero` found no conditional or loop jump in any of the
six Clang/GCC O2 assembly, portable, or forced binaries. Clang used vector
OR plus `sete`; GCC used vector OR and `sete`/zero extension. This is lowering
evidence, not a formal constant-time proof.

Clang AArch64 compile-only commands used
`clang --target=aarch64-linux-gnu -DSECP256K1_BUILD [-DUSE_FORCE_WIDEMUL_INT64=1]
-std=c99 -O{0,2,3,s} [-DVERIFY -DVALGRIND] -I src -I include -c`.
All 16 native/forced, normal/VERIFY objects compiled. `aarch64-linux-gnu-objdump
-d --no-show-raw-insn --disassemble=probe_is_zero` found no branch mnemonic in
any probe; Clang lowered the O2 native probe to vector OR, `cmp`, and `cset`.
The object hashes, in native O0/O2/O3/Os normal then VERIFY order, were:

    962e24871e372256d69dd073fb2daed5a5fc959ea88c9294dabeecf788a56758
    a78ac39dfc673bb37cd31b7fcc34635a3d3ac636f96d7b1cb5e351d3c557d012
    0ca872ebccc1bc89591168b3c5b89f81efcda78502687475c6cdb1cb2b3a2933
    d1a1a0ade3eb2bd45d5a2d44b06bafc9720ce0aa4949dbf05ea304592c57dadb
    864642845833017c827003310bbc1fdf405fefe1b9a02e8cb433a05cae5dbb66
    11c0e290197a045ac0ab18aef929b8f11f3eccaf273cac4212e077f8969b2c77
    d83ffc1748180b9ff697214265057561222b682c277ce6475b9117d9aa0dad41
    ad4b54e00d333acbf1b50049b5884dcbbc25661b5b921691b07760d8132d7e60

The forced-int64 normal then VERIFY hashes were:

    40b5a77ac700a6bd5547c0a70f6d5422991b151bd1a097b2764ef807eebd89c4
    718d9824808e57fde1a0fa8be78d5468b43fe361234096206c2648a8673a0875
    ebcadf24dd3330ed22c06ca95ab09bd1ecdc42509740179ae1c37b3082087435
    d0bf502dae01e17af0a9f0860b74cdba90ce3be1f6c9bbdaeb1de644b11db697
    1b76d75f8549fac9c9e199de79c304e154375208a69bb599061f14e49cbbdc73
    9155e46c87f86a97a1e8f699350b67b7b9a91ba95dcc67ca734ce982d5cf0b6f
    4a5df99a1cead6f2b98757caf2566570c0d3fa2f5fa0fad571ec380bc0396106
    4e5a94f89f75573af181a7a452a617134b35e07eda1a7306dae4545c987ef92a

All strings above are recorded exactly as emitted by `sha256sum`; no runtime
AArch64, GCC AArch64, ARMv7/RISC-V, or formal translation validation was
available. The long hash list is retained as raw artifact identity, while the
behavioral result is the common C++/C digest.

#### Mutation controls

Scratch copies changed only the OR aggregate to an AND aggregate in
`scalar_4x64_impl.h` and `scalar_8x32_impl.h`. Clang O2 C and C++ runners for
both copies rejected the mutation at the first nonzero value:

    zero mismatch case=1 expected=0 actual=1
    cpp zero mismatch case=1 expected=0 actual=1

This proves that both independent oracles distinguish a single nonzero limb
and are not merely checking zero inputs or a passing corpus.

#### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 assembly, portable, and forced-int64 paths; O0/O2/O3/Os,
LTO, ASan/UBSan/VERIFY, fresh project tests and scalar corpus, and Clang
AArch64 code generation. The native and forced implementations agree with
both independent oracles at the reduction boundary and no lowering mismatch,
undefined behavior, or reachable production defect was found. No production
code, regression test, or fix commit is justified. Limitations are no
AArch64 runtime, GCC AArch64, ARMv7/RISC-V, or formal constant-time proof.
The next queue is another distinct scalar compiler/architecture helper; do
not repeat this predicate unless new source, compiler, architecture, or
performance evidence changes.

### Cycle 2026-07-27: scalar one predicate translation

#### Scope and hypothesis

The twenty-fourth goal-78 hypothesis targeted the direct
`secp256k1_scalar_is_one` predicate in the native 4x64 backend
(`src/scalar_4x64_impl.h:238-242`) and forced 8x32 backend
(`src/scalar_8x32_impl.h:286-290`). Each implementation XORs the lowest
limb with one, ORs the remaining limbs, and compares the aggregate with zero.
The trust boundary is a canonical scalar produced by
`secp256k1_scalar_set_b32`; raw input bytes are untrusted until conversion.
The question was whether a limb layout, order-reduction boundary, compiler,
or optimized lowering mismatch could classify a non-one scalar as one or
disagree across backends.

Existing coverage was used as a seed: the scalar fuzzer's
`scalar zero one predicates` trigger at `src/fuzz/scalar.c:1041-1060` checks
only zero, one, `n-1`, and `n` for this predicate, while the general fuzzer
checks parity but supplies no independent one oracle. History and blame
point to the constant-time predicate introduced with the scalar
implementations in `1d52a8b1`, with later relocation/verification changes in
`aa404d53`, `44015000`, `1e0e885c`, and `a0fb68a2`.

#### Independent oracle evidence

The C harness
`/tmp/secp256k1-translation-78/scalar-one-harness.c` has SHA-256
`b3b07677a16a664ca352e427ef66d46179882b7c15850e8bf18b1c1a62b22764`. It
constructs big-endian values independently, computes the expected reduction
with byte subtraction, verifies production serialization, and checks whether
the reduced bytes equal one. It covers 648 values: zero, one, `n-1`, `n-2`,
`n/2`, all 256 powers of two and their order complements, 128 deterministic
values, `n`, `n+1`, and all ones. After correcting an initial 31-byte order
constant in the new scratch harness, every accepted run printed:

    ok values=648 cases=648 digest=e0992ce3f53f682e

The C++ harness
`/tmp/secp256k1-translation-78/scalar-one-cpp-harness.cpp` has SHA-256
`d9ba265b6dd16ecb05242c4fb677beeae30522b2e7999480a0a8e348a1f2761d`, and
its C shim `/tmp/secp256k1-translation-78/scalar-one-c-shim.c` has SHA-256
`b6c1e118114378dafa85710be2826fcb77249330b0725e5b78dc71675277e610`. It
decodes each input to an independent Boost `cpp_int`, computes `input mod n
== 1`, and calls the production predicate through the shim. Its 327-value
set includes zero, one, `n-1`, `n-2`, all powers of two, 64 deterministic
values, `n`, `n+1`, and all ones. Every accepted run printed:

    ok cpp-values=327 cpp-cases=327 digest=a4d7ffa194fdd729

The initial faulty C run stopped at `case=645` with an overflow mismatch and
was discarded; the corrected C and independently correct C++ harnesses were
then rerun from scratch before any matrix result was accepted.

#### Compiler and project evidence

The exact C matrix used `-DSECP256K1_BUILD`, `-std=c99`, `-Wall -Wextra`, and
`-I src -I include` with Clang 22.1.7 and GCC 16.1.0. It covered `O0`, `O2`,
`O3`, and `Os` for x86_64 assembly (`-DUSE_ASM_X86_64=1`), portable native C,
and forced 8x32 (`-DUSE_FORCE_WIDEMUL_INT64=1`): 24 executions, all with the C
digest above. Six additional `-O2 -flto` executions, both compilers and all
three selectors, matched. Six C++ bridge executions, both compilers and all
three selectors, matched the C++ digest.

The sanitizer matrix used `-O1 -DVERIFY -DVALGRIND
-fsanitize=address,undefined -fno-sanitize-recover=all` and ran all six
compiler/backend combinations with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`; all six matched without diagnostics.

The existing fresh CMake/Ninja native and forced-int64 builds from this HEAD
were rebuilt and reran with
`ctest --test-dir <build> --output-on-failure -R 'scalar|fuzz.scalar'`.
Each configuration ran seven tests and reported `100% tests passed, 0 tests
failed out of 7`, including `scalar_tests`, malformed scalar tests,
ElligatorSwift bad scalar tests, and `fuzz.scalar.seeds`.

For optimized x86 production probes, `objdump -d --no-show-raw-insn
--disassemble=probe_is_one` found zero conditional or loop jumps in all six
Clang/GCC O2 assembly, portable, and forced binaries. Clang lowered the
native probe to a scalar XOR/OR aggregate and `sete`; GCC's forced probe
used 32-bit limb ORs and `sete`/zero extension. This is lowering evidence,
not a formal constant-time proof.

Clang AArch64 compile-only commands used
`clang --target=aarch64-linux-gnu -DSECP256K1_BUILD [-DUSE_FORCE_WIDEMUL_INT64=1]
-std=c99 -O{0,2,3,s} [-DVERIFY -DVALGRIND] -I src -I include -c`.
All 16 native/forced, normal/VERIFY objects compiled, and
`aarch64-linux-gnu-objdump -d --no-show-raw-insn
--disassemble=probe_is_one` found no branch mnemonic in any probe. The
native normal hashes in O0/O2/O3/Os order were:

    4fa0314e6a1c7b6bcbfb3ef63302e3fe4bbc57f80c047a8c10b2f13bbc34f174
    5e243d3d1d59e3301b94bf17e81d2b1c962b5130d3e27ef118825bd6a3772f26
    c27c72b840bd7bdb6754b403d29e4fd8e7666828039e45b97f562896873e1c7c
    b7d66fb96ca9dcc05135be123be72c9413e5898f00f66dccaa762cfd194dfc31

The native VERIFY hashes in the same order were:

    0cd0d0e0858bfb51c317f0090b407017578410db997d6b715089148a9e134911
    1e0cd2d16e37fcd12dcb08bac1c72bf64a61ace152df0342b5e86106d80c0a4a
    eb062284667e029d24f3295e47c328e175c6bab8b9be0768a0b80a75a4e4e258
    39625c3094d6043e01e9adb3b8128907d78480e376a088e697fce64ba3c467fe

The forced-int64 normal hashes were:

    13faad4a67d89f1e9bd064e088e6e3c9d114f762ef3134cbcfe398f957bf7fc6
    df549019f4538c4716081a00bdb62562d2875d9a150527e1ad1fe924f6d19585
    c1d592239fb4185f832657fa5edcecd75b00dc328611d0e045282a0b98a9b131
    0475c6038960346fd09f9e03c96bcc18dac3f68d7dafd147268d97ff2186fb59

The forced-int64 VERIFY hashes were:

    159fc84da09ab4e48d782a47aa9970918e5486af56c368eb6558ba4053646b63
    48127559203769cf39b0480539f0051f63917a190ad3f6ab372ff2cc75d3f850
    ef5649b7cc492cb1f19b3cf6c928f1eee1268efb81660d156528a6c0dd56a94d
    9d6115a9f629709cbf904cfb0a57060c11f81edda071aa42605ca42c998c4c02

No runtime AArch64, GCC AArch64, ARMv7/RISC-V, or formal translation
validation was available.

#### Mutation controls

Scratch copies changed only the OR aggregate to an AND aggregate in
`scalar_4x64_impl.h` and `scalar_8x32_impl.h`. Clang O2 C and C++ runners for
both copies rejected the mutation at zero:

    one mismatch case=0 expected=0 actual=1
    cpp one mismatch case=0 expected=0 actual=1

This proves that both independent oracles distinguish zero from one and are
not merely checking the positive one case or a passing corpus.

#### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 assembly, portable, and forced-int64 paths; O0/O2/O3/Os,
LTO, ASan/UBSan/VERIFY, project scalar tests and corpus, and Clang AArch64
code generation. The native and forced implementations agree with both
independent oracles at the reduction boundary and no lowering mismatch,
undefined behavior, or reachable production defect was found. No production
code, regression test, or fix commit is justified. The next queue is another
distinct scalar compiler/architecture helper; do not repeat this predicate
unless new source, compiler, architecture, or performance evidence changes.

### Cycle 2026-07-27: scalar even predicate translation

#### Scope and hypothesis

The twenty-fifth goal-78 hypothesis targeted the direct
`secp256k1_scalar_is_even` predicate in the native 4x64 backend
(`src/scalar_4x64_impl.h:1004-1008`) and forced 8x32 backend
(`src/scalar_8x32_impl.h:819-823`). Both implementations verify the scalar
and return the inverse of the low limb's least-significant bit. The trust
boundary is a canonical scalar produced by `secp256k1_scalar_set_b32`; raw
bytes are untrusted until conversion. The question was whether limb
endianness, reduction of `n`/`n+1`, compiler optimization, or backend
selection could invert parity.

Existing coverage was used as a seed: the general scalar fuzzer checks
`secp256k1_scalar_is_even(a) == ((a32[31] & 1u) == 0)` at
`src/fuzz/scalar.c:592-594`, and its boundary trigger checks half-order and
half-order-plus-one at `src/fuzz/scalar.c:1093-1099`. Those checks do not
independently reduce arbitrary raw inputs or compare backend representations.
History search found the per-implementation move in `aa404d53`, with the
constant-time scalar implementation ancestry in `44015000`.

#### Independent oracle evidence

The C harness
`/tmp/secp256k1-translation-78/scalar-even-harness.c` has SHA-256
`5a8f659038193a30d3ddac31a0e4045198379605a5b6abab3165b1fca39af7fb`. It
constructs big-endian values independently, computes the expected reduction
with byte subtraction, verifies production serialization, and derives parity
from the reduced bytes. It covers 648 values: zero, one, `n-1`, `n-2`, `n/2`,
all 256 powers of two and their order complements, 128 deterministic values,
`n`, `n+1`, and all ones. Every successful run printed:

    ok values=648 cases=648 digest=80f78c7b48a35aaf

The C++ harness
`/tmp/secp256k1-translation-78/scalar-even-cpp-harness.cpp` has SHA-256
`0e1ff69421f599ca0af238f13bf309bfaa28658808571d008d8eeca316f0d089`, and
its C shim `/tmp/secp256k1-translation-78/scalar-even-c-shim.c` has SHA-256
`10161b574d4a0b6afbe173c4da3d2fb3cc277bec0f3cb712446b4a95330fe71c`. It
decodes input with Boost `cpp_int`, computes `(input mod n) mod 2 == 0`, and
calls the production predicate through the shim. Its 327-value set includes
zero, one, `n-1`, `n-2`, all powers of two, 64 deterministic values, `n`,
`n+1`, and all ones. Every successful run printed:

    ok cpp-values=327 cpp-cases=327 digest=17d3c11fa9e857a1

The explicit modulo-before-parity step is necessary because the group order
is odd: the raw parity of `n` is odd while its reduced scalar is zero and
even.

#### Compiler and project evidence

The exact C matrix used `-DSECP256K1_BUILD`, `-std=c99`, `-Wall -Wextra`, and
`-I src -I include` with Clang 22.1.7 and GCC 16.1.0. It covered `O0`, `O2`,
`O3`, and `Os` for x86_64 assembly (`-DUSE_ASM_X86_64=1`), portable native C,
and forced 8x32 (`-DUSE_FORCE_WIDEMUL_INT64=1`): 24 executions, all with the C
digest above. Six additional `-O2 -flto` executions, both compilers and all
three selectors, matched. Six C++ bridge executions, both compilers and all
three selectors, matched the C++ digest.

The sanitizer matrix used `-O1 -DVERIFY -DVALGRIND
-fsanitize=address,undefined -fno-sanitize-recover=all` and ran all six
compiler/backend combinations with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`; all six matched without diagnostics.

The native and forced-int64 CMake/Ninja builds were rebuilt and reran with
`ctest --test-dir <build> --output-on-failure -R 'scalar|fuzz.scalar'`.
Each configuration ran seven tests and reported `100% tests passed, 0 tests
failed out of 7`, including `scalar_tests`, malformed scalar tests,
ElligatorSwift bad scalar tests, and `fuzz.scalar.seeds`.

For optimized x86 production probes, `objdump -d --no-show-raw-insn
--disassemble=probe_is_even` found zero conditional or loop jumps in all six
Clang/GCC O2 assembly, portable, and forced binaries. Clang lowered the
native probe to `not` plus `and $1`; this is lowering evidence, not a formal
constant-time proof.

Clang AArch64 compile-only commands used
`clang --target=aarch64-linux-gnu -DSECP256K1_BUILD [-DUSE_FORCE_WIDEMUL_INT64=1]
-std=c99 -O{0,2,3,s} [-DVERIFY -DVALGRIND] -I src -I include -c`.
All 16 native/forced, normal/VERIFY objects compiled, and
`aarch64-linux-gnu-objdump -d --no-show-raw-insn
--disassemble=probe_is_even` found no branch mnemonic in any probe. The
native normal hashes in O0/O2/O3/Os order were:

    0d8c9077e7b6001357ca8749d86e2e155d2ab4c00abe0a8863ac8d4555591110
    bc7461bf93d0f53deb8d7b6540b5983deecb4f47fa814ae68455243690b8dac9
    587987cde84729ccda6a26a81e0673e64f04b60facab6583c031d2f77180a311
    595809f81e7612cda7884355c69d2fd56760f933d5396c724d548c90ba7d32a4

The native VERIFY hashes in the same order were:

    33ff4bc9929a27f16836fe0fb12e70c9f91120f645222b07ffb1213947c20ff1
    9faebf21317473902ab2b40a45d0646f25ed8205a20f82f86d2d419d045925d7
    5e9867c1353c60f95f19b0ddf1be1217d02ab0c9ffdb37d39377eac5fb1e7d97
    d6e0d02ca551777c2c67692d3c44048cea5ab11273624d2c96c7b07799da6e3b

The forced-int64 normal hashes were:

    e7434f682e026ba5d2200caae05946c4b1ab4c68e03a1286c814deddebea6312
    9225d463fcb3c871253b7944ea90f3de4146f2a0ceec10f1421c2482c7fd80bd
    a5fcb2761d504d8a9904e6473a762fe3fc7edc24633a254cf4fa72829eff485d
    081cc0f447a52d3a86d72e0aeaee2c4b0f74e300729032ef46534179f85a8c07

The forced-int64 VERIFY hashes were:

    64e2360f5183a0c8c991cdc652fab790a11a37d12a99594a45843bcf8747136c
    885921532307c553dc6762e86e27f606b88955eccb5357d174c01a944ebe7fbc
    d4e298ff27c7216a4c26daa09f7b86e8afb45122de00b9dd8e7716cbdd71548f
    eab3155b1ee02ac15209328324c9c563aba36864ee1be919fcc6f7f42d1b67ce

No runtime AArch64, GCC AArch64, ARMv7/RISC-V, or formal translation
validation was available.

#### Mutation controls

Scratch copies changed only the low-limb `& 1` to `| 1` in
`scalar_4x64_impl.h` and `scalar_8x32_impl.h`. Clang O2 C and C++ runners for
both copies rejected the mutation at zero:

    even mismatch case=0 expected=1 actual=0
    cpp even mismatch case=0 expected=1 actual=0

The compiler also emitted a tautological-bitwise-compare warning for the
mutation. The behavioral failure proves that both independent oracles cover
the positive even case rather than only odd values.

#### Finding and verdict

The compiler/representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 assembly, portable, and forced-int64 paths; O0/O2/O3/Os,
LTO, ASan/UBSan/VERIFY, project scalar tests and corpus, and Clang AArch64
code generation. The native and forced implementations agree with both
independent oracles, and no parity, endianness, undefined-behavior, lowering,
or reachable production defect was found. No production code, regression test,
or fix commit is justified. The next queue is another distinct scalar
compiler/architecture helper; do not repeat this predicate unless new
source, compiler, architecture, or performance evidence changes.

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
seventeen dismissed hypotheses unless compiler, source, or architecture
evidence changes.
