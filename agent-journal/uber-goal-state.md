# Uber-Goal State

## Controller

- Catalog: `agent-journal/reusable-continuous-agent-goals.md`
- Controller: `agent-journal/uber-goal.md`
- State initialized: 2026-07-27
- Repository worktree: `/tmp/secp256k1-oracles-next`
- Existing audit branch: `codex/fuzz-oracles`
- Current status: active goal 78 (`translation-validation`)
- First draw seed: `4179223777703642971`
- First draw: `61`
- First draw timestamp: `2026-07-27`
- Second draw seed: `3923475549`
- Second eligible slot: `77` of 98
- Second draw: `78`
- Second draw timestamp: `2026-07-27`

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

Goals `0` through `60` and `62` through `77`, plus `79` through `98`, remain
`pending`; this is the eligible set used for the second draw. Goal `61` is
`exhausted` for its bounded stateful-fuzzer cycle.
Goal `78` is `active`; its cycle journal is
`agent-journal/translation-validation.md`. The catalog is the source of
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

## Handoff

The active cycle must verify the worktree and remotes, read the selected goal
journal and prior evidence, perform a bounded experiment, and record a verdict
before drawing another pending or reopened goal.
