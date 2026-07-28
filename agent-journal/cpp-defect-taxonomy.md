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
