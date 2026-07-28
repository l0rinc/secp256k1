# Uber-Goal State

## Controller

- Catalog: `agent-journal/reusable-continuous-agent-goals.md`
- Controller: `agent-journal/uber-goal.md`
- State initialized: 2026-07-27
- Repository worktree: `/tmp/secp256k1-oracles-next`
- Existing audit branch: `codex/fuzz-oracles`
- Current status: active goal 49 (`critical-history-sweep`)
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

Goals `0` through `48` and `50` through `60`, plus `62` through `77`, `79`
through `91`, and `94` through `98`, remain `pending`; this was the eligible
set used for the latest draw after prior-cycle exclusions. Goal `61` is
`exhausted` for its bounded stateful-fuzzer cycle. Goal
`78` is `exhausted` for the completed scalar compiler/representation helper
queue, with its journal at `agent-journal/translation-validation.md`; reopen
it only for new source, caller, compiler, architecture, or specification
evidence. Goals `92` and `93` are `exhausted` for their current bounded ABI
and Linux RPC-cookie fault hypotheses; reopen them only for new ABI/platform,
caller, or partial-I/O evidence. Goal `49` is `active`; its cycle journal is
`agent-journal/critical-history-sweep.md`. The catalog is the source of
titles, slugs, and campaign scope.

## Latest Cycle

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
