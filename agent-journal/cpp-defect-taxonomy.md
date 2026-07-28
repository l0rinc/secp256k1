# Goal 97: C and C++ defect-taxonomy sweep

## Cycle 75

Status: dismissed for the selected GCC diagnostic / DataStream byte-span
hypothesis.

Controller draw:

- Draw seed: `10507514901928514153`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`.
- Selected index: `8`.
- Selected goal: `97`, `cpp-defect-taxonomy`.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, HEAD `b7e7b3c0f2afeee6f674e7dcf26676bbfa510d57`.
- Protected Bitcoin Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. Its only dirty entries were
  the pre-existing `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- Protected secp256k1 remained clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- No fuzz, sanitizer, daemon, test, or profiling process remained after the
  cycle. All disposable probes and the GCC CMake build directory were removed.

Hypothesis and trust boundary:

GCC 16.1.0's `-Warray-bounds` diagnostic at the one-byte serialization path
might indicate that `ser_writedata8` forms a span that permits an out-of-bounds
read when `DataStream::write` inserts its range. The trust boundary is a
local serialization helper used by network, disk, database, undo, and test
streams. The relevant current source is `src/serialize.h:57-60`, where
`std::span{&obj, 1}` is converted with `std::as_bytes`, and
`src/streams.h:244-248`, where the range is synchronously inserted into the
owned byte vector.

Source and history evidence:

- The span has one element and does not escape the synchronous `write` call;
  `DataStream::write` uses `src.begin()` and `src.end()` only for the insert.
  There is no pointer arithmetic or lifetime extension in the project code.
- Blame traces the current form to `fade0b5e5e6`, the 2024 scripted migration
  from the project `Span` wrapper to `std::span`. The old and new forms have
  the same one-element contract. The project default
  `CMAKE_COMPILE_WARNING_AS_ERROR` is OFF; the warning was seen only in the
  GCC sanitizer/inlining probe.
- No matching current issue, PR, or prior audit finding was found. Other
  GCC-only warnings in unrelated secp256k1 probes were already classified as
  compiler-differential negative evidence.

Independent reduction and runtime controls:

1. A standard-library-only probe using `std::vector<std::byte>` and the same
   `std::as_bytes(std::span{&value, 1})` range compiled with GCC 16.1.0
   `-Werror=stringop-overread` and Clang `-Werror`, then appended 100,000
   bytes under ASan/UBSan. Both exited 0. Source hash:
   `658ea539f48816066e382ce092bf169cce11a2d2dca22b938e986379540df3bf`.
2. A custom `zero_after_free_allocator<std::byte>` probe also compiled and
   ran cleanly under GCC and Clang ASan/UBSan. Source hash:
   `4ee99e284ff625cc7306259a8b7a5d3687ca3264cc2c6282d945a713a15262bf`.
3. The exact `DataStream` probe emitted GCC 16.1.0's diagnostic while
   compiling with O2, `-Wall -Wextra`, and ASan/UBSan:
   `stl_algobase.h:424:32: warning: ... forming offset 1 is out of the
   bounds [0, 1] of object 'obj' [-Warray-bounds]`. With
   `-Werror=array-bounds` the compilation failed, but with that warning
   explicitly non-fatal the executable appended 100,000 bytes and exited 0.
   The same exact probe compiled and ran under Clang O2 ASan/UBSan with
   `-Wno-unused-parameter -Werror`, with no diagnostic. This is a compiler
   flow-analysis difference, not a runtime invalid access.
4. The exact GCC probe compiled with `-Werror=array-bounds` in the
   non-sanitized Release-style configuration and emitted no warning. The
   current `src/serialize.h` and `src/streams.h` hashes were
   `a6f778a2e3d0638d88651798f498488d746caf939d08cc148ca23b7d39ba987b` and
   `216fa033e467ecb86bdcf2db7ad14b0d81a0c85a9a3ba3d8eb2dde8138146fca`.

Project build control:

A clean disposable GCC 16.1.0 CMake configuration with
`CMAKE_COMPILE_WARNING_AS_ERROR=ON`, Release, tests enabled, GUI/ZMQ/fuzz
disabled, and ccache disabled configured successfully. The `test_bitcoin`
target reached 223/543 objects, including the Core serialization and node
objects, without the selected DataStream warning. The build then stopped on
an unrelated Boost.MultiIndex diagnostic in
`/usr/include/boost/multi_index/detail/bucket_array.hpp:165` while compiling
`src/txmempool.cpp`:

`-Werror=stringop-overflow: __builtin_memset ... specified bound ... exceeds maximum object size`

That dependency/compiler warning is recorded as a separate goal-12/91/97
lead and is not evidence against `DataStream`.

Verdict:

The selected hypothesis is dismissed. The one-byte span has a valid lifetime
and extent, standard and custom-allocator reductions are clean, Clang and
release-style GCC controls pass, and ASan/UBSan finds no invalid access. The
GCC sanitizer-only warning is a false-positive diagnostic caused by inlining
through the dynamic-extent span and libstdc++ range insert. No production
source change, warning suppression, or regression test is justified.

Limitations and handoff:

Only GCC 16.1.0 and the installed Clang were available; no GCC version
bisect or upstream compiler report was made. The full Werror build stopped at
an unrelated dependency warning, so it is not a complete project-wide Werror
proof. Reopen this cell if the warning appears in a supported release build,
becomes a CI failure, or is accompanied by a sanitizer/runtime report. Keep
the Boost warning as a separate dependency/compiler hypothesis; do not merge
the two diagnostics.

Next queue: retain goal `97` for other defect classes and continue with the
distinct cells in `74,77,81,82,84,87,89,95`, excluding this DataStream warning
cell and the already recorded goal-77 VarInt/CompactSize/SHA cells.

## Cycle 97: hsort heap-index and stride arithmetic

Status: dismissed for the selected custom-sort arithmetic hypothesis.

Controller draw:

- Draw seed: `18403467706833038537`.
- Eligible pool: `77 95 97`.
- Selected index: `2`.
- Selected goal: `97`, `cpp-defect-taxonomy`.
- Cycle timestamp: `2026-07-28T11:46:56Z`.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, starting and ending HEAD
  `7cd572a02c29aaf3f9c68b20dd996798565ec7f0`; source remained clean.
- The protected Bitcoin Core checkout was not modified. Its pre-existing
  entries remain `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- The protected secp256k1 checkout was not modified.
- Disposable sort binaries, source copies, analyzer output, and processes
  were removed after verification.

### Hypothesis and trust boundary

The selected fresh cell was the custom iterative heapsort in
`src/hsort_impl.h:16-123`. Its `size_t` child-index arithmetic and
`index * stride` pointer arithmetic could, in principle, wrap and access
outside the array when a public or module caller supplied a large count or
element size. The relevant callers are the public
`secp256k1_ec_pubkey_sort` at `src/secp256k1.c:385-403` and the internal
Silent Payments recipient sort at `src/modules/silentpayments/main_impl.h:42-49`.
The public header describes a qsort-like array/count contract; this is not a
parser-owned allocation length.

Blame attributes the current hsort implementation to `7d2591ce1` (2024-04-17,
`Add secp256k1_pubkey_sort`). No previous journal finding or current issue
matching this hsort cell was found. The hsort unit tests already exercise
element sizes 1, 64, and 65 and random counts, while the API and Silent
Payments tests cover their production callers.

### Independent verification

The disposable harness `/tmp/goal97-hsort.c` has SHA-256
`8aabd4d3a4929118910263d2d76bdeeae3a4c924affd8c8fea0103e30ea546e3`.
It includes the production `hsort_impl.h` directly and uses an independent
byte-wise insertion sort as the expected result. Each case fills a fresh
array with deterministic xorshift data, compares the complete sorted bytes,
and checks 32-byte `0xA5` canaries before and after the array. The schedule
covers element sizes 0, 1, 2, 3, 7, 31, 64, 65, and 127; every count from 0
through 257; and 128 additional deterministic random counts for each size.
It also checks the child formulas at their `SIZE_MAX` preconditions and
guards every disposable allocation against multiplication overflow.

The exact commands and key output were:

```text
clang -I src -I include -std=c99 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -DVERIFY /tmp/goal97-hsort.c -o /tmp/goal97-hsort-clang-verify
HSORT_RESULT PASS sizes=9 exhaustive-counts=258 random-cases=128

gcc -I src -I include -std=c99 -O2 -DVERIFY /tmp/goal97-hsort.c -o /tmp/goal97-hsort-gcc-verify
HSORT_RESULT PASS sizes=9 exhaustive-counts=258 random-cases=128

clang -I src -I include -std=c99 -O3 -flto -DVERIFY /tmp/goal97-hsort.c -o /tmp/goal97-hsort-clang-lto
HSORT_RESULT PASS sizes=9 exhaustive-counts=258 random-cases=128

gcc -I src -I include -std=c99 -O3 -flto -DVERIFY /tmp/goal97-hsort.c -o /tmp/goal97-hsort-gcc-lto
HSORT_RESULT PASS sizes=9 exhaustive-counts=258 random-cases=128

clang --analyze -I src -I include -std=c99 -DVERIFY /tmp/goal97-hsort.c
# no diagnostics
```

The Clang O1 run used ASan and UBSan with recovery disabled. GCC O2 was
compiled with `-Wall -Wextra -Wconversion -Wsign-conversion -Wpointer-arith`;
the remaining warnings in the first control were unrelated assignments in
the shared `util.h` byte-order helpers, not hsort findings. The final harness
was corrected to use `size_t` format arguments before these results were
accepted.

The in-tree controls also passed:

```text
./build-integrated-asan/bin/tests --target=utils --iterations=1 --seed=97 --log=1
Test hsort_tests PASSED (0.000 sec)
Test checked_size_mul_tests PASSED (0.000 sec)

./build-integrated-asan/bin/tests --target=ec --iterations=1 --seed=97 --log=1
Test ec_pubkey_parse_test PASSED (0.877 sec)

./build-integrated-asan/bin/tests --target=silentpayments --iterations=1 --seed=97 --log=1
Test test_recipient_sort PASSED (0.063 sec)
```

The full Silent Payments module was not used as evidence because its vector
run exceeded the bounded command window and was terminated with the other
disposable test processes; the targeted production sort test completed
successfully.

### Caller and arithmetic proof

In `secp256k1_heap_down`, the loop condition is `i < heap_size/2`. Therefore
`2*i + 1 < heap_size` and `2*i + 2 <= heap_size`, so the child expressions do
not wrap and the second child is used only after its `< heap_size` check. The
outer hsort loops pass `i-1` from `count/2` or `count-1`, preserving those
conditions. At the pointer layer, a valid qsort-like array of `count`
elements of `size` bytes has a representable object extent; a caller cannot
provide such an object when `count * size` wraps. Both production callers use
fixed pointer-sized strides. `secp256k1_ec_pubkey_sort` additionally checks
every pointer before sorting. The Silent Payments sender checks
`n_recipients > 0`, every recipient/output pointer, and each recipient's
index before calling the sorter at `main_impl.h:207-235,295`.

A 32-bit compile-only attempt was blocked by the host's missing multilib
header `bits/libc-header-start.h`. Clang AArch64 `-O2 -fsyntax-only` passed.
No runtime 32-bit, big-endian, MSVC, or huge-object test was available; the
last is constrained by the same valid-object contract rather than being an
attacker-controlled allocation path.

### Mutation control and verdict

In a disposable copy only, the heap-child comparison was changed from
`0 <= cmp(child2, child1)` to `0 > cmp(child2, child1)`. The same harness
failed immediately with `sort mismatch count=3 size=1`, proving the
independent oracle is sensitive to a realistic heap-selection defect. The
copy was deleted and the audit worktree source was unchanged.

Verdict: **dismissed**. No index wrap, out-of-bounds access, undefined
behavior, comparator contract failure, or reachable caller defect was shown.
The arithmetic guards and valid-array precondition are sufficient for the
current fixed-stride callers. No production source change, warning
suppression, or regression test is justified.

Next queue: retain goal `97` for a different defect class/subsystem cell;
retain goals `77` and `95` in the immediate rotation. Keep the old DataStream
warning cell excluded, and keep the unrelated Boost compiler/dependency
warning as a separate lead rather than merging it with hsort.

## Cycle 106 - Non-owning ASMap lifetime

### Scope and hypothesis

The fresh Goal97 cell targeted lifetime misuse of non-owning views and
iterators after the earlier DataStream diagnostic false positive and secp256k1
heapsort arithmetic cell had been excluded. The concrete candidate was
Bitcoin Core's `NetGroupManager::WithEmbeddedAsmap` in
`src/netgroup.h:24-34`. Before the repair it accepted a `std::span` by value,
stored that span in `m_asmap`, and returned a manager that could outlive the
source container. `GetAsmapVersion()` at `src/netgroup.cpp:14-17` and
`GetMappedAS()` at `src/netgroup.cpp:82-109` dereference that stored span.

The protected Core checkout was at HEAD
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6` with only its pre-existing dirty
paths: `src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log`.
The audit secp256k1 worktree was at `6cb967b5` before its journal updates.
The protected Bitcoin source was not modified. History search found the
factory introduced by `cf4943fdcdd167a56c278ba094cecb0fa241a8f8`,
`refactor: Use span instead of vector for data in util/asmap`; that refactor
deliberately split static embedded data from runtime-owned vectors, but left
the generic span factory callable with a temporary vector. The current dynamic
fuzz target was also calling the embedded factory for a vector, even though
the vector happened to remain alive for that immediate use.

### Independent reproduction

The disposable C++20 probe used the production Core header and ASan/UBSan
Core libraries. Its source hash was
`38056394ace5bb11c5d38e6dac5e438cdfe697880c4d84a37b3e45d219bf154c`, the
object hash was
`3580825f01f94ef8e8ebcee593190ea7c7d240dc2a7aac4dc813a2d94fdfa91c`, and
the linked probe hash was
`56c9da87a0bb69c40b9301df9e6f6fe07cd5450e6adfb3bffcb940745b75e4e5`.
The compile used `/usr/bin/clang++ -O1 -g -std=c++20
-fsanitize=address,undefined -fno-sanitize-recover=all -fPIE` with the
current `build_fuzz` headers and Core libraries. The safe control was:

```text
ASAN_OPTIONS=abort_on_error=1:detect_leaks=0 timeout 15s /mnt/my_storage/goal97-netgroup-probe safe
OWNED_VERSION f98c4e9736d8eb8bb46299798906695c755369a3df99a93ffdded1713f1cf6e2
```

The failing pre-fix invocation was:

```text
ASAN_OPTIONS=abort_on_error=1:halt_on_error=1:detect_leaks=0 timeout 5s /mnt/my_storage/goal97-netgroup-probe
OWNED_VERSION f98c4e9736d8eb8bb46299798906695c755369a3df99a93ffdded1713f1cf6e2
=================================================================
==2086585==ERROR: AddressSanitizer: heap-use-after-free
READ of size 4
```

The source-level trace is `WithEmbeddedAsmap(std::vector<std::byte>(64))` ->
`NetGroupManager(std::span, {})` -> destruction of the temporary vector ->
`GetAsmapVersion()` -> `AsmapVersion(m_asmap)`. The ASan report was bounded
without a symbolized stack by the disposable timeout, but the direct source
trace and reproducible first read identify the dangling span. No probe or
Core test process remained afterward.

An independent compile probe using the patched header accepted lvalue
`std::array` and lvalue `std::span` calls. Reintroducing the exact temporary
vector expression produced this diagnostic and a nonzero status:

```text
error: no matching function for call to 'WithEmbeddedAsmap'
note: candidate function [with T = std::vector<std::byte>] not viable: expects an lvalue for 1st argument
NEGATIVE_COMPILE_STATUS=1
```

### Repair and verification

The minimal repair was made in disposable Core worktree commit
`781bc452ca37cf13e342361eed6369318ab46271`, authored as
`Lőrinc <pap.lorinc@gmail.com>`. `WithEmbeddedAsmap` now takes `T&`, making
temporary containers ill-formed while retaining lvalue static arrays and
spans. The dynamic `src/test/fuzz/asmap.cpp` path now moves its vector into
`WithLoadedAsmap`. A focused `asmap_loaded_data_lifetime` test computes the
version before moving the vector and checks the manager after the source
vector has been destroyed. The repaired source hashes were:

```text
src/netgroup.h             689f8447b50d250c1afa46123af183b4964307a90e9c3574a06bdc61bbed6f9f
src/test/fuzz/asmap.cpp    255c1fb55c4c496f0cb27a6611399abc7e591d562d98e09f24c546f451c9b630
src/test/netbase_tests.cpp ed88ca415ebdf2e63225eeb8028320da6956d3cd76d2928c69ccecdc01a73ea6
```

The exact project validation was:

```text
cmake -S . -B /mnt/my_storage/bitcoin-goal97-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF -DWITH_QT=OFF -DWITH_GUI=OFF
cmake --build /mnt/my_storage/bitcoin-goal97-build --target test_bitcoin -j2
/mnt/my_storage/bitcoin-goal97-build/bin/test_bitcoin --run_test=netbase_tests --log_level=test_suite
Running 17 test cases...
*** No errors detected

cmake -S . -B /mnt/my_storage/bitcoin-goal97-build -DBUILD_FUZZ_BINARY=ON
cmake --build /mnt/my_storage/bitcoin-goal97-build --target fuzz -j2
[100%] Built target fuzz
```

The release fuzz binary was not executed because it correctly requires
`BUILD_FOR_FUZZING=ON` or a Debug build; the complete fuzz target did compile,
including `asmap.cpp`. `git diff --check` passed. The protected checkout
still has exactly its pre-existing dirty paths and no source change.

### Verdict and limitations

Verdict: **confirmed and repaired**. The old API admitted a directly
reachable heap-use-after-free; the new API rejects the dangerous temporary at
compile time and routes runtime fuzz data through the owning factory. The
focused unit suite, complete Release test target, and complete Release fuzz
target build passed. A Debug/`BUILD_FOR_FUZZING` runtime fuzz campaign,
Windows/32-bit execution, and external callers outside the checked-out tree
were not run. The API still requires callers using lvalue embedded storage to
keep that storage alive, as documented by the factory's name and comment.

Next queue: retain Goal97 for a different C/C++ defect class and subsystem;
exclude this ASMap lifetime cell, the DataStream warning, and the hsort cell.
Retain Goals `77`, `82`, `84`, `87`, and `95` in the immediate rotation. Keep
the unrelated Boost warning as a separate dependency/toolchain lead.

## Cycle 109

### Cell and hypothesis

This cycle selected the C/C++ **unchecked-result** class in Bitcoin Core's
chainstate/UTXO database cursor. The fresh hypothesis was that
`CCoinsViewDB::Cursor()` in `src/txdb.cpp:240-257` ignores the boolean result of
the initial `CDBIterator::GetKey(entry)` call. `CoinEntry::key` defaults to the
valid database tag `DB_COIN` (`'C'`), so a malformed key beginning with `C` but
ending before the hash and output index could leave `keyTmp.first == DB_COIN`.
That would make `Valid()` and `GetKey()` report a usable cursor even though the
key was not decoded.

The static path review found that `CDBIterator::GetKey()` catches parse
exceptions and returns false, while `CCoinsViewDBCursor::Valid()` is only
`keyTmp.first == DB_COIN`. The cursor is consumed by UTXO statistics, script
lookup, UTXO-set copying, and snapshot paths, so this was an externally
observable persistence/error-contract issue rather than a dead pattern. The
recent `9972242ce4` `dbwrapper: surface iterator read errors` change was
reviewed: it added LevelDB status handling and iterator tests but did not cover
the higher-level `CCoinsViewDB::Cursor()` decode result.

### Independent reproduction

In disposable current-HEAD Core worktree `/tmp/bitcoin-goal97-109` at base
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, a test-only `TruncatedCoinKey`
serialized exactly one byte, `C`, as a LevelDB key and wrote a valid `Coin`
value. The protected checkout `/mnt/my_storage/bitcoin` was not modified and
retained its pre-existing `src/test/blockencodings_tests.cpp`, `fuzz-0.log`,
and `fuzz-1.log` paths.

The disposable Release test binary was configured with CMake/Ninja and built
successfully. Before the product change:

```text
/tmp/bitcoin-goal97-109-build/bin/test_bitcoin --run_test=coins_tests/coins_cursor_rejects_truncated_key --log_level=test_suite
./test/coins_tests.cpp(1105): error: in "coins_tests/coins_cursor_rejects_truncated_key": check !cursor->Valid() has failed
*** 1 failure is detected in the test module "Bitcoin Core Test Suite"
```

This is the first-invalid-operation proof: the one-byte key reaches the
production cursor, `GetKey()` fails while parsing the missing hash/index, and
the ignored result leaves the default valid tag cached. The same fixture is
independent of the implementation test's expected state and exercises the
actual LevelDB serialization/deserialization boundary.

### Repair and verification

Disposable Core commit `78bea7f913` (`fix: reject malformed initial coins
cursor key`), authored as `Lőrinc <pap.lorinc@gmail.com>`, checks the return of
the initial `GetKey()` and invalidates the cached key on failure. It adds the
minimal regression test without changing the protected checkout.

After the repair, the exact focused test passed:

```text
/tmp/bitcoin-goal97-109-build/bin/test_bitcoin --run_test=coins_tests/coins_cursor_rejects_truncated_key --log_level=message
*** No errors detected
```

The complete `coins_tests` suite also passed:

```text
/tmp/bitcoin-goal97-109-build/bin/test_bitcoin --run_test=coins_tests --log_level=message
Running 14 test cases...
*** No errors detected
```

`git diff --check` passed before the disposable commit. The Core worktree was
clean after the commit; the audit worktree remained clean before this journal
update. The build emitted an existing GCC/Boost `bucket_array` warning in
`txmempool.cpp` and related test translation units; it was unrelated to this
cell and did not fail the build.

### Verdict and limits

Verdict: **confirmed and repaired** for the initial cursor key. A malformed
persistent key could be exposed as a valid UTXO cursor entry because a failed
decode was ignored. The repair converts the failed decode into an invalid
cursor, preventing callers from treating the partially decoded outpoint as
valid. The test covers an on-disk malformed-key fixture, but this cycle did
not fuzz arbitrary LevelDB corruption, exercise every caller under a damaged
database, or change `Next()`'s existing failure-as-end behavior. Those remain
separate hypotheses and must not be conflated with this finding.

### Coverage ledger and next queue

Goal97 class coverage now includes: GCC serialization warning (dismissed),
hsort heap arithmetic (dismissed), NetGroupManager non-owning span lifetime
(confirmed/repaired), and CCoinsViewDB initial cursor key unchecked result
(confirmed/repaired). Exclude all four exact cells. Keep the unchecked-result
class eligible for a different subsystem and retain data race, deadlock,
iterator invalidation, error cleanup, format mismatch, and checked-arithmetic
cells for later cycles. The next controller draw must select a fresh goal or
fresh cell and preserve this exclusion ledger.

## Cycle 111

### Cell and hypothesis

This cycle selected the C/C++ **format-mismatch / invalid-enum decode** class in
Bitcoin Core's Qt serialization boundary. The fresh hypothesis was that
`BitcoinUnit::operator>>` in `src/qt/bitcoinunits.cpp:260-267` accepts an
arbitrary serialized `qint8`, passes it to `FromQint8`, and has no safe
failure contract. The old helper only handled values `0..3`, asserted for
other values, and then fell off a non-void function. The stream operator also
called it after every read without checking whether the stream had supplied a
byte, so a truncated stream could pass an uninitialized byte to the same
helper.

The trust boundary is the public `QDataStream` operator declared in
`src/qt/bitcoinunits.h`; callers can supply corrupted, truncated, or
version-mismatched serialized GUI values. This is a local/API and persisted
configuration boundary, not a consensus or network claim. History showed the
operators were introduced with the QVariant/QSettings BitcoinUnit conversion
in commit `75832fdc37`; no existing Qt test exercised malformed stream data.
The completed Goal97 cells (DataStream warning, hsort arithmetic, ASMap
lifetime, and malformed coins cursor key) were excluded before this scan.

### Independent reproduction

In disposable current-HEAD Core worktree `/tmp/bitcoin-goal97-111` at base
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, a temporary Qt test fed a single
byte `04` to the production `operator>>(QDataStream&, BitcoinUnit&)` with the
destination initialized to `BitcoinUnit::SAT`. The pre-fix Debug GUI test
binary was built with:

```text
cmake -B /tmp/bitcoin-goal97-111-build -DCMAKE_BUILD_TYPE=Debug -DBUILD_GUI=ON -DBUILD_GUI_TESTS=ON -DBUILD_TESTS=ON -DBUILD_DAEMON=ON -DWITH_BDB=OFF -DWITH_SQLITE=ON
cmake --build /tmp/bitcoin-goal97-111-build --target test_bitcoin-qt -j2
QT_QPA_PLATFORM=minimal /tmp/bitcoin-goal97-111-build/bin/test_bitcoin-qt bitcoinUnitStreamRejectsInvalidValue -v2
```

The test reached the production helper and terminated with exit 134:

```text
test_bitcoin-qt: ./qt/bitcoinunits.cpp:251: BitcoinUnit {anonymous}::FromQint8(qint8): Assertion `false' failed.
Received signal 6 (SIGABRT)
```

This is a first-invalid-operation proof that a malformed serialized value is
not rejected by the stream contract; it aborts the GUI test process instead.
An independent static check of the original source, run from the protected
Core checkout without modifying it, was:

```text
git show HEAD:src/qt/bitcoinunits.cpp | clang++ -std=c++20 -DNDEBUG -Wreturn-type -fsyntax-only -Isrc $(pkg-config --cflags Qt6Core Qt6Gui Qt6Widgets) -x c++ -
```

Clang reported:

```text
<stdin>:252:1: warning: non-void function does not return a value in all control paths [-Wreturn-type]
```

The project `Release` profile used for the executable still had assertions
enabled, so the runtime release reproduction remained an abort rather than a
runtime fall-through. The `-DNDEBUG` compiler diagnostic independently
establishes the undefined return path when assertions are disabled. The
truncated-byte branch was also covered in the repaired test; the old source
unconditionally called `FromQint8` after a failed `QDataStream` read.

### Repair and verification

The minimal repair was made in disposable Core worktree commit
`2b6e66dbf8`, authored as `Lőrinc <pap.lorinc@gmail.com>`. `FromQint8` now
reports whether it decoded a valid value. `operator>>` checks the read status
before decoding, sets `QDataStream::ReadCorruptData` for an invalid byte, and
leaves the destination unchanged on both invalid and truncated input. The
temporary regression test checks byte `04` and an empty stream.

After the repair, both Debug and optimized GUI test targets built. The focused
test passed in each build, including both malformed cases. The complete
optimized Qt test executable also passed:

```text
cmake -B /tmp/bitcoin-goal97-111-release -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DBUILD_GUI_TESTS=ON -DBUILD_TESTS=ON -DBUILD_DAEMON=ON -DWITH_BDB=OFF
cmake --build /tmp/bitcoin-goal97-111-release --target test_bitcoin-qt -j4
QT_QPA_PLATFORM=minimal /tmp/bitcoin-goal97-111-release/bin/test_bitcoin-qt -silent
```

The final run reported `URITests::bitcoinUnitStreamRejectsInvalidValue()`
passed, all AppTests, OptionTests, RPCNestedTests, WalletTests, and
AddressBookTests passed, and `All tests passed.` `git diff --check` passed.
The repaired source produced no Clang `-Wreturn-type` diagnostic under the
same `-DNDEBUG` syntax check. The protected Bitcoin Core checkout was not
modified and still has exactly its pre-existing
`src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log` paths.

### Verdict and limitations

Verdict: **confirmed and repaired**. A malformed public stream value could
abort the GUI in assertion builds and invoked undefined behavior in builds
where assertions were disabled; truncated input additionally reached the
decoder with no valid byte. The repair provides a conventional Qt stream
failure status and preserves caller state. The tests cover the direct
QDataStream API and the complete available Qt test executable, but this cycle
did not fuzz actual QSettings files, exercise every external Qt consumer, or
run Windows/32-bit GUI builds. The remaining `ToQint8` assertion is an
internal invalid-enum guard and was not conflated with the untrusted decode
cell.

Goal97 coverage now includes this Qt invalid-enum/stream-status cell as
confirmed/repaired in addition to the four previously closed cells. Exclude
this exact `BitcoinUnit::operator>>` cell from future Goal97 scans; retain
other format mismatches, checked arithmetic, iterator invalidation, error
cleanup, data race, deadlock, and resource-lifetime cells. The next cycle
must select a different subsystem or defect shape.
