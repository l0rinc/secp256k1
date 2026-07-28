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
