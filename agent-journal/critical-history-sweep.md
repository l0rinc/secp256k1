# Critical Whole-History Must-Fix Sweep

## Selection

- Catalog goal: `49`
- Slug: `critical-history-sweep`
- Draw seed: `1744820529`
- Eligible slot: `49` of 97
- Selected on: `2026-07-28`
- Branch at selection: `codex/fuzz-oracles`
- HEAD at selection: `555aef04`
- Base: `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`
- Status: active

## Campaign Scope

Progress from the repository's initial implementation to current HEAD for
reachable critical defect shapes: undefined behavior, memory safety,
remotely triggerable resource exhaustion, consensus or protocol divergence,
funds/key/privacy loss, parser acceptance, races, secret handling, and
critical-check bypasses. Historical commits and advisories are seeds, not
proof. Every candidate needs a current caller/trust-boundary trace, a master-
relative severity assessment, independent reproduction or a rigorous bounded
proof, and a duplicate search before it can become a finding.

This is a continuing bounded investigation, not a claim that the repository
is complete. Use scratch data and fixed inputs, preserve unrelated work, and
commit only a confirmed defect or one focused journal snapshot. High/Critical
claims require a concrete invalid acceptance, consensus divergence, key/funds/
privacy loss, or comparable remotely reachable primitive. A historical bug
shape without current reachability remains a dismissed or inconclusive journal
candidate.

## Cycle 1: historical subgroup parser hardening

### Seed and duplicate search

The selected distinct historical seed was `08d7d89299a6492bf9388b4662b709d268c8ea29`,
"Make pubkey parsing test whether points are in the correct subgroup". Its
September 2020 patch added `secp256k1_ge_is_in_correct_subgroup` and placed
the check in the public EC and x-only parsers. The current tree still has the
same public checks, at `src/secp256k1.c:300-315` and
`src/modules/extrakeys/main_impl.h`, and the current MuSig internal parser
also checks at `src/modules/musig/session_impl.h:35-45`.

The prior-finding search found no exact hash in `src/fuzz/README.md` or the
journals, but it found the same semantic boundary in the existing negative
coverage notes: the cofactor-one curve has no valid non-subgroup production
input, and the Silent Payments opaque-state campaigns already validate
stored labels with both curve and subgroup checks. The historical candidates
`104f53ea` (vulnerability-review edge cases), `2277af5f` (large-count
overflow), `03bfc07b` (uninitialized group doubling), and `4861f836`
(recovered-key infinity) were excluded before this cycle because their
current code paths and regression evidence are already represented by prior
findings.

### Current path and trust boundary

`src/modules/silentpayments/main_impl.h:365-378`, introduced by the current
Silent Payments label API, calls the internal `secp256k1_eckey_pubkey_parse`
and saves the point without an immediate subgroup check. Its paired loader at
lines 356-362 checks curve validity and subgroup membership before using an
opaque label. This is a real contract asymmetry in reduced-order exhaustive
builds, but the current `tests_exhaustive.c` does not register the Silent
Payments test module. The exported code is present in the exhaustive library
build, so it was exercised directly rather than mistaken for exhaustive test
coverage.

The production branch of `src/group_impl.h:926-946` returns true for every
valid point because the real secp256k1 curve has cofactor one. The current
Bitcoin Core checkout has no `silentpayments`, `BIP352`, or
`secp256k1_silentpayments` caller. Its relevant production callers use the
public EC/x-only parsers in `src/musig.cpp` and `src/pubkey.cpp`, which already
apply the subgroup gate. Therefore the missing reduced-order check has no
current network, consensus, wallet, key, or remote production reachability.

### Independent reproduction

The scratch probe was `/tmp/critical-history-49/subgroup_probe.c`; it included
the current source directly with `EXHAUSTIVE_TEST_ORDER=7` and called the
exported Silent Payments parser, the public parser, and the label serializer.
The exact build and run were:

```sh
cc -O0 -g -std=c90 -Wall -Wextra -Wno-unused-function \
  -DEXHAUSTIVE_TEST_ORDER=7 -DENABLE_MODULE_ECDH=1 \
  -DENABLE_MODULE_ELLSWIFT=1 -DENABLE_MODULE_EXTRAKEYS=1 \
  -DENABLE_MODULE_MUSIG=1 -DENABLE_MODULE_SCHNORRSIG=1 \
  -DENABLE_MODULE_SILENTPAYMENTS=1 -DUSE_ASM_X86_64=1 \
  -I/tmp/secp256k1-oracles-next/include -I/tmp/secp256k1-oracles-next/src \
  /tmp/critical-history-49/subgroup_probe.c \
  -o /tmp/critical-history-49/subgroup_probe
/tmp/critical-history-49/subgroup_probe
```

Output:

```text
zero x=0 label=0 public=0 serialize=-1 illegal=0
two x=2 label=0 public=0 serialize=-1 illegal=0
order x=7 label=1 public=0 serialize=0 illegal=1
order+1 x=8 label=1 public=0 serialize=0 illegal=1
```

For an independent arithmetic check, a separate Python big-integer affine
implementation evaluated `y^2 = x^3 + 6` modulo the secp256k1 field and
multiplied the even-parity points by 7. It reported:

```text
x=7 curve=1 parity=0 [7]P_infinity=0 [7]P_x=0x718c02d73752905cf697ddcc6b5d452c3d34cba60641ff67ff1cc475ae751f1
x=8 curve=1 parity=0 [7]P_infinity=0 [7]P_x=0x8ef35be71d9d76fa4b2e39f2ae778d2862ec57b936514318d366b2cdd01cb232
```

This independently establishes that the two accepted label inputs are
finite curve points outside the order-7 subgroup. The public parser rejects
both, and the label serializer rejects the resulting opaque objects through
its existing loader check.

### Supported-build controls

The native and forced-int64 Silent Payments unit slices passed one iteration
each with a fixed seed:

```text
./bin/tests -t=silentpayments -i=1 --seed=0000000000000000000000000000000000000000000000000000000000000049
Total execution time: 23.451 seconds
Total execution time: 23.277 seconds
```

The Clang ASan/UBSan `fuzz_silentpayments` replay loaded all 14 tracked
corpus files and completed 15 runs with exit status 0. The order-7
`exhaustive_tests 1` control completed with `no problems found`; it does not
exercise Silent Payments because that module is not registered in
`tests_exhaustive.c`.

### Verdict

**Dismissed as a current production defect; retain as an exhaustive-only test
mode hardening candidate.** The parser asymmetry is reproducible and would be
worth closing if Silent Payments is added to exhaustive module coverage, but
the production subgroup predicate is tautologically true for every valid
secp256k1 point, and Bitcoin Core has no caller for this optional API in the
surveyed checkout. Adding a production check would be speculative and would
not change supported behavior. No source fix or finding commit is justified.

### Next history slice

Continue from the unexamined historical commits after the excluded parser and
serialization seeds. Prioritize a distinct fix with a current caller before
revisiting reduced-order-only subgroup paths. Preserve this exact probe and
the two arithmetic witnesses if Silent Payments gains exhaustive coverage or
Bitcoin Core adds a caller.

## Cycle 2: ecmult NULL generator versus zero scalar

### Selection and scope

The next history draw used seed `2265895816` over the distinct candidate
prefixes `8479eafa 3a403639 c6306238 d7125e51 89a54b5a`, selecting index 1:
`3a403639dc07e39aa6dc48fbcecfe3cb77f09770`, “eckey: Call ecmult with NULL
instead of zero scalar”. The historical patch changed the tweak-add path in
`src/eckey_impl.h` from
`secp256k1_ecmult(&pt, &pt, tweak, &secp256k1_scalar_zero)` to the semantically
equivalent `... tweak, NULL`, avoiding WNAF setup for an absent generator
term. This cycle tested whether that optimization remains correct and whether
an adjacent caller still relies on the old representation.

The current `src/eckey_impl.h:82-91` already uses `NULL`. In
`src/ecmult_impl.h:253-376`, the generator term is constructed only under
`if (ng)`; a non-NULL zero scalar reaches `secp256k1_wnaf_fixed`, whose zero
case writes no generator digits and returns zero. Thus both inputs should
produce the same result, with `NULL` avoiding the unnecessary setup. The
existing direct internal callers and tests include intentional NULL/zero
comparisons at `src/tests.c:4819-4831` and `6197-6200`; no adjacent current
caller showed a different contract.

### Independent reproduction

The scratch probe `/tmp/critical-history-49/ecmult_null_probe.c` included the
current implementation and linked the generated `precomputed_ecmult.c` and
`precomputed_ecmult_gen.c` tables. It constructed nine point scalars
`{0,1,2,3,7,13,31,127,255}` and ten generator scalars
`{0,1,2,3,7,13,31,127,255,511}`. For every pair it compared
`secp256k1_ecmult(..., scalar, NULL)` with
`secp256k1_ecmult(..., scalar, &secp256k1_scalar_zero)` using the internal
Jacobian equality oracle. The exact command was:

```sh
cc -O0 -g -std=c90 -Wall -Wextra -Wno-unused-function \
  -DUSE_ASM_X86_64=1 \
  -I/tmp/secp256k1-oracles-next/include -I/tmp/secp256k1-oracles-next/src \
  /tmp/critical-history-49/ecmult_null_probe.c \
  /tmp/secp256k1-oracles-next/src/precomputed_ecmult.c \
  /tmp/secp256k1-oracles-next/src/precomputed_ecmult_gen.c \
  -o /tmp/critical-history-49/ecmult_null_probe
/tmp/critical-history-49/ecmult_null_probe
```

Output:

```text
ok pairs=90
```

This is independent of the historical test's random scalar generation and
also covers the zero point and zero generator cases directly. The static
`if (ng)`/zero-WNAF path inspection agrees with the equality result.

### Supported-build controls

Both the forced-int64 and native test binaries passed the relevant slices:

```text
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests -t=point_times_order -i=1
exit 0; Total execution time: 0.104 seconds
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests -t=ecmult -i=1
exit 0; skipped test_ecmult_constants_sha 2048 and test_ecmult_constants_2bit; Total execution time: 6.777 seconds
/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests -t=point_times_order -i=1
exit 0; Total execution time: 0.381 seconds
/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests -t=ecmult -i=1
exit 0; skipped test_ecmult_constants_sha 2048 and test_ecmult_constants_2bit; Total execution time: 68.043 seconds
```

The skipped constant-table checks are iteration-count guards from the test
binary, not failures or hidden errors. The native ecmult run completed after
the full 68.043-second execution.

### Verdict

**Dismissed as a current defect.** The selected historical change is already
present, the current implementation has an explicit NULL-versus-zero
contract, the independent 90-case probe found no output difference, and both
native and forced-int64 ecmult slices passed. No production or test change is
justified. Preserve this cycle as evidence that future searches should focus
on ecmult callers with a nonzero or conditionally initialized generator term,
not reopen the fixed zero-term cleanup.

### Next history slice

Continue with a distinct unexamined historical seed from the remaining
critical-fix queue. Prefer a seed with a current public, persistence,
concurrency, crypto, or untrusted-input caller; retain this probe and the
existing NULL/zero unit assertions as duplicate-search evidence.

## Cycle 3: duplicate search for MuSig counter-nonce cleanup

### Selection and duplicate result

The next draw used seed `3611919104` over the remaining candidate prefixes
`8479eafa c6306238 d7125e51 89a54b5a`, selecting index 0:
`8479eafa5720421d4b7f4b524a35e0a7edf291c7`, “musig: always clear out secret
key in `secp256k1_musig_nonce_gen_counter`”. The historical patch moves
`secp256k1_memclear_explicit(seckey, sizeof(seckey))` after the internal nonce
generator and returns its result, covering the internal failure branch.

This candidate is an exact duplicate of an existing current-branch finding,
not an unexamined variant. `src/fuzz/README.md:4095-4131` records the
“Partial Keypair MuSig Counter-Nonce Oracle” against clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; it explicitly identifies the
same `keypair_load` rejection, the same `nonce_gen_counter` consumer, and the
same cleanup contract. The current fuzzer's
`secp256k1_fuzz_check_musig_nonce_gen_counter_failure_cleanup` at
`src/fuzz/musig.c:4273-4292` covers invalid keyagg-cache failure, while the
partial-keypair helper at `:4356-4391` covers both invalid secret and invalid
public opaque halves. Both require failure, one illegal callback, and zeroed
caller-owned nonce outputs. `git blame` traces the first helper to
`620fc269`, and the partial-keypair extension to `a2788b9d`; both are already
in the prior-finding ledger.

The current implementation at `src/modules/musig/session_impl.h:541` already
clears the derived `seckey` after every reachable
`secp256k1_musig_nonce_gen_internal` return. The pre-load `keypair_load`
failure path cannot have populated `seckey`, and the public wrapper rejects
NULL output/keypair arguments before deriving it. Reopening the historical
patch would duplicate an existing finding and test oracle, so no new
reproduction, source change, or finding commit is justified.

### Verdict

**Deduplicated and dismissed from this campaign queue.** Preserve the README
entry, fuzzer seeds, and current cleanup line as the authoritative evidence;
do not count this historical commit as a new defect.

### Next history slice

Redraw from `c6306238 d7125e51 89a54b5a` and the broader critical-fix queue,
requiring a current caller and a mechanism not already indexed by the fuzz
README or existing journals.

## Cycle 4: exhaustive ECDH test-only history seed

### Selection and scope

The next draw used seed `198155241` over `c6306238 d7125e51 89a54b5a`,
selecting index 0: merge commit
`c63062380f9610084409ac445af723a057a90f6b`, “Merge
bitcoin-core/secp256k1#1852: Add exhaustive test for ECDH module”. Its only
changes add `src/modules/ecdh/tests_exhaustive_impl.h` and register that test
in `src/tests_exhaustive.c`; the test enumerates reduced-order key pairs,
checks ECDH commutativity, and compares the default hash result with a direct
calculation.

This is a test-coverage addition rather than a historical production defect
fix. The current tree contains the same exhaustive helper and ECDH module
registration, and the source diff has no parser, arithmetic, state, cleanup,
or API behavior change to carry into a current trust-boundary hypothesis. The
historical test's purpose is to detect future ECDH regressions, not to document
a known invalid acceptance or security incident. Running it would therefore
re-prove existing coverage rather than validate a must-fix candidate.

### Verdict

**Excluded as non-defect/test-only history.** No independent production
reproduction, mutation, or source change is justified for this commit. It is
retained in the campaign ledger only so later history sampling does not treat
test additions as critical findings.

### Next history slice

Redraw from `d7125e51` and `89a54b5a` only if their diffs indicate a real
behavioral defect; otherwise move to the broader high-risk history queue,
prioritizing parser, scalar-bound, secret-handling, and remotely reachable
protocol fixes not already indexed in `src/fuzz/README.md`.

## Cycle 5: MuSig dead assertion test repair

### Selection and scope

The next draw used seed `3691603698` over `d7125e51 89a54b5a`, selecting
index 0: `d7125e517d45507df4a3f19c8ca90393a8290480`, “test: musig: fix dead
`aggnonce` encodes two points at infinity check”. The one-line change adds
`CHECK(... == 1)` around an existing `secp256k1_ge_is_infinity` call in
`src/modules/musig/tests_impl.h`; it does not change library code, public
outputs, or production validation.

The current tree contains the corrected assertion. The original test already
constructs the two infinity points, so the change only makes a failed
expectation abort instead of discarding the predicate result. This is a test
oracle-strengthening commit, not evidence of a current invalid acceptance,
secret exposure, resource exhaustion, or protocol divergence. The adjacent
`89a54b5a` seed is separately indexed in `src/fuzz/README.md` as latest-master
x-only parity invariant hardening, so neither remaining recent candidate
opens a critical current hypothesis.

### Verdict

**Excluded as test-only maintenance.** No production reproduction or source
fix is justified. The corrected assertion remains useful regression coverage,
but this campaign must not count test-oracle repairs as critical defects.

### Next history slice

Widen the random pool beyond the recent candidate prefixes and select from
older production changes with concrete parser, arithmetic-bound, secret,
protocol, persistence, or remote-caller impact. Exclude the already indexed
`89a54b5a` VERIFY invariant and the test-only `c6306238`/`d7125e51` changes.

## Cycle 6: duplicate DER parser pointer-arithmetic seed

### Selection and duplicate search

The next history draw used seed `4201848889` over this widened seven-entry
pool, with draw index `0` (`4201848889 % 7`):

```text
9be7b0f08340a063d961547b5d2663405f3fc162
3cb057f8429c812b5dbfcd43299658463162b740
ec8f20babd8595f119e642d3833ee90bacc12873
2277af5ff07322dca6efb0474aea863b21d82279
45f37b650635e46865104f37baed26ef8d2cfb97
248f0466112c96b9851c662fa829f20d28d16344
01ee1b3b3c665bf22e06b92afa832ccc54de5119
```

The selected commit `9be7b0f08340a063d961547b5d2663405f3fc162`, "Avoid
computing out-of-bounds pointer", changes the old DER integer guard from
`rlen == 0 || *sig + rlen > sigend` to the subtraction form
`rlen == 0 || rlen > (size_t)(sigend - *sig)`. The source-level mechanism is
valid: the old expression can form an out-of-bounds pointer before rejecting
a malformed length, while the subtraction form tests the remaining range
without pointer addition.

This seed is already an exact semantic duplicate of the current-branch
`cd8c9f17` DER-offset finding. `src/fuzz/README.md:982-989` records the
clean-master `SIZE_MAX` witness, its Low direct-API severity, the caller
array-length contract, and the offset-parser repair. The same witness is
indexed again at `src/fuzz/README.md:31729-31732`. The current
`src/ecdsa_impl.h:36-169` parser tracks `pos` and compares lengths against
`size - pos`; no second production patch or new finding is justified.

### Trust boundary and historical proof

The public entry point is
`secp256k1_ecdsa_signature_parse_der` at `src/secp256k1.c:445-460`. Its
contract requires `input` to reference an array containing `inputlen` bytes.
Therefore a one-byte `{0x00}` buffer paired with `(size_t)-1` violates the
caller-owned array contract and is not a remote DER, memory-corruption, or
cryptographic primitive. It still provides a useful direct-API UB regression:
the historical parser formed `sig + size` before it inspected the first byte.
Bitcoin Core's serialized-signature callers pass actual buffer lengths, and no
invalid-block or invalid-witness acceptance follows from this witness.

The historical parent was checked out in the isolated worktree
`/tmp/critical-history-49/old-der`. Because that old tree generates its
ecmult context, its setup was:

```sh
./autogen.sh
./configure --disable-shared --enable-ecmult-static-precomputation
make -j2 src/ecmult_static_context.h
```

The independent old/current probes were
`/tmp/critical-history-49/der_size_max_probe_old.c` and
`/tmp/critical-history-49/der_size_max_probe_current.c`. Both call the public
parser with `{0x00}`, `(size_t)-1`, and a prefilled opaque signature. The old
parent was compiled with:

```sh
clang -O1 -g -std=c99 -fsanitize=undefined,pointer-overflow \
  -fno-omit-frame-pointer -Wno-unused-function \
  -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 \
  -I/tmp/critical-history-49/old-der/include \
  -I/tmp/critical-history-49/old-der/src \
  /tmp/critical-history-49/der_size_max_probe_old.c \
  -o /tmp/critical-history-49/der_size_max_old
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  /tmp/critical-history-49/der_size_max_old
```

The historical execution emitted the independent sanitizer witness before
the process was stopped after the diagnostic:

```text
/tmp/critical-history-49/old-der/src/ecdsa_impl.h:154:39: runtime error: addition of unsigned offset to 0x5800a39d1c44 overflowed to 0x5800a39d1c43
```

The fixed current probe was compiled with the current generated precomputed
tables and the same sanitizer options. It returned:

```text
ret=0 zero=1
```

An earlier short-form DER fixture under GCC ASan/UBSan also returned
`ret=0 zero=1` on both versions; it did not expose the pointer-overflow
diagnostic because that form stays within the process address range. The
one-byte/`SIZE_MAX` fixture is the sensitive witness and matches the prior
finding exactly.

### Current controls

The focused DER test passed in both available representations:

```text
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests --target=ecdsa_der_parse --iterations=8 --seed=49
Tests running silently. Use '-log=1' to enable detailed logging
iterations = 8
Total execution time: 0.012 seconds

/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests --target=ecdsa_der_parse --iterations=8 --seed=49
Tests running silently. Use '-log=1' to enable detailed logging
iterations = 8
Total execution time: 0.039 seconds
```

The current API round-trip corpus also passed in both fuzzer builds with
`-runs=1 -timeout=60 src/fuzz/corpora/api_roundtrip`. The int64 build loaded
62 files and finished 63 runs with `cov: 6084`, exit 0, in 10 seconds. The
native build loaded the same 62-file corpus and finished 63 runs with
`cov: 4253`, exit 0, in 6 seconds. The corpus includes the existing
`der-input-size-max`, `ecdsa-der-parser-boundaries`, and
`privkey-der-overflow-length` fixtures.

### Verdict

**Deduplicated and dismissed from this campaign queue.** The historical
parent reproduces the exact pointer-overflow UB, current source removes it,
and the independent focused/fuzz controls pass. The mechanism, severity,
trust-boundary qualification, regression oracle, and repair are already
recorded by `cd8c9f17` and the README ledger. Do not create another source or
duplicate finding commit for `9be7b0f0`.

### Next history slice

Continue with the remaining widened pool, but exclude the complete DER length
family (`9be7b0f0`, `3cb057f8`, `ec8f20ba`, and `01ee1b3b`) unless a new caller
or an unindexed contribution parser changes the trust boundary. Prefer
`2277af5f`, `45f37b65`, or `248f0466` only after checking their current
callers and exact semantic hashes against the finding ledger; otherwise draw
from an older unindexed production-impact fix.

## Cycle 7: obsolete fixed-WNAF initialization seed

### Selection and current-surface check

The next draw used seed `2365800521` over the three remaining preferred
history seeds, selecting index `2` (`2365800521 % 3`):

```text
2277af5ff07322dca6efb0474aea863b21d82279
45f37b650635e46865104f37baed26ef8d2cfb97
248f0466112c96b9851c662fa829f20d28d16344
```

The selected commit `248f0466112c96b9851c662fa829f20d28d16344`, "Make sure
we're not using an uninitialized variable in `secp256k1_wnaf_const(...)`",
adds `VERIFY_CHECK(w > 0)` and `VERIFY_CHECK(size > 0)` and changes the old
helper's loop from `while` to `do ... while`. The old code could skip the
loop for `size == 0` and then write `wnaf[word] = u * global_sign` with
`u` uninitialized.

The current surface is materially different. `git log -S'secp256k1_wnaf_const'`
shows that the helper was removed by `115fdc72` (`Remove unused
secp256k1_wnaf_const`). Current `src/ecmult_const_impl.h:123-276` has the
fixed signed-bit table implementation with no `size` parameter and no
`secp256k1_wnaf_const` caller. The current `src/fuzz/README.md:7516-7555`
already records an independent fixed-WNAF byte oracle and mutation proof, and
the later ecmult-const state matrix at `:30865-30935` covers the replacement
implementation. This prevents treating the old helper's historical issue as
a current source variant.

### Historical reproduction and trust boundary

The historical parent `452d8e4d2a2f9f1b5be6b02e18f1ba102e5ca0b4` was checked
out in the isolated worktree `/tmp/critical-history-49/old-wnaf`. Its generated
context setup was:

```sh
./autogen.sh
./configure --disable-shared --enable-ecmult-static-precomputation \
  --disable-tests --disable-benchmark
make -j2 src/ecmult_static_context.h
```

The direct static-helper probe
`/tmp/critical-history-49/wnaf_uninitialized_probe_old.c` initializes a scalar
to one, passes `w = 4` and `size = 0`, and preinitializes two output words.
The successful compile and replay were:

```sh
clang -O0 -g -std=c99 -fPIE -pie \
  -fsanitize=memory -fsanitize-memory-track-origins=2 \
  -fno-omit-frame-pointer -Wno-unused-function -DHAVE_CONFIG_H \
  -I/tmp/critical-history-49/old-wnaf \
  -I/tmp/critical-history-49/old-wnaf/include \
  -I/tmp/critical-history-49/old-wnaf/src \
  /tmp/critical-history-49/wnaf_uninitialized_probe_old.c -lgmp \
  -o /tmp/critical-history-49/wnaf_uninitialized_old_o0
MSAN_OPTIONS=halt_on_error=1:print_stacktrace=1:exit_code=86 \
  /tmp/critical-history-49/wnaf_uninitialized_old_o0
```

MemorySanitizer reported:

```text
==1650175==WARNING: MemorySanitizer: use-of-uninitialized-value
```

The sanitizer process did not terminate cleanly after the warning under this
container's MSan runtime and was stopped with Ctrl-C; the warning occurs at
the old `wnaf[word] = u * global_sign` use. A separate optimized `-O1` MSan
build did not preserve the uninitialized state, so the `-O0` diagnostic is
the retained historical witness. This is a direct internal invalid-domain
reproducer, not a public-input proof: the old production caller passed a
positive `size` (`128` for split values or its positive caller-supplied
width), and no current caller can reach this removed helper.

### Current replacement controls

The current native and forced-int64 unit slices passed:

```text
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests --target=ecmult_const_tests --iterations=4 --seed=248
exit 0; Total execution time: 0.035 seconds
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests --target=wnaf --iterations=4 --seed=248
exit 0; Total execution time: 0.000 seconds
/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests --target=ecmult_const_tests --iterations=2 --seed=248
exit 0; Total execution time: 0.097 seconds
/mnt/my_storage/secp256k1-build/current-full-native-20260726/bin/tests --target=wnaf --iterations=2 --seed=248
exit 0; Total execution time: 0.001 seconds
```

The current 11-file, 406-byte `ecmult_const` corpus passed with
`-runs=1 -timeout=120` in the forced-int64 and native fuzzer builds. Both
completed 12 runs with exit 0 (`cov: 4299` and `cov: 2658`). The current MSan
build replayed the same corpus and completed 12 runs with exit 0 and
`cov: 399`, with no diagnostic. These controls exercise the replacement
fixed-WNAF and table-selection paths, not the removed helper.

### Verdict

**Excluded as obsolete historical hardening.** The historical parent has a
real uninitialized use for an invalid internal `size == 0` call, but the
helper and its call graph were later removed. The current replacement has
independent WNAF oracles, mutation coverage, native/forced-int64 tests, and an
MSan corpus replay. There is no current production defect, caller, or source
change to commit, and no severity gate is met.

### Next history slice

Do not redraw `248f0466` or the removed `wnaf_const` family. The RFC6979
message-reduction and ecmult-multi large-count seeds are already represented
by current transcript/batch ledgers; draw from older unindexed production
history after checking exact semantic fingerprints and current callers.

## Handoff

Verify the current worktree, remotes, history range, existing findings, and
review precedent before the next cycle. Select one distinct history seed or
defect shape, state its trust boundary and severity gate, reproduce it on
clean HEAD, and use an independent oracle or mutation/fixture control. Record
exact commands, output, source commits, duplicate results, limitations, and
the next unchecked history slice before drawing another goal or continuing.

## Cycle 8: public NULL context-destroy contract

### Selection and historical provenance

The forty-third controller cycle continued catalog goal `49`,
`critical-history-sweep`. The random draw used seed `3614493335` over this
ordered pool of older, unindexed production-history candidates:

```text
adec5a16383f1704d80d7c767b2a65d9221cee08
ad52495d723648948970850f01a9445d061e85f7
b0be6aba910392e06aa85a87d2240a1aadb2fff5
603c33bc8079f7e1a4851dbef629a2b91e13bbef
a5759c572ed4948c660a06430b074bbc913fafc6
f4edfc758142d6e100ca5d086126bf532b8a7020
```

`3614493335 % 6 = 5`, selecting
`f4edfc758142d6e100ca5d086126bf532b8a7020`, parent
`0440945fb5ce69d335fed32827b5166e84b02e05`, dated 2020-07-30, with subject
`Improve consistency for NULL arguments in the public interface`. The seed
updates public header comments and `SECP256K1_ARG_NONNULL` annotations across
the API headers. The existing finding ledger has no exact context-destroy
nonnull-contract entry; broad context NULL lifecycle entries and the earlier
ecmult NULL-generator sweep are different code paths and trust boundaries.

### Current contract and reachability

Current `src/secp256k1.c:186-207` implements both
`secp256k1_context_preallocated_destroy(NULL)` and
`secp256k1_context_destroy(NULL)` as defined no-ops. Current
`src/tests.c:366-368` labels the behavior `Defined as no-op` and directly
executes both calls. Historical `9aac008038261ddd865d1461137ffc1b0a6c6646`
is explicitly titled `secp256k1_context_destroy: Allow NULL argument as a
no-op` and removed the nonnull annotation. Later
`4b6df5e33` retained the NULL guard while forbidding destruction of the static
context. In contrast, the current public declarations at
`include/secp256k1.h:330-332` and
`include/secp256k1_preallocated.h:126-128` still carry
`SECP256K1_ARG_NONNULL(1)`. The macro is disabled for internal
`SECP256K1_BUILD` consumers, which explains why the repository test does not
expose the public-header mismatch.

The trust boundary is an external C/C++ consumer compiling against the
installed public headers. A caller may use a NULL-safe cleanup path, while
the declaration rejects that valid call at compile time and gives the
compiler a nonnull assumption. This is an API/toolchain contract defect, not
a consensus, cryptographic, memory-safety, or remotely reachable defect.

### Independent reproduction

Before the patch, this external-consumer probe used the normal shared
library in `/mnt/my_storage/secp256k1-build/oracles-next-int64/lib`:

```sh
printf '%s\n' '#include <secp256k1.h>' \
  '#include <secp256k1_preallocated.h>' \
  'int main(void) { secp256k1_context_destroy(NULL); secp256k1_context_preallocated_destroy(NULL); return 0; }' |
  clang -std=c99 -O2 -Wall -Werror=nonnull -Iinclude \
  -L/mnt/my_storage/secp256k1-build/oracles-next-int64/lib \
  -Wl,-rpath,/mnt/my_storage/secp256k1-build/oracles-next-int64/lib \
  -lsecp256k1 -x c - -o /tmp/f4ed-public-null-error
```

The command exited `1` with two `-Wnonnull` errors, one for each NULL
destroy call. The same source compiled with `-Wno-error=nonnull` and ran
against the normal library with exit `0`; an internal `-DSECP256K1_BUILD`
strict compile and runtime also exited `0`. This isolates the failure to the
external public declaration. The current ASan/UBSan context fuzzer corpus
also passed before the header-only change (`-runs=1`, 13 runs, exit `0`), but
the normal build did not provide a `fuzz_context` binary.

### Fix and controls

The minimal fix removes `SECP256K1_ARG_NONNULL(1)` from both destroy
declarations and documents `If ctx is NULL, this function is a no-op.` in
both public headers. No runtime implementation change was needed.

After the patch, the same public probe compiled and ran with exit `0` under
both Clang and GCC using `-Werror=nonnull`. `git diff --check` passed. The
isolated forced-int64 build was regenerated and rebuilt with:

```sh
cmake --build /mnt/my_storage/secp256k1-build/oracles-next-int64 \
  --target tests -j2
/mnt/my_storage/secp256k1-build/oracles-next-int64/bin/tests \
  --target=all_proper_context_tests --iterations=2 --seed=3614493335
```

The build completed successfully and the focused context test exited `0`.
The Clang and GCC public compile/runtime probes provide independent
compiler-facing verification; the library runtime and internal context test
provide the behavioral controls.

### Verdict

**Confirmed, Low severity.** The implementation and historical contract
define NULL destruction as a no-op, but external users receive a contradictory
nonnull diagnostic and potentially contradictory optimizer assumptions. The
repair is limited to the two public declarations and their documentation.
There is no production runtime behavior change and no broader test-suite
expansion is justified for a header contract whose regression is directly
exercised by the two external compiler probes. The result is not a duplicate
of an existing finding.

### Next history slice

Do not redraw `f4edfc75` or its immediate NULL-annotation migration family.
After the commit, search and draw from the remaining pool beginning with
`ad52495d723648948970850f01a9445d061e85f7`, then
`b0be6aba910392e06aa85a87d2240a1aadb2fff5`,
`603c33bc8079f7e1a4851dbef629a2b91e13bbef`, and
`a5759c572ed4948c660a06430b074bbc913fafc6`, excluding any seed already
covered by the finding ledger or a current-source duplicate.
