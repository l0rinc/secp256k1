# Database Semantics Differential Journal

## Cycle 72 - LevelDB iterator error status is lost by `CDBIterator`

Status: **confirmed Medium local persistence-integrity defect** for the
selected current hypothesis. A focused repair and regression test were built
in a disposable Bitcoin Core worktree. No source change was made in either
protected checkout.

### Selection and state

- Catalog goal: `95`, `database-semantics-differential`.
- Draw seed: `3189239557`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`; length `9`; zero-based index
  `7`; selected goal `95`.
- Audit branch/base at cycle start: `codex/fuzz-oracles`, HEAD
  `5f089750124a5528d1de57214c76fbb3d3a5a8f5`.
- Protected Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. Its pre-existing dirty state
  remained `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- Protected secp256k1 remained detached and clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- No real fuzz, sanitizer, daemon, benchmark, or profiling process was
  running before or after the experiments.

### Hypothesis and trust boundary

Embedded LevelDB's public iterator contract distinguishes normal end of
iteration from a read failure through `Iterator::status()`. The hypothesis was
that Core's `CDBIterator::Valid()` drops this distinction, allowing a corrupt
or unreadable table to look like a clean end of a scan. The trust boundary is
local persisted database data and the filesystem, not attacker-controlled
serialized input. The consequence is silent truncation of database-backed
startup/index/cursor scans; no invalid-block acceptance, consensus divergence,
key/funds loss, or remote primitive was demonstrated.

### Source and history evidence

At protected Core HEAD:

- `src/leveldb/include/leveldb/iterator.h:72-73` requires callers to obtain a
  non-OK status when an iterator error occurs.
- `src/dbwrapper.cpp:404-406` implements `CDBIterator::Valid()` as only
  `m_impl_iter->iter->Valid()`; `CDBIterator` has no status accessor in
  `src/dbwrapper.h:149-157`.
- `src/node/blockstorage.cpp:123-157` loads the block index with
  `while (pcursor->Valid())` and returns normally after the loop; it does not
  have a post-loop status check.
- `src/txdb.cpp:39-43` uses `Valid()` to decide whether a legacy key exists,
  and `src/txdb.cpp:240-282` turns the same iterator into a cursor whose
  `Valid()` state ends when the wrapped iterator becomes invalid. Index code
  has the same `Seek`/`Valid`/`Next` pattern.

History confirms this boundary has been treated as a read-error surface:
`84b1e47bab` added database corruption tests and left iterator error handling
as an explicit TODO, while `82d5847f2a` routed direct `CDBWrapper::ReadImpl()`
failures through a fatal path. The current file still has no equivalent
status handling for iterator traversal. No prior audit journal contained this
exact current-HEAD iterator-status cell.

### Independent reproduction

A disposable C++20 harness created 1,000 serialized string records
`key0` through `key999`, compacted them with `max_file_size=1024`, and
produced one `000005.ldb` table of 23,270 bytes. A separate raw-LevelDB
harness used `ReadOptions.verify_checksums=true`; a separate CDBWrapper
harness used the production iterator loop shape. The harness source hashes
were:

- CDBWrapper harness:
  `83864a721567ad4d3b503316bac4acc8ca7dd1d87e6133f12d2bf265480bf3a2`.
- Raw LevelDB oracle:
  `0862001dc2c3789fbf40bd3a0227464a09ccf1a5594abe6d5f002e7ed6470670`.

Clean control:

```text
LevelDB:    count=1000 valid=0 status=OK
CDBWrapper: is_empty=0 count=1000 decode_ok=1 first=key0 last=key999 loop_returned=true
```

After flipping one byte at offset zero in the table:

```text
LevelDB:    count=806 valid=0 status=Corruption: block checksum mismatch: /tmp/goal95-db/000005.ldb
CDBWrapper: is_empty=0 count=806 decode_ok=1 first=key194 last=key999 loop_returned=true
```

The raw oracle proves that `Valid()==false` represents a checksum failure, not
normal exhaustion. The CDBWrapper loop silently loses 194 records and returns
normally because the wrapper exposes no way to inspect the status. A sweep of
22 corruption offsets found the same non-OK status at multiple data-block
locations; offsets outside data blocks correctly kept `status=OK`, so this is
not an artifact of treating every byte mutation as an error.

### Repair and regression proof

A clean detached worktree at the protected Core HEAD was used at
`/tmp/bitcoin-goal95-fix`. The minimal repair checks the underlying LevelDB
status in `CDBIterator::Valid()` and calls the existing `HandleError()` before
returning the boolean validity result. The regression test writes and compacts
the records, corrupts the first table byte after closing the database, then
expects `dbwrapper_error` while traversing.

The repair commit is:

```text
9972242ce42a332c89565439e922c2d035e6e906
dbwrapper: surface iterator read errors
```

It changes only `src/dbwrapper.cpp` and `src/test/dbwrapper_tests.cpp` (54
insertions, 1 deletion), was authored as `Lőrinc <pap.lorinc@gmail.com>`,
passed `git diff HEAD^ --check`, and compiled both affected translation units
against the current Core build libraries. The pre-repair binary failed the
new test with:

```text
exception dbwrapper_error expected but not raised
before_exit=201
```

The repaired binary passed the same focused test with exit `0`, and the full
`dbwrapper_tests` suite ran all 10 cases with `*** No errors detected`.
The repaired test binary hash was
`a5d0b4d1a8a35a8e1f18d6a0d8279ad3eb20ebfe78f70c2892bc7511c3ca109c`; the
pre-repair control hash was
`2b767c3264f7b1e1b6a6dad2ec035d0d275a61a59f425f6de88e95c008c35c4e`.

### Verdict, impact, and limitations

The hypothesis is **confirmed**. Current Core can interpret a LevelDB
iterator checksum/read failure as ordinary range exhaustion. This can make
`LoadBlockIndexGuts` report success after a partial block-index load and can
truncate chainstate/index cursor scans without surfacing the storage error.
The demonstrated impact is local persistence integrity and availability,
classified Medium; this cycle did not show a consensus, wallet-secret,
privacy, or remotely reachable primitive.

The evidence is Linux x86_64 with the embedded LevelDB implementation. No
RocksDB or Pebble installation was available, and no cross-platform or
32-bit run was performed. The reproduction exercises the CDBWrapper boundary
and the exact production loop shape, while the caller impact is established
by source tracing rather than a full-node corrupted block-index startup.
Reopen with an actual `BlockTreeDB::LoadBlockIndexGuts` corrupted-database
integration run, alternate-engine comparison, or a platform-specific Env
fault. Do not repeat the same clean/corrupt iterator cell.

Next queue remains `74,77,81,82,84,87,89,95,97`, with this exact goal-95
iterator-status cell excluded and the goal still active for distinct database
contracts such as WAL recovery, snapshots, batches, comparator ordering, or
backend portability.

## Cycle 77 - CDBBatch ordered-operation and persistence differential

Status: **dismissed for this bounded WriteBatch cell**. No current Core
source defect or repair is justified.

### Selection and state

- Catalog goal: `95`, `database-semantics-differential`.
- Draw seed: `565229968`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`; length `9`; zero-based index
  `7`; selected goal `95`.
- Audit branch/base at cycle start: `codex/fuzz-oracles`, HEAD
  `82eb4a56f78ba906a2ff58fcd2e0e41c3f05d63f`.
- Protected Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. Its pre-existing dirty state
  remained exactly `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- Protected secp256k1 remained detached and clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- No real fuzz, sanitizer, daemon, benchmark, or profiling process was
  running before or after the experiments.

### Hypothesis and contract

The fresh hypothesis was that the Core `CDBBatch` wrapper could diverge from
the LevelDB `WriteBatch` contract when a batch contains duplicate puts and
deletes, empty or binary values, a `Clear()` and reuse, obfuscation, alternating
sync writes, or repeated disk reopen. The trust boundary is local persisted
database state and the filesystem. It is not an untrusted network parser, and
no remote, consensus, wallet-secret, or privacy primitive was assumed.

The embedded LevelDB contract is explicit in
`src/leveldb/include/leveldb/write_batch.h:6-16`: updates are applied in the
order added. Its example is `Put`, `Delete`, `Put`, `Put`, leaving the last
value. `src/leveldb/db/write_batch.cc:35-40` makes `Clear()` reset all buffered
records; lines 98-109 append each operation and increment the count; lines
132-137 replay records in order into the memtable. LevelDB documents
`ApproximateSize()` as implementation-dependent metrics, so it was not used
as an exact semantic oracle.

The wrapper path is `src/dbwrapper.h:98-124` and
`src/dbwrapper.cpp:171-194`: `CDBBatch::Clear()` resets the LevelDB batch,
`WriteImpl()` obfuscates the value scratch stream and passes a `Slice` to
`WriteBatch::Put`, and `EraseImpl()` appends `Delete`. `WriteBatch()` submits
the complete batch with either normal or synchronous write options at
`src/dbwrapper.cpp:288-296`. The existing `dbwrapper_batch` test at
`src/test/dbwrapper_tests.cpp:153-195` covers three keys, one erase-before-
commit, and one post-`Clear()` write, but does not cover repeated duplicate
operations, empty/binary values, disk reopen, or many batches. History shows
the test originated in `14885068726` and `cb1ab0a716` later added batch reuse;
no prior journal contained this random ordered-operation cell.

### Independent model and exact evidence

The disposable C++20 harness used an independent `std::map<std::string,
std::string>` committed-state model and a separate pending-operation map. It
generated 600 batches per run, 1-32 operations per batch, 32 keys, duplicate
puts/deletes, values of length 0-95 containing arbitrary bytes, random
`Clear()` calls, alternating `fSync`, and a deterministic map update only
after `CDBWrapper::WriteBatch()` returned. Disk runs reopened the wrapper
every 37 batches with `max_file_size=1024`; memory-only runs exercised the
same operation path without pretending memory state survives destruction.
Each mode compared every key through the public `Read()` API after every
batch and after every reopen. A final expected-value mutation appended one
byte to an anchor value; every mode detected the mismatch, proving the oracle
was sensitive.

Release-style harness source and binary hashes were:

- source before the release run:
  `883e1c402df365a4d828208a3b339d687bd00a78bd33c7ea2293a89b55ecca01`;
- binary:
  `a365591c0b6c6f2979d6f0c89987f99721d4d89f7f15243414ccc925e36cde3e`.

The exact release compile used `/usr/bin/c++ -O2 -std=c++20` with the current
Core `build/src` and `src` include roots, then linked the current
`build/lib/libbitcoin_node.a`, `libbitcoin_common.a`, `libbitcoin_util.a`,
`libbitcoin_crypto.a`, `libbitcoin_consensus.a`, embedded `src/libleveldb.a`,
`src/libcrc32c.a`, secp256k1, univalue, wallet, CLI, IPC, and system SQLite/
Cap'n Proto libraries. The run command was:

```text
/mnt/my_storage/goal95-batch-model
```

All twelve mode/seed combinations passed. The stable results were:

```text
seed 0x95BA7C20260728: digest 12301f8787ba0829, keys 18
seed 0x95BA7C20260729: digest b70e9b9d1ae06b81, keys 13
seed 0xD15EA5E5:       digest 982d2e7b78e4db84, keys 12
each seed: disk/plain, disk/obfuscated, memory/plain, memory/obfuscated
each result: mutation_detected=1
```

The sanitized run rebuilt the harness with `-fsanitize=address,undefined
-fno-sanitize-recover=all -fno-omit-frame-pointer`, linked the existing
`build_fuzz` Core/LevelDB archives, and ran with
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Its source hash was
`3c9d5cf2b42447550ff446658ba8da991cd4e235b6603f45deb73c17de83c33d` and
binary hash was
`c22187ca2f1ad43b084367999bdc83d1f9896280376bb1587f9d81ef25b4993a`.
It reproduced all twelve passing results and emitted no ASan, UBSan, or leak
diagnostic. The fuzz-built `dbwrapper.cpp.o` was newer than the source and a
dry-run build scheduled only build-info generation, not a stale dbwrapper
compile.

The current Core focused control also passed:

```text
./build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
Running 9 test cases...
*** No errors detected
```

No RocksDB or Pebble installation was discoverable through `pkg-config` or
the bounded `/usr`, `/opt`, and `/tmp` search, so this cycle used the
embedded LevelDB implementation and the independent model rather than
claiming an alternative-engine differential.

### Verdict, limitations, and handoff

The hypothesis is **dismissed**. No duplicate-operation, scratch-reuse,
obfuscation, empty-value, `Clear()`, sync-option, reopen, or small-SST
semantic mismatch was observed. No production source change or audit repair
commit is warranted. The expected-value mutation was a harness control only;
it was not left in a repository or production build.

This cycle did not simulate power loss or injected WAL/MANIFEST failures, run
RocksDB/Pebble/Pebble-style alternate engines, execute on Windows or 32-bit,
or prove whole-node recovery after a damaged database. Reopen with an
installed alternate engine, a deterministic LevelDB Env fault schedule, a
crash/restart fixture, or new wrapper code. Do not repeat this exact ordered
batch cell or the prior iterator-status cell.

The scratch source, binaries, database directories, and object files were
removed after hashing; no relevant process remains. The goal stays active for
distinct WAL/MANIFEST recovery, snapshot lifetime, comparator/seek, and
backend-portability cells. Next queue remains
`74,77,81,82,84,87,89,95,97`.

## Cycle 98 - WAL sync boundary, crash replay, and injected sync failure

Status: **dismissed for this bounded persistence cell**. The CDBWrapper
sync/recovery behavior matched an independent durability model. No current
Core source defect or repair is justified.

### Selection and scope

- Catalog goal: `95`, `database-semantics-differential`.
- Controller draw seed: `13524800685825278971`.
- Eligible pool: `77 95`; zero-based index `1`; selected goal `95`.
- Audit branch at cycle start: `codex/fuzz-oracles`, HEAD
  `4865bfa132c6e607bc8f467ef1013c832e72fa18`.
- Protected Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. Its pre-existing dirty state
  remained exactly `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- No protected checkout was modified. No real fuzz, daemon, sanitizer,
  benchmark, or profiling process remained running after the cycle.

The fresh hypothesis was that the CDBWrapper `fSync` path or its injected
`DBParams::testing_env` boundary could misclassify WAL durability across a
crash: an unsynchronized put/delete might survive when it should not, a
synchronized operation might be lost, or a failed `Sync()` might be accepted
or leave the wrapper usable for later writes. The trust boundary is local
filesystem/database state and the LevelDB environment. This is not an
untrusted parser or a consensus oracle.

### Source and contract trace

At the protected Core HEAD:

- `src/dbwrapper.h:234-256` passes the caller's `fSync` choice through both
  single-key writes and erases to `WriteBatch()`.
- `src/dbwrapper.cpp:223-233` enables checksum-verified reads and selects the
  caller-provided LevelDB environment; `src/dbwrapper.cpp:288-296` submits
  the batch with `syncoptions` when `fSync` is true and routes a non-OK status
  through `HandleError()` as `dbwrapper_error`.
- `src/leveldb/include/leveldb/db.h:63-78` documents that callers should
  consider `WriteOptions::sync=true` for puts, deletes, and batches. The
  implementation at `src/leveldb/db/db_impl.cc:1180-1225` appends the batch,
  calls `logfile_->Sync()` for a sync write, and records a background error
  when that sync fails because the log state is indeterminate.
- `src/leveldb/util/env.cc:34-63` distinguishes ordinary and synchronous
  file writes. `src/leveldb/db/filename.cc:123-138` uses the synchronous form
  before renaming `CURRENT`, so the MANIFEST/CURRENT boundary is included in
  the trace.
- `src/test/fuzz/dbwrapper.cpp:40-105,184-279` currently controls background
  compaction with a deterministic environment, but its in-memory target does
  not exercise persistent WAL loss. That is a coverage limitation, not proof
  of a wrapper defect.

The apparent alternative lead that `CDBWrapper::CompactFull()` drops a
LevelDB status was dismissed during source inspection: this embedded LevelDB
version declares `DB::CompactRange()` as `void` at
`src/leveldb/include/leveldb/db.h:138-147`, so there is no return status for
the wrapper to preserve.

### Independent durability model

A disposable C++20 harness at `/mnt/my_storage/goal95_wal_recovery.cpp`
wrapped the production POSIX `leveldb::Env` and tracked every writable file's
appended position and last successful `Sync()` position. After each bounded
operation window it destroyed `CDBWrapper`, truncated scratch database files
to the last successful sync boundary, reopened through the same public
`CDBWrapper` API, and compared every known key with an independent
`std::map<std::string, std::string>` model. The model updates its durable map
only at sync operations, while the working map tracks all operations. It
included 64 binary-safe string keys, 24 initial synchronous writes, 24 cycles
of nine mixed puts/deletes with one-third sync probability, 1 KiB SST files,
periodic `CompactFull()`, repeated close/crash/reopen, and both obfuscation
modes.

The final harness source hash was
`9a5bef0e8bcec0e125964315a5ddb764400639329977ab6c1f8a5b49b40bf0ea` and the
release binary hash was
`27bc80a45ea521061e3f8998883d0553b006cd7ef9894d9d26e29b6b5a5f62b2`. The
release command was:

```text
timeout 240 /mnt/my_storage/goal95-wal-recovery
```

It passed all six persistence runs:

```text
WAL_RESULT seed=2632763952203560 obfuscate=0 keys=30 digest=37d64ffd95d009d pass=1
WAL_RESULT seed=2632763952203560 obfuscate=1 keys=30 digest=37d64ffd95d009d pass=1
WAL_RESULT seed=3512640997 obfuscate=0 keys=20 digest=7778b9090f3a1df7 pass=1
WAL_RESULT seed=3512640997 obfuscate=1 keys=20 digest=7778b9090f3a1df7 pass=1
WAL_RESULT seed=1327405879 obfuscate=0 keys=30 digest=2a87c74d644cbaf1 pass=1
WAL_RESULT seed=1327405879 obfuscate=1 keys=30 digest=2a87c74d644cbaf1 pass=1
```

The failure variant armed one injected `Sync()` error after a durable seed
write. The first write threw `dbwrapper_error`, a later write also threw due
to LevelDB's recorded background error, the seed survived the simulated
restart, and the failed record was absent:

```text
SYNC_ERROR_RESULT seed=2512049637 first_threw=1 later_threw=1 base_ok=1 fault_lost=1 pass=1
```

The independent Clang sanitizer build used the existing `build_fuzz` Core
and LevelDB archives with ASan and UBSan. Its object hash was
`eb7b197ace3b4b2e1f6969574a47357581bb0cc47457816f151e2057e25a45f9` and its
binary hash was
`28607fef7f9952f6d0912f813bcd2c207f14ba9bbbbeacb5691f29d754f13119`.
With `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`, it reproduced all seven passing results with
no sanitizer diagnostic. The protected Core control also passed:

```text
git diff --check
./build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=message
Running 9 test cases...
*** No errors detected
```

### Oracle sensitivity, verdict, and limitations

To test the model rather than merely trust it, a disposable mutation changed
`CrashToLastSync()` to truncate to the full current file position instead of
the last synced position. The mutated binary failed six WAL model checks and
the injected failure check, including `fault_lost=0`, and exited `1`. The
mutation binary hash was
`4827b7dc10393c5de27cce52c10aef67531f85a8118e74cd96b3c2b517c65883`.

The hypothesis is **dismissed**. The public CDBWrapper sync flag, custom Env
injection boundary, WAL replay, obfuscation, deletes, and compacted-table
recovery matched the independent model. No production source change, test
change, or audit repair commit is warranted.

Evidence is Linux x86_64 using the embedded LevelDB 1.22 implementation. The
crash model truncates data files rather than emulating every filesystem power
loss behavior; it does not yet drop directory entries, inject a partial
`Append()`, or fail a MANIFEST rename. No RocksDB/Pebble installation was
available, and no Windows, 32-bit, or full-node chainstate/wallet recovery run
was performed. Those remain distinct future cells. Scratch sources, binaries,
database directories, and mutation artifacts were removed after hashing.

Next queue remains `74,77,81,82,84,87,89,95,97`; exclude this exact
sync-boundary model and the earlier Goal95 iterator-status and ordered-batch
cells.

## Cycle 104 - Initial chainstate cursor key failure

Status: **confirmed Medium local persistence-integrity defect**, repaired in a
disposable Core worktree. This is distinct from the earlier Goal95 LevelDB
iterator-status cell: LevelDB returned a successful record here, but the
serialized Core key was malformed and the consumer-side cursor ignored the
initial decode result.

### Selection, trust boundary, and contract

The controller selected Goal `95`, `database-semantics-differential`, from the
pool `77 82 84 87 95 97` with seed `1228823428`, index `4`, at
`2026-07-28T13:24:31Z`. The protected Core checkout was at
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`; its only dirty paths remained
`src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log`.

The hypothesis was that `CCoinsViewDB::Cursor()` could expose a malformed
initial chainstate key as a valid cursor entry because it ignored
`CDBIterator::GetKey()`'s boolean result, while `CCoinsViewDBCursor::Next()`
already invalidated the cursor when the same decode failed. The trust boundary
is a locally persisted LevelDB record with a valid LevelDB checksum but an
invalid Core serialization; this is not a network or consensus parser path.
The public `CCoinsViewCursor` contract uses `Valid()` to gate `GetKey()` and
`GetValue()` calls, so a false-valid cursor can make UTXO statistics, snapshot,
scan, or copy callers process a default/stale key instead of stopping cleanly.

The relevant code was `src/txdb.cpp:247-255` on the protected HEAD. The
initial path constructed `CoinEntry` with its namespace byte defaulted to
`DB_COIN`, called `GetKey(entry)` without checking it, and then copied that
byte into `keyTmp.first`. `Next()` at `src/txdb.cpp:282-290` did check the
result and set `keyTmp.first = 0`, establishing the intended failure
contract. Production consumers include `src/kernel/coinstats.cpp:125-129`
and `src/rpc/blockchain.cpp:2234-2237,3261-3266,3431-3437`.

### Independent reproduction

A standalone C++20 probe used the production `CDBWrapper` and `CCoinsViewDB`
libraries. Its raw serializer wrote exactly one byte, `0x43` (`'C'`), as the
database key and an empty value. The probe source hash was
`a1f2387f6de6c5bec05f3b4ac00cd1dcd29cf8bdc98bbd083c2e1ea15830ebd4`.

Against the protected pre-fix build, binary hash
`06e94715e3cf1be373101fbdea435b4022f9bcf0b954bf13d9152312e8189dc7`, it
reported:

```text
MALFORMED_KEY valid=1 key_ok=1 value_ok=0 outpoint=COutPoint(0000000000, 4294967295)
```

This proves that `GetKey()` failure was converted into a successful cached
default outpoint. The minimal repair in disposable worktree commit
`91afd8627342903d86360e94f73ebd55bdfed71c` checks the initial decode and
invalidates `keyTmp` on failure. The fixed probe binary hash was
`11303195990680a11adb706e529e35ca2fcdbc3d8ad75f8bfb66c76d9f799a4c` and
reported:

```text
MALFORMED_KEY valid=0 key_ok=0 value_ok=0 outpoint=COutPoint(0000000000, 4294967295)
```

The fix commit was authored as `Lőrinc <pap.lorinc@gmail.com>` and changed
only `src/txdb.cpp` plus the regression in `src/test/coins_tests.cpp`.

### Regression and validation

The isolated Release build was configured at
`/mnt/my_storage/bitcoin-goal95-build` from the disposable worktree
`/mnt/my_storage/bitcoin-goal95-current-fix` and completed
`cmake --build ... --target test_bitcoin -j2` with 543/543 steps. The resulting
`test_bitcoin` hash was
`4cf80dbe6305b808781fc01a8371fa20a4f97fba8abfa0794430c111879c93b3`.

The new focused test passed with `*** No errors detected`:

```text
TMPDIR=/mnt/my_storage/bitcoin-goal95-test-tmp /mnt/my_storage/bitcoin-goal95-build/bin/test_bitcoin --run_test=coins_tests/coins_db_cursor_rejects_malformed_initial_key --log_level=test_suite
```

The adjacent nine-case database suite passed:

```text
TMPDIR=/mnt/my_storage/bitcoin-goal95-dbwrapper-tmp /mnt/my_storage/bitcoin-goal95-build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=test_suite
```

The full 14-case coins suite passed:

```text
TMPDIR=/mnt/my_storage/bitcoin-goal95-coins-tmp /mnt/my_storage/bitcoin-goal95-build/bin/test_bitcoin --run_test=coins_tests --log_level=message
```

`git diff --check` passed before commit, the disposable worktree is clean,
and no test process remains. No protected checkout was modified.

### Verdict and limitations

The hypothesis is **confirmed** and the smallest repair is validated in an
isolated current-HEAD worktree. The malformed key requires database
corruption, an incompatible writer, or an equivalent local fault; no remote
attacker path was demonstrated. The cycle did not emulate full filesystem
power loss, alternate database engines, Windows/32-bit execution, or a full
node restart with a damaged chainstate. The earlier Goal95 iterator-status,
ordered-batch, and WAL sync/recovery cells remain excluded.

Next work must select a distinct Goal95 comparator/seek, snapshot lifetime,
partial-I/O, MANIFEST, corruption, or alternate-backend contract rather than
repeating this initial-key failure.

## Cycle 105 - Persistent iterator snapshot lifetime

### Scope and hypothesis

Goal95 selected the persistence-semantics cell for an iterator held across
durable writes, deletes, overwrites, and full compaction. The hypothesis was
that `CDBWrapper::NewIterator()` might not preserve the LevelDB creation-time
view once the database had been reopened from disk, or that compaction could
invalidate the view. This is a local persistence contract, distinct from the
earlier memory-only iterator test and from the already-tested initial cursor
decode failure.

The protected Bitcoin Core checkout was at HEAD
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6` with only its pre-existing dirty
paths (`src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log`).
No protected source or data directory was modified.

### Source and model

`CDBWrapper::NewIterator()` at `src/dbwrapper.cpp:380-382` passes the wrapper's
checksum-verifying, non-cache-filling read options to LevelDB. LevelDB's
`DBImpl::NewInternalIterator()` at `src/leveldb/db/db_impl.cc:1074-1090`
captures `versions_->LastSequence()` and retains references to the active
memtable, immutable memtable, and version. `src/test/dbwrapper_tests.cpp:337-375`
checks the same semantic idea, but only with `memory_only = true` and without
deletes, overwrites, or compaction.

The independent model used fixed four-character string keys `k000` through
`k127`, so `std::map` ordering matches the serialized LevelDB byte ordering.
It retained an iterator after the initial 96 writes, performed three rounds of
deterministic overwrites/deletes/additions with synchronous writes, compacted
the full persistent database after each round, and compared both the held
iterator with its saved snapshot and a fresh iterator with the model. It ran
four seeds in both `obfuscate = false` and `obfuscate = true` modes. The probe
also deliberately changed a saved key/value in memory to prove that the
comparison oracle detects mutations.

### Independent reproduction

The standalone C++20 probe source hash was
`74d18fd57c2a2e0ac033d18461e7646f23315e0ca2b19dbf7c488bed992a6961`; the
Release-linked probe binary hash was
`3a264a5c1a1eedf496b5418a52b18441b77dbc22e30fed57d79df76b8a67139f`.
The exact execution was:

```text
TMPDIR=/mnt/my_storage/goal95-snapshot-tmp /mnt/my_storage/goal95-snapshot-probe
```

It produced:

```text
SNAPSHOT_RESULT seed=3 obfuscate=0 pass=1
SNAPSHOT_RESULT seed=3 obfuscate=1 pass=1
SNAPSHOT_RESULT seed=17 obfuscate=0 pass=1
SNAPSHOT_RESULT seed=17 obfuscate=1 pass=1
SNAPSHOT_RESULT seed=41 obfuscate=0 pass=1
SNAPSHOT_RESULT seed=41 obfuscate=1 pass=1
SNAPSHOT_RESULT seed=89 obfuscate=0 pass=1
SNAPSHOT_RESULT seed=89 obfuscate=1 pass=1
SNAPSHOT_MUTATION_ORACLE=pass
```

The protected current build's nine-case wrapper suite also passed with
`*** No errors detected`:

```text
TMPDIR=/mnt/my_storage/goal95-snapshot-dbwrapper-tmp /mnt/my_storage/bitcoin/build/bin/test_bitcoin --run_test=dbwrapper_tests --log_level=test_suite
```

### Verdict and limitations

The hypothesis is **dismissed** for the tested persistent LevelDB contract.
The held iterator retained its original view across durable mutations and
compaction in both wrapper obfuscation modes, and fresh iteration matched the
independent model. No source fix or journal-only finding commit was created.

This cycle did not exercise filesystem power loss, injected short writes or
`EIO`, MANIFEST corruption, alternate database engines, 32-bit/Windows
execution, or a full node restart between each mutation. Those remain valid
distinct Goal95 cells. The earlier iterator-status, ordered-batch, WAL
sync/recovery, and malformed initial-key cells remain excluded.
