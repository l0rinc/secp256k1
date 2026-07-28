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

Goals `0` through `48` and `50` through `60`, plus `62` through `77` and `79`
through `98`, remain `pending`; this was the eligible set used for the third
draw. Goal `61` is `exhausted` for its bounded stateful-fuzzer cycle. Goal
`78` is `exhausted` for the completed scalar compiler/representation helper
queue, with its journal at `agent-journal/translation-validation.md`; reopen
it only for new source, caller, compiler, architecture, or specification
evidence. Goal `49` is `active`; its cycle journal is
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

## Handoff

The active cycle must verify the worktree and remotes, read the selected goal
journal and prior evidence, perform a bounded experiment, and record a verdict
before drawing another pending or reopened goal.
