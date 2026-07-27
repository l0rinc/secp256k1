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
probe bodies had no conditional or loop jumps. All four bounded hypotheses
were dismissed; no production finding or fix commit resulted. The goal
remains active with another compiler/architecture constant-time helper queued.
Scratch artifacts are under `/tmp/secp256k1-translation-78`.

## Handoff

The active cycle must verify the worktree and remotes, read the selected goal
journal and prior evidence, perform a bounded experiment, and record a verdict
before drawing another pending or reopened goal.
