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
