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
overflow-sensitive helper queued.
Scratch artifacts are under `/tmp/secp256k1-translation-78`.

## Handoff

The active cycle must verify the worktree and remotes, read the selected goal
journal and prior evidence, perform a bounded experiment, and record a verdict
before drawing another pending or reopened goal.
