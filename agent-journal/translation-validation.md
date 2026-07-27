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
seven dismissed hypotheses unless compiler, source, or architecture evidence
changes.
