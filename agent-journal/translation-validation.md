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
fifteen dismissed hypotheses unless compiler, source, or architecture
evidence changes.
