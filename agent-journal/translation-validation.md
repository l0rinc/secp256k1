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

## Handoff

Start by checking the worktree, compiler versions, supported optimization and
sanitizer builds, and any existing Alive2/IR/compiler evidence in
`src/fuzz/README.md` and `agent-journal/`. Select one bounded arithmetic,
aliasing, shift, overflow, or constant-time kernel with a deterministic
oracle. Separate source undefined behavior, test failure, inline-assembly
contract, and compiler defect. Keep any reduction and generated artifacts in
`/tmp`, record exact commands and hashes, and do not claim a compiler bug from
an optimization difference without a minimized reproducer and independent
verification.
