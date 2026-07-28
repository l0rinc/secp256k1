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
