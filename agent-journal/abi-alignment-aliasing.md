# ABI layout, alignment, aliasing, and object-lifetime audit

## Cycle 1: 2026-07-28

Status: current hypothesis dismissed; no production source change.

### Controller selection and repository state

- Controller cycle: 56.
- Catalog goal: `92:abi-alignment-aliasing`.
- Draw seed: `8541954469880052534`.
- Eligible pool size: 17; selected index: 12.
- Pool after excluding the active goal campaigns was:
  `52,53,72,73,74,77,81,82,84,87,88,89,92,93,95,97,98`.
- Attachment SHA-256: `1639d16123a404f70037ff15f15464f26fbd7ee0fe363f441064c6dd15f72102`.
- Audit checkout: `/tmp/secp256k1-oracles-next`, branch `codex/fuzz-oracles`, base `origin/master` at `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`, HEAD at cycle start `98a5c8af8df53eda627e64bd44254eac8f86e459`.
- Audit checkout was clean and 1220 commits ahead of `origin/master`; remotes were `origin=https://github.com/bitcoin-core/secp256k1` and `l0rinc=https://github.com/l0rinc/secp256k1`.
- `/mnt/my_storage/secp256k1` was clean, detached at `e153e2681f7bf1dd74894e2170213e3983030989`.
- `/mnt/my_storage/bitcoin` remained untouched with its pre-existing state: branch `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, modified `src/test/blockencodings_tests.cpp`, and untracked `fuzz-0.log`/`fuzz-1.log`.
- No worker, daemon, fuzz, sanitizer, test, benchmark, CMake, Ninja, or Make process was running at the start or end of the cycle.

### Scope and hypotheses

The selected surface was opaque C structs, raw byte storage, scratch allocation alignment, and C/C++ ABI boundaries. The falsifiable hypothesis was that optimized or cross-language callers could observe a real failure from one of these sites:

1. `secp256k1_xonly_pubkey` is explicitly cast to `secp256k1_pubkey` in `src/modules/extrakeys/main_impl.h:14-16` and `:30`.
2. The public-key half of `secp256k1_keypair.data` is viewed as a `secp256k1_pubkey*` in `:181-184` and `:226-234`.
3. Bitcoin Core's `KeyPair` stores 96 bytes in `std::array<unsigned char, 96>` and reinterprets that storage as `secp256k1_keypair*` in `src/key.cpp:409-430`.
4. Scratch allocations and internal serialized storage might lose the alignment or layout required by field/group types on another ABI.

### Contract and history evidence

- `include/secp256k1.h` defines `secp256k1_pubkey` as `unsigned char data[64]` and promises 64 bytes and safe copy/move, while stating that the representation is implementation-defined.
- `include/secp256k1_extrakeys.h` defines `secp256k1_xonly_pubkey` as `unsigned char data[64]` and `secp256k1_keypair` as `unsigned char data[96]`, with the same implementation-defined and safe-copy/move contract.
- The x-only cast sites originate in `4cd2ee474d178bd1b5602486104db346a7562c67` (initial x-only implementation, 2020-05-12). The keypair byte-half casts originate in `58254463f9a2e96d893157a341c9953c440fdf60` (initial keypair implementation, 2020-05-12). The history search found no later compiler, architecture, or ABI regression attached to these sites.
- `src/secp256k1.c:266-298` validates opaque public-key bytes and loads/saves the internal representation; `src/group_impl.h:982-1000` uses `memcpy` and static size assertions for `secp256k1_ge_storage`, rather than aliasing the serialized buffer to an internal field/group object.
- `src/scratch_impl.h:13-105` rounds the header and allocations to the project's 16-byte alignment, checks rounding and size overflow, and rolls back through checkpoints on allocation failure. The candidate casts are from malloc storage, not from a declared object with a stronger conflicting alignment.
- `src/assumptions.h` enforces the project's 8-bit-byte and integer implementation assumptions. All public opaque types in this scope contain only `unsigned char`, so their required alignment is 1; the Core wrapper's `std::array<unsigned char, 96>` has the same storage and alignment requirements.
- Existing `src/modules/extrakeys/tests_impl.h:48-81` intentionally compares/casts the x-only and full public-key byte representations, and `src/fuzz/xonly_tweak.c:1015-1053` exercises malformed opaque parity. Existing later fixes `402fd672` and related keypair-state work validate the logical contents rather than changing the byte-layout mechanism. `86dfec56` separately retracted an unsupported public output/tweak alias oracle; it is not a duplicate of this ABI question.
- Bitcoin Core's actual callers are `KeyPair::KeyPair` and `KeyPair::SignSchnorr` in `src/key.cpp:409-439`, plus direct typed keypair use in `src/musig.cpp` and `src/test/key_tests.cpp:367-392`. These are local wallet/signing paths, not peer or consensus input paths.

### Verification

Scratch-only source `/tmp/abi-92/abi_probe.cpp` compared true typed objects against Core-shaped `std::array<unsigned char, 96>` and byte-backed x-only objects. It ran 4096 deterministic secret keys, 4096 successful deterministic tweaks, secret/public/x-only projections, serialization, and typed-versus-reinterpreted byte equality.

- Clang 22.1.7, `-O3 -fstrict-aliasing -flto`: passed, `abi probe passed: 4096 cases, 4096 successful tweaks`.
- GCC 16.1.0, `-O3 -fstrict-aliasing -flto`: passed with only the LTO serial-compilation note, same result.
- Clang ASan/UBSan with alignment, object-size, and pointer-overflow checks: passed, same result.
- Existing current native focused tests: `current-full-native-20260726/bin/tests -t=extrakeys -log=1`; all 7 x-only/keypair tests passed.
- Existing current forced-wide-multiply `int64` focused tests: `current-full-int64-20260726/bin/tests -t=extrakeys -log=1`; all 7 tests passed.
- Bitcoin Core Release `build/bin/test_bitcoin --run_test=key_tests/* --log_level=test_suite`: all 7 key tests passed and Boost reported `*** No errors detected`. This exercised direct `KeyPair` creation/signing/tweaking and the direct typed API smoke test.
- A clean GCC build with `-O3 -fstrict-aliasing -Wstrict-aliasing=3 -Wcast-align=strict -Wconversion -Wpointer-arith` completed successfully. It emitted 146 unrelated conversion/sign-conversion warnings but no strict-aliasing, cast-alignment, or audited extrakeys diagnostics.
- Host GCC and Clang C11 layout probes passed for 64/96-byte sizes and alignment 1. `gcc -m32` and `clang -m32` could not run because the host lacks `bits/libc-header-start.h`; this is an external toolchain limitation, not a failed ABI result.
- Clang TypeSanitizer was tested as an independent diagnostic. Its instrumented library reported type-aliasing violations in existing field-limb arithmetic during context initialization, first at `src/field_5x52_impl.h:81` via `secp256k1_ge_neg`, before an opaque-key result could be isolated. A minimal reproduction of the exact two byte-array struct cast passed TypeSanitizer and UBSan, so the broad TypeSanitizer output is retained as a tool limitation, not evidence against these casts.

### Verdict

Dismissed as a current defect. The investigated pointers have byte-only layouts and byte alignment; the implementation's actual representation conversion is otherwise guarded by `memcpy` and size checks. Both optimized compilers, ASan/UBSan, native and forced-int64 tests, warning analysis, and the actual Core key callers produced identical successful behavior. No failing-before/passing-after regression, compiler miscompile, alignment fault, or object-lifetime symptom was found.

The C/C++ standard-conformance concern is worth retaining as a maintenance question if a supported compiler supplies a diagnostic specifically for these opaque accesses or a supported non-x86 ABI produces divergent behavior. It is not a confirmed security, wallet, consensus, memory-safety, or production correctness finding on this evidence. No source fix is justified because replacing the casts with extra byte copies would be speculative and would add work to hot key paths without a demonstrated failure.

### Limitations and handoff

- No 32-bit execution was available because the host lacks multilib headers; ARM, big-endian, and Windows execution were not available in this cycle.
- TypeSanitizer could not be used as a clean whole-library oracle because of unrelated field-representation reports.
- The scratch harness and all `/tmp/abi-92` build/probe artifacts are disposable and are not part of the committed repository.
- Reopen this hypothesis only with a new compiler/ABI, a diagnostic localized to `main_impl.h:14`, `:30`, `:183`, or `:228`, or a reproducible output divergence from the Core-shaped storage path.

Next queue: draw a fresh eligible catalog goal, excluding active campaigns `49`, `61`, and `78`, and keep goal 92 out until new ABI evidence appears. Current high-risk unindexed candidates are `52`, `53`, `72`, `73`, `74`, `77`, `81`, `82`, `84`, `87`, `88`, `89`, `93`, `95`, `97`, and `98`.
