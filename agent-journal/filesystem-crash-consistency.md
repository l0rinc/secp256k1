# Goal 72: filesystem, power-loss, and crash-consistency injection

## Cycle 61

- Timestamp: 2026-07-28T05:26:01Z.
- Draw seed: `435702422`.
- Eligible pool: `52 53 72 74 77 81 82 84 87 89 95 97`.
- Selected index: `2`; selected goal: `72` (`filesystem-crash-consistency`).
- Audit branch base before this cycle: `43f35811` (`journal: record MuSig duplicate participant audit`).
- Protected Bitcoin Core tree: `/mnt/my_storage/bitcoin`, branch `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`; pre-existing dirty state remained `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, `?? fuzz-1.log`.
- Protected libsecp256k1 tree: `/mnt/my_storage/secp256k1`, detached HEAD `e153e2681f7bf1dd74894e2170213e3983030989`, clean.

## Scope and hypothesis

The selected surface was durable ordering around chainstate, block files,
indexes, and restart recovery. The falsifiable hypothesis was:

> If the active block or undo file fails its durability operation, current
> `Chainstate::FlushStateToDisk` logs the failure but still calls
> `WriteBlockIndexDB`, allowing durable block-index metadata to refer to block
> data whose flush was not successful.

The trust boundary is the local filesystem and the OS durability contract,
including a crash or power loss after an `EIO` from `fdatasync`/`fsync`. This is
not a remote-input or consensus-acceptance hypothesis. The relevant code is
in the current Core base, not only in an old release.

## Source and history evidence

On the clean disposable checkout at the protected Core base,
`src/validation.cpp:2899-2919` checks disk space, calls
`m_blockman.FlushChainstateBlockFile(m_chain.Height())`, logs a warning when
it returns false, and then calls `m_blockman.WriteBlockIndexDB()`.
`src/node/blockstorage.cpp:797-809` returns the result of `FlushBlockFile` for
the active cursor. `src/util/fs_helpers.cpp:108-137` makes `FileCommit` return
false when `fflush` or the platform durability operation fails; on Linux the
relevant operation is `fdatasync`, except for the explicitly ignored `EINVAL`.

The exact historical rationale is commit
`f0207e00303a1030eca795ede231e3c0d94df061`,
`blockstorage: Return on fatal block file flush error`. That commit changed
the flush helper to return a status and explicitly recorded that the caller
should decide whether to return early so `WriteBlockIndexDB` cannot publish an
entry that does not refer to a fully flushed block. The current HEAD contains
that commit as an ancestor, but the caller still contains the TODO and the
warning-only behavior. This cycle tested the unresolved TODO directly rather
than treating the old commit message as proof by itself.

Actual Core callers include block connect and disconnect paths at
`src/validation.cpp:3114` and `:3224`, periodic writes in the validation loop
around `:3626`, and shutdown `ForceFlushStateToDisk` calls in `src/init.cpp`
around `:382` and `:409`. The method also has explicit callers later in
validation for flush/recovery operations. A failed result is therefore
normally available to callers, but the current block-file branch converts the
failure back into success and proceeds with index publication.

## Reproducer and verification

All production changes and test changes below were made only in the disposable
checkout `/mnt/my_storage/bitcoin-goal72`, detached at the protected Core
base. The checkout was configured and built with:

```text
cmake -S /mnt/my_storage/bitcoin-goal72 -B /mnt/my_storage/bitcoin-goal72-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_BENCH=OFF -DBUILD_GUI=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF -DENABLE_WALLET=ON
ninja -C /mnt/my_storage/bitcoin-goal72-build test_bitcoin
```

The build completed all `543/543` Ninja steps. A temporary
`chainstate_write_tests/block_file_flush_failure_is_reported` test ran the
real `FlushStateToDisk` path. A temporary Linux `LD_PRELOAD` interposer
returned `-1` with `errno=EIO` from `fdatasync`/`fsync` only for paths matching
the active `blocks/blk*.dat` files, and logged each injected call to
`FAILFSYNC_LOG`. The log contained `fdatasync fd=6` (repeated across the
controlled setup and focused run).

With the clean current source and the fault schedule:

```text
TMPDIR=/mnt/my_storage/bitcoin-goal72-tmp FAILFSYNC_LOG=/mnt/my_storage/bitcoin-goal72-fsync.log LD_PRELOAD=/mnt/my_storage/bitcoin-goal72-failfsync.so /mnt/my_storage/bitcoin-goal72-build/bin/test_bitcoin --run_test=chainstate_write_tests/block_file_flush_failure_is_reported --log_level=test_suite
```

the test failed because `FlushStateToDisk` returned true despite the injected
durability failure. The process logged the lower-level
`Flushing block file to disk failed` error and then reached the test assertion
that expected `!FlushStateToDisk(...)`; the run ended with one Boost failure.
This is the failing-before proof.

The smallest repair in the disposable checkout changed the warning-only
branch to:

```cpp
return FatalError(m_chainman.GetNotifications(), state, _("Failed to flush block file."));
```

The repaired source was rebuilt. Without fault injection,
`--run_test=chainstate_write_tests` ran all 3 cases and ended with
`*** No errors detected`. With the same injected `EIO`, the focused one-case
test ended with `*** No errors detected`; the expected fatal diagnostics were
`Flushing block file to disk failed` and `Failed to flush block file.`. The
disposable production/test patch was recorded as buildable commit
`3c2d36f1ab` (`validation: stop after block file flush failure`).

The full chainstate test suite was not run under the broad interposer as a
passing criterion: that interposer intentionally fails every matching block
file sync, so existing tests that exercise unrelated successful flushes also
fail. The no-fault suite and the isolated injected test are the relevant
controls. No production patch was applied to the protected Core tree.

## Verdict

**Confirmed, Medium local crash-consistency/integrity defect.** The current
caller can return success and publish block-index state after a block-file
durability operation has failed. If a subsequent crash loses the dirty block
data while the index metadata survives, startup or later block reads can see
an index position that is not durable. The source ordering, the injected
first-invalid operation, and the historical rationale establish the ordering
defect. The interposer does not emulate a particular disk's post-power-loss
media state, so this cycle does not claim a complete hardware crash replay.

This does not meet the controller's High/Critical gates: there is no proof of
invalid-block or invalid-witness acceptance, consensus divergence, key/funds
loss, privacy loss, or a remote primitive. The impact is local persistence
integrity and recovery availability, with potentially expensive reindex or
recovery consequences.

Existing tests missed the defect because the chainstate write tests covered
timing and activation behavior but did not inject a block-file `EIO` and
assert that block-index publication stops. The regression oracle must remain
focused on the return contract and ordering; it must not treat all injected
flush failures in unrelated tests as expected success.

## Limitations and handoff

- Verification used Linux/x86_64 dynamic interposition and a Release test
  binary; no real power-loss run, `dm-flakey` device, Windows/macOS sync
  primitive, or alternate filesystem was used.
- The fault was an injected `EIO` at `fdatasync`; short writes, directory
  metadata durability, rename ordering, and recovery from a damaged block file
  remain separate cells for later cycles.
- The disposable Core checkout, build, interposer, log, temporary data, and
  test scratch must be removed after the journal commit. The protected Core
  and libsecp256k1 worktrees must remain unchanged.
- Reopen this goal for the remaining durable-boundary cells, but do not
  duplicate the confirmed active-block flush ordering hypothesis. The next
  queue after this cycle is `52,53,72,74,77,81,82,84,87,89,95,97`, with this
  exact goal-72 cell excluded and other goal-72 cells still eligible.
