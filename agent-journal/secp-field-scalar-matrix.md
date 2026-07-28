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

## Cycle 82: normalized field comparison across limb backends

### Selection and scope

- Draw: `2026-07-28T11:31:30Z`, seed `14643566124539001421`, pool
  `77 82 95 97`, index 1, goal 82. Cycle execution began from the clean
  audit HEAD `5abd9bc579e5ed3bb6704dae87603e4a1ea5ade6`; the protected
  libsecp256k1 checkout and the protected Bitcoin Core checkout were not
  modified.
- The fresh hypothesis was that `secp256k1_fe_cmp_var` could compare the
  representation-specific limbs in the wrong significance order, causing
  native 5x52 and forced 10x26 to disagree for high-bit, near-modulus, or
  adjacent normalized values. The public internal contract is at
  `src/field.h:174-180`: both inputs are valid normalized field elements and
  the result is the integer order in `[0,p)`. The VERIFY wrapper enforces
  normalization at `src/field_impl.h:256-264`; the backend implementations
  scan limbs from their most significant index down (`src/field_5x52_impl.h`
  and `src/field_10x26_impl.h`).
- The helper is not dead representation code. ECDSA verification compares an
  x-coordinate against `p-n` at `src/ecdsa_impl.h:262`, and recoverable ECDSA
  recovery makes the same boundary decision at
  `src/modules/recovery/main_impl.h:131`. Existing unit/fuzz checks include
  comparison behavior, but the current fuzz expected value is derived after
  production serialization. This cycle used the original canonical input
  bytes for the expected order and therefore does not reuse `fe_get_b32` as
  its oracle.

### Independent oracle and schedule

The disposable harness `/tmp/goal82-cmp.c` has SHA-256
`2057083140de22dc2502cb9ddbb4c2befa3185a42bfaf41e713f1f71667ecb45`. It
implements only byte-array comparison, one-subtraction reduction for random
256-bit values, and increment/decrement helpers. Every test input is strictly
less than the field prime, is loaded with `secp256k1_fe_set_b32_limit`, and is
then passed to `secp256k1_fe_cmp_var`; expected results come from comparing
the original 32-byte big-endian values. Guarded field objects check prefix and
suffix canaries, and every pair also checks antisymmetry and reflexivity.

The schedule covered zero, one, `p-1`, `p-2`, `p-3`, high-bit values, low-byte
values, the `p`-adjacent tail, every power of two through bit 255 with its
neighbors where valid, 16 deterministic boundary cross-pairs, and 25,000
deterministic random pairs. The clean digest was stable in every linked
configuration:

    CMP_RESULT PASS pairs=25800 seed=0x8b6f14ea9fbf8b0b

### Backend, compiler, and project evidence

Clang 22.1.7 and GCC 16.1.0 passed native 5x52 and forced 10x26 at `O0`,
`O2`, `O3`, and `Os` where the standalone translation unit linked. GCC native
`O0` required the harness-only linker cleanup flags
`-ffunction-sections -fdata-sections -Wl,--gc-sections` to discard unrelated
unused inline helpers; the forced 10x26 build passed plain `O0`. Clang and GCC
native/forced `-O2 -flto` builds passed. Clang and GCC native/forced
`-O1 -DVERIFY -fsanitize=address,undefined -fno-sanitize-recover=all`
builds passed with no diagnostic. Clang native and forced AArch64
`-fsyntax-only -O2` checks also passed; this is compile-only evidence.

The existing Clang ASan default and no-VERIFY binaries passed the complete
`field` module (10 tests) and `ec` module (4 tests), including comparison
callers and field conversion/overflow cases:

    build-integrated-asan/bin/tests -t=field -i=1 -j=2
    build-integrated-asan/bin/noverify_tests -t=field -i=1 -j=2
    build-integrated-asan/bin/tests -t=ec -i=1 -j=2
    build-integrated-asan/bin/noverify_tests -t=ec -i=1 -j=2

### Mutation and verdict

As an oracle-sensitivity control, the shared VERIFY wrapper was temporarily
changed from `return secp256k1_fe_impl_cmp_var(a, b)` to its sign-negated
result. Both native and forced Clang `O1 -DVERIFY` binaries failed on the
first pair with `mismatch case=0 expected=-1 actual=1` (exit 4). The source
mutation was restored before the clean status check; no production source or
regression-test change is justified.

The normalized comparison hypothesis is **dismissed** for the tested Clang/
GCC x86_64 native 5x52 and forced 10x26 paths, optimization/LTO matrix,
VERIFY and ASan/UBSan controls, AArch64 syntax lowering, and integrated field
and EC tests. No backend order mismatch, canary overwrite, invalid result,
undefined behavior, or reachable production defect was found. The comparator
has no hunk in `git diff origin/master`; the result is master-relative none.

Limitations are no runtime AArch64/GCC-AArch64, big-endian, 32-bit, or MSVC
execution. The schedule loads canonical inputs through `set_b32_limit`, so
the combined parser/order path is tested; the sign-flip mutation isolates the
comparison wrapper itself. Reopen this cell for a new backend/ABI, a changed
normalization contract, or a failing caller. Remove the disposable harness
and binaries after this journal commit; retain this exact command/output
record as the handoff.

## Cycle 108: exhaustive scalar-low arithmetic matrix

### Selection and scope

- The controller selected Goal 82 at `2026-07-28T15:10:30Z` with seed
  `11301387392826193687`, index 1 from pool `77 82 84 87 95 97`. The audit
  branch started clean at `60c6173a9bfd7d0e15f4152804ae3f5a00d86a1b`.
  The protected libsecp256k1 checkout and the protected Bitcoin Core checkout
  were not modified; the latter retained only its documented pre-existing
  `src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log` state.
- Prior Goal82 and translation-validation cycles exercised normal full-order
  4x64 and 8x32 scalar arithmetic, but explicitly excluded the simplified
  exhaustive implementation in `src/scalar_low_impl.h`. This cycle targets
  that remaining representation and its interaction with exhaustive orders 7,
  13, and 199. The production field selector is still exercised separately
  through native 5x52 and forced 10x26 exhaustive binaries.
- The falsifiable hypothesis was that the reduced scalar implementation could
  disagree with its small-order arithmetic contract at a carry/reduction,
  bit-extraction, inversion, halving, or conditional-move boundary. A defect
  would make at least one independent expected value differ or cause an
  assertion/sanitizer diagnostic in the exhaustive configuration.

### Independent scalar oracle

The disposable translation unit
`/tmp/secp256k1-oracles-next/exhaustive-scalar-probe.c` had SHA-256
`1a2b1227790ff70afd07b7d58057d54be8772f58d7d768ce1890b66d972314f5` before
cleanup. It included the production scalar implementation only to invoke the
operations under test. Expected values were computed with ordinary bounded
`uint64_t` arithmetic and explicit small-order formulas, not with another
libsecp field or scalar helper.

For every order it tested all 65,536 `set_int` inputs and every ordered pair
of reduced scalars for addition, multiplication, overflow reporting,
equality, conditional moves, conditional negation, zero/one/even/high
predicates, negation, halving, and both inverse variants. It also exercised
all legal bit offsets and counts for `get_bits_limb32`/`get_bits_var`, every
valid non-overflowing `cadd_bit` case, 128-byte boundary inputs plus one
nonzero byte at every position for `set_b32`, split-128, and input/output
state checks. The `cadd_bit` schedule deliberately stayed within its
documented no-overflow domain; the exhaustive helper's `bit >= 32` VERIFY
assertion is test-only behavior and was not treated as a production defect.

Clang 22.1.7 and GCC 16.1.0 at `-DVERIFY -O2` both printed:

    order=7:   PASS cases=151916 digest=6748598efcc4dcf8
    order=13:  PASS cases=226255 digest=c202980c50466211
    order=199: PASS cases=2888471 digest=96e7ae2f1b8d999d

Clang and GCC order-199 runs at `O0`, `O3`, and `Os` repeated the same
`96e7ae2f1b8d999d` digest. Clang `O1 -DVERIFY -DVALGRIND` with
`-fsanitize=address,undefined -fno-omit-frame-pointer`, leak detection,
halt-on-error, and the order-199 schedule also passed with the same digest
and no diagnostics.

### Exhaustive field/backend evidence

The native CMake exhaustive order-7 binary at
`/mnt/my_storage/secp256k1-build/oracles-next-exhaustive-7/bin/exhaustive_tests`
completed one fixed-seed run with `no problems found`. A fresh Clang build
with `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`, `SECP256K1_ASM=OFF`, and
`-DEXHAUSTIVE_TEST_ORDER=13` completed the same fixed-seed run from
`/mnt/my_storage/secp256k1-build/oracles-next-exhaustive-13-int64/bin/exhaustive_tests`
with `no problems found`. This provides native 5x52 and forced 10x26
exhaustive field coverage around the scalar-low probe. A full order-199
exhaustive group run was started but exceeded the bounded session budget and
was terminated; its result is not counted. The independent scalar oracle
does cover all scalar operations at order 199.

### Mutation and verdict

As an oracle-sensitivity control, `src/scalar_low_impl.h` was temporarily
changed from `(*a * *b) % EXHAUSTIVE_TEST_ORDER` to
`(*a * *b + 1) % EXHAUSTIVE_TEST_ORDER`. Clang `O2 -DVERIFY` order-7 and
order-199 probes both failed immediately at `mul a=0 b=0 actual=1 expected=0`
with exit 1. The source mutation was restored, the disposable translation
unit was deleted, and the audit worktree returned clean.

The exhaustive scalar-low representation hypothesis is **dismissed** for
orders 7, 13, and 199 under the tested Clang/GCC optimization matrix,
Clang ASan/UBSan/VERIFY/VALGRIND, and native/forced-10x26 exhaustive test
configurations. No arithmetic mismatch, stale state, boundary error,
undefined behavior, or reachable production defect was found. No production
or regression-test commit is justified.

Limits are no runtime AArch64, 32-bit, big-endian, MSVC, or GCC sanitizer
coverage; no full order-199 group run; and no claim about the exhaustive
helper's intentionally unsupported `cadd_bit` inputs at or above bit 32.
Exclude this exact scalar-low schedule from future Goal82 work. Keep
unexamined field metadata/backend cells and changed architecture or compiler
evidence eligible; do not repeat the already-fixed malformed opaque-storage
canonicality issue without new source evidence.

## Cycle 110: complete field small-multiplier domain

### Selection and scope

- The controller selected Goal `82`, `secp-field-scalar-matrix`, at
  `2026-07-28T15:58:02Z` with seed `11779137628743502051`, index `1`, from
  the eligible pool `77 82 84 87 95 97`. The audit branch was clean at
  `c32a81c16322d3eca35addaf7f835199321ec527`; its base remained
  `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`. The protected
  libsecp256k1 checkout and protected Bitcoin Core checkout were not modified;
  Core retained its documented pre-existing `blockencodings_tests.cpp`
  modification and `fuzz-0.log`/`fuzz-1.log` files.
- The prior Goal82 arithmetic work covered `fe_add_int`, `fe_half`,
  normalization, inversion, comparison, storage, and a scalar-low exhaustive
  implementation. Existing fuzz history also has the single multiplier-5
  `small multiplication` oracle and uses `mul_int_unchecked` to construct
  raised representations. This cycle therefore targeted the unclosed
  representation contract for `secp256k1_fe_mul_int_unchecked`, rather than
  repeating the multiplier-5 or add-int seeds.
- The falsifiable hypothesis was that one backend could multiply only part of
  its limb representation correctly, or mishandle a legal multiplier at the
  maximum representation magnitude. The wrapper contract at
  `src/field_impl.h:310-320` allows `0 <= a <= 32`, requires
  `a * magnitude <= 32`, multiplies in place, sets the output magnitude to
  that product, and leaves it nonnormalized. The backend loops are at
  `src/field_5x52_impl.h:293-299` and `src/field_10x26_impl.h:364-375`.

### Independent oracle and schedule

The disposable translation unit
`/tmp/secp256k1-oracles-next/field-mul-int-probe.c` had SHA-256
`ad14008fafe06d95839a195743d2b105d61198272bc52abe973f89b52287db00` before
cleanup. It includes the production field implementation only to invoke the
operation under test. Expected values use a separate 32-byte big-endian
algorithm: repeated modular additions of the canonical input, with explicit
carry and subtraction of the field prime. The expected path does not use any
field limb, normalization, or multiplication helper.

The value schedule has 645 canonical inputs: zero, one, two, `p-1`, `p-2`,
every power of two and its `p`-complement, and 128 deterministic random
values below `p`. For magnitude zero it tests all multipliers `0..32` on the
zero element. For each magnitude `1..32`, it constructs the same residue as
the canonical input plus `m-1` independent prime representations using
`fe_add`, then tests every multiplier `0..floor(32/m)`. This produces 97,428
valid cases, including multiplier zero, multiplier 32 at magnitude one, and
all magnitude/multiplier products at the limit 32. Every case checks the
VERIFY metadata transition before normalization and the final canonical
bytes after normalization.

### Backend and compiler evidence

Clang 22.1.7 and GCC 16.1.0 passed native 5x52 and forced 10x26 at `O0`,
`O2`, `O3`, and `Os`. Every run printed:

    MUL_INT_RESULT PASS values=645 cases=97428 digest=36bd699b37d86dd8

Clang and GCC native/forced `O2 -flto` builds printed the same digest. The
native and forced builds with `O1 -DVERIFY -fsanitize=address,undefined
-fno-sanitize-recover=all -fno-omit-frame-pointer`, leak detection enabled,
and halt-on-error also printed the same digest with no sanitizer diagnostic.

The existing integrated Debug field suites were run in both
`/mnt/my_storage/secp256k1-build/current-full-native-20260726` and
`current-full-int64-20260726`, each with `bin/tests -t=field -i=1 -j=2` and
`bin/noverify_tests -t=field -i=1 -j=2`. All four runs exited zero.

### Mutation and verdict

As an oracle-sensitivity control, the native 5x52 implementation was
temporarily changed from `r->n[0] *= a` to
`r->n[0] = r->n[0] * a + (a != 0)`. The focused Clang `O2 -DVERIFY` probe
aborted at `src/field_5x52_impl.h:22` on the internal limb-bound check, so a
single-limb arithmetic defect cannot silently pass the schedule. The source
mutation was restored before the clean replay, which again printed the
expected digest; `git diff --check` passed and no production source remained
modified.

The complete small-multiplier representation hypothesis is **dismissed**
for the tested Clang/GCC x86_64 native 5x52 and forced 10x26 implementations,
O0/O2/O3/Os, LTO, VERIFY, ASan/UBSan, and integrated field/no-VERIFY suites.
No backend mismatch, incorrect magnitude transition, out-of-bounds write,
undefined behavior, or reachable production defect was found. No production
or regression-test commit is justified; master-relative severity is none.

Limits are no runtime AArch64, 32-bit, big-endian, MSVC, or GCC
cross-architecture execution. The independent fixture construction uses the
separate field addition helper to create nonnormalized `m*p` representations,
while the expected output and multiplier sweep are byte-level independent.
Reopen this cell for a new backend, ABI, compiler diagnostic, or caller
contract. Exclude this exact full-domain multiplier schedule from future
Goal82 work and retain other untested architecture/backend cells.
