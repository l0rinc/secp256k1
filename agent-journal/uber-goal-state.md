# Uber-Goal State

## Controller

- Catalog: `agent-journal/reusable-continuous-agent-goals.md`
- Controller: `agent-journal/uber-goal.md`
- State initialized: 2026-07-27
- Repository worktree: `/tmp/secp256k1-oracles-next`
- Existing audit branch: `codex/fuzz-oracles`
- Current status: active rotating cycle goal 89 (`bitcoin-p2p-accounting`)
- First draw seed: `4179223777703642971`
- First draw: `61`
- First draw timestamp: `2026-07-27`
- Second draw seed: `3923475549`
- Second eligible slot: `77` of 98
- Second draw: `78`
- Second draw timestamp: `2026-07-27`
- Third draw seed: `1744820529`
- Third eligible slot: `49` of 97
- Third draw: `49`
- Third draw timestamp: `2026-07-28`
- Latest draw seed: `2009882816`
- Latest eligible pool: `74 77 81 82 87 89 95 97`
- Latest selected index: `0`
- Latest draw: `74`
- Latest draw timestamp: `2026-07-28T10:09:34Z`
- Current draw seed: `4015881993`
- Current eligible pool: `77 81 82 87 89 95 97`
- Current selected index: `4`
- Current draw: `89`
- Current draw timestamp: `2026-07-28T10:39:38Z`

## Selection rules

1. Draw from goals marked `pending` or `reopened`; record the random seed,
   draw, timestamp, and eligible set.
2. Work one bounded cycle on a distinct falsifiable hypothesis.
3. Mark `exhausted` only after the goal-specific evidence ledger supports that
   verdict; mark `blocked` only for a real external/resource blocker.
4. Reopen affected goals when source, callers, tools, or findings change.
5. Keep findings, dismissed candidates, hashes, fixtures, and next actions in
   the per-goal journal and summarize them here.

## Goal ledger

Goals `0` through `48` and `50` through `60`, plus `62` through `72`, `74`
through `77`, `79` through `87`, `89` through `91`, and `94` through `98`, remain `pending`; this was the eligible
set used for the latest draw after prior-cycle exclusions. Goal `61` is
`exhausted` for its bounded stateful-fuzzer cycle. Goal
`78` is `exhausted` for the completed scalar compiler/representation helper
queue, with its journal at `agent-journal/translation-validation.md`; reopen
it only for new source, caller, compiler, architecture, or specification
evidence. Goals `92` and `93` are `exhausted` for their current bounded ABI
and Linux RPC-cookie fault hypotheses; reopen them only for new ABI/platform,
caller, or partial-I/O evidence. Goal `88` is `exhausted` for its current
SQLite master-key write-failure hypothesis; reopen it for new wallet
descriptor, keypool, backup, migration, recovery, or fault-injection evidence.
Goal `73` is `exhausted` for the current socket-level zero/short-write
hypothesis; reopen it only for new transport, platform, or state-machine
evidence.
Goal `49` remains recorded as active from its earlier long-running campaign;
its cycle journal is `agent-journal/critical-history-sweep.md`. The current
rotating cycle is goal `74`; the catalog is the source of titles, slugs, and
campaign scope.

## Historical Cycle Summaries

Goal `78` tested `secp256k1_int_cmov` and
`secp256k1_memzero_explicit` under Clang/GCC `O0`, `O2`, `O3`, `Os`, and
`O2+LTO`, compared native `5x52`/`4x64` against forced-int64 `10x26`/`8x32`
field/scalar cmov paths, and checked `secp256k1_scalar_cadd_bit` with an
independent byte-level oracle. The helper and backend fuzz oracles passed
their deterministic vectors and all 31 corpus inputs in each backend
configuration (62 input replays across both configurations); short mutation
runs added coverage without artifacts. The cadd-bit harness passed 767
boundary/carry cases with the same digest across Clang/GCC, both backends,
`O0/O2/O3/Os`, `O2+LTO`, and ASan/UBSan/VERIFY. Eight optimized cadd-bit
probe bodies had no conditional or loop jumps. A subsequent
`secp256k1_scalar_cond_negate` byte-level oracle passed 769 cases with the
same digest across the same compiler/backend matrix and its eight optimized
probe bodies also had no conditional or loop jumps; the focused production
fuzzer trigger passed in both sanitized backends. The first five bounded
hypotheses were dismissed; no production finding or fix commit resulted. A
sixth hypothesis then compiled the same helper for AArch64 with native and
forced-int64 representations at `O0/O2/O3/Os`, found no conditional branch
mnemonics in the probe or O0 helper disassemblies, and passed `VERIFY`/
`VALGRIND` compile-only builds. AArch64 execution was unavailable because no
runner or sysroot was installed; ARMv7/RISC-V compilation likewise stopped at
the missing host sysroot header. A seventh hypothesis then tested
`secp256k1_scalar_add` with an independent 773-value, 597,529-pair oracle,
all native/forced-int64 Clang/GCC optimization and LTO runs, sanitized
execution, mutation controls, and AArch64 codegen; all results matched and
all seven bounded hypotheses were dismissed. No production finding or fix
commit resulted. An eighth hypothesis then tested `secp256k1_scalar_half`
with an independent 773-value parity oracle, all native/forced-int64
Clang/GCC optimization and LTO runs, sanitized execution, mutation controls,
and AArch64 codegen; all results matched and no conditional branch
mnemonics were emitted. All eight bounded hypotheses were dismissed; no
production finding or fix commit resulted. A ninth hypothesis then tested
`secp256k1_scalar_is_high` with an independent threshold oracle, all
native/forced-int64 Clang/GCC optimization and LTO runs, sanitized execution,
mutation controls, and AArch64 codegen; all results matched and no
conditional branch mnemonics were emitted. A tenth hypothesis then tested
`secp256k1_scalar_mul_shift_var` with an independent byte-level
512-bit product/rounding oracle across 645 values, 4,355 pairs, all
documented shift boundaries including 512, 513, 514, and `UINT_MAX`, native
and forced-int64 Clang/GCC `O0/O2/O3/Os`, LTO, sanitized execution, mutation
controls, and AArch64 code generation. Every execution matched the same
digest; the deliberate `> 512` to `>= 512` mutation failed at shift 512 in
both backends. All ten bounded hypotheses were dismissed; no production
finding or fix commit resulted. An eleventh hypothesis then tested
`secp256k1_scalar_split_128` with an independent serialized low/high-half
oracle across 645 canonical values, native and forced-int64 Clang/GCC
`O0/O2/O3/Os`, LTO, sanitized execution, mutation controls, and AArch64 code
generation. Every execution matched the same digest, all probe bodies had
zero conditional branches, and a deliberately wrong high-half limb failed
at `n-1` in both backends. All eleven bounded hypotheses were dismissed; no
production finding or fix commit resulted. The goal remains active with
another compiler/architecture constant-time or overflow-sensitive helper
queued. A twelfth hypothesis then tested scalar bit extraction with an
independent bit-order oracle across 645 values, 4,963,920 variable-width
cases, 2,724,480 legal same-limb cases, native and forced-int64 Clang/GCC
`O0/O2/O3/Os`, LTO, sanitized execution, mutation controls, and AArch64 code
generation. Every execution matched the same digest; the deliberately
truncated output mask failed on the first high-bit case in both backends.
All twelve bounded hypotheses were dismissed; no production finding or fix
commit resulted. The goal remains active with another compiler/architecture
constant-time or overflow-sensitive helper queued.
A thirteenth hypothesis then tested `secp256k1_scalar_split_lambda` with an
independent base-256 product/reduction oracle across 645 canonical values,
native and forced-int64 Clang/GCC `O0/O2/O3/Os`, LTO, sanitized execution,
mutation controls, and AArch64 code generation. Every execution matched the
same digest, all optimized probe bodies had zero conditional branches, and a
high-byte `g1` mutation failed at `n-1` in both backends. All thirteen bounded
hypotheses were dismissed; no production finding or fix commit resulted. The
goal remains active with another compiler/architecture constant-time or
overflow-sensitive helper queued. A fourteenth hypothesis then tested
`secp256k1_scalar_inverse` and `_var` with an independent high-level
`cpp_int` Fermat-inverse vector oracle across 645 canonical values, including
zero and in-place calls, native and forced-int64 Clang/GCC `O0/O2/O3/Os`, LTO,
sanitized execution, focused existing inverse/modinv/scalar/endomorphism
tests, mutation controls, and AArch64 code generation. Every execution
matched `digest=609caf56698b92b5`; regular-inverse branch structure remained
fixed-count while `_var` retained data-dependent divstep branches, and
forcing both inverse outputs to zero failed at the first nonzero vector in
both backends. All fourteen bounded hypotheses were dismissed; no production
finding or fix commit resulted. The goal remains active with another
compiler/architecture constant-time or overflow-sensitive helper queued.
The fifteenth hypothesis then tested field normalization across native 5x52
and forced-int64 10x26 representations with an independent prime-based oracle
covering 645 canonical values, all magnitude bounds 0 through 32, complementary
bound sums, and raised magnitude-32 cases. Native and forced-int64 Clang/GCC
`O0/O2/O3/Os`, LTO, ASan/UBSan/VERIFY execution, focused field tests, mutation
controls, and AArch64 code generation all matched
`digest=f1bcc6afb297fe94`; the deliberate `0x3D1` to `0x3D0` reduction
mutation failed at the first raw-bound case in both backends. Regular
normalization retained fixed control flow on x86, while variable-time probes
showed only their expected reduction branch; AArch64 compile-only probes used
conditional-select lowering. All fifteen bounded hypotheses were dismissed;
no production finding or fix commit resulted. The goal remains active with
another compiler/architecture constant-time or overflow-sensitive helper
queued. Scratch artifacts are under `/tmp/secp256k1-translation-78`.
The sixteenth hypothesis then tested `secp256k1_fe_half` across native 5x52
and forced-int64 10x26 representations with an independent modular-halving
oracle covering 645 canonical values, magnitudes 0 through 31, magnitude-31
carry sums, and every raised magnitude. Correctly selected native and forced
Clang/GCC `O0/O2/O3/Os`, LTO, ASan/UBSan/VERIFY execution, focused field
tests, mutation controls, and AArch64 code generation matched
`digest=19d79583eb93cd67`; the deliberate odd-prime-addend mutation failed
at the first nonzero value in both backends. The compiler hypothesis was
dismissed. The audit also confirmed that the `fe_half` header comment wrongly
promised normalized output; a minimal source-comment correction was committed
with the cycle journal and state. The goal remains active with another
compiler/architecture constant-time or overflow-sensitive helper queued.

The seventeenth hypothesis then tested `secp256k1_fe_negate` across native
5x52 and forced-int64 10x26 representations with an independent byte-level
prime-negation oracle covering 645 canonical values, all input magnitudes 1
through 31, 32 raw bound cases, aliasing cases, and 19,995 raised cases.
Correctly selected native and forced Clang/GCC `O0/O2/O3/Os`, LTO,
ASan/UBSan/VERIFY execution, focused field/fuzzer checks, mutation controls,
and Clang AArch64 code generation matched
`digest=0315022cfbcaee3d`; the deliberate field-prime-constant mutations
failed immediately in both backends. The compiler hypothesis was dismissed.
The goal remains active with another compiler/architecture constant-time or
overflow-sensitive helper queued.

The eighteenth hypothesis then tested `secp256k1_scalar_mul` with two
independent modular-product oracles: a base-2^16 C reducer covering 645
values and 7,474 aliased pairs (`digest=739447fca5d13916`), and a Boost
`cpp_int` verifier covering 581 values and 5,810 aliased pairs
(`digest=79f3943e75212f57`). Native assembly, native portable C, and
forced-int64 Clang/GCC optimization/LTO matrices, ASan/UBSan/VERIFY runs,
focused tests, scalar corpus replays, mutation controls, and Clang AArch64
compile-only code generation all matched; the deliberate scalar-order
constant mutation failed at case 22 in every backend. The correctness and
compiler hypothesis was dismissed. The cycle also reproduced the open
Clang assembly performance observation from issue #1682: local Release
`scalar_mul` averaged 0.0411 us with Clang x86_64 assembly versus 0.0336 us
without it, while GCC measured 0.0376 versus 0.0412 us. This is an open
performance lead for goal 70, not a proven compiler defect or automatic
production fix. Goal 78 remains active with another distinct helper queued.

The nineteenth hypothesis then tested `secp256k1_scalar_negate` with an
independent byte-subtraction oracle covering 645 canonical values, zero,
order-adjacent values, powers/complements, double negation, and aliasing
(`digest=3a235f391877b00c`), plus a Boost `cpp_int` verifier covering 581
values and aliases (`digest=820b82c9f494768e`). Native assembly, native
portable C, and forced-int64 Clang/GCC optimization/LTO matrices,
ASan/UBSan/VERIFY runs, focused tests, scalar corpus replays, mutation
controls, and Clang AArch64 compile-only code generation all matched. The
deliberate `N_0 + 1` to `N_0 + 2` mutation failed immediately at the first
nonzero value in both backends and both verifiers. The compiler hypothesis
was dismissed; no production change was made. Goal 78 remains active with
another distinct helper queued.

The twentieth hypothesis then tested `secp256k1_scalar_set_b32` with an
independent byte compare/subtract oracle covering 395 raw values, including
`n-1`, `n`, `n+1`, maximum 256-bit inputs, powers of two, half-order
boundaries, full-width random values, both overflow-pointer forms, and
`digest=98dd70c0a3477278`, plus a Boost `cpp_int` verifier with the same
digest. Native assembly, native portable C, and forced-int64 Clang/GCC
optimization/LTO matrices, ASan/UBSan/VERIFY runs, focused tests, scalar
corpus replays, mutation controls, and Clang AArch64 compile-only code
generation all matched. Production disassembly showed only the intentional
NULL check for the optional overflow output pointer; input comparisons used
conditional selects. The deliberate reduction-constant mutation failed at
the exact `n` case in both backends and both verifiers. The compiler
hypothesis was dismissed; no production change was made. Goal 78 remains
active with another distinct helper queued.

The twenty-first hypothesis then tested `secp256k1_scalar_set_b32_seckey`
with an independent `0 < input < n` validity oracle and canonical-output
checks across 395 raw boundary/power/random values
(`digest=c178fe0c22775966`), plus a Boost `cpp_int` verifier with the same
digest. Native assembly, native portable C, and forced-int64 Clang/GCC
optimization/LTO matrices, ASan/UBSan/VERIFY runs, focused tests, scalar
corpus replays, mutation controls, and Clang AArch64 compile-only code
generation all matched. The deliberate validity operator mutation from `&`
to `|` failed at zero in both backends and both verifiers. The compiler
hypothesis was dismissed; no production change was made. Goal 78 remains
active with another distinct helper queued.

The twenty-second hypothesis then tested `secp256k1_scalar_eq` with an
independent serialized-byte oracle covering 645 values and 6,450 symmetric
pair cases (`digest=36554ee1f215b27b`), plus a C++ verifier covering 581 and
5,810 pairs (`digest=022065fdd7629ffb`). Native assembly, native portable C,
and forced-int64 Clang/GCC optimization/LTO matrices, ASan/UBSan/VERIFY
runs, focused tests, scalar corpus replays, mutation controls, and Clang
AArch64 compile-only code generation all matched. The deliberate first OR to
AND mutation failed at the first unequal pair in both backends and both
verifiers. The compiler hypothesis was dismissed; no production change was
made. Goal 78 remains active with another distinct helper queued.

The twenty-third hypothesis then tested `secp256k1_scalar_is_zero` with an
independent byte-level reduction oracle covering 648 values, including `n`,
`n+1`, all ones, every power of two, order complements, and deterministic
values (`digest=17cf173f2d144c3b`), plus a Boost `cpp_int` modulo oracle
covering 327 values (`digest=c1bacbbe8c75b942`). Native assembly, portable C,
and forced-int64 Clang/GCC optimization/LTO matrices, six ASan/UBSan/VERIFY
runs, fresh native and forced CMake scalar tests/corpus runs, and Clang
AArch64 compile-only normal/VERIFY code generation all matched. The direct
probe had no conditional or loop branches in the tested optimized x86 and
AArch64 objects. The deliberate OR-to-AND mutation failed at the first
nonzero input in both backends and both verifiers. The compiler/representation
hypothesis was dismissed; no production change was made. Limitations are no
AArch64 runtime, GCC AArch64, ARMv7/RISC-V, or formal constant-time proof.
Goal 78 remains active with another distinct scalar helper queued.

The twenty-fourth hypothesis then tested `secp256k1_scalar_is_one` with an
independent byte-reduction oracle covering 648 values, including `n`, `n+1`,
all ones, every power of two, order complements, and deterministic values
(`digest=e0992ce3f53f682e`), plus a Boost `cpp_int` modulo oracle covering 327
values (`digest=a4d7ffa194fdd729`). A newly found 31-byte scratch order
constant was corrected before accepted results. Native assembly, portable C,
and forced-int64 Clang/GCC optimization/LTO matrices, six ASan/UBSan/VERIFY
runs, native and forced CMake scalar tests/corpus runs, and Clang AArch64
compile-only normal/VERIFY code generation all matched. The direct probe had
no conditional or loop branches in the tested optimized x86 and AArch64
objects. The deliberate OR-to-AND mutation failed at zero in both backends
and both verifiers. The compiler/representation hypothesis was dismissed; no
production change was made. Limitations are no AArch64 runtime, GCC AArch64,
ARMv7/RISC-V, or formal constant-time proof. Goal 78 remains active with
another distinct scalar helper queued.

The twenty-fifth hypothesis then tested `secp256k1_scalar_is_even` with an
independent byte-reduction oracle covering 648 values, including `n`, `n+1`,
all ones, every power of two, order complements, and deterministic values
(`digest=80f78c7b48a35aaf`), plus a Boost `cpp_int` modulo/parity oracle
covering 327 values (`digest=17d3c11fa9e857a1`). Native assembly, portable C,
and forced-int64 Clang/GCC optimization/LTO matrices, six ASan/UBSan/VERIFY
runs, native and forced CMake scalar tests/corpus runs, and Clang AArch64
compile-only normal/VERIFY code generation all matched. The direct probe had
no conditional or loop branches in the tested optimized x86 and AArch64
objects. The deliberate `& 1` to `| 1` mutation failed at zero in both
backends and both verifiers. The compiler/representation hypothesis was
dismissed; no production change was made. Limitations are no AArch64 runtime,
GCC AArch64, ARMv7/RISC-V, or formal constant-time proof. Goal 78 remains
active with another distinct scalar helper queued.

The twenty-sixth hypothesis then tested `secp256k1_scalar_cmov` with an
independent full-output byte oracle covering 648 values, 6,480 pairs, both
flags, and aliased destinations (`digest=7d8cb7e35cecbc29`), plus a Boost
`cpp_int` verifier covering 327 values and 3,270 pairs
(`digest=c1bb10e893ff03ef`). Native assembly, portable C, and forced-int64
Clang/GCC optimization/LTO matrices, six ASan/UBSan/VERIFY runs, native and
forced CMake scalar tests/corpus runs, and Clang AArch64 compile-only
normal/VERIFY code generation all matched. The direct probe had no
conditional or loop branches in the tested optimized x86 and AArch64 objects.
The deliberate mask-complement mutation failed at pair 2, flag 0, in both
backends and both verifiers. The compiler/representation hypothesis was
dismissed; no production change was made. Limitations are no AArch64 runtime,
GCC AArch64, ARMv7/RISC-V, or formal constant-time proof. Goal 78 remains
active with another distinct scalar helper queued.

The twenty-seventh hypothesis then tested `secp256k1_scalar_check_overflow`
with a direct raw-limb C oracle covering 648 values
(`digest=1c44e430fc548543`) and a separate Boost `cpp_int` verifier covering
326 values (`digest=40c92813f144e2f5`). Native assembly, portable C, and
forced-int64 Clang/GCC O0/O2/O3/Os matrices, six LTO runs, six C++ bridge
runs, six ASan/UBSan/VERIFY runs, native and forced CMake scalar/corpus runs,
and Clang AArch64 compile-only normal/VERIFY code generation all matched.
The optimized probes had no conditional or loop branches in the tested x86
or AArch64 objects. Changing the inclusive final `>= n` comparison to `> n`
failed at the exact order value in both C and C++ mutation controls. The
compiler/representation hypothesis was dismissed; no production change was
made. Limitations are no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
or formal constant-time proof. Goal 78 remains active with another distinct
scalar compiler/architecture helper queued.

The twenty-eighth hypothesis then tested `secp256k1_scalar_reduce` with a
direct raw-limb C oracle covering 776 cases
(`digest=11ef97f81a04e006`) and an independent Boost `cpp_int` verifier
covering 392 cases (`digest=d36b339d72b8e407`). Native assembly, portable C,
and forced-int64 Clang/GCC O0/O2/O3/Os matrices, six LTO runs, six C++ bridge
runs, six ASan/UBSan/VERIFY runs, native and forced CMake scalar/corpus runs,
and Clang AArch64 compile-only normal/VERIFY code generation all matched.
Normal optimized x86 and AArch64 probes had no conditional or loop branches;
the optimized AArch64 VERIFY probes had only the expected invariant-check
branches. Changing the first negative-order constant by one failed at the
exact `n` input in both C and C++ mutation controls. The
compiler/representation hypothesis was dismissed; no production change was
made. Limitations are no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
or formal constant-time proof. Goal 78 remains active with another distinct
scalar compiler/architecture helper queued.

The twenty-ninth hypothesis then tested `secp256k1_scalar_get_b32` with a
direct raw-limb C serialization oracle covering 646 canonical values
(`digest=4ea96c3b19984f71`) and an independent Boost `cpp_int` verifier
covering 392 values (`digest=a57e37bca6bc2e1a`). Native assembly, portable C,
and forced-int64 Clang/GCC O0/O2/O3/Os matrices, six LTO runs, six C++ bridge
runs, six ASan/UBSan/VERIFY runs, native and forced CMake scalar/corpus runs,
and Clang AArch64 compile-only normal/VERIFY code generation all matched.
Normal optimized x86 and AArch64 probes had no conditional or loop branches;
the optimized AArch64 VERIFY probes had only the expected scalar invariant
check. Replacing the highest serialized limb with the next lower limb failed
in both C and C++ mutation controls for both representations. The
compiler/representation hypothesis was dismissed; no production change was
made. Limitations are no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
or formal constant-time proof. Goal 78 remains active with another distinct
scalar or cross-backend compiler/architecture helper queued.

The thirtieth hypothesis then tested `secp256k1_scalar_mul_512` across the
native 4x64 and forced 8x32 representations with independent C and C++ raw
512-bit-product oracles. The C schedule covered 646 values and 1,612 pairs
(`digest=2c24eb068441449f`); the C++ schedule covered 646 values and 1,612
pairs (`digest=6f3d46054b0655a0`). Clang/GCC O0/O2/O3/Os and LTO matrices,
Clang++/G++ bridge runs, six ASan/UBSan/VERIFY runs, native and forced CMake
scalar/corpus tests, and 16 Clang AArch64 compile-only objects all matched.
Normal no-inline helper objects had no conditional or loop branches; VERIFY
objects retained diagnostic branches. Mutating one product term failed at
pair 2 in both C and C++ controls for both representations. The
compiler/representation hypothesis was dismissed; no production change was
made. Limitations are no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
or formal constant-time proof. Goal 78 remains active with another distinct
scalar or cross-backend compiler/architecture helper queued.

The thirty-first hypothesis was selected by draw seed `538901017` from the
eligible helper queue at index 1: `scalar_set_int`. An independent C oracle
covered 69,708 unsigned values, including all 65,536 low-word values, full
32-bit boundaries, powers/complements, and 4,096 deterministic values; a
separate Boost C++ bridge used the same schedule. All 24 C O0/O2/O3/Os runs,
six C++ bridge runs, six LTO runs, six ASan/UBSan/VERIFY runs, native and
forced CMake scalar/corpus selections, and 16 Clang AArch64 compile-only
objects matched `digest=5fb02c6d07de71fe`. Isolated normal helper symbols
had no conditional or loop branches. Low-limb and upper-limb-zero mutations
failed at value 0 in both C and C++ controls for both representations. The
compiler/representation hypothesis was dismissed; no production change was
made. Limitations are no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
formal translation proof, or unusual non-32-bit unsigned-int ABI. Goal 78
remains active with another distinct uncovered scalar helper queued.

The thirty-second hypothesis was selected by draw seed `913401491` from the
remaining helper queue at index 3: `scalar_clear`. This wrapper-level cycle
was distinct from the earlier generic memzero dead-store check. Guarded C and
independent C++ oracles covered 4,102 secret patterns and all 32 scalar bytes
plus canaries, agreeing on `digest=46d5db336e4be564`. All 24 C O0/O2/O3/Os
runs, six C++ bridge runs, six LTO runs, six ASan/UBSan/VERIFY runs, native
and forced CMake scalar/corpus selections, and 16 Clang AArch64 compile-only
objects matched. The wrapper had only an unconditional transfer and no
conditional or loop branches. A `sizeof(scalar)-1` mutation failed in both C
and C++ controls for both representations. The compiler/representation
hypothesis was dismissed; no production change was made. Limitations are no
AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2, formal erasure proof, or
unusual scalar ABI. Goal 78 remains active with another distinct uncovered
scalar helper queued.

The thirty-third hypothesis was selected by draw seed `1493336985` from the
eligible queue `scalar_reduce_512 scalar_from_signed scalar_to_signed` at index
0: `scalar_reduce_512`. A direct byte-wise C reduction oracle and an
independent Boost `cpp_int` modulo oracle covered 2,566 full-width 512-bit
inputs with matching `digest=d2748d46a2df4b00`. All 24 C compiler/backend/
optimization runs, six C++ bridge runs, six LTO runs, six ASan/UBSan/VERIFY
runs, native and forced CMake scalar/corpus selections, and 16 Clang AArch64
compile-only objects passed. Isolated x86 and AArch64 reducer symbols had no
conditional or loop branches. Changing the first portable reduction constant
in isolated native and forced source copies caused both independent oracles to
fail at value 5. The translation hypothesis was dismissed; no production
change was made. Limitations remain no AArch64 runtime, GCC AArch64,
ARMv7/RISC-V, Alive2, formal translation proof, or unusual scalar ABI. Goal
78 remains active with `scalar_from_signed scalar_to_signed` queued.

The thirty-fourth hypothesis was selected by draw seed `1475233831` from the
remaining queue `scalar_from_signed scalar_to_signed` at index 1:
`scalar_to_signed`. Independent bit-by-bit C and Boost `cpp_int` oracles
covered 67,075 canonical scalars. Native 4x64 matched
`8bc3373654bfbae5`; forced 8x32 matched `34b50c7803289a6f`. All 24 C
compiler/backend/optimization runs, six C++ bridge runs, six LTO runs, six
ASan/UBSan/VERIFY runs, native and forced CMake scalar/corpus selections, and
16 Clang AArch64 compile-only objects passed. Isolated x86 and AArch64
conversion symbols had no conditional or loop branches. Mutating the first
62-bit or 30-bit boundary shift caused both independent oracles to fail at
value 65,536. The hypothesis was dismissed with no production change. The
goal-78 scalar conversion subqueue is exhausted for the tested contract;
limitations remain no AArch64 runtime, GCC AArch64, ARMv7/RISC-V, Alive2,
formal translation proof, or unusual scalar ABI. The controller should draw a
new catalog goal.

The thirty-fifth cycle selected catalog goal `49`,
`critical-history-sweep`, by seed `1744820529` at eligible slot 49 of 97.
Goal 78's scalar compiler/representation queue was marked exhausted for its
tested contract. The new goal journal is
`agent-journal/critical-history-sweep.md`; its first bounded history slice
must record a distinct seed, reachable trust boundary, severity gate,
independent verification, and exact next queue.

The thirty-sixth cycle tested historical seed `08d7d892` (subgroup checks in
the public parsers). The current Silent Payments recipient-label parser still
omits the check after its internal compressed-point parse, and an order-7
scratch build accepted finite non-subgroup points with x=7 and x=8 while the
public parser and label serializer rejected them. An independent Python
big-integer affine calculation confirmed both points are on the order-7 test
curve but `[7]P` is non-infinity. The normal native and forced-int64
Silent Payments unit slices, 14-input ASan/UBSan fuzz corpus, and order-7
exhaustive control passed. The production subgroup predicate is always true
on secp256k1's cofactor-one curve, the exhaustive test runner does not
register Silent Payments, and the surveyed Bitcoin Core checkout has no
Silent Payments caller. The candidate was dismissed as a production defect
and retained as test-mode hardening only; no source change was made. Full
evidence and the scratch command are in
`agent-journal/critical-history-sweep.md`.

The thirty-seventh cycle continued catalog goal `49`, `critical-history-sweep`, using draw seed `2265895816`. It selected historical seed `3a403639dc07e39aa6dc48fbcecfe3cb77f09770`, which replaced a zero generator scalar with `NULL` in `ecmult` to skip unnecessary WNAF setup. Current `src/eckey_impl.h` already contains the change, and `src/ecmult_impl.h` explicitly skips the generator path for NULL while zero WNAF produces no digits. An independent direct-source probe compared both forms over 90 point/scalar pairs, including zero and boundary values, and reported `ok pairs=90`. Native and forced-int64 `point_times_order` and `ecmult` slices all passed; the ecmult slices skipped only their documented high-iteration constant-table checks. The candidate was dismissed as a current defect with no source change. Full commands, output, and next history slice are in `agent-journal/critical-history-sweep.md`.

The thirty-eighth controller cycle continued catalog goal `49` with draw seed `3611919104` and selected historical seed `8479eafa5720421d4b7f4b524a35e0a7edf291c7`, the MuSig counter-nonce secret cleanup fix. The prior-finding ledger at `src/fuzz/README.md:4095-4131` is an exact duplicate: it covers the same `keypair_load` failure, the same `nonce_gen_counter` path, and the same zeroized-output contract, with current fuzzer helpers for invalid caches and both partial keypair halves. Current `session_impl.h` already clears the derived secret after every reachable internal return. The candidate was deduplicated and dismissed without new source changes; the next draw must use `c6306238`, `d7125e51`, `89a54b5a`, or another unindexed critical history seed. Full evidence is in `agent-journal/critical-history-sweep.md`.

The thirty-ninth controller cycle continued catalog goal `49` with draw seed `198155241` and selected merge seed `c63062380f9610084409ac445af723a057a90f6b`. The commit only adds and registers an exhaustive ECDH test for commutativity and an independent hash calculation; it changes no production code or API contract. The current tree already contains the helper and registration, so the seed was excluded as non-defect/test-only history without a reproduction or source change. The next draw must prefer `d7125e51`, `89a54b5a`, or a higher-risk unindexed historical fix. Full evidence is in `agent-journal/critical-history-sweep.md`.

The fortieth controller cycle continued catalog goal `49` with draw seed `3691603698` and selected `d7125e517d45507df4a3f19c8ca90393a8290480`. The commit only changes a MuSig test from evaluating and discarding `secp256k1_ge_is_infinity` to asserting it; current production code is unchanged and the corrected assertion is already present. The seed was excluded as test-only maintenance. The next draw must widen beyond `c6306238`, `d7125e51`, and the already indexed `89a54b5a` invariant to older production-impact history. Full evidence is in `agent-journal/critical-history-sweep.md`.

The forty-first controller cycle continued catalog goal `49` with draw seed `4201848889`. It selected historical `9be7b0f08340a063d961547b5d2663405f3fc162` from a widened seven-entry pool. The historical parent reproduced the exact DER `sig + SIZE_MAX` pointer-overflow diagnostic under Clang `undefined,pointer-overflow`; the current offset parser returned `ret=0 zero=1`. Both native and forced-int64 DER test slices and both current API-roundtrip fuzzer corpus replays passed. The seed is an exact duplicate of the already recorded `cd8c9f17` direct-API Low parser-robustness finding at `src/fuzz/README.md:982-989` and `31729-31732`, so it was deduplicated and dismissed without source changes. The next draw must exclude the full DER-length family unless a new caller changes its trust boundary, and should prioritize `2277af5f`, `45f37b65`, `248f0466`, or another unindexed production-impact history seed. Full evidence is in `agent-journal/critical-history-sweep.md`.

The forty-second controller cycle continued catalog goal `49` with draw seed `2365800521` and selected `248f0466112c96b9851c662fa829f20d28d16344`. The historical parent’s removed `secp256k1_wnaf_const` reports a MemorySanitizer uninitialized-use warning for the invalid internal `size=0` domain; current history removed that helper in `115fdc72`, and current fixed-WNAF tests/oracles have no corresponding caller. Native, forced-int64, and current MSan ecmult-const corpus controls passed. The seed was excluded as obsolete historical hardening without source changes. The next draw must move beyond the removed WNAF family and the already ledgered RFC6979/large-count mechanisms toward older unindexed production-impact history. Full evidence is in `agent-journal/critical-history-sweep.md`.

## Handoff

The active cycle must verify the worktree and remotes, read the selected goal
journal and prior evidence, perform a bounded experiment, and record a verdict
before drawing another pending or reopened goal.

The forty-third controller cycle continued catalog goal `49`,
`critical-history-sweep`, with draw seed `3614493335` over the ordered pool
`adec5a16 ad52495d b0be6aba 603c33bc a5759c57 f4edfc75`, selecting
`f4edfc758142d6e100ca5d086126bf532b8a7020`, the 2020 public NULL-argument
annotation migration. Current implementation and internal tests define both
context destroy NULL calls as no-ops, while the public headers still mark
both parameters nonnull. An external C probe failed under Clang
`-Werror=nonnull` before the change, while the same calls ran successfully
against the normal library; internal `SECP256K1_BUILD` compilation also
passed. The minimal fix removed both contradictory attributes and documented
the no-op behavior. Afterward, strict public probes passed under both Clang
and GCC, both runtimes exited 0, the forced-int64 tests target rebuilt, and
`all_proper_context_tests --iterations=2 --seed=3614493335` exited 0. The
finding is confirmed Low-severity public API/toolchain contract drift, with
no runtime implementation change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude this
NULL-annotation family and continue from `ad52495d`, `b0be6aba`, `603c33bc`,
or `a5759c57` after duplicate search.

The forty-fourth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `2504559595` over the ordered
four-entry history pool `ad52495d b0be6aba 603c33bc a5759c57`, selecting
`a5759c572ed4948c660a06430b074bbc913fafc6` (`Check return value of malloc`).
The 2014 parent was built in a disposable worktree and a malloc-always-NULL
probe reproduced a startup segmentation fault (`139`); the selected fix
converted the same probe to its checked `ret != NULL` abort (`134`). Current
production core has no raw allocation at those sites: the only remaining
raw `malloc` in the filtered core inventory is the implementation inside
`src/util.h`'s checked allocator, with scratch and context callers handling
its result or using the default aborting callback. The current context corpus
(12 files, 13 runs, `cov: 3074 ft: 4960`), ecmult-multi allocation corpus (29
files, 30 runs, `cov: 3757 ft: 10784`), and forced-int64 context tests all
exited 0. The seed was dismissed as obsolete historical hardening with no
production change; full evidence is in
`agent-journal/critical-history-sweep.md`. The next draw must exclude this
malloc family and select from `ad52495d`, `b0be6aba`, or `603c33bc` after a
fresh duplicate/current-caller search.

The forty-fifth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `4254972856` over the ordered
three-entry pool `ad52495d`, `b0be6aba`, `603c33bc`, selecting
`b0be6aba910392e06aa85a87d2240a1aadb2fff5`, the 2013 secret-key validation
logic inversion. The historical parent and selected fix were built in
disposable worktrees and an exact zero/one/order/maximum probe printed
`1/0/1/1` before the fix and `0/1/0/0` after it. Current
`secp256k1_ec_seckey_verify` delegates to the scalar validity predicate and a
direct current public probe printed `zero=0`, `one=1`, `order=0`,
`maximum=0`. Native and forced-int64 ASan/UBSan API-roundtrip corpus replays
each completed 63 runs with exit `0` (`cov: 4253 ft: 9535` and
`cov: 6227 ft: 15743`), and the focused context test passed. Bitcoin Core's
current callers were checked in `src/key.cpp` and the signing/raw-transaction
RPCs without modifying its dirty checkout. The historical inversion is a
real key-integrity/availability defect, but current history and current
oracles already contain the repair; it is dismissed as obsolete hardening
with no source change. Full evidence is in
`agent-journal/critical-history-sweep.md`. The next draw must exclude this
secret-key validation family and choose from `ad52495d` or `603c33bc` after a
fresh duplicate/current-caller search.

The forty-sixth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `830138877` over the ordered
two-entry pool `ad52495d`, `603c33bc`, selecting
`603c33bc8079f7e1a4851dbef629a2b91e13bbef`, the 2014 short-signing-buffer
return fix. A historical parent/fix probe using the old raw-byte signing API
returned `small_ret=1`/`small_ret=0` respectively for a 10-byte buffer, while
both returned success for the 72-byte path. Current history later replaced
that API with opaque signatures; current DER serialization returns failure,
reports the required length, and zeroes the requested short buffer. The
current direct probe printed `sign_ret=1 small_ret=0 small_len=70
small_prefix=00`, and native/forced-int64 ECDSA tests plus focused sanitized
API-fuzzer inputs exited 0. Bitcoin Core uses the modern API and a fixed
72-byte DER capacity (`src/key.cpp:215-231`, `src/pubkey.h:39`), so its
ignored serializer return cannot reach the short-buffer branch under the
documented bound. The seed is dismissed as obsolete historical API hardening
with no source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the immediate two-entry pool is
exhausted and the next draw must widen to a new unindexed production-impact
history pool.

The forty-seventh controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `2796178318` over the ordered
four-entry pool `354ffa33`, `d907ebc0`, `bbe67d8b`, `bb5aa4df`, selecting
`bbe67d8b29f31b140d6987d82912f48539c8bcb7`, the historical infinity
public-key serialization fix. The parent/fix probe on the group-order
secret printed `ret=1 publen=65 prefix=04 changed=65` before and
`ret=0 publen=65 prefix=a5 changed=0` after. Current opaque key creation
returned `ret=0 changed=64 all_zero=1`; native/forced-int64 edge-case
tests and the focused sanitized invalid-seckey fuzzer input exited 0.
Current Bitcoin Core callers were checked without modifying its dirty
checkout. The seed is dismissed as obsolete historical hardening with no
source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude this
infinity-serialization family and select a fresh unindexed production-impact
history seed after duplicate search.

The forty-eighth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `3643183100` over the ordered
three-entry pool `354ffa33`, `d907ebc0`, `bb5aa4df`, selecting
`bb5aa4df557c5abfabf25c72144a1a071c69aa83`, the historical tweak
failure-output consistency fix. Parent/fix probes showed private add/mul and
overflowing public add leaving valid bytes unchanged before the fix, while
the selected fix zeroized all failed outputs. Current native/forced-int64
probes, edge-case tests, and focused API/group/x-only sanitizer corpus inputs
all exited 0. Bitcoin Core's reachable BIP32 private/public derivation paths
check failures and clear/discard failed child state; its checkout remained
dirty and untouched. The seed is dismissed as obsolete historical hardening
with no source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude this
tweak-output family and select `354ffa33`, `d907ebc0`, or a fresh unindexed
production-impact history seed after duplicate search.

The fifty-first controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `1785204375023205436` over a
fresh one-entry pool containing `e82144ed`, selecting the historical
`Fixup skew before global Z fixup` commit. Parent and fix ECDH-enabled builds
passed their historical test suites; a deterministic 10,000-sample arbitrary
peer `2G` callback probe found no parent/fix mismatch. Current native and
forced-int64 targeted tests, all current ecmult-const and ECDH corpus inputs,
and the independent affine/generic multiplication oracles exited `0`. Core's
current transport path uses ElligatorSwift ECDH rather than the optional
public ECDH module. The seed is dismissed as obsolete hardening with no
current finding or source change; the negative historical differential is
recorded as a limitation. The next draw must exclude the skew/global-Z and
already-covered ecmult-const family and widen to a fresh unindexed
production-impact history seed.

The forty-ninth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `2987336874` over the ordered
two-entry pool `354ffa33`, `d907ebc0`, selecting
`354ffa33e6b0d6c1270a6d9d228f692b70ad7ff4`, the historical oversized-secret
public-key creation fix. The legacy parent accepted exact group-order plus
one with `ret=1 len=65` and emitted the same public key as secret `1`; the
fix returned `ret=0 len=0` and preserved the prefilled output. Current
native/forced-int64 probes rejected the same secret and zeroed opaque output,
current edge-case tests and focused invalid-secret/keypair fuzz inputs exited
0, and Bitcoin Core validates CKey storage and checks creation failures. Its
dirty checkout remained untouched. The seed is dismissed as obsolete
historical hardening with no source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude this
oversized-secret/public-key creation family and select `d907ebc0` or a fresh
unindexed production-impact history seed after duplicate search.

The fiftieth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `1785203785` over the remaining
one-entry pool `d907ebc0`, selecting
`d907ebc0e386ea17a96d34cd3008be9207b6f94f`, the historical field-setter
bounds fix. The true parent `bb2cd94e` accepted a compressed `x = p + 1`
wire coordinate and decompressed it to the same point as valid `x = 1`; the
fix rejected it. Current native/forced-int64 direct probes, field-boundary
tests, and SEC1 parser corpus inputs rejected `p + 1` and exited 0. Existing
README mutation evidence already covers this modular-aliasing contract, and
Bitcoin Core parses public keys before verification, decompression, derivation,
and MuSig use. Its dirty checkout remained untouched. The seed is dismissed
as already-covered historical hardening with no source change. Full evidence
is in `agent-journal/critical-history-sweep.md`; the next draw must exclude
the field-setter, `p + 1`, modular-aliasing, and SEC1 parser family and choose
a fresh unindexed production-impact history seed after duplicate search.

The fifty-second controller cycle continued catalog goal `49`,
`critical-history-sweep`, with random seed `1785205910014061620` over a
one-entry eligible pool after semantic deduplication. It selected historical
`765ef53335a3e0fafdafe1e757f6fe0789f2797f`, the Jacobian point-multiplication
cleanup fix. Parent and fix builds, normal and no-VERIFY tests, and Valgrind
ctime tests all passed. Current native and forced-int64 ECDH/Schnorr/MuSig
tests and all 9 ECDH, 18 Schnorr, and 81 MuSig corpus files also passed. The
parent's four uncleared Jacobian temporaries were a credible historical secret
residue weakness, but current source has the original clears and the later
`_ecmult_gen_ge` helper (`a3296d5e`) that centralizes generator conversion and
cleanup. Bitcoin Core directly uses current Schnorr and MuSig paths but has no
direct ECDH caller. The seed is dismissed as obsolete hardening with no
production source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude the
Jacobian-clear/helper and secret-lifetime family and choose a fresh unindexed
production-impact history seed.

The fifty-third controller cycle continued catalog goal `49`,
`critical-history-sweep`, with fresh draw seed `14158664958069679963` over a
one-entry eligible pool after screening obsolete API, removed ecmult, and old
restrict-typo candidates. It selected `7506e064d791e529d2e57bb52c156deb33b897ef`,
the historical Strauss scratch-allocation NULL-arithmetic fix. Parent and fix
worktrees configured and built successfully; deterministic count-zero tests
and Valgrind ctime tests passed on both, while the broad randomized legacy DER
test was an unrelated failure on both revisions. A dedicated Clang
undefined/pointer-overflow probe reproduced the parent's `applying non-zero
offset 704 to null pointer` at `src/ecmult_impl.h:599`; the fixed revision
returned failure with the scratch cursor unchanged. Current native and
forced-int64 `ecmult_multi_tests` plus eight focused allocation/checkpoint/
callback corpus inputs per backend all passed. Current source separately
allocates and checks the Strauss temporaries, and Bitcoin Core's MuSig path
does not expose scratch sizing to wire data. The seed is dismissed as repaired
historical hardening with no production source change. Full evidence is in
`agent-journal/critical-history-sweep.md`; the next draw must exclude
`7506e064`, the removed `pre_a_lam` family, and generic scratch-boundary
duplicates, then widen to a distinct unindexed production-impact history seed.

The fifty-fourth controller cycle continued catalog goal `49`,
`critical-history-sweep`, with draw seed `10837087523217713569`. After
semantic screening of VERIFY-only inverse bounds, infinity/context API,
exhaustive recovery-test, and already-oracled candidates, the two-entry pool
was `2241ae6d14df187e2c8d6fe5b44e3d850474af38` and
`a39c2b09de304b8f24716b59219ae37c2538c242`; index 1 selected the latter.
The historical parent leaves constant-time field CMOV destinations and
failed-signing scalars uninitialized. Parent/fix builds, deterministic tests,
and Valgrind passed; Clang Static Analyzer reports the parent scalar garbage
read and the preprocessed source proves the parent field CMOV reads an
uninitialized `tmpa` because `VERIFY_SETUP` is empty in normal library builds.
Current native/forced-int64 ecmult-constant and ECDH tests, all 11 and 9
matching corpus inputs per backend, and four current ECDSA failure/signing
fixtures per backend passed. Core uses validated ECDSA signing and EllSwift
XDH, not the old optional ECDH entry point. The historical defect is confirmed
in its parent but dismissed as repaired current hardening; no source change is
made. Full evidence is in `agent-journal/critical-history-sweep.md`. The next
draw must exclude `a39c2b09`, the CMOV/uninitialized family, and the semantic
callback-cleanup/constant-multiplication oracles, then choose a distinct
unindexed production-impact history seed.
The fifty-fifth controller cycle continued catalog goal 49,
critical-history-sweep, with draw seed 9687478665518611701. After
semantic deduplication, the eligible pool contained only
2241ae6d14df187e2c8d6fe5b44e3d850474af38, the historical
ecmult_const secret-dependent sign-branch fix, so it was selected. Parent
and fixed disposable worktrees both built with ECDH and passed ./tests 0.
GCC production assembly independently showed a parent jg on the signed
WNAF digit in the -O2 multiplier and fixed cmovs/arithmetic; a small GCC
and Clang expression probe showed that compiler context matters. Current Core
uses the newer branchless x-only multiplier through EllSwift XDH for BIP324,
and current native/forced-int64 ecmult/EllSwift/ECDH controls passed. The
historical standalone-library timing defect is confirmed in its parent but is
repaired on current master; no production source change was made. Full
evidence is in agent-journal/critical-history-sweep.md. The next draw must
exclude this constant-time branch family and the already indexed current
constant-multiplication oracles, then widen to a fresh unindexed
production-impact history seed.

The fifty-sixth controller cycle selected catalog goal `92`,
`abi-alignment-aliasing`, with draw seed `8541954469880052534` over the
17-entry eligible pool
`52,53,72,73,74,77,81,82,84,87,88,89,92,93,95,97,98`; index 12 selected
goal 92. The distinct hypothesis was that x-only/public-key representation
casts in `src/modules/extrakeys/main_impl.h`, keypair `data[32]` casts, or
Bitcoin Core's `std::array<unsigned char, 96>` reinterpretation could produce
an optimized-build aliasing, alignment, or object-lifetime defect. Current
headers guarantee byte-only 64/96-byte opaque storage, the internal group
serialization uses `memcpy`, and the source history shows these casts are the
original intentional representation bridge. Clang and GCC O3/LTO Core-shaped
probes compared 4096 deterministic keypairs and 4096 successful tweaks with
no divergence; Clang ASan/UBSan also passed. Current native and forced-int64
extrakeys tests passed all 7 cases each, and Bitcoin Core's 7 key tests passed.
A GCC strict-aliasing/cast-alignment build completed without diagnostics at
the audited sites; host layout probes passed, while -m32 was unavailable due
missing libc headers. TypeSanitizer produced unrelated field-limb reports
before these casts could be isolated, and the exact minimal byte-array cast
passed it. The hypothesis is dismissed as no current defect: no production
source change is justified. Full evidence is in
`agent-journal/abi-alignment-aliasing.md`; the next draw excludes goal 92
until new compiler/ABI evidence appears, along with active goals 49, 61, and
78, and uses the fresh queue
`52,53,72,73,74,77,81,82,84,87,88,89,93,95,97,98`.

The fifty-seventh controller cycle selected catalog goal `93`,
`system-fault-injection`, with draw seed `2318999372` over the 16-entry
eligible pool `52,53,72,73,74,77,81,82,84,87,88,89,93,95,97,98`; index 12
selected goal 93. Existing libsecp context-clone, scratch, ecmult callback,
and rollback fault paths were already indexed, so the cycle selected the
distinct Bitcoin Core RPC cookie path. `GenerateAuthCookie` in
`src/rpc/request.cpp:100-146` checked only `is_open()` before writing and
renaming `.cookie.tmp`, despite `src/rpc/request.h:32-43` defining `Ok` as
auth data saved to disk. A deterministic Linux I/O interposer failed writes
to the temporary cookie on the clean Core daemon: it created a zero-byte
cookie, logged successful cookie authentication, kept HTTP RPC running, and
returned HTTP `401` to a request using the on-disk cookie. The normal control
created a 75-byte cookie and returned HTTP `200`; explicit libc close
interposition did not trigger under this libstdc++ path and is not claimed as
an independent finding.

A disposable Core worktree applied the smallest local-pattern repair: check
`file.fail()` after insertion and after `file.close()`, remove the temporary
file on either failure, and return `AuthCookieResult::Error` before rename.
Release `bitcoind` built successfully and the repair was recorded as
disposable Core commit `34415a3962` (`rpc: reject failed cookie writes`). The
no-fault repaired control authenticated successfully; the injected write
case produced no pid, no `.cookie`, no `.cookie.tmp`, and logged the cookie
failure followed by HTTP startup failure. This is a confirmed Low/Medium
local RPC availability and contract defect, not a consensus, key/funds,
privacy, or remote primitive. No source change was made in the audit
checkout. Full evidence and limitations are in
`agent-journal/system-fault-injection.md`.

The next draw excludes active campaigns `49`, `61`, and `78`, goal 93 until
new platform or partial-I/O evidence appears, and goal 92 until new ABI
evidence appears. The next eligible queue is
`52,53,72,73,74,77,81,82,84,87,88,89,95,97,98`.

The fifty-eighth controller cycle selected catalog goal `88`,
`bitcoin-wallet-recovery`, with random seed `2193848575` over the 15-entry
eligible pool `52,53,72,73,74,77,81,82,84,87,88,89,95,97,98`; index 10
selected goal 88. The distinct hypothesis was that
`CWallet::ChangeWalletPassphrase` in Core mutates the in-memory master-key
record, ignores the boolean from `WalletBatch::WriteMasterKey`, logs success,
and returns true after a failed SQLite write. The source contract trace
covered `src/wallet/wallet.cpp:636-666`, `walletdb.cpp:151-154`,
`sqlite.cpp:490-521`, and the RPC caller `wallet/rpc/encrypt.cpp:159-163`.
History/blame found no existing current repair or direct database-failure
test for this path.

A deterministic Linux SQLite interposer returned `SQLITE_IOERR` once for the
master-key `INSERT or REPLACE` statement. On clean Core, the faulted RPC
returned success, the same process accepted the new passphrase and rejected
the old one, and a restart accepted the old passphrase and rejected the new
one. The log contained both the passphrase-change success message and
`Unable to execute write statement: disk I/O error`. This proved a durable
wallet passphrase split-brain, not a theoretical I/O concern.

A disposable Core worktree/build applied the smallest repair: encrypt a
`CMasterKey` copy, require `WriteMasterKey` to succeed, publish the copy only
after the write, and restore the prior lock state on the new failure exits.
Release `bitcoind` built successfully and the source repair was committed as
Core commit `1c1300e8b7` (`wallet: preserve passphrase on database failure`).
The identical fault against the repaired binary returned failure; the old
passphrase worked in-process and after restart, while the new passphrase was
rejected in both cases. The existing RPC mapping still labels the failure as
incorrect passphrase, which is recorded as a separate API-contract lead. A
no-fault repaired control logged a successful change, accepted the new
passphrase in-process, and after restart returned `error:null` for
`new-good-88` while rejecting `old-pass-88`.

The finding is confirmed as a current Low/Medium local wallet integrity and
persistence-contract defect. It is not a consensus, key/funds, privacy, or
remote primitive and does not meet the controller's High/Critical gates.
Evidence included source/caller/database contracts, clean-master behavior,
restart differential, and repaired behavior under the same fault schedule.
Existing tests missed it because they do not inject a failed SQLite master-key
write and verify passphrase durability across restart. Full evidence is in
`agent-journal/bitcoin-wallet-recovery.md`.

Limitations are Linux/x86_64 dynamic SQLite interposition, no BDB or other
platform coverage, and no integrated Core test because the user's Core tree
was already dirty. The disposable daemon, wallet datadirs, worktree, build,
and interposer are removed after handoff. Goal 88 is exhausted only for this
passphrase-write hypothesis; descriptor/keypool/backup/migration/recovery
cells remain reopenable. The next eligible queue is
`52,53,72,73,74,77,81,82,84,87,89,95,97,98`.

The fifty-ninth controller cycle selected catalog goal `73`,
`network-state-machine`, with draw seed `1209637965` over the 13-entry
eligible pool `52,53,72,73,74,77,81,82,84,87,89,95,97`; index 3 selected
goal 73. The distinct hypothesis was that a zero-byte nonblocking socket
write followed by a short positive write could lose or duplicate V1/V2 wire
bytes, clear transport state incorrectly, corrupt `nSendBytes`, or cause the
receive side to disconnect or deadlock. Prior malformed net-message queue
oracles from `d93e4f7e26` were excluded as duplicate evidence.

The current Core source trace in `src/net.cpp:1607-1684` shows that positive
writes call `MarkBytesSent` for exactly the returned count and stop on short
writes, while zero and accepted nonblocking errors retain the transport state.
`SocketHandlerConnected` at `src/net.cpp:2147-2247` suppresses receiving only
after positive send progress with data remaining. A disposable Core test
worktree added a scripted socket returning zero, one byte, then full chunks;
it sent a V1 PING through the actual `CConnman::PushMessage` and
`SocketHandlerPublic` path, independently constructed expected V1 wire bytes,
and checked exact output, empty transport state, and `nSendBytes`.

The first `/tmp` build reached the final link but failed with
`final link failed: No space left on device`; the disk-backed Release build
then completed all `543/543` Ninja steps. The focused oracle exited `0` with
`*** No errors detected`. Existing `net_tests/v2transport_test` exited `0`,
and the full `net_tests` suite exited `0` after `19` cases. The first oracle
version expected three sends; mock instrumentation showed lengths
`24,24,23,8`, which is normal V1 header/payload framing rather than a
duplicate. After correcting the test expectation to four calls, all checks
passed.

The cycle is dismissed for this socket-level hypothesis. No production Core
source change or Core fix commit resulted. The custom socket control is V1;
V2 was exercised by the existing deterministic transport test, and this cycle
did not cover every-byte EOF, network reordering, platform socket semantics,
or a full V2 CConnman handshake fixture. The per-goal evidence and limitations
are in `agent-journal/network-state-machine.md`. The next eligible queue is
`52,53,72,74,77,81,82,84,87,89,95,97`.

The sixtieth controller cycle selected catalog goal `84`,
`secp-nonce-session`, with draw seed `3695385067` over the 12-entry eligible
pool `52,53,72,74,77,81,82,84,87,89,95,97`; index 7 selected goal 84. The
distinct hypothesis was that Core accepts a valid repeated-key `musig()`
descriptor but cannot represent all participant slots because signing and PSBT
nonce/partial-signature maps are keyed by `CPubKey`. The source and caller
trace covered `src/script/descriptor.cpp`, `src/musig.cpp`,
`src/script/sign.cpp`, PSBT serialization, and wallet functional callers.

BIP327 v1.0.3 says duplicate individual-key support is optional at the
application layer, while BIP390 v0.2.0 permits repeated participant keys in a
descriptor; BIP373's keydata structure is keyed by participant public key. A
disposable clean-Core Release build completed all `543/543` Ninja steps. Its
temporary `bip328_tests/duplicate_participant_state` expanded
`rawtr(musig(key1,key1,key2))`, generated valid nonces for the two unique keys,
and confirmed that partial signing returned null at the map-cardinality check.
The full four-case temporary `bip328_tests` suite passed with `*** No errors
detected`. No crash, partial signature, nonce consumption, or nonce reuse was
observed.

The verdict is inconclusive as a compatibility contract question but dismissed
as a current security/correctness defect: this is a safe application
limitation, not a cryptographic failure. No production source change or fix
commit resulted. The exact evidence and reopen conditions are in
`agent-journal/secp-nonce-session.md`; exclude this map-cardinality hypothesis
until a BIP/Core contract decision or a real duplicate-slot caller appears.
The audit branch remains the only modified worktree, with the journal commit
pending. The next queue is `52,53,72,74,77,81,82,84,87,89,95,97`, excluding the
completed duplicate-participant cell while retaining other goal-84 cells.

The sixty-first controller cycle selected catalog goal `72`,
`filesystem-crash-consistency`, with draw seed `435702422` over the 12-entry
eligible pool `52,53,72,74,77,81,82,84,87,89,95,97`; index 2 selected goal
72. The distinct hypothesis was that current Core logs and ignores a failed
active block-file durability operation in `Chainstate::FlushStateToDisk`,
then calls `WriteBlockIndexDB`, allowing block-index metadata to be published
after the block or undo file did not successfully flush. The source trace
covered `src/validation.cpp:2899-2919`,
`src/node/blockstorage.cpp:797-809`, and
`src/util/fs_helpers.cpp:108-137`. Historical commit
`f0207e00303a1030eca795ede231e3c0d94df061` explicitly identified the same
ordering concern and left the current caller TODO unresolved.

A disposable clean-Core Release build completed all `543/543` Ninja steps.
A temporary chainstate test plus a Linux `LD_PRELOAD` interposer returned
`EIO` from `fdatasync`/`fsync` for active `blocks/blk*.dat` files. On current
master the focused test failed because `FlushStateToDisk` returned true after
the injected failure; the interposer log contained `fdatasync fd=6`. The
smallest disposable repair returned `FatalError` before `WriteBlockIndexDB`.
The repaired no-fault `chainstate_write_tests` suite passed all 3 cases, and
the identical injected focused test passed its 1 case with the expected fatal
diagnostics. The production/test repair is disposable commit `3c2d36f1ab`,
`validation: stop after block file flush failure`.

This is a confirmed Medium local crash-consistency/integrity defect: after a
real crash, index metadata can refer to block data that was not durable. It
does not meet the High/Critical gates because no invalid-block acceptance,
consensus divergence, key/funds/privacy loss, or remote primitive was shown.
Existing tests did not inject block-file `EIO` and assert the publication
ordering. The per-goal evidence, exact commands, caller scope, limitations,
and reopen conditions are in `agent-journal/filesystem-crash-consistency.md`.
No protected Core or libsecp256k1 file changed. The next queue is
`52,53,72,74,77,81,82,84,87,89,95,97`, excluding this exact active-block
flush-ordering cell while retaining other goal-72 durable-boundary cells.

The sixty-second controller cycle selected catalog goal `53`,
`statistical-timing-side-channel`, with draw seed `1794593845` over the
12-entry eligible pool `52,53,72,74,77,81,82,84,87,89,95,97`; index 1 selected
goal 53. The fresh hypothesis measured `secp256k1_ec_pubkey_create` for low
and high Hamming-weight secret scalars and separately checked whether recent
opaque-key and MuSig validation changes crossed a ctime boundary. A pinned
120,000-sample-per-class harness produced equal medians/p95 values and Welch
t-statistics `0.671323`, `-0.451355`, `-0.879812`, and `-0.241282`; scheduler
outliers polluted means. This timing candidate was dismissed as a measurable
signal in the tested environment, with the explicit limitation that timing
statistics are not proof.

The audit-branch ctime control before repair exited 99 under Valgrind with
reports through `keypair_load`/`ge_eq_var`, MuSig nonce and partial-signing,
and silent-payment paths. A disposable worktree at clean `origin/master`
`0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a` passed the identical ctime run, so
the regression was attributed to earlier audit-branch changes rather than
clean upstream master. The smallest repair declassified the derived public
point before comparing it to the public half of an opaque keypair and replaced
secret-derived MuSig `||` validity aggregation with bitwise `|`, declassifying
only the final flag before the public error branch. The repaired ctime run
exited 0 with no Memcheck output. The same Release/Valgrind-enabled build's
normal test binary built and passed 16 iterations in 61.264s (random seed
`2924b30fd7983a2288cba2cbdbe347ac`).

This is a confirmed audit-branch constant-time validation regression, not a
clean-origin finding: master-relative severity is none for current upstream,
with Medium branch-local constant-time hygiene risk if the short-circuit code
were retained. No remote timing primitive or key disclosure was shown. The
per-goal evidence, source/history trace, exact commands/output, limitations,
and reopen conditions are in `agent-journal/timing-side-channel.md`. The
source repair and this state update were committed as `5f3bee9c`,
`ctime: preserve secret declassification boundaries`. The next queue remains
`52,53,72,74,77,81,82,84,87,89,95,97`, excluding this exact goal-53 cell while
retaining other goal-53 secret-operation cells.

The sixty-third controller cycle selected catalog goal `82`,
`secp-field-scalar-matrix`, with draw seed `3687378918` over the 12-entry
eligible pool `52,53,72,74,77,81,82,84,87,89,95,97`; index 6 selected goal
82. Prior field normalization, `fe_half`, negation, zero-predicate, and
scalar-helper cells were excluded from this fresh cycle because their
independent evidence is already recorded in `translation-validation.md` and
the preceding field commits. The distinct hypothesis was a native 5x52 versus
forced 10x26 mismatch in canonical `secp256k1_fe_storage` packing/unpacking.

The standalone `/tmp/secp256k1-goal82-storage.c` harness used an independent
32-byte little-endian storage oracle, guarded storage canaries, canonical
edge values, every power of two through bit 255, and 1,024 deterministic
random values: 1,283 cases. Clang 22.1.7 and GCC 16.1.0 native and forced
10x26 O2 runs, Clang O0/O3/Os runs, and Clang/GCC LTO runs all printed
`ok values=1283 digest=3835fa7c29cf43ce`. Four VERIFY/Valgrind/ASan/UBSan
runs matched without diagnostics. Flipping one expected byte caused the
negative control to fail at value 0. The rebuilt native and forced-int64
project builds each passed all 8 focused field tests.

The hypothesis is dismissed: no cross-backend mismatch, lost bit, endian
packing error, canary overwrite, undefined behavior, or reachable production
defect was found. Master-relative severity is none. This does not cover
big-endian execution or noncanonical raw storage; those remain reopenable
cells. The exact source/history trace, commands, outputs, limitations, and
handoff are in `agent-journal/secp-field-scalar-matrix.md`. This is a focused
journal-only cycle with no production source change. The next queue remains
`52,53,72,74,77,81,82,84,87,89,95,97`, excluding this exact goal-82 storage
cell while retaining other field/scalar representation cells.

The sixty-fourth controller cycle selected catalog goal `52`,
`integer-arithmetic-audit`, with draw seed `1851309276` over the 12-entry
eligible pool `52,53,72,74,77,81,82,84,87,89,95,97`; index 0 selected goal 52.
The distinct hypothesis was a wrong trailing-zero result in the 32/64-bit
de Bruijn fallback tables or compiler-selected ctz builtins at an extreme bit
position. The source contract is nonzero input, and the relevant modinv
callers use sentinel bits to preserve it. The DER-length, scratch-size,
scalar-inverse, and ecmult-count families were excluded as already indexed.

A disposable independent loop-oracle harness checked all 32 and 64 single-bit
values plus 100,000 deterministic randomized nonzero values at each width.
Clang 22.1.7 and GCC 16.1.0 builtin and forced-fallback builds all printed
`ok values32=100032 values64=100064 digest=bd480fdd8c6e1c9f`. Clang
`VERIFY`+ASan/UBSan and GCC `VERIFY`+UBSan fallback runs matched. The native,
forced-int64, and MSan project binaries each passed the focused `ctz_tests`
and `modinv_tests`. A deliberate fallback mutation failed immediately at
`x=00000001`, proving the oracle sensitivity.

The hypothesis is dismissed: no table-index, width-selection, undefined
behavior, or modinv defect was found. Master-relative severity is none. No
production source change is justified; the exact harness hash, commands,
caller scope, outputs, limitations, and reopen conditions are in
`agent-journal/integer-arithmetic-audit.md`. This is a focused journal-only
cycle. The next queue remains
`52,53,72,74,77,81,82,84,87,89,95,97`, excluding this exact goal-52 ctz cell
while retaining other integer-arithmetic cells.

The sixty-fifth controller cycle selected catalog goal `53`,
`statistical-timing-side-channel`, with draw seed `3987825996` over the
11-entry post-cycle pool `53,72,74,77,81,82,84,87,89,95,97`; index 0 selected
goal 53. The prior public-key-create timing and branch-local ctime cells were
excluded. ECDH timing and historical ecmult timing families were also excluded
as already covered by `critical-history-sweep.md`. The fresh hypothesis was a
stable secret-MSB timing difference in public Schnorr signing through
Bitcoin Core's `KeyPair::SignSchnorr` path at `src/key.cpp:426-439`.

A clean CMake Release shared library was built from cycle-start HEAD with all
modules, x86_64 assembly, GCC 16.1.0, and `-O2`. The independent matrix
harness created 32 valid deterministic keypairs per class, randomized key
selection and class order, pinned CPU 0, fixed the public message and aux
inputs, and measured 60,000 samples per class. Clang runs produced Welch
statistics `-0.402522`, `-0.100478`, and `1.009761`; GCC runs produced
`0.619772` and `-1.173609`. Median/p95 differences remained within roughly
64 cycles, scheduler outliers reached millions of cycles, and every run
returned digest `d66d86584fdffd74`. The final harness SHA-256 is
`f7e3be25bd25d46afe8a00c42de7a60c5e24ef99d944ae666faa226e21b966fc`.

Release disassembly showed fixed `0x100`/`0x20` loop bounds in
`secp256k1_ecmult_gen_gej` and `sete`/mask-based table selection. The
hypothesis is dismissed: no stable timing signal or clean-master defect was
found, and no source change is justified. Master-relative severity is none.
This is x86_64 evidence rather than a proof for every backend or
microarchitecture. The exact commands, caller scope, outputs, assembly notes,
limitations, and reopen conditions are in
`agent-journal/timing-side-channel.md`. This is a focused journal-only cycle.
The next queue is `53,72,74,77,81,82,84,87,89,95,97`, excluding this exact
randomized Schnorr signing cell and the prior cycle-62 timing cells.

## Cycle 66 - goal 72 (`filesystem-crash-consistency`)

Draw seed `3445880270` selected index 0 from the distinct-cell pool
`72,74,77,81,82,84,87,89,95,97`. The prior goal-72 active block-file flush
ordering cell was excluded. This cycle audited short-write behavior for the
Bitcoin Core banlist persistence path at protected Core HEAD
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`.

`CBanDB::Write` passed the final `banlist.json` directly to
`common::WriteSettings`, whose `std::ofstream::open` truncates an existing
file before output. `BanMan::DumpBanlist` clears `m_is_dirty` before the write
and restores it only after failure. A deterministic Linux `LD_PRELOAD`
interposer returned `EIO` from `write`/`writev` for the banlist descriptor. A
temporary focused test wrote valid prior JSON, injected the failure, and
observed the destination become empty; the clean-source run failed with the
exact assertion `actual_contents == old_contents` and one Boost failure.

Actual callers include RPC `setban`, the node interface, startup/destructor
dumps, and the periodic scheduled dump. The clean-source build completed all
`543/543` Ninja steps in `/mnt/my_storage/bitcoin-goal72-banlist-build`.
The disposable repair wrote `banlist.json.tmp`, removed it on write failure,
and called `RenameOver` only after successful close. The repaired injected test
and the existing `banman_tests/file` parser test both ended with
`*** No errors detected`. Disposable repair commit:
`3d49e2ee12cd2be6ce50ebaf47c53df357297997`.

Verdict: **confirmed Medium local persistence-integrity defect** in current
Core. It can destroy the last valid persisted peer-ban policy on an I/O
failure and subsequent restart; no consensus, key/funds, privacy, or remote
primitive was shown. No source change was made in the protected Core tree or
the secp256k1 audit branch. The full evidence, commands, interposer hashes,
repair, limitations, and reopen cells are in
`agent-journal/filesystem-crash-consistency.md`. The next queue is
`74,77,81,82,84,87,89,95,97`, retaining separate goal-72 directory/rename/
recovery cells.

## Cycle 67 - goal 84 (`secp-nonce-session`)

Draw seed `862797388` selected index 4 from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. The prior goal-84 repeated-participant map
cardinality hypothesis was excluded. This cycle audited the public
`musig_nonce_gen_counter` lifecycle at counter zero, 32-bit rollover, and both
64-bit extremes, including equivalence with the explicit nonce API.

Audit base was `319d56edbc85f0c71b28ffd11efd689e8dc0874c`; protected secp remained
clean at `e153e2681f7bf1dd74894e2170213e3983030989`, and protected Core remained
at `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6` with only its recorded dirty
files. The source writes the big-endian counter into the first eight bytes of
a zeroed 32-byte transcript. Counter zero is intentionally distinct from
explicit `nonce_gen`, which rejects an all-zero session-random buffer.

An independent public-API probe covered 8 edge counters x 8 combinations of
message/cache/extra-input presence. It required success, repeat determinism,
boundary separation, and exact explicit-API equivalence for every nonzero
counter. Audit native ASan/UBSan, audit forced-int64 Clang and GCC release
builds, and a clean-origin release build each printed
`PASS counter-boundary cases=64 combinations=8`. The audit MuSig test binary
also passed 16 iterations with all 12 tests passing. Initial wrong-oracle
controls failed at counter zero and at counter one with trailing-byte encoding,
then passed after correction.

Verdict: **dismissed**, no master-relative defect or production change. The
evidence is x86_64-only with one fixed keypair and does not cover external
wrappers or big-endian execution. The exact source trace, probe hash, commands,
outputs, limitations, and reopen conditions are in
`agent-journal/secp-nonce-session.md`. The next queue remains
`74,77,81,82,84,87,89,95,97`, excluding this exact goal-84 counter cell while
retaining explicit lifecycle/error-output and wrapper cells.

## Cycle 68 - goal 74 (`memory-pressure-allocator`)

Draw seed `1314649620` selected index 0 from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. The standalone PoolResource lifecycle and
fragmentation cell was excluded because Core commit `7ebd0c7962` already
contains an independent 721-input lifecycle oracle and mutation proof. The
fresh hypothesis was that reusable `CCoinsViewCache` pool chunks might be
underreported or drift from `DynamicMemoryUsage()` after clear/flush/reset,
causing cache sizing to miss retained memory during chainstate workloads.

The source/history trace found the intentional contract: `src/coins.cpp:38-40`
and `src/memusage.h:199-216` count allocated pool chunks, while
`src/coins.cpp:260-289` retains or reconstructs the resource according to the
caller-selected `reallocate_cache` policy. Existing tests at
`src/test/coins_tests.cpp:957-965` explicitly require erased-cache usage not to
increase and `:1088-1110` checks resource accounting. History `5e4ac5abf5`
introduced explicit resource reallocation because pool-backed `clear()` keeps
memory, and `3e0fd0e4ddd` clarified the current parameter semantics.

Focused `pool_tests,coins_tests` passed all 17 selected cases. The release-like
`coinscache_sim` replay passed 306 runs with peak RSS 58,716 KB; its
AddressSanitizer/UndefinedBehaviorSanitizer replay passed 306 runs with peak
RSS 217,840 KB. The release-like and sanitizer `coins_view` replays passed
2,767 runs each with peak RSS 62,320 KB and 266,248 KB. The largest
`coinscache_sim` input passed a 128 MiB RSS limit at 62,380 KB. Core remained
at its pre-existing dirty state and no process or artifact remained.

Verdict: **dismissed**. No undercount, monotonic retention defect, allocator
fragmentation failure, or reachable OOM cleanup defect was found, and no
production change is justified. This does not cover allocation-failure
injection, alternate allocators, cgroup pressure, or long-running full-node
IBD/mempool/wallet profiles. The exact commands, source evidence, limits, and
reopen conditions are in `agent-journal/memory-pressure-allocator.md`. This
is a focused journal-only cycle. The next queue retains goal 74's separate
fault-injection/live-workload cells, followed by `77,81,82,84,87,89,95,97`.

## Cycle 69 - goal 77 (`symbolic-model-checking`)

Draw seed `3820308283` selected index 1 from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. CBMC, KLEE, and ESBMC were unavailable, so this
cycle used a finite-state bounded proof rather than claiming symbolic-tool
coverage. The selected hypothesis was a wrong discriminator, canonicality,
range, byte-consumption, or truncation result in Bitcoin Core's
`ReadCompactSize` parser at `src/serialize.h:326-363`.

An independent byte-span oracle checked every one-byte prefix, all 65,536
payloads under the 253 discriminator, all 65,536 low payloads under both 254
and 255, all size/range thresholds, maximum values, and selected truncation
lengths. It also checked `WriteCompactSize` and `GetSizeOfCompactSize` against
the independent model. The Clang O2 run printed
`PASS compactsize bounded-cases=393955 digest=bb07dc5cf1d0389f`.
Clang O0, GCC 16.1.0 O2, and Clang ASan/UBSan produced the same result. The
repository `serialize_tests` suite passed all 15 selected cases.

A temporary scratch mutation changing `chSize == 253` to `chSize == 254`
failed immediately at `prefix=253 expected=1:0 actual=3:0`. After restoring the
source, the identical clean control passed again. The scratch worktree and
mutation were removed; Core remains at its pre-existing dirty state.

Verdict: **dismissed**. No parser defect or production change was found. This
is a bounded proof, not an unbounded CBMC/KLEE result; alternate streams,
platforms, untested full-width payload combinations, and caller-level resource
effects remain outside scope. The exact source trace, caller scope, commands,
hashes, mutation, limitations, and reopen conditions are in
`agent-journal/symbolic-model-checking.md`. This is a focused journal-only
cycle. The next queue retains `74,77,81,82,84,87,89,95,97`, excluding this exact
CompactSize cell.

## Cycle 70 - goal 74 (`memory-pressure-allocator`)

Draw seed `2229230088` selected index 0 from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. The prior goal-74 CCoinsViewCache retained
capacity and standalone PoolResource cells were excluded. This cycle tested
whether production-like `tx_pool` state-machine replay retains live Core
allocations or crosses realistic RSS limits after transaction acceptance,
block removal/reorg, eviction, expiry, and cleanup.

The release-like fuzzer completed all 5,659 corpus inputs at a 256 MiB limit
with peak RSS 140 MiB. A deliberate 128 MiB limit failed at 131 MiB after
5,629 inputs; the artifact was captured and removed. The same replay passed at
160 MiB with 142 MiB peak, and passed at 160 MiB with `MALLOC_ARENA_MAX=1`
and `MALLOC_TRIM_THRESHOLD_=131072` at 136 MiB peak. The Core `mempool_tests`
suite passed all 4 cases.

The default ASan/UBSan replay emitted a 538 MiB RSS-limit diagnostic caused by
253 MiB of ASan quarantine versus 88 MiB live heap. With
`ASAN_OPTIONS=quarantine_size_mb=0:detect_leaks=1:halt_on_error=1`, the same
5,660-run replay completed cleanly at 208 MiB peak with no leak or sanitizer
diagnostic. No Core source defect was shown.

Verdict: **dismissed**. This is a focused journal-only cycle; no production
change is justified. The exact source/history trace, binaries, corpus metadata,
commands, RSS matrix, sanitizer diagnosis, limitations, and reopen conditions
are in `agent-journal/memory-pressure-allocator.md`. The next queue retains
goal 74's allocation-failure, wallet/RPC, recovery, and full-node cells,
followed by `77,81,82,84,87,89,95,97`, excluding this `tx_pool` corpus
threshold cell.

## Cycle 71 - goal 77 (`symbolic-model-checking`)

Draw seed `2791938646` selected index `1` from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. The previous CompactSize parser proof was
excluded. This cycle bounded Core's SHA-256 streaming and padding state at
`src/crypto/sha256.cpp:699-731`, including `CSHA256::Write(nullptr, 0)`,
empty spans forwarded by `CHash256`, exact block transitions, in-place
double-hash output, and callers in hashing, script, witness, descriptor, and
address paths.

An independent OpenSSL 3.5.3 oracle checked every deterministic message length
`0..1024`, every two-part split (`525825` cases), byte-wise streaming through
length `256`, and one in-place alias case. Clang ASan/UBSan/pointer-overflow,
GCC ASan/UBSan, and Core's auto-detected SSE4/SSE41/AVX2 implementation all
printed `PASS ... lengths=1025 split_cases=525825 bytewise_lengths=257
alias_case=1`. A temporary `>=64` to `>64` production mutation failed at
`length=0` with `actual=6a09e667 expected=e3b0c442`, proving oracle
sensitivity. Core `crypto_tests` ran all `17` cases and ended with
`*** No errors detected`.

Verdict: **dismissed**. No streaming, padding, empty-input, aliasing, or
backend divergence was found, so no production repair is justified. This is a
bounded proof rather than CBMC/KLEE coverage; large total lengths, 32-bit and
non-x86 backends, and arbitrary message-byte exhaustive coverage remain open.
The detailed commands, source/history trace, hashes, mutation, limitations,
and reopen conditions are in `agent-journal/symbolic-model-checking.md`.
The next queue remains `74,77,81,82,84,87,89,95,97`, excluding this exact
SHA-256 cell.

## Cycle 72 - goal 95 (`database-semantics-differential`)

Draw seed `3189239557` selected index `7` from the distinct-cell pool
`74,77,81,82,84,87,89,95,97`. The selected hypothesis was that embedded
LevelDB iterator read/checksum failures are silently treated as normal end of
range because `CDBIterator::Valid()` forwards only `Valid()` and exposes no
`Iterator::status()` contract.

At protected Core HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, an
independent raw-LevelDB oracle iterated a compacted 1,000-record table after a
single-byte SSTable corruption and reported `count=806 valid=0 status=Corruption:
block checksum mismatch`. The current CDBWrapper production-shaped loop
reported `is_empty=0 count=806 decode_ok=1 first=key194 last=key999
loop_returned=true`, whereas the clean control reported all 1,000 records and
`status=OK`. Actual caller shapes include `BlockTreeDB::LoadBlockIndexGuts` at
`src/node/blockstorage.cpp:123-157` and the chainstate/index cursors in
`src/txdb.cpp` and `src/index`.

The hypothesis is **confirmed Medium local persistence-integrity defect**. A
clean detached Core worktree produced repair commit
`9972242ce42a332c89565439e922c2d035e6e906` (`dbwrapper: surface iterator read
errors`), which checks LevelDB status in `CDBIterator::Valid()` and adds a
deterministic corruption regression test. The pre-repair test failed with
`exception dbwrapper_error expected but not raised` and exit `201`; the
repaired focused test passed, and all 10 `dbwrapper_tests` cases passed with
`*** No errors detected`. No source change was made in the protected Core
checkout; its pre-existing dirty files remain untouched.

The detailed source/history trace, exact commands, hashes, raw and wrapper
outputs, repair, limitations, and reopen conditions are in
`agent-journal/database-semantics-differential.md`. The selected exact cell
is excluded; the goal remains active for distinct batch, snapshot, WAL,
comparator, corruption, and alternate-backend cells. The next queue remains
`74,77,81,82,84,87,89,95,97`.

## Cycle 73 Summary

Cycle 73 selected goal `74`, `memory-pressure-allocator`, with draw seed
`3097406016`, index `0`, from `74 77 81 82 84 87 89 95 97`. The distinct
hypothesis was that the production-shaped `wallet_create_transaction` fuzz
path could retain wallet/map/recipient allocations across inputs or cross a
realistic RSS limit while constructing up to 10,000 wallet transactions and
100 recipients.

The release-like wallet fuzzer replayed all 1,187 corpus runs at both 512 MiB
and 128 MiB libFuzzer RSS limits, exited 0 in about 65.5 seconds each time,
and reported 102 MiB peak RSS (104,988 and 105,100 KiB maximum RSS). The
ASan/UBSan/LeakSanitizer replay with
`quarantine_size_mb=0:detect_leaks=1` completed 1,188 runs at a 1 GiB limit,
exited 0, reported 174 MiB peak RSS (179,056 KiB maximum RSS), and emitted no
sanitizer or leak diagnostic. No artifact or process remained.

Verdict: **dismissed** for this bounded wallet cell. No current Core source
defect or repair is justified. The cell is excluded from future draws, while
goal 74 remains active for distinct allocation-failure, recovery, full-node,
and other wallet/RPC workload cells. The detailed source trace, binary hashes,
commands, outputs, limitations, and reopen conditions are in
`agent-journal/memory-pressure-allocator.md`.

The next queue is `74,77,81,82,84,87,89,95,97`, with this exact wallet corpus
cell excluded.

## Cycle 74 Summary

Cycle 74 selected goal `77`, `symbolic-model-checking`, with draw seed
`7234767300004701769`, index `1`, from `74 77 81 82 84 87 89 95 97`. The
distinct hypothesis was a boundary, overflow, truncation, or size mismatch in
Core's VarInt encoder/decoder used by undo, chainstate, block-index, flat-file,
compressed-object, and database-key serialization.

An independent `cpp_int` model checked all eight unsigned and documented
nonnegative-signed integer modes. Each mode covered 116,152 raw byte inputs,
including every one- and two-byte sequence, continuation/termination patterns
through length 12, and 50,000 deterministic random inputs, plus up to 20,031
boundary/random value round trips. Clang O2 ASan/UBSan, GCC 16.1.0 O2
ASan/UBSan, and Clang O0 controls all printed
`PASS varint-bounded digest=a6e66cc80af7db40`. The repository's 15-case
`serialize_tests` suite passed with no errors.

A disposable mutation changing `n++` to `n += 2` failed at `u8 value=128`,
proving oracle sensitivity. The mutation was restored and its worktree
removed. The GCC generic `DataStream` byte-write warning is recorded as an
unrelated goals-12/97 static-analysis lead.

Verdict: **dismissed** for this bounded VarInt cell. No current Core source
defect or repair is justified. Goal 77 remains active for distinct bounded
kernels; this VarInt cell is excluded along with its prior CompactSize and
SHA-256 cells. Detailed evidence is in
`agent-journal/symbolic-model-checking.md`.

The next queue is `74,77,81,82,84,87,89,95,97`, with the exact goal-77 VarInt,
CompactSize, and SHA-256 cells excluded.

## Latest Cycle

Cycle 75 selected goal `97`, `cpp-defect-taxonomy`, with draw seed
`10507514901928514153`, index `8`, from `74 77 81 82 84 87 89 95 97`. The
distinct hypothesis was that GCC 16.1.0's sanitizer/inlining warning in
Core's one-byte `ser_writedata8` and `DataStream::write` path represented an
actual bounds or lifetime defect.

Standard-library and custom zero-after-free allocator reductions passed under
GCC/Clang ASan/UBSan. The exact DataStream probe executed cleanly under ASan,
UBSan, and Clang warnings-as-errors; GCC emitted `-Warray-bounds` only in the
sanitizer/inlined form. The same GCC path was warning-free with
`-Werror=array-bounds` in a non-sanitized Release-style compile. A disposable
GCC CMake `CMAKE_COMPILE_WARNING_AS_ERROR=ON` build passed all serialization and
node objects before stopping at an unrelated Boost.MultiIndex warning in
`txmempool.cpp`; no DataStream warning was emitted there.

Verdict: **dismissed** as a GCC false-positive diagnostic, with no current
Core source change. The Boost warning remains a separate compiler/dependency
lead. Detailed source/history evidence, reduction hashes, commands, build
output, limitations, and reopen conditions are in
`agent-journal/cpp-defect-taxonomy.md`.

The next queue is `74,77,81,82,84,87,89,95,97`, excluding this DataStream
warning cell and the completed goal-74 wallet and goal-77 VarInt/CompactSize/
SHA cells.

## Cycle 76 Summary

Cycle 76 selected goal `82`, `secp-field-scalar-matrix`, with draw seed
`585213204`, index `3`, from `74 77 81 82 84 87 89 95 97`. The distinct
hypothesis was a wrong-word, out-of-bounds, or backend/compiler-dependent
result in `secp256k1_fe_storage_cmov`: native 5x52 uses four 64-bit words and
an xor mask, while forced 10x26 uses eight 32-bit words and complementary
and/or masks. The exact self-alias case was included; unsupported partial
overlap was not treated as a bug.

The independent scratch harness
`/tmp/goal82-storage-cmov.c` (SHA-256
`6524cbe2a2f4fea6f16bc1d500e06663470386f6a70fd02e680c51731059284b`)
checked 20,000 deterministic arbitrary-storage pairs for both flags, full
byte output, exact aliasing, and canary preservation. Clang 22.1.7 and GCC
16.1.0 native and forced-int64 ASan/UBSan VERIFY builds passed at O0/O2/O3/Os
with identical `ok pairs=20000 digest=53424292715b02f4`. Native and forced
Clang/GCC LTO controls matched. An output-byte mutation failed immediately
in both backend controls. x86_64 O2 and Clang AArch64 O2 helper disassembly
showed mask arithmetic without conditional or loop branches.

Clang Debug CMake controls with assembly disabled passed `cmov_tests` and the
`ecmult_gen_ge`/`ecmult_gen_blind` storage consumers in both VERIFY and
no-VERIFY native and forced-int64 builds. All disposable artifacts were
removed and no relevant process remains.

Verdict: **dismissed**. No backend mismatch, invalid write, undefined
behavior, flag-dependent branch, or reachable current-master defect was
found; no production source change is justified. Runtime AArch64/GCC-AArch64,
big-endian, 32-bit, MSVC, and formal timing evidence remain unavailable.
The exact storage-cmov cell is excluded, while malformed-storage validation,
other field arithmetic, and new architecture/backend cells remain eligible.
The selected-goal journal is `agent-journal/secp-field-scalar-matrix.md`.

## Cycle 77 Summary

Cycle 77 selected goal `95`, `database-semantics-differential`, with draw seed
`565229968`, index `7`, from `74 77 81 82 84 87 89 95 97`. The fresh
hypothesis was that Core's `CDBBatch` could diverge from LevelDB's ordered
`WriteBatch` semantics under duplicate puts/deletes, empty and binary values,
`Clear()` and reuse, obfuscation, alternating sync writes, disk reopen, or
small-SST compaction boundaries.

A disposable independent `std::map` model drove 600 batches per run over 32
keys and compared every public `CDBWrapper::Read()` result after every batch
and disk reopen. Three deterministic seeds covered disk/memory and plain/
obfuscated modes, for 12 combinations. Release and ASan/UBSan-linked runs
matched all expected states and digests; every deliberate expected-value
mutation was detected. The current Core `dbwrapper_tests` suite passed all 9
cases with `*** No errors detected`. No RocksDB or Pebble installation was
available for an alternate-engine run.

Verdict: **dismissed** for this bounded batch cell. No source change or repair
commit resulted. The exact iterator-status defect from cycle 72 remains
excluded. This goal stays active for deterministic WAL/MANIFEST fault and
crash recovery, snapshot lifetime, comparator/seek, and backend-portability
cells. Scratch artifacts were removed and no relevant process remains. The
next queue remains `74,77,81,82,84,87,89,95,97`.

## Cycle 79 Selection

Cycle 79 selected goal `74`, `memory-pressure-allocator`, with draw seed
`2009882816`, index `0`, from `74 77 81 82 87 89 95 97`. Goal `84` is
excluded from this immediate draw because its randomized/clone context cell
was just completed; its other lifecycle and wrapper cells remain pending.

## Cycle 79 Summary

Cycle 79 confirmed a distinct goal-74 RPC memory-pressure cell: an
authenticated client could hold one blocking `waitforblockheight` request and
pipeline 100,000 `getblockcount` requests on the same keep-alive connection.
The unmodified Core daemon's RSS grew from `50,792` KiB to `104,880` KiB,
and the retained per-client request deque survived client disconnect while
the active request remained blocked. Static tracing showed that the global
`-rpcworkqueue` check did not cover this per-client deque.

A disposable Core worktree produced repair commit `0cb250e09b`
(`http: bound per-client pipelined request queue`), authored by
`Lőrinc <pap.lorinc@gmail.com>`. The fix caps per-client parsing at the
configured queue depth and disconnects/clears a busy client that continues
to send after the cap. The new production-socket regression test, all six
`httpserver_tests` cases, a complete disposable `test_bitcoin` build, and a
patched 100,000-request runtime replay passed. Patched RSS stayed at
`50,728` KiB baseline versus `50,804` KiB after the burst.

Verdict: **confirmed authenticated/local RPC resource exhaustion**, with no
unauthenticated P2P or consensus impact shown. The protected Core checkout
was not edited and its pre-existing dirty files remain untouched. Goal 74
stays active for allocation-failure, recovery, full-node, and other workload
cells; this exact per-client HTTP queue cell is excluded from future draws.
Detailed evidence is in `agent-journal/memory-pressure-allocator.md`.

## Cycle 80 Selection

Cycle 80 selected goal `89`, `bitcoin-p2p-accounting`, with draw seed
`4015881993`, index `4`, from `77 81 82 87 89 95 97`. The exact goal-74
per-client HTTP queue cell is excluded from this immediate draw; goal 74
remains pending for distinct memory-pressure workloads.

## Cycle 78 Summary

Cycle 78 continued goal `84`, `secp-nonce-session`, with draw seed
`1056055882`, index `4`, from `74 77 81 82 84 87 89 95 97`. The distinct
hypothesis was that MuSig nonce generation, session processing, partial
signing, or final aggregation could depend on generator blinding or diverge
across cloned/randomized contexts. Existing context evidence covered ECDSA
and ordinary Schnorr; existing MuSig evidence covered transcript and custom
SHA behavior, but not this cross-campaign context lifecycle.

A disposable public-API matrix used fixed keys 1 and 2, eight deterministic
message/extra/session-random variants, `musig_nonce_gen` for one signer,
`musig_nonce_gen_counter` for the other, full nonce/session/partial/final
signing, independent partial verification, static Schnorr verification, and
byte-for-byte serialization comparisons. Four cases compared separately
randomized contexts and four compared both contexts after NULL randomization
reset. Clang and GCC ASan/UBSan probe builds, native and forced-int64
libraries, and optimized Clang/GCC forced-int64 libraries all passed with the
same digest sequence:
`bc3280984d63da4c`, `e43a9e3ace4d1307`, `5a8003777f985cd1`,
`b2c4a46839dca473`, `54ccdd10d5c0d610`, `4964bd6f6c555dad`,
`d00a49934dbd2182`, `f54cb68413539ab9`.

The existing MuSig suite passed all 12 groups and 16 iterations in native
Clang, forced-int64 Clang, and forced-int64 GCC builds. A temporary partial
response mutation caused the focused matrix to fail at `partial_sig_verify`,
then was restored and rebuilt before the clean replay. No production source
change or repair commit resulted.

Verdict: **dismissed**. No context divergence, invalid signature, sanitizer
finding, or current Bitcoin Core consensus/invalid-block impact was shown.
The selected goal remains active for other lifecycle and wrapper cells; this
exact randomized/clone context matrix is excluded. Scratch artifacts and
processes were cleaned. The next queue is `74,77,81,82,87,89,95,97` after
excluding the just-selected goal for the next draw.
