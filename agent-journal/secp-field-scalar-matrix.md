# secp256k1 field and scalar representation matrix

## Cycle 63: field storage packing and unpacking

### Selection and scope

- Draw: `2026-07-28T06:02:42Z`, seed `3687378918`, pool size 12,
  pool `52 53 72 74 77 81 82 84 87 89 95 97`, index 6, goal 82.
- Branch: `codex/fuzz-oracles`; cycle-start HEAD
  `5f3bee9c8d8f21351e6460024b2f7ea483a254e4`; base
  `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`.
- The audit worktree was clean at selection. Protected libsecp256k1 remained
  detached at `e153e2681f7bf1dd74894e2170213e3983030989`, and protected
  Bitcoin Core remained at `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6` with
  its pre-existing `blockencodings_tests.cpp` modification and `fuzz-0.log`,
  `fuzz-1.log` untracked files.
- Prior `translation-validation.md` cycles already covered field
  normalization, `fe_half`, negation, scalar conversion, predicates, and
  most scalar arithmetic. Existing field fixes `97727abd`, `512370b1`, and
  `3bacc91c` were read and their exact magnitude/zero/half cells were
  excluded from this cycle.

The fresh cell was the representation boundary not independently tested in a
dedicated journal: packing a normalized field element into
`secp256k1_fe_storage` and unpacking it again in both field backends. The
contract is at `src/field_impl.h:377-389`: `to_storage` accepts only a valid
normalized field, while `from_storage` reconstructs a normalized magnitude-1
field. The implementation mappings are at
`src/field_5x52_impl.h:402-415` and `src/field_10x26_impl.h:1118-1140`.

The trust boundary for this cell is a canonical field value. Raw opaque
storage is a separate validation concern: `secp256k1_ge_storage_is_canonical`
at `src/secp256k1.c:262-284` normalizes and reserializes before
`secp256k1_pubkey_load` accepts it. No Bitcoin Core caller directly reaches
these internal storage helpers; Core reaches them through libsecp256k1 public
key operations. Internal libsecp callers include `secp256k1_ge_to_storage`,
`secp256k1_ge_from_storage`, generator precomputation, and public-key loading.

### Independent oracle

The scratch harness `/tmp/secp256k1-goal82-storage.c` has SHA-256
`112b0ce8264971eef0d3f91d92d3e2ee0692f172cac9a67ec40876a678489819`. It
includes the production implementation in one translation unit, but computes
the expected storage bytes independently: the canonical big-endian input is
reversed into the 32-byte little-endian storage image. It does not use the
production field serializer as its expected value.

The 1,283-value schedule contains zero, one, `p-1`, `p-2`, every power of two
through bit 255, and 1,024 deterministic xorshift values reduced into the
field range. Each case:

1. Parses and normalizes the canonical input with `fe_set_b32_limit` and
   `fe_normalize`.
2. Packs into a guarded `secp256k1_fe_storage`, checking 64-bit prefix and
   suffix canaries and all 32 expected bytes.
3. Unpacks with `fe_from_storage`, serializes the resulting normalized field,
   and compares it with the original canonical input.

The little-endian raw-storage oracle is intentionally limited to the tested
little-endian host; it is not evidence about a big-endian ABI.

### Backend and compiler evidence

The direct matrix used Clang 22.1.7 and GCC 16.1.0 with
`-std=c99 -O2 -I src -I include`, compiling once with the native 5x52
selector and once with `-DUSE_FORCE_WIDEMUL_INT64` for 10x26. All four runs
printed the identical result:

```
ok values=1283 digest=3835fa7c29cf43ce
```

Clang native and forced 10x26 runs also passed at O0, O3, and Os. Clang and
GCC native and forced LTO runs (`-O2 -flto`) matched the same digest. The
production precomputed sources were linked into each standalone harness so
the complete internal translation unit was exercised.

The four additional `-O1 -DVERIFY -DVALGRIND
-fsanitize=address,undefined -fno-sanitize-recover=all` runs, covering both
compilers and both field selectors, all printed the same digest with no ASan
or UBSan diagnostic. The negative control compiled with `-DORACLE_MUTATION`
and flipped one expected storage byte; it failed immediately at value 0 with
`storage mismatch`, proving the oracle is sensitive to a packing error rather
than only checking the round trip.

The existing native and forced-int64 Debug CMake builds were rebuilt from the
audit source and ran:

```
ctest --test-dir <build> --output-on-failure -R 'field|fuzz.field'
```

Both produced `100% tests passed, 0 tests failed out of 8`, covering
`noverify_tests` and `tests` for `field_half`, `field_misc`, `field_convert`,
and `field_be32_overflow`. No sanitizer, build, or focused field test left a
failure or artifact.

### Verdict

The field storage representation hypothesis is **dismissed** for the tested
Clang/GCC x86_64 native 5x52 and forced 10x26 paths, O0/O2/O3/Os, LTO,
VERIFY, ASan/UBSan, guarded storage boundaries, canonical edge values, and
the project field tests. There was no cross-backend mismatch, lost bit,
wrong endian word, canary overwrite, undefined behavior, or reachable
production defect. No production source or regression test is justified.

Master-relative severity is none: the exact implementation is shared by
clean upstream and the audit branch, and the independent matrix found no
current-master defect. This does not prove behavior on big-endian hosts,
other ABIs, or noncanonical raw storage. The latter remains a separate
contract/validation cell and must not be silently marked complete from this
round-trip result.

### Handoff

- Keep this storage-packing cell excluded unless field layout, compiler, ABI,
  or a new caller changes.
- Remaining goal-82 work includes field arithmetic not covered by this cell,
  especially storage conditional moves, malformed-storage handling, and any
  new backend or architecture. Do not repeat the already documented
  normalization, `fe_half`, negation, zero-predicate, or scalar-helper cells
  without new evidence.
- Scratch harnesses and logs under `/tmp/secp256k1-goal82-*` are removed after
  the journal commit. The next controller queue remains
  `52,53,72,74,77,81,82,84,87,89,95,97`, with this exact goal-82 cell excluded
  and other goal-82 cells retained.

## Cycle 76: storage conditional-move backend matrix

### Selection and preflight

- Draw: `2026-07-28T09:23:42Z`, seed `585213204`, pool
  `74 77 81 82 84 87 89 95 97`, index 3, goal 82.
- The audit branch was clean after cycle 75 at `fe3c360180e9682572d35a50600ee522f735ffad`;
  its base remained `origin/master`. Protected libsecp256k1 was detached and
  clean at `e153e2681f7bf1dd74894e2170213e3983030989`. Protected Bitcoin Core
  remained at `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6` with only its
  pre-existing `src/test/blockencodings_tests.cpp` modification and
  `fuzz-0.log`/`fuzz-1.log` untracked files. No relevant process was running.
- The prior storage pack/unpack cell is excluded. Existing translation
  validation covered non-storage field/scalar cmov and scalar aliasing, but
  no dedicated randomized `secp256k1_fe_storage_cmov` matrix was found.

### Hypothesis and contract

The backend-specific storage conditional move might select a wrong word or
write outside its 32-byte object for some representation, compiler, or mask
path. The 5x52 implementation stores four `uint64_t` words and uses an
xor-mask; the 10x26 implementation stores eight `uint32_t` words and uses
complementary and/or masks (`src/field_5x52_impl.h:389-400` and
`src/field_10x26_impl.h:1098-1113`). The contract at `src/field.h:312-313`
requires initialized operands and `flag` 0 or 1, retaining `r` for zero and
copying `a` for one. The direct internal consumer is
`secp256k1_ge_storage_cmov` in the constant-time generator-table lookup;
the exact self-alias case is also tested. Partial overlapping storage is not
claimed as a supported contract.

History review found the shared 2015 implementation, the 2023 volatile-flag
hardening (`4a496a36`), and the current 5x52 xor-mask change (`d7a9b2a8`).
No source, issue, PR, or prior journal evidence identified a current storage
cmov defect or a backend-specific semantic difference.

### Independent harness and matrix

The scratch harness `/tmp/goal82-storage-cmov.c` had SHA-256
`6524cbe2a2f4fea6f16bc1d500e06663470386f6a70fd02e680c51731059284b`.
It generated 20,000 deterministic pairs of arbitrary initialized 32-byte
storage values. For each pair it called both flags with distinct operands and
with `r == a`, compared every output byte to an independent `memcpy` oracle,
checked prefix/suffix `uint64_t` canaries, and accumulated a digest. The
expected output is independent of either field representation and does not
parse or reserialize the storage value.

Clang 22.1.7 and GCC 16.1.0, native and
`-DUSE_FORCE_WIDEMUL_INT64=1`, with `-DVERIFY -fsanitize=address,undefined`
and `-fno-sanitize-recover=all`, all passed at `O0`, `O2`, `O3`, and `Os`:

    ok pairs=20000 digest=53424292715b02f4

Clang and GCC native and forced `-O2 -flto` controls printed the same digest.
Compiling with `-DORACLE_MUTATION` to flip one selected output byte caused
both native and forced Clang controls to exit 1 at
`mismatch pair=0 flag=1 kind=distinct`, proving the oracle detects a changed
storage word rather than only checking canaries or a round trip.

The no-VERIFY Clang and GCC x86_64 `O2` probe bodies contained only mask
arithmetic and loads/stores, with no conditional or loop branch. Clang
`--target=aarch64-linux-gnu` compiled native and forced backends at `O0`,
`O2`, `O3`, and `Os`; the O2 helper bodies likewise contained only
subtract/negate, and/or, and stores, with no branch. This is code-generation
evidence, not a formal constant-time proof.

### Integrated controls

Disposable Clang Debug CMake builds with `SECP256K1_ASM=OFF` were configured
once with native detection and once with
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`. Both built `tests` and
`noverify_tests`. In each configuration and binary,
`-t=cmov_tests -i=1 -j=2 -log=1` passed. The same eight controls for
`ecmult_gen_ge` and `ecmult_gen_blind`, which exercise the storage-table
consumer, also passed. The build directories, binaries, mutation outputs,
and object files were removed; no process or artifact remains.

### Verdict and handoff

The selected storage conditional-move hypothesis is **dismissed** for the
tested Clang/GCC x86_64 native and forced 10x26 paths, all four sanitizer
optimization levels, LTO, VERIFY/no-VERIFY integration, and Clang AArch64
compile-only lowering. No wrong word, canary overwrite, undefined behavior,
flag-dependent branch, backend divergence, or reachable production defect was
found. Master-relative severity is none, and no production source or
regression-test commit is justified.

Limitations are no runtime AArch64/GCC-AArch64, big-endian, 32-bit, MSVC, or
formal timing proof. The exact-alias cases pass; partial overlap remains
outside the documented contract. Reopen this cell for a new backend, ABI,
compiler diagnostic, caller contract, or runtime failure. Keep malformed raw
storage validation and other field arithmetic cells in the goal-82 queue.
