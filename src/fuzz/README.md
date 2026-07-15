# secp256k1 Fuzz Oracles

Current upstream `master` did not contain tracked fuzz targets or seed corpora
before this directory was added. These targets are intentionally small public-API
oracles that exercise contract boundaries rather than only maximizing coverage.

Targets:

- `fuzz_api_roundtrip`: compressed/uncompressed/hybrid pubkey wire parsing, two-, three-, four-, eight-, and sixteen-term public-key combine with intermediate-infinity transitions, NULL-member combine cleanup, four-, eight-, and sixteen-key public-key sorting with duplicate-pointer preservation, independent byte-level tweak arithmetic at the order-minus-one boundary, independent ECDSA low-S half-order boundary, ECDSA compact, arbitrary-signature verification equation, fixed- and variable-nonce equations, valid- and invalid-secret nonce callback key- and message-domain checks, valid-nonce retry and post-retry failure cleanup, NULL-argument ECDSA signing cleanup, NULL-output public-key serialization cleanup, empty/NULL/invalid sort, DER, independently parsed private-key DER, signing, verification, normalization
- `fuzz_context`: context randomize, clone, reset, NULL-reset deterministic ECDSA and Schnorr signing, valid legacy-flag matrix, invalid-flag rejection, deterministic signing consistency, and a standalone tagged-SHA reference
- `fuzz_hash`: shared standalone SHA-256 reference, raw-SHA256 HMAC reference, arbitrary multi-block midstate reference, full-stream RFC6979 sequencing, chunking consistency, and finalized-state cleanup
- `fuzz_scalar`: scalar bit-extraction boundaries and rounded multiply-shift
  boundaries against independent byte/product references
- `fuzz_field`: internal field normalization, arithmetic, nonnormalized arithmetic, maximum-magnitude multiplication aliasing, strict input parsing, encoding, field cleanup, add-int boundaries, maximum-magnitude consistency and inversion representation invariance, zero-predicate false-positive barriers, byte-level maximum-residue references, and independent byte-level negation, small-multiplier, add-int, and square-root references
- `fuzz_group`: Jacobian/affine group-operation agreement, independent canonical-coordinate equality, positive and negative Jacobian/affine equality including affine-infinity mismatches, fractional curve-membership, finite and mixed-infinity batch conversion, direct inverse-Z affine conversion, nonnormalized affine-to-storage conversion, normalized and nonnormalized rescale scales, rescale aliasing, invalid opaque public-key operation barriers, lambda-degenerate alternate-slope addition, affine-point cleanup, and state cleanup
- `fuzz_ecmult_const`: constant-time multiplication, affine generator conversion, NULL-generator equivalence, direct odd-multiples-table omitted-Z reconstruction, and normalized/non-normalized rational x-only fractions
- `fuzz_ecmult_multi`: internal scratch/no-scratch multi multiplication consistency, independent serialized-coordinate result equality, false-positive equality barriers, callback batching/failure barriers including a fixed sixteen-point direct batch and distinct three-batch Pippenger transcripts, scratch accounting and checkpoint-prefix preservation, checked allocation multiplication, and defined scalar-state transitions
- `fuzz_ecdh`: ECDH symmetry with a standalone default-SHA reference, coordinate passthrough hashers, built-in callback NULL-input output cleanup, and invalid-scalar callback-point postconditions
- `fuzz_ellswift`: EllSwift encode/decode, modulo-alias wire encodings, randomizer influence, inverse-branch round trips and degenerate rejection guards, an independent BIP324 decode vector and SHA transcript, both-party raw XDH point consistency, XDH symmetry, built-in hash cleanup, built-in callback NULL-input output cleanup, invalid-secret callback-X postconditions, and custom hash callback encoded-party domain checks
- `fuzz_xonly_tweak`: x-only serialization, standalone byte-level curve-membership parsing, parity, tweak, keypair equivalence, invalid keypair-creation cleanup, partial keypair projections and tweak rejection, invalid and NULL full-pubkey conversion, invalid comparator ordering
- `fuzz_recovery`: recoverable ECDSA round trips, arbitrary parsed-signature recovery, independent recovery point equations, zero-`s` recovery rejection, no-curve-point recovery failure cleanup, nonce callback key- and message-domain checks, valid-nonce retry, and post-retry failure cleanup when recovery is enabled
- `fuzz_schnorrsig`: Schnorr sign/verify, standalone BIP340 tagged-SHA reference, arbitrary-signature BIP340 verification equation, empty-message pointer equivalence, `sign32`/`sign_custom` equivalence, nonce callback message-domain checks, signing precondition cleanup, and an independent BIP340 point-equation model
- `fuzz_musig`: MuSig key aggregation, zero-length key/nonce/partial-signature aggregation boundaries, one- through sixteen-key independent coefficient transcripts, valid duplicate-key first-distinct coefficient transcripts, optional aggregate outputs, opaque cache curve/state barriers, tweak equivalence, x-only-tweak signing, standalone tagged-SHA transcripts, one- through sixteen-signer nonce/signature round trips, consumed-secnonce reuse rejection, failure-path secnonce invalidation, zero secret-nonce scalar load rejection, second secret-nonce scalar overflow rejection, NULL-argument partial-sign cleanup, NULL-member nonce/final-signature aggregation cleanup, counter-nonce optional-input equivalence, partial-keypair counter-nonce rejection, optional-secret-key nonce-input equivalence, deterministic zero-derived-nonce failure, second derived-nonce scalar zero rejection, mixed-infinity effective-nonce modeling, NULL-input and invalid-cache nonce-process cleanup, arbitrary parseable partial-signature verification equations, invalid opaque partial-signature verification state, and independent partial- and final-signature point equations

Standalone corpus replay:

```sh
cmake -B build-fuzz -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_BUILD_TESTS=ON
cmake --build build-fuzz -j"$(nproc)"
ctest --test-dir build-fuzz -L secp256k1_fuzz --output-on-failure
```

Autotools standalone build:

```sh
./autogen.sh
mkdir -p build-autotools-fuzz
cd build-autotools-fuzz
../configure --enable-fuzz --enable-module-recovery
make -j"$(nproc)" fuzz_api_roundtrip fuzz_context fuzz_hash fuzz_scalar fuzz_field fuzz_group fuzz_ecmult_const fuzz_ecmult_multi fuzz_ecdh \
  fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig
make check TESTS="fuzz_api_roundtrip fuzz_context fuzz_hash fuzz_scalar fuzz_field fuzz_group fuzz_ecmult_const fuzz_ecmult_multi fuzz_ecdh fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig"
```

ASan/UBSan replay:

```sh
CC=clang cmake -B build-fuzz-asan -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_APPEND_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DSECP256K1_APPEND_LDFLAGS="-fsanitize=address,undefined"
cmake --build build-fuzz-asan -j"$(nproc)"
ctest --test-dir build-fuzz-asan -L secp256k1_fuzz --output-on-failure
```

libFuzzer multi-worker run:

```sh
CC=clang cmake -B build-fuzz-libfuzzer -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_FUZZ_USE_LIBFUZZER=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_APPEND_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DSECP256K1_APPEND_LDFLAGS="-fsanitize=address,undefined"
cmake --build build-fuzz-libfuzzer -j"$(nproc)"
mkdir -p fuzz-work/api_roundtrip
cp src/fuzz/corpora/api_roundtrip/* fuzz-work/api_roundtrip/
build-fuzz-libfuzzer/bin/fuzz_api_roundtrip \
  -print_funcs=0 -workers=4 -jobs=4 -max_total_time=60 \
  fuzz-work/api_roundtrip
```

Extended core oracle campaign (2026-07-13): from the clean-master baseline
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the ASan/UBSan libFuzzer build
replayed the repository corpora for `fuzz_api_roundtrip`, `fuzz_ecmult_multi`,
`fuzz_field`, and `fuzz_musig` with
`-workers=4 -jobs=4 -max_total_time=300`. Each target therefore ran four
independent workers for
roughly five minutes; the workers executed 10,341, 2,551, 317,703, and 1,122
inputs respectively. High-water coverage was 3,125, 2,624, 1,342, and 3,415
edges in the same order. Every worker emitted its normal `Done` and final
statistics record. No ASan/UBSan diagnostic, assertion failure, timeout, OOM,
crash artifact, or nonzero worker result was observed. This is additional
negative evidence for the current oracles, not a claim that the tested branch
or a later fork patch proves clean-master behavior safe.

The same ASan/UBSan campaign was run from the original repository corpora for
the remaining targets with `-workers=4 -jobs=4 -max_total_time=300` per target
and four independent workers per target. The 2026-07-13 module batches
completed as follows:

- `fuzz_context`, `fuzz_ecdh`, `fuzz_ellswift`, and `fuzz_xonly_tweak` ran
  23,741, 9,315, 4,586, and 25,897 inputs respectively, with high-water
  coverage of 2,618, 2,213, 2,448, and 2,274 edges.
- `fuzz_hash`, `fuzz_scalar`, `fuzz_group`, and `fuzz_ecmult_const` ran
  1,090,494, 32,867, 26,993, and 14,790 inputs respectively, with high-water
  coverage of 302, 1,722, 2,357, and 2,143 edges.
- `fuzz_recovery` and `fuzz_schnorrsig` ran 37,997 and 13,218 inputs, with
  high-water coverage of 2,628 and 2,773 edges.

Every worker emitted its normal `Done` and final-statistics record. No
ASan/UBSan diagnostic, assertion failure, timeout, OOM, crash artifact, or
nonzero worker result was observed across these batches. These runs add
negative evidence for the current oracles and do not change any
master-relative severity rating.

Focused recoverable ECDSA point-equation replay (2026-07-13): the recovery
corpus now has 7 tracked inputs totaling 436 bytes, including
`recovery-point-equation`, `arbitrary-recovery-equation`,
`arbitrary-recovery-failure-cleanup`, and `recovery-r-plus-n-equation`. The
two arbitrary seeds are 111-byte printable inputs sharing an accepted first
64-byte `(r,s)` pair: byte 109 is `d` (`recid == 0`) for successful recovery
and `f` (`recid == 2`) for a recoverable-point failure. The fixed `(4,4)`
vector drives successful `recid` 2/3 recovery through the `r + n` field
branch. The independent model reconstructs candidate `R` from serialized `r`
and `recid`, reduces the message scalar with byte arithmetic, and checks
`rQ = sR - zG` using public point operations rather than
`secp256k1_ecdsa_verify` or the recovery implementation's internal
multiscalar path. It now covers signer-generated and arbitrary parsed
signatures, the successful `r + n` branch, and failed-recovery output
cleanup. Default and forced-int64 ASan/UBSan fixed replays each executed all
7 inputs plus the empty input, reaching 2,654 and 4,807 edges; matching MSan
replays completed without a diagnostic. Focused
`-workers=2 -jobs=2 -max_total_time=20` runs completed two jobs per backend
with exit code 0: default jobs executed 298 and 303 inputs at 2,663 edges,
while forced-int64 jobs executed 180 and 180 at 4,815 edges. The equation
isolation proof used a temporary production mutation that skipped
`secp256k1_scalar_negate(&u1, &u1);` only for the arbitrary seed's
`recid == 0` and top-16-bit `r == 0x564e` condition; with the delegated
`ecdsa_verify` check disabled, the new equation aborted with exit 134 on both
backends, while disabling the equation let the same mutation pass with exit
0. The failure-state proof omitted `memset(pubkey, 0, sizeof(*pubkey))` only
for the failing seed's `recid == 2` and the same `r` prefix; the new assertion
aborted with exit 134 on both backends, while disabling it let the mutation
pass with exit 0. Finally, omitting the `r + n` field addition only for
`(r,s)=(4,4)` and `recid` 2/3 aborted in the fixed equation on both backends;
disabling only that helper let the mutation pass. All temporary changes were
restored before the clean replays. This is informational oracle hardening,
not a current-master production finding, and does not change any severity
rating.

Cross-backend campaign (2026-07-13): a separate ASan/UBSan libFuzzer build
used `-DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`, selecting the 10x26
field implementation on this platform. The original corpora were replayed
with `-workers=4 -jobs=4 -max_total_time=300` for each target. The arithmetic
batch ran 191,344 `fuzz_field`, 11,284 `fuzz_scalar`, 15,817 `fuzz_group`, and
1,445 `fuzz_ecmult_multi` inputs, reaching high-water coverage of 2,628,
3,482, 4,298, and 4,470 edges. The public/module batch ran 6,052
`fuzz_api_roundtrip`, 627 `fuzz_musig`, 10,847 `fuzz_recovery`, and 3,751
`fuzz_schnorrsig` inputs, reaching 5,290, 5,556, 4,760, and 4,840 edges.
All eight campaign managers returned exit code 0, and all 32 worker logs
emitted final statistics. No
ASan/UBSan diagnostic, assertion failure, timeout, OOM, crash artifact, or
nonzero worker result was observed. The different coverage is useful
cross-implementation evidence, but this clean run does not replace a
master-relative mutation proof or change any severity rating. The matching
forced-int64 `tests` executable also completed its full 16-iteration
deterministic suite with exit code 0 in 407.898 seconds.

Differential clean-master replay (2026-07-14): a disposable worktree at clean
`origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297` received only the
fuzz sources and build wiring from this branch. Two private helper names used
by the newer harness (`SECP256K1_SHA256_MAX_SIZE` and `checked_size_mul`) were
defined in that overlay solely to compile the oracle; no production fix from
this branch was copied into it. The Clang ASan/UBSan standalone build then
replayed focused inputs against both trees. The fixed tree passed every
tracked input for all 14 registered targets. Clean master reproduced the
existing invalid opaque-state, callback-boundary, RFC6979 length, ECDH load,
and EllSwift zero-`u` stops before the corresponding repaired oracles could
run. Those multi-oracle stops are corroboration, not new independent bug
claims; the mutation-backed commit proofs below remain authoritative for
severity.

The field result was isolated by backend. Clean master passed the two focused
field seeds under its default 5x52 backend, but the forced-int64/10x26 build
aborted the dedicated `zero-predicate-false-positive` and magnitude-32
replays with exit 134. The fixed forced-int64 tree passed all 15 tracked field
inputs under ASan/UBSan. This preserves the existing **Medium/latent** rating
for the 10x26 false-zero production defect and keeps it distinct from the
separate magnitude-32 normalization repair and from the informational field
oracle hardenings.

The upstream exhaustive ECDH module model was also replayed on 2026-07-13.
Default ASan/UBSan order-13 and order-7 binaries, plus the forced-int64/10x26
ASan/UBSan order-7 binary, each ran two iterations across every reduced-order
key combination and reported exit code 0 with `no problems found`. This is
reduced-model and cross-backend evidence only; it does not change any
master-relative severity rating.

Focused ECDH invalid-scalar callback replay (2026-07-13): the ECDH corpus now
contains 5 tracked inputs totaling 189 bytes, including
`invalid-scalar-callback-point`. The implementation deliberately replaces a
zero or overflowing scalar with one before invoking a custom hash callback;
the new oracle records the callback's `x32` and `y32` and compares them with
the serialized input point for zero, `n`, and `n+1`. The previous harness
checked only that the callback ran and that built-in failure output was zero,
so it would not detect the callback receiving the infinity state after a
fallback regression. Clean focused replay passed on default, forced-int64,
and MSan builds. The complete fixed MSan replay passed all 5 seeds plus the
empty input. Default and forced-int64 isolated
`-workers=2 -jobs=2 -max_total_time=15` campaigns ran 2 jobs each and exited
0; the jobs executed 226/225 and 131/132 inputs and reached 2,154 and 4,090
edges respectively. For the independence proof, the production
`secp256k1_scalar_cmov(&s, &secp256k1_scalar_one, overflow)` fallback was
temporarily disabled. The focused seed aborted with exit 134 on default,
forced-int64, and MSan; disabling only the new callback-point helper made the
identical mutation pass with exit 0. All temporary changes were restored.
This is informational oracle hardening, not a current-master production
finding, and does not change any severity rating.

Focused EllSwift invalid-secret callback replay (2026-07-13): the EllSwift
corpus contains 10 tracked inputs totaling 536 bytes, including
`invalid-secret-callback-x`. Clean master deliberately substitutes scalar one
for zero, `n`, and overflowing `n+1` secrets before invoking the custom hash
callback. The new oracle records the callback X coordinate and compares it
with the compressed X coordinate of the selected remote EllSwift encoding for
both `party` values. The prior target checked only return values, output
cleanup, and callback-derived output, so a wrong non-infinity fallback could
pass without proving which point was hashed. Clean focused replay passed on
default, forced-int64, and MSan builds; the complete fixed MSan replay passed
all 10 corpus inputs with no diagnostic. Isolated default and forced-int64
`-workers=2 -jobs=2 -max_total_time=15` campaigns exited 0 without artifacts;
their jobs executed 97/98 and 58/59 inputs and reached 2,508 and 4,572 edges.
For the independence proof, the production scalar-one fallback was
temporarily replaced with scalar two only for invalid secrets. The focused
seed aborted with exit 134 on all three builds; disabling only the new
callback-X helper made the identical mutation pass with exit 0 on all three.
All temporary changes were restored. This is informational oracle hardening,
not a current-master production finding, and does not change any severity
rating.

Focused EllSwift custom-hash encoded-party replay (2026-07-14): the EllSwift
corpus contains 11 tracked inputs totaling 551 bytes, including
`bip324-independent-reference`. The masked custom callback now checks that
`secp256k1_ellswift_xdh` forwards the exact 64-byte `ell_a64` and `ell_b64`
encodings documented by the API, in addition to checking its shared-X and
output domains. For the differential proof, the arbitrary-callback branch in
`secp256k1_ellswift_xdh` was temporarily changed to pass `ell_b64` for both
encoded-party arguments while retaining the correct remote encoding for
shared-X computation. The focused seed aborted with exit 134. Removing only
the two new encoded-party `memcmp` checks let all 11 corpus seeds pass under
the same production mutation, proving that the prior X-coordinate, point,
symmetry, and output checks did not detect this callback transcript
regression. The mutation and oracle bypass were restored before replay.
Default and forced-int64 two-worker/two-job 15-second campaigns exited 0
without sanitizer diagnostics or artifacts, and the native GCC EllSwift
corpus test passed. This is informational oracle hardening, not a
current-master production finding, and does not change any severity rating.

MemorySanitizer corpus campaign (2026-07-13): a clang build with
`-fsanitize=memory -fsanitize-memory-track-origins=2` was linked and runtime
checked with `fuzz_ecmult_multi` before replay. All 14 tracked corpora were
then replayed once: 115 corpus files resulted in 129 total libFuzzer
executions, with no MSan diagnostic, assertion, crash artifact, or nonzero
target result. The unfiltered MSan `tests` binary separately stopped at the
existing `rfc6979_hmac_sha256_tests` check in `src/tests.c:874`: the test
deliberately reads an object after `secp256k1_memclear_explicit` has marked it
undefined under `VERIFY`, so this is the intended use-after-clear detector,
not a production uninitialized read. Re-running the other 111 registered
tests with `-i=1` and the same fixed seed passed in 145.194 seconds. This
limitation is recorded so future MSan runs do not misclassify the deliberate
poison check as a new production finding.

The same MSan build then ran generated-input campaigns with
`-workers=2 -jobs=2`. `fuzz_api_roundtrip`, `fuzz_ecmult_multi`, `fuzz_ellswift`,
and `fuzz_musig` ran for 30 seconds per manager; the other ten targets ran for
15 seconds per manager. Across all 14 targets the managers completed 330,620
executions, with no MSan diagnostic, assertion, crash artifact, or nonzero
worker result. This is useful negative evidence against sanitizer-visible
undefined-state propagation, but it is not a proof that arbitrary future
mutations are safe and does not change any master-relative severity.

Cross-backend MemorySanitizer campaign (2026-07-13): the same instrumented
corpus replay was rebuilt with `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`,
selecting the 10x26 field backend. All 14 corpora replayed successfully with
no MSan diagnostic, assertion, crash artifact, or nonzero target result. The
arithmetic and state-heavy subset (`fuzz_api_roundtrip`, `fuzz_field`,
`fuzz_group`, `fuzz_ecmult_const`, `fuzz_ecmult_multi`, and `fuzz_musig`) then
ran with `-workers=2 -jobs=2 -max_total_time=20`, completing 134,983 generated
executions with the same clean result. This cross-backend evidence does not
replace the clean-master mutation proofs or change any severity rating.

Current-branch oracle replay (2026-07-13, after `c7cabce`): the default
ASan/UBSan libFuzzer binaries replayed all 14 isolated copies of the tracked
corpora with `-workers=2 -jobs=2 -max_total_time=60` per target. Every target
manager and worker completed with exit code 0. No sanitizer diagnostic,
assertion failure, timeout, OOM, crash artifact, or nonzero worker result was
observed, including the new `api_roundtrip/null-pubkey-sort` seed. LibFuzzer
was allowed to expand only the temporary copies; the tracked source corpora
were unchanged. This is fresh negative evidence for the current oracle set,
not a production finding or a replacement for master-relative mutation proof.

Focused group-boundary replay (2026-07-13, after the mixed-infinity oracle):
the default ASan/UBSan `fuzz_group` binary ran two independent managers with
`-workers=2 -jobs=2 -max_total_time=30` over an isolated copy of all six group
seeds. Both managers exited 0 after 1,359 executions, reaching high-water
coverage of 2,365 edges. No sanitizer diagnostic, assertion failure, timeout,
OOM, crash artifact, or nonzero worker result was observed. The temporary
corpus was discarded; tracked seeds were unchanged.

Focused MuSig mixed-infinity replay (2026-07-13): the copied MuSig corpus
started with its 29 tracked inputs. The restored ASan/UBSan `fuzz_musig` target
then ran two 30-second managers with `-workers=2 -jobs=2`; the jobs executed 57
and 58 inputs and reached high-water coverage of 3,424 and 3,425 edges. Both
returned exit code 0 without a sanitizer diagnostic, assertion failure,
timeout, OOM, crash artifact, or nonzero worker result. A separate MSan replay
ran `aggregate-no-outputs`, `infinity-nonce-final-verification`, and
`noncecoef-reference` once each with the new mixed-state checks; it also
completed without a diagnostic. LibFuzzer mutations were kept in `/tmp` and
the tracked corpus was unchanged.

Focused Schnorr equation replay (2026-07-13): the copied Schnorr corpus
started with 9 tracked inputs. The restored ASan/UBSan `fuzz_schnorrsig` target
replayed all 9 inputs once, then ran two 30-second managers with
`-workers=2 -jobs=2`; the jobs executed 320 and 324 inputs and reached
high-water coverage of 2,773 and 2,774 edges. Both returned exit code 0 without
a sanitizer diagnostic, assertion failure, timeout, OOM, crash artifact, or
nonzero worker result. A separate MSan replay executed four representative
inputs, including variable-length and empty messages, with no diagnostic. The
temporary corpus and artifacts were kept outside the repository.

Focused arbitrary-signature Schnorr verification replay (2026-07-13): the
Schnorr corpus now contains 10 tracked inputs totaling 397 bytes, including
`arbitrary-signature-verification-equation`. Default ASan/UBSan and forced-int64
replayed all inputs plus the empty input for 11 executions, reaching 2,741 and
4,776 edges; matching MSan replays reached 570 and 575 edges without a
diagnostic. Isolated default and forced-int64 managers then ran with
`-workers=2 -jobs=2 -max_total_time=30`; both jobs on each backend returned
zero without sanitizer diagnostics, assertion failures, timeouts, OOMs, crash
artifacts, or nonzero worker results. Temporary corpus mutations and worker
logs stayed outside the repository.

For mutation proof, the production Schnorr verifier's `s >= n` overflow branch
was temporarily changed to return success for the all-`0xFF` response scalar,
and the pre-existing all-`0xFF` negative assertion was bypassed only for
isolation. The independent point-equation reference aborted the focused seed on
both backends. A control run with the same production mutation and old
assertion bypass, but the new reference disabled, passed on both backends. All
temporary changes were restored before the clean replay. This is informational
oracle hardening, not a current-master production finding, and does not change
any severity rating.

Focused Schnorr nonce-callback message-domain replay (2026-07-14): the
restored forced-int64 Clang ASan/UBSan target passed all 11 tracked Schnorr
inputs, including `src/fuzz/corpora/schnorrsig/sign32-custom`. The checked
custom nonce callback now verifies the exact message bytes and length supplied
by `secp256k1_schnorrsig_sign_custom`, in addition to its existing key, x-only
key, and algorithm-domain checks.

For the differential proof, `src/modules/schnorrsig/main_impl.h` was
temporarily changed so that, when `msglen == 32`, both the default and custom
nonce paths receive the normalized secret-key buffer as their message while
the signing challenge continues to use the real message. The focused seed
aborted with exit 134. Removing only the new message assertion let all 11
Schnorr seeds pass under the identical production mutation: signature
equivalence and the independent BIP340 equation remained green because both
nonce paths saw the same wrong input, while the direct nonce transcript
reference does not exercise `sign_internal`'s callback argument routing. The
mutation and assertion bypass were restored before replay.

The fixed corpus passed all 11 seeds, and a restored two-worker/two-job
ASan/UBSan campaign completed both jobs with 73 executions each, exit code 0,
and no sanitizer diagnostics, assertion failures, timeouts, OOMs, or artifacts.
Native GCC CTest passed all 239 tests. A matching MSan build was produced but
could not start in this environment because MemorySanitizer could not map its
shadow memory or disable ASLR; no MSan result is claimed. This is informational
oracle hardening, not a current-master production finding, and does not change
any master-relative severity rating.

Focused field maximum-residue replay (2026-07-13): the copied field corpus
started with 4 tracked inputs. Default ASan/UBSan managers with
`-workers=2 -jobs=2 -max_total_time=30` executed 16,520 and 16,429 inputs,
reaching 1,338 and 1,338 inline-coverage edges. The restored forced-int64
10x26 managers executed 10,004 and 9,979 inputs, reaching 2,624 and 2,624
edges. All four inputs also passed an explicit int64 MSan replay. Every job
returned exit code 0 without a sanitizer diagnostic, assertion failure,
timeout, OOM, crash artifact, or nonzero worker result; mutated corpora and
artifacts stayed outside the repository.

Focused variable-nonce ECDSA replay (2026-07-13): the `api_roundtrip` corpus
started with 23 tracked inputs totaling 893 bytes, including
`ecdsa-variable-nonce-equation`. The default ASan/UBSan and forced-int64
10x26 binaries replayed the corpus once with 24 total libFuzzer executions
and final coverage of 3,135 and 5,300 edges respectively. Both matching MSan
replays executed all 23 tracked inputs without a diagnostic. Isolated default
ASan/UBSan managers then ran for 30 seconds with `-workers=2 -jobs=2`,
executing 523 and 522 inputs and reaching 3,141 edges; forced-int64 managers
executed 315 and 312 inputs and reached 5,308 edges. Every job returned zero
with no sanitizer diagnostic, assertion failure, timeout, OOM, or artifact.
The temporary mutated corpora and worker logs were kept outside the tracked
corpus.

Focused ECDSA nonce-callback key-domain replay (2026-07-14): the restored
forced-int64 Clang ASan/UBSan target passed all 29 tracked `api_roundtrip`
inputs, including the existing `ecdsa-variable-nonce-equation` seed. The
custom RFC6979 passthrough callback now independently checks that the signer
passes the exact secret-key bytes supplied to `secp256k1_ecdsa_sign`; the
ordinary signature comparison only proves that two nonce paths agree and can
therefore miss the same wrong callback context being supplied to both paths.

For the differential proof, `secp256k1_ecdsa_sign_inner` was temporarily
changed to pass one all-zero 32-byte buffer to both the default and custom
nonce callbacks while retaining the real signing scalar. The focused seed
aborted with exit 134. Removing only the new callback `memcmp` let all 29 API
seeds pass under that identical production mutation, proving the earlier
signature-equivalence, retry, and independent equation checks did not detect
the callback-domain regression. The mutation and assertion bypass were
restored before replay. The fixed corpus passed all 29 seeds, and a restored
two-worker/two-job ASan/UBSan campaign completed both jobs successfully
without sanitizer diagnostics, assertion failures, timeouts, OOM, or artifacts.
This is informational oracle hardening, not a current-master production
finding; a real failure would be a callback-context or nonce-domain
regression, not a direct key compromise, so no master-relative severity
rating changes.

Focused ECDSA nonce-callback message-domain replay (2026-07-14): the restored
forced-int64 Clang ASan/UBSan target passed all 29 tracked `api_roundtrip`
inputs, including `ecdsa-variable-nonce-equation`. The custom RFC6979
passthrough callback now independently checks that the signer passes the exact
32-byte message hash supplied to `secp256k1_ecdsa_sign`, matching the public
callback contract; signature equivalence alone does not establish this because
both nonce paths can receive the same wrong message.

For the differential proof, `secp256k1_ecdsa_sign_inner` was temporarily
changed to pass one all-zero 32-byte buffer to both the default and custom
nonce callbacks while retaining the real message for the ECDSA signing
equation. The focused seed aborted with exit 134. Removing only the new
message `memcmp` let all 29 API seeds pass under that identical production
mutation, proving the prior signature-equivalence, retry, verification, and
independent equation checks did not detect the callback-domain regression. The
mutation and assertion bypass were restored before replay. This is
informational oracle hardening, not a current-master production finding; a
real failure would be callback-context corruption rather than a direct key
compromise, so no master-relative severity rating changes.

  Focused ECDSA invalid-secret nonce-domain replay: the failure-cleanup path
  now calls a custom nonce callback with zero and group-order secret keys and
  requires the callback to receive each exact raw 32-byte key and message
  before signing returns failure and clears the signature. The existing
  invalid-secret checks used only the built-in nonce function, so they could
  not detect a fallback scalar being exposed to a custom callback. For the
  differential proof, `secp256k1_ecdsa_sign_inner` was temporarily changed to
  pass the message buffer as `key32` only when the supplied secret was
  invalid. The dedicated `invalid-seckey-nonce-domain` seed aborted with exit
  134; removing only this helper let all 30 API seeds pass under the same
  mutation. The mutation and bypass were restored before replay. This is
  informational invalid-input callback-oracle hardening, not a current-master
  production finding; severity is unchanged because the caller supplied an
  invalid secret and the public operation still fails closed. The restored
  default and forced-int64 Clang ASan/UBSan replays passed all 30 API seeds;
  native x86_64 GCC ASan/UBSan CTest passed the 30-seed API test. Two-worker,
  two-job, 15-second campaigns on both Clang sanitizer configurations exited
  0, with 33 inputs per job and no diagnostics or artifacts.

Focused ECDSA retry-failure cleanup replay (2026-07-14): the new
`ecdsa-retry-failure-cleanup` seed makes the nonce callback return a zero
scalar, then the group order, then failure on the third invocation. The
fuzzer requires `secp256k1_ecdsa_sign` to return zero, make exactly three
callback calls, and leave the signature object zeroed. Clean master passes
the complete tracked `api_roundtrip` corpus, including this seed.

For the mutation proof, `secp256k1_ecdsa_sign_inner` was temporarily changed
to set `ret = 1` only when a callback failure occurred after `count != 0`.
With the new helper enabled, the focused seed aborted; removing only the new
helper call let the prior API corpus pass under the identical mutation. This
proves that the existing first-attempt callback-failure oracle did not cover
failure after invalid and overflowing nonce retries. The production mutation
and all harness isolation changes were restored before replay. Normal GCC
and forced-int64 Clang ASan/UBSan corpus replays passed; a two-worker,
two-job, ten-second forced-int64 ASan/UBSan libFuzzer run also returned zero
for both jobs with no diagnostics or artifacts. This is informational oracle
hardening, not a current-master production finding. A real regression here
would be a retry/failure-state availability issue, not a cryptographic nonce
compromise, so it does not change any master-relative severity rating.

Focused recoverable ECDSA retry-failure cleanup replay (2026-07-14): the
recovery corpus now includes `recoverable-retry-failure-cleanup`. Its callback
returns zero, then the group order, then failure on attempt two. The recovery
wrapper must propagate failure after the rejected retries and clear its
opaque recoverable-signature output, while recording exactly three callback
calls. This is distinct from the ordinary ECDSA assertion because the
recoverable wrapper serializes the inner `(r, s, recid)` state even when the
inner signer fails.

For the mutation proof, `secp256k1_ecdsa_sign_inner` was temporarily changed
to set `ret = 1` only for callback failure after `count != 0`. The focused
recovery seed aborted with exit 134; removing only the new recovery helper
call left all eight recovery corpus seeds green under the identical mutation.
The production mutation and harness bypass were restored. Normal GCC and
forced-int64 Clang ASan/UBSan fixed replays passed. A two-worker, two-job,
ten-second forced-int64 ASan/UBSan libFuzzer run executed 97 and 99 inputs,
reached 4,821 features in both jobs, and returned zero without diagnostics
or artifacts. This is informational/Low wrapper-state hardening, not a
current-master production finding; no master-relative severity rating changes.

Focused recoverable ECDSA nonce-callback domain replay (2026-07-14): the
restored forced-int64 Clang ASan/UBSan target passed all eight tracked recovery
inputs, including `recoverable-valid-nonce-retry`. Every custom recovery nonce
callback now checks the exact 32-byte message hash and secret-key buffers
supplied by `secp256k1_ecdsa_sign_recoverable`, in addition to its existing
retry and callback-argument checks.

For the differential proof, `secp256k1_ecdsa_sign_inner` was temporarily
changed to pass one all-zero 32-byte buffer to both the default and custom
nonce callbacks while retaining the real signing scalar and message scalar.
The focused valid-retry seed aborted with exit 134. Removing only the new
message/key `memcmp` checks let all eight recovery seeds pass under the same
production mutation, showing that signature comparison, recovery equations,
and retry-state checks did not establish the callback domain. The mutation
and assertion bypass were restored before replay. This is informational oracle
hardening, not a current-master production finding; a real failure would be
callback-context corruption rather than a direct key compromise, so the
master-relative severity ledger is unchanged. The restored portable default
and forced-int64 Clang ASan/UBSan replays passed all eight seeds. Both
two-worker/two-job 15-second campaigns exited 0 without sanitizer diagnostics
or artifacts, and native GCC CTest passed 113/113 selected tests (112 unit
tests plus the recovery corpus).

Focused MuSig consumed-secnonce replay (2026-07-14): the MuSig signing path now
calls `secp256k1_musig_partial_sign` a second time after a successful partial
signature has zeroed the secret nonce. The `secnonce-reuse-after-sign` seed
requires that call to return zero, invoke the illegal callback exactly once,
clear the partial-signature output, and leave the consumed secnonce zeroed.
The previous fuzzer checked the zeroized bytes but never exercised this next
state transition.

For the mutation proof, `secp256k1_musig_secnonce_load` was temporarily changed
to accept an all-zero secnonce as `k0 = k1 = 1` with public nonce point `G`.
The focused seed aborted with exit 134. The proof uses a generator keypair on
the replay call so the synthetic point passes the normal keypair-binding check
and the mutation reaches the signing state; removing only the new reuse helper
left the prior MuSig corpus green under the identical mutation. The production
mutation and harness bypass were restored before replay. Normal GCC and
forced-int64 Clang ASan/UBSan fixed corpus replays passed. A two-worker,
two-job, ten-second forced-int64 Clang ASan/UBSan libFuzzer run executed 42
inputs per job, reached 5,676 features in both jobs, and returned zero without
diagnostics or artifacts. This is informational/Low state-machine hardening,
not a current-master production finding. It concerns the cryptographically
meaningful secret secnonce; it is distinct from non-critical public nonce
cleanup, so no master-relative severity rating changes.

Focused MuSig failed-partial-sign cleanup replay (2026-07-14): the existing
partial-sign failure helper now also requires a valid secret nonce to be
zeroed when an invalid cache is rejected and when the nonce magic is malformed.
The `partial-sign-secnonce-failure-cleanup` seed drives this path directly.
This is a separate transition from successful-sign reuse: an application can
hit an invalid opaque cache after loading a fresh secret nonce, and the API
contract still promises that `partial_sign` consumes that nonce.

For the mutation proof, `secp256k1_musig_partial_sign` was temporarily changed
to skip `secp256k1_memzero_explicit(secnonce, ...)` only when the cache magic
was deliberately corrupted by this helper. The focused seed aborted with exit
134; removing only the two new secnonce postconditions left all 42 MuSig seeds
green under that same targeted mutation. A broader mutation that delayed
invalidation past every validation barrier was rejected as non-independent
because the pre-existing invalid-signer oracle also detected it. The source
mutation was restored before fixed replay. The fixed forced-int64 Clang
ASan/UBSan build passed all 42 seeds individually, and two workers across two
jobs each completed 43 executions with 5,676 features and no diagnostic. A
native GCC build passed the MuSig corpus and the complete 238-test CTest run.
This is informational/Low state-cleanup hardening, not a current-master
production finding. A regression would reopen a cryptographically meaningful
secret-nonce reuse hazard only after a caller ignored an API failure, so it is
distinct from non-critical public-nonce cleanup; clean master consumes the
secret nonce before every later validation barrier. The existing unit tests
already cover the same API promise; this fuzzer check adds independent
corpus-driven coverage of the opaque-cache failure transition.

Focused arbitrary-signature ECDSA verification replay (2026-07-13): the
`api_roundtrip` corpus started with 24 tracked inputs totaling 931 bytes,
including `ecdsa-arbitrary-verification-equation`. Default ASan/UBSan and
forced-int64 replayed all inputs plus the empty input for 25 executions, with
final coverage of 3,159 and 5,324 edges; matching MSan replays also completed
without a diagnostic. Isolated default and forced-int64 ASan/UBSan managers
then ran with `-workers=2 -jobs=2 -max_total_time=30`; both jobs on each
backend returned zero without sanitizer diagnostics, assertion failures,
timeouts, OOMs, crash artifacts, or nonzero worker results. The temporary
mutated corpora and worker logs remained outside the repository.

For mutation proof, `secp256k1_ecdsa_sig_verify` was temporarily changed to
accept every nonzero scalar pair, and the public low-S gate was temporarily
disabled. The pre-existing high-S-specific fuzzer check was bypassed only to
isolate this oracle. The focused seed aborted in the independent equation on
both backends; delegating the new reference back to `secp256k1_ecdsa_verify`
under the same mutation made the seed pass. All temporary changes were
restored before the clean replay. This is informational oracle hardening, not
a current-master production finding, and does not change any severity rating.

Focused independent-HMAC replay (2026-07-13): the hash corpus started with
six tracked inputs totaling 288 bytes, including
`hmac-independent-reference`. Default and forced-int64 ASan/UBSan replayed all
six inputs once with seven total libFuzzer executions and final coverage of
314 edges. Both matching MSan replays executed all six inputs without a
diagnostic. Isolated default managers then ran for 30 seconds with
`-workers=2 -jobs=2`, executing 60,960 and 60,782 inputs; forced-int64 managers
executed 60,680 and 60,757 inputs. All four managers reached 317 edges and
returned zero without a sanitizer diagnostic, assertion failure, timeout, OOM,
or artifact. Mutated corpora and worker logs remained outside the repository.

Focused MuSig partial-signature equation replay (2026-07-13): the
partial-signature-focused subset of the MuSig corpus contained 30 tracked
inputs totaling 1,381 bytes, including
`partial-sig-equation`. The independent oracle recomputes each signer's
KeyAgg coefficient from the public key list, recomputes the BIP340 challenge
with generic SHA256 in the `r || aggregate_x || message` order, applies final
nonce and aggregate-cache parity, and checks `s_i*G = R1 + b*R2 + e*mu_i*P_i`
through public point operations without calling
`secp256k1_musig_partial_sig_verify`. Default and forced-int64 ASan/UBSan
fixed replays completed all 30 inputs; the corresponding MSan replays also
completed without a diagnostic. Isolated default ASan/UBSan managers with
`-workers=2 -jobs=2 -max_total_time=30` executed 43 and 45 inputs and reached
3,465 edges; forced-int64 managers executed 31 and 31 inputs and reached
5,606 edges. All four managers returned zero without a sanitizer diagnostic,
assertion failure, timeout, OOM, crash artifact, or nonzero worker result.
For mutation proof, temporarily negating `k[0]` immediately before the
production nonce-term addition changed `s = e*mu*d + R` into `s = e*mu*d - R`;
the focused seed aborted in this independent oracle on both backends. The
production mutation was restored before the clean replays. This is oracle
hardening, not a clean-master production finding, and does not change any
severity rating.

Focused arbitrary MuSig partial-signature verification replay (2026-07-13):
the complete MuSig corpus contains 33 tracked inputs, including
`arbitrary-partial-signature-equation`. For every signer/session combination,
the target parses zero, one, order-minus-one, and an input-derived reduced
scalar as externally supplied partial signatures. It compares
`secp256k1_musig_partial_sig_verify` with the independent public point equation
and therefore checks both valid and invalid parseable signatures without
delegating the expected result to the production verifier. Default and
forced-int64 ASan/UBSan isolated managers used
`-workers=2 -jobs=2 -max_total_time=15`; each job executed the 33 tracked
seeds plus the empty input and exited 0, reaching 3,464 and 5,606 edges.
Matching fixed-input MSan and MSan-int64 replays also completed all 33 tracked
seeds plus the empty input without diagnostics. For mutation proof,
`secp256k1_musig_partial_sig_verify` was temporarily changed to return success
after loading a zero scalar. The focused seed aborted with exit 134 on both
ASan/UBSan backends; disabling only the new arbitrary-signature comparison
made the identical mutation pass with exit 0. All temporary changes were
restored before the clean replay. This is informational oracle hardening, not
a current-master production finding, and does not change any severity rating.

Focused MuSig final-signature equation replay (2026-07-13): the complete MuSig
corpus contained 31 tracked inputs totaling 1,424 bytes, including
`final-signature-equation` and `tweaked-signing-parity`. The independent oracle
reconstructs the even-Y BIP340 final nonce from the signature X coordinate,
serializes the even aggregate key, recomputes the tagged challenge with the
generic SHA256 reference, checks the stored session challenge, and verifies
`s*G = R + e*P` with public point operations. It deliberately does not call
`secp256k1_schnorrsig_verify`; the session's recorded nonce parity is handled
by the signing convention, which negates an odd pre-adjustment nonce before
the final signature is emitted. Default and forced-int64 ASan/UBSan fixed
replays completed all 31 inputs with 3,474 and 5,616 edges; matching MSan
replays completed without diagnostics with 819 and 815 edges. Two-worker,
two-job, 30-second managers returned zero on both backends: default jobs each
executed 40 and 41 inputs, and forced-int64 jobs executed 32 inputs each. No
sanitizer diagnostic, assertion failure, timeout, OOM, crash artifact, or
nonzero worker result was observed.

For mutation proof, `secp256k1_musig_partial_sig_agg` was temporarily changed
to flip `sig64[63]` immediately after serializing the aggregate scalar. The
pre-existing final Schnorr-verifier assertion was temporarily replaced by
`FUZZ_CHECK(1)` solely to isolate the new oracle. The
`final-signature-equation` seed aborted in the independent point equation on
both default and forced-int64 builds. Both temporary changes were restored
before the clean replay. This is informational oracle hardening, not a
current-master production finding, and does not change any severity rating.

When a target fails, replay the generated input against this branch and clean
`master`, then classify the finding as a production bug, stale oracle, invalid
domain construction, sanitizer-only issue, or already-covered behavior.

## Current-Master Finding Ledger

The following severities describe the exact `origin/master` audit baseline
(`ebf5943` when recorded), before the corresponding branch fixes. A later
minor fix must not be allowed to hide a more serious master failure. Every
production finding has a focused corpus and a mutation or deterministic proof
documented in its commit message.

- **Informational / low fuzzer-infrastructure:** before the shared callback
  source was added, configuring CMake with
  `SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS=ON` left every fuzz executable
  except `fuzz_context` with undefined references to the required external
  default callback functions. The same omission affected Autotools fuzz
  targets. This is a build-availability defect in the fuzz harness, not a
  clean-master production vulnerability. Default builds did not expose it
  because they use the callbacks defined in `util.h`, and the earlier external
  build only linked `fuzz_context`. `external_callbacks.c` is now linked into
  every CMake and Autotools fuzz target; the exact external build and all
  available corpus replays are part of the commit verification. While proving
  the Autotools path, the clean-master `fuzz_ellswift` target also failed for a
  separate reason: its implementation-inclusive source requires the
  precomputed internal library, but its Autotools rule linked only the public
  library. Its rule now matches the internal CMake target, and the prior link
  failure plus the post-fix 14-target `make check` are recorded in the commit.

- **Low / informational:** `ecmult_multi/scratch-wrap-create` (`b827e0e`). The
  internal scratch constructor can wrap `base_alloc + size` before allocation,
  but caller-reachability is limited to static test, benchmark, and fuzz
  helpers: the public `secp256k1_scratch_space_create` symbols were removed and
  production MuSig aggregation currently uses the no-scratch path. The guard is
  still useful internal robustness and future-proofing, but this is not a
  remotely reachable or exported production memory-corruption primitive. The
  original commit's High label is superseded by this clean-master reachability
  audit.
- **Low / informational:** a magic-valid scratch object whose `alloc_size`
  exceeds `max_size` makes clean master subtract in unsigned arithmetic before
  the next allocation. The object is internal and no valid master path was
  found that can create this state, so this is not a remotely reachable
  production vulnerability. The shared scratch validator now rejects the
  accounting violation through the error callback before checkpoint, capacity,
  allocation, or destruction code can use it. The deterministic scratch test
  and `scratch-accounting-boundary` corpus assert the rejection and preserve
  the backing sentinel; removing the accounting guard makes the focused seed
  fail. This is hardening against a future internal state-transition bug, not a
  clean-master finding.
- **Medium:** invalid and noncanonical opaque public-key state
  (`5ad8052`, `334bae0`, `2b7a931`), inconsistent opaque keypairs used for
  signing or nonce creation (`9e70605`), and opaque ECDSA and recoverable
  signature scalars/metadata (`786bd4b`, `6c8a008`). These require malformed
  local opaque state or an invalid direct API call; they are not wire-format
  remote attacks, but they can otherwise let invalid objects cross a signing,
  verification, serialization, or recovery boundary.
- **Medium:** MuSig opaque state semantics: invalid aggregate-cache points
  (`cefe4c8`), cache parity/tweak normalization (`9484be8`), nonce points and
  secret nonce scalars (`2b2c15d`), session parity/nonce/scalar fields
  (`1a5f517`), and overflowing partial-signature scalars (`0863a8b`). These
  magic-preserving mutations can alter signing state or make verification
  builds and release builds disagree; each has a focused mutation or corpus
  proof. X-only odd-Y state (`f812b84`) is the corresponding extrakeys parity
  barrier.
- **Medium:** direct NULL dereferences in exported built-in nonce callbacks
  (`ab0cb33`), ECDH/EllSwift hash callbacks (`35ffb87`), and missing prefix
  data in the EllSwift built-in XDH callback (`067d4a3`). Clean master can
  dereference required callback input before returning ordinary failure.
  Direct RFC6979 callback attempts at `UINT_MAX` (`c7d8760`) are a separate
  availability bug: the unsigned retry loop can wrap and hang. These are
  public callback/API misuse paths, not wire-format attacks.
- **Medium:** HMAC/RFC6979 secret-state retention after finalization
  (`5cfe7f7`). This is a lifetime/cleanup finding rather than a demonstrated
  disclosure; severity must not be raised to critical without a memory-read
  primitive.
- **Medium, latent 10x26 field correctness:** clean master also lets both
  `secp256k1_fe_normalizes_to_zero` variants lose two 32-bit carry contributions on a valid
  magnitude-32 representation. The deterministic 10x26 state with limbs
  `n[0]=0xffff0f91`, `n[1]=0xfffff040`, `n[9]=0x0fc00000` and all other limbs
  zero represents `63*p + 2^58 + 2^32`, but the old uint32 carry chain reports
  it as zero. This can poison internal equality, inversion, or exceptional-state
  decisions if the representation is reached. No public API path naturally
  producing this exact state has been demonstrated, so the master-branch rating
  is Medium/latent, with potentially High arithmetic impact if the documented
  maximum-magnitude state becomes reachable; it is not claimed as a remote key
  or signature vulnerability.
- **Informational oracle hardening:** `fuzz_hash` compares arbitrary HMAC
  outputs against a raw-SHA256 reference that independently performs key
  shortening, ipad/opad construction, message sequencing, and finalization.
  The RFC6979 reference uses that same independent HMAC model for its complete
  96-byte stream. The earlier reference used the production HMAC wrapper, so a
  keyed-pad or HMAC-finalization defect could pass both sides; the existing
  fixed vectors and chunk-equivalence checks did not cover every derived state.
  Clean master passes. A temporary production mutation that flips one output
  bit only when the inner HMAC state has 113 bytes makes the dedicated
  `hmac-independent-reference` seed abort with the new comparison enabled;
  disabling that comparison leaves the prior hash checks green under the same
  mutation. The production mutation was restored. This is oracle hardening,
  not a current-master production finding, and does not change any severity
  rating. The prior one-shot-versus-later-retry relation remains intentionally
  absent because RFC6979 changes state between generate calls.
  The RFC6979 comparison now extends the one-shot and split streams from 96 to
  160 bytes, exercising five consecutive HMAC blocks instead of stopping after
  three. The dedicated `rfc6979-long-stream` seed and the full hash corpus pass
  on the unmutated implementation. A temporary mutation that flips the first
  byte emitted after the third block only when the original request exceeds
  96 bytes makes the new seed abort, while the prior 96-byte checks remain
  green under the same mutation. This is informational oracle hardening, not
  a current-master production finding, and does not change any severity rating.
  The same target now includes a standalone SHA-256 compression, schedule, and
  padding model. It compares production one-shot and split writes against that
  model at lengths 0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 191, and 192,
  using a nonzero deterministic message pattern as well as the input-derived
  message. The eight tracked hash inputs total 386 bytes, including
  `sha256-independent-reference` and `midstate-prefix-model`; fixed replays pass on default, forced-int64,
  MSan, and MSan-int64. A temporary mutation changed the direct compression
  count from `n_blocks` to `n_blocks - 1` only for `len == 192`. The new
  reference aborted the dedicated seed with `-handle_abrt=0` and exit 134,
  while disabling only the new reference let the identical mutation exit 0.
  Existing fixed vectors stop at 65 bytes, and the prior HMAC/RFC6979
  references use the production SHA-256 write path, so they did not provide
  this independent long-block barrier. The production mutation and oracle
  bypass were restored. Default and forced-int64 isolated
  `-workers=2 -jobs=2 -max_total_time=15` campaigns exited 0 without
  diagnostics or artifacts. This is informational oracle hardening, not a
  current-master production finding, and does not change any severity rating.
  The midstate path is also checked independently for 128- and 192-byte
  prefixes, with suffixes up to 129 bytes, using the reference compression
  state before production `secp256k1_sha256_initialize_midstate`. The dedicated
  `midstate-prefix-model` seed selects a 128-byte prefix and a 56-byte suffix,
  and checks both one-shot and split suffix writes. A temporary production
  mutation changed `hash->bytes = bytes` to
  `hash->bytes = bytes + (bytes == 128 ? 64 : 0)`; the focused seed aborted
  with exit 134, while disabling only the new helper let the same mutation pass
  the fixed 64-byte midstate and existing hash checks. The production mutation
  and oracle bypass were restored. Default, forced-int64, and MSan fixed
  replays passed; default and forced-int64 two-worker/two-job 15-second
  campaigns exited 0 without diagnostics. This is informational oracle
  hardening, not a current-master production finding, and does not change any
  severity rating.
- **Informational oracle hardening:** the standalone SHA-256 model is now
  shared by `fuzz_hash`, `fuzz_context`, `fuzz_ecdh`, and `fuzz_ellswift`.
  `fuzz_context` independently constructs
  `SHA256(tag) || SHA256(tag) || msg`; `fuzz_ecdh` manually builds the
  compressed shared-point input from an uncompressed public serialization; and
  `fuzz_ellswift` independently builds the BIP324/prefix transcript. The
  previous context, ECDH, and EllSwift expected-value paths reused production
  SHA processing. Clean master passes the new
  `sha256-independent-tagged`, `default-hash-independent-reference`, and
  `bip324-independent-reference` seeds, plus the existing corpora, on default,
  forced-int64, MSan, and MSan-int64.
  For context and ECDH, a temporary finalization mutation that flips one output
  bit for post-padding `hash->bytes` values 33, 64, 224, or 256 left the old
  production-derived checks green but made the new focused seeds abort with
  exit 134. For EllSwift, the exact mutation limited to `hash->bytes == 256`
  left the old target green on default and forced-int64 while the new focused
  seed aborted on all four builds. All mutations and control changes were
  restored. Default, forced-int64, MSan, and MSan-int64 isolated
  `-workers=2 -jobs=2 -max_total_time=15` campaigns for all four affected
  targets exited 0 without diagnostics or artifacts. This is informational
  oracle hardening, not a current-master production finding, and does not
  change any severity rating.
- **Informational oracle hardening:** `fuzz_ecdh` now checks the coordinates
  delivered to custom hash callbacks when the scalar is invalid. Clean master
  deliberately substitutes one for zero, `n`, and overflowing `n+1` scalars
  before invoking the callback; the prior fuzzer checked only callback count
  and built-in output clearing. The focused
  `invalid-scalar-callback-point` seed passes on default, forced-int64, and
  MSan. Disabling the production scalar fallback makes the new coordinate
  assertion abort, while disabling only the new helper leaves the identical
  mutation green on all three builds. This closes an ECDH postcondition gap,
  not a current-master production vulnerability, and does not change any
  severity rating.
- **Informational oracle hardening:** `fuzz_ellswift` now checks the X
  coordinate delivered to custom hash callbacks for invalid zero, `n`, and
  overflowing `n+1` secrets under both `party` selections. Clean master
  deliberately uses scalar one for these invalid secrets; the prior fuzzer
  checked only return values and callback-derived output, so it did not prove
  that the callback received the selected remote point. Replacing the
  production scalar-one fallback with scalar two makes the focused seed abort
  on default, forced-int64, and MSan, while disabling only the new helper
  keeps the same mutation green on all three builds. The fixed corpus replay
  and isolated two-worker campaigns pass cleanly. This closes an EllSwift
  postcondition gap, not a current-master production vulnerability, and does
  not change any severity rating.
- **Informational oracle hardening:** `fuzz_musig` independently recomputes
  each signer's KeyAgg coefficient and BIP340 challenge, then checks the
  partial-signature point equation through public point operations with final
  nonce and aggregate-cache parity. The prior target called
  `secp256k1_musig_partial_sig_verify`, which is a useful API check but shares
  internal coefficient/session state and does not independently recompute the
  challenge transcript. The clean current branch, whose partial-signing
  equation is unchanged from `origin/master`, passes the new
  `partial-sig-equation` seed on both field backends and under MSan. A
  temporary production mutation that negates `k[0]` immediately before the
  nonce-term addition makes the seed abort in the new oracle on both backends;
  the mutation was restored before replay. This is oracle hardening, not a
  current-master production finding, and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_musig` now evaluates arbitrary
  parseable partial-signature scalars, including zero and order boundaries,
  with an independent public point equation and compares that result with
  `secp256k1_musig_partial_sig_verify`. The previous target checked the
  independent equation only for signer-produced partial signatures, while
  parseable arbitrary values were merely round-tripped. The focused seed
  passes on clean master across default, forced-int64, MSan, and MSan-int64.
  A temporary zero-scalar acceptance mutation in the production verifier
  aborts the new comparison on both ASan/UBSan backends; disabling only the new
  comparison leaves the same mutation green. This closes a verifier-oracle
  gap, not a current-master production vulnerability.
- **Informational oracle hardening:** `fuzz_musig` independently reconstructs
  the even-Y final BIP340 nonce and aggregate key, recomputes the challenge,
  and checks `s*G = R + e*P` through public point operations instead of relying
  on `secp256k1_schnorrsig_verify`. The clean current branch passes the
  `final-signature-equation` seed across both field backends and under MSan.
  Flipping `sig64[63]` immediately after production partial-signature
  aggregation makes that seed abort even with the prior verifier assertion
  disabled in the temporary harness isolation run. The production mutation and
  harness bypass were restored before replay. This is oracle hardening, not a
  current-master production finding, and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_schnorrsig` now verifies generated
  and deliberately invalid signatures with an independent BIP340 model. It
  rejects `r >= p` and `s >= n`, independently computes the tagged challenge,
  reconstructs `R = sG - eP` through public point operations, and requires a
  finite even-Y point whose X coordinate equals `r`. The prior target checked
  the independent equation only for signer-produced signatures and delegated
  invalid-signature outcomes to `secp256k1_schnorrsig_verify`. Clean master
  passes `arbitrary-signature-verification-equation` on both field backends and
  under MSan. Changing the verifier's scalar-overflow rejection to success makes
  the all-`0xFF` case abort in the new reference; disabling the new comparison
  makes the same isolated mutation pass. This closes a verifier-oracle gap, not
  a current-master production vulnerability.
- **Informational oracle hardening:** `fuzz_schnorrsig` computes the BIP340
  auxiliary-key, nonce, custom-tag, and arbitrary-signature challenge
  transcripts with the standalone SHA-256 model. The previous expected-value
  helper used the production SHA initialization, write, and finalization path,
  so a SHA defect could make both the production operation and its expected
  value agree. Clean master passes all 11 tracked Schnorr inputs, including
  `sha256-independent-tagged`, on default, forced-int64, MSan, and
  MSan-int64 builds. Default and forced-int64 two-worker campaigns also pass
  with `-workers=2 -jobs=2 -max_total_time=15`. A temporary production
  finalization mutation, `if (hash->bytes == 192 || hash->bytes == 256)
  out32[0] ^= 1`, leaves the old production-derived control green on all 11
  inputs but makes the new seed abort on all four builds. The mutation was
  restored before the clean replay. This is oracle hardening, not a
  current-master production finding, and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_musig` now computes its auxiliary,
  nonce, nonce-coefficient, KeyAgg list/coefficient, and BIP340 challenge
  transcripts with the standalone SHA-256 model. The previous MuSig
  expected-value helper used production SHA initialization, write, and
  finalization, so a defect in those paths could make both a generated value
  and its expected value agree. The new `sha256-independent-tagged` seed and
  all 31 pre-existing MuSig inputs pass on clean master. For the proof
  mutation, `secp256k1_sha256_finalize` was temporarily changed to flip
  `out32[0]` after digest serialization when post-padding `hash->bytes` was
  192 or 256. The pre-change target at `82011b3` passed all 31 existing inputs
  under that mutation on default, forced-int64, MSan, and MSan-int64 builds;
  the new focused seed aborted on all four builds. The mutation was restored
  before the clean replay. This is an oracle-hardening change only; it does
  not change any severity rating or claim a current-master production
  vulnerability.
- **Informational oracle hardening:** `fuzz_recovery` independently checks the
  recoverable ECDSA equation `rQ = sR - zG` after reconstructing `R` from `r`
  and `recid`, including the overflow branch, for both signer-generated and
  arbitrary parsed signatures. It also asserts that failed arbitrary recovery
  clears the output public key. The previous arbitrary-input path compared
  only with `secp256k1_ecdsa_verify`, which shares the production-derived
  recovered key, and did not check failure cleanup. The clean current branch
  passes the two arbitrary seeds, the `r+n` vector, and the complete
  seven-input recovery corpus on both field backends and under MSan.
  Seed-specific temporary mutations of the `u1` negation, failure `memset`,
  and `r+n` addition, with the delegated checks isolated, abort in the new
  assertions on both backends; disabling the corresponding new assertion
  makes each mutation pass. All temporary changes were restored before
  replay. This is oracle hardening, not a current-master production finding,
  and does not change any severity rating.
- **Medium:** malformed long-form lengths in
  `contrib/lax_der_privatekey_parsing` (`d334351`). Clean master forms an
  out-of-range pointer while evaluating a short caller buffer before rejecting
  the claimed length. This is parser-level undefined behavior in a
  caller-reachable contribution, distinct from the core DER signature parser.
- **Low:** core ECDSA DER input-length pointer construction (`cd8c9f1`). On
  clean master, a direct `secp256k1_ecdsa_signature_parse_der` call with a
  one-byte non-DER buffer and `SIZE_MAX` input length forms `sig + size` before
  rejecting the encoding; UBSan reports pointer overflow. The public API
  requires an array of `inputlen` bytes, so this is not a remote DER or
  cryptographic vulnerability. The offset parser removes the undefined
  behavior while preserving valid DER behavior, and the `fuzz_api_roundtrip`
  regression checks rejection plus cleared output state.
- **Low to Medium:** fixed and variable output state left live after failed API
  calls (`c02dc5e`, `799a080`), including documented invalidation on NULL
  tweaks (`4759bd8`, `f48cd99`). A caller that ignores a failure can
  accidentally reuse a prior key, signature, hash, encoding, or serialized
  prefix. The known output sizes make these paths testable and fail-closed, but
  there is no direct memory corruption and callers are required to check return
  values.
- **Low:** the contrib BER private-key exporter left its documented 279-byte
  output buffer unchanged when an invalid secret key made export fail
  (`36a009f`). Clean master still reset the output length to zero, so callers
  that ignored the return value could retain stale private-key encoding bytes;
  this was stale state rather than a cryptographic failure or memory-safety
  issue. The deterministic `ecdsa` regression and the dedicated
  `privkey-der-export-failure` corpus seed prove the failure for both encoding
  modes and preserve the boundary between the cleared 279 bytes and the rest
  of the caller's buffer.
- **Informational oracle hardening:** `fuzz_api_roundtrip` now independently
  parses the SEC1 private-key DER emitted by the contrib exporter. It checks
  the exact length forms, version, private-key octets, explicit curve OID and
  parameters, generator, order, cofactor, and compressed/uncompressed embedded
  public key. The previous export/import round trip was weaker because
  `ec_privkey_import_der` stops after the version and private-key octet string;
  exporter corruption in the parameter block could therefore be accepted by
  both sides. Clean master passes all 25 tracked API inputs on default,
  forced-int64, MSan, and MSan-int64. Changing the explicit-parameter OID in
  both exporter templates from `2A 86 48 CE 3D 01 01` to
  `2A 86 48 CE 3D 01 02` made `privkey-der-structure` abort with
  `-handle_abrt=0` and exit 134; disabling only the new parser let the same
  mutation exit 0. The production mutation and oracle bypass were restored.
  Default and forced-int64 isolated `-workers=2 -jobs=2 -max_total_time=15`
  campaigns exited 0 without diagnostics or artifacts. This is informational
  serialization-oracle hardening, not a current-master production finding,
  and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_api_roundtrip` now independently
  checks a three-term `secp256k1_ec_pubkey_combine`. It adds the two
  fuzz-derived canonical secret scalars and one generator scalar with byte
  arithmetic modulo the group order, compares the returned point and zero-sum
  failure state with `secp256k1_ec_pubkey_create`, and repeats the combine in a
  different order. The focused `three-term-pubkey-combine` seed and all 26 API
  seeds pass on default and forced-int64 ASan/UBSan builds. A temporary
  production mutation that added `G` after a three-input combine whose final
  input was `G` made the focused seed abort with exit 134 on both backends;
  disabling only the new call made both controls exit 0. Default and
  forced-int64 two-worker 15-second campaigns also exited 0 without
  diagnostics or artifacts. The mutation was removed before clean replay.
  This is an informational oracle gap, not a clean-master production finding,
  and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_api_roundtrip` now checks that
  `secp256k1_ec_pubkey_combine` resumes after an intermediate cancellation. It
  verifies `P + (-P) + Q = Q`, a reordered cancellation path, and two
  four-term zero-sum paths whose output must be rejected and zeroed. The
  focused `intermediate-infinity-pubkey-combine` seed and all 27 API inputs
  pass on default and forced-int64 ASan/UBSan builds. A temporary production
  mutation that skipped the third term only for `n == 4`, `i == 2`, after the
  accumulator reached infinity made the focused seed abort with exit 134 on
  both backends. Disabling only this new helper let all 26 pre-existing API
  inputs pass on both mutated backends, proving the prior three-term oracle did
  not cover the four-term transition. Default and forced-int64 two-worker,
  two-job 15-second campaigns exited 0 without sanitizer diagnostics or
  artifacts. The mutation and bypass were restored before clean replay. This
  is informational oracle hardening, not a clean-master production finding,
  and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_api_roundtrip` now exercises the
  seven-input `secp256k1_ec_pubkey_combine` loop with two fuzz-derived public
  keys followed by generator multiples 1 through 5. It independently sums
  the seven canonical scalars with byte arithmetic modulo the group order,
  compares the result with generator multiplication, and repeats the call in
  reverse order. The `long-pubkey-combine` seed is 136 bytes; the helper is
  gated at 128 bytes so all 27 pre-existing API inputs remain a differential
  control. Clean default and forced-int64 ASan/UBSan replays pass the focused
  seed and the complete 28-input corpus; the forced-int64 MSan focused replay
  also passes. Default and forced-int64 two-worker/two-job 20-second campaigns
  exit 0 without sanitizer diagnostics or artifacts. For the differential
  proof, `secp256k1_ec_pubkey_combine` was temporarily changed to skip only
  index 6 when `n == 7`. The 27 pre-existing inputs remained green, while the
  new seed aborted with exit 134 in the independent scalar model. The
  production mutation was restored before the clean replays. This is
  informational oracle hardening, not a current-master production finding,
  and does not change any severity rating.
- **Informational oracle hardening:** `fuzz_api_roundtrip` extends the same
  independent public-key combine oracle to eight inputs, with two
  fuzz-derived public keys followed by generator multiples 1 through 6. It
  independently sums all eight canonical scalars with byte arithmetic modulo
  the group order, compares the result with generator multiplication, and
  repeats the call in reverse order. The existing `long-pubkey-combine` seed
  is 136 bytes and crosses the 128-byte gate, so no corpus mutation is needed
  to reach the new tail. Clean default and forced-int64 ASan/UBSan focused and
  fixed-corpus replays pass all 28 files plus the empty input, as do the
  corresponding bounded two-worker/two-job campaigns without sanitizer
  diagnostics or artifacts. Matching default and forced-int64 MSan fixed
  replays pass the same 28 files plus the empty input without diagnostics. For
  the differential proof, a temporary production mutation skipped only index 7
  when `n == 8`. The parent seven-term target remained green on all 28 current
  API inputs, while the extended target's long seed aborted with exit 134 in
  the independent scalar model. The mutation was restored before clean
  replay. This is informational oracle hardening, not a current-master
  production finding, and does not change any severity rating.
- **Low to Medium:** secret-derived stack/helper temporaries left live after use
  (`a3e30b3`, `a6f0b14`, `f94fec5`, `a884a2d`). These are code-path-proven
  lifetime reductions for scalar, field, Jacobian, EllSwift, and tweak state;
  no read primitive is demonstrated. They are more relevant than public
  nonce-buffer cleanup, but should not be described as critical erasure without
  a memory-disclosure path.
- **Low:** the internal `secp256k1_gej_rescale` scale-alias contract
  (`61259f9`) also covers `&r->x`, not only the previously tested `&r->y` and
  `&r->z` cases. On clean master, the exact `rescale-x-alias` seed and the
  `gej_rescale_alias` unit test abort at the field overlap check; the existing
  scale snapshot fixes all three aliases. This remains an internal availability/
  correctness issue with no current public reachability or cryptographic impact.
- **Medium, with low practical exploitability:** impossible SHA256 lengths
  (`ab36b78`). Clean master can read beyond a caller's short buffer after it
  accepts a `size_t` length at the SHA256 boundary; the condition requires an
  incoherent pointer/length pair and is not a remote cryptographic attack by
  itself. The direct clean-master replay below upgrades the earlier Low entry
  to reflect the demonstrated sanitizer-visible memory-safety failure.
- **Low:** scalar rounded-shift bounds (`5bb982d`), EllSwift zero-`u`
  normalization (`119b407`), built-in ECDH failure-output cleanup (`bb15eb0`),
  NULL preallocated context storage (`a9253c2`), and partial `ecmult_multi`
  results after a later batch fails (`5bd9ae8`). The latter is an internal
  helper boundary: callers must honor the failure return, so it is state
  hygiene rather than a remotely reachable cryptographic defect.
- **Informational oracle hardening:** `fuzz_ecmult_multi` now forces a
  Pippenger invocation across two batches, independently checks the repeated
  point equation, and rejects callback failures immediately before and after
  the batch boundary. Clean master passes; using a relative callback offset
  for the second batch makes the focused `repeated-pippenger-batches` seed
  abort. This is internal callback-index and failure-state coverage, not a
  current-master production finding; the existing target already covered
  single-batch Pippenger and multi-batch Strauss behavior.
- **Informational oracle hardening:** `fuzz_ecmult_multi` now allocates a
  32-byte `0xA5` prefix before the scratch checkpoint used by its callback-
  failure path, snapshots it, and verifies that rollback preserves those
  caller-owned bytes in addition to checking the allocation counter, callback
  trace, and infinity output. The prior failure oracle checked only the
  counter, so a rollback could corrupt an earlier allocation while still
  passing its existing checks. The dedicated
  `ecmult_multi/scratch-prefix-preservation` seed is 28 bytes. Clean default
  and forced-int64 ASan/UBSan fixed replays passed all 12 corpus files (490
  bytes total), and the matching MSan replay passed the same corpus. Isolated
  default and forced-int64 two-worker/two-job campaigns ran for 10 seconds
  and exited 0 without sanitizer diagnostics, assertion failures, timeouts,
  OOMs, or artifacts. For the mutation proof, a temporary change in
  `secp256k1_scratch_apply_checkpoint` flipped byte 31 whenever the checkpoint
  was 32: the focused seed aborted with exit 134 when the new sentinel check
  was enabled, while bypassing only that check left the same mutation green
  with exit 0. The mutation and bypass were restored before replay. This is
  an internal failure-state oracle, not a clean-master production finding,
  and does not change any severity rating.
- **Sanitizer-only fuzzer fix:** the `ecmult_multi` harness now uses defined
  scalar-zero initialization when constructing zero or overflow fallback cases.
  Before this correction, MemorySanitizer reported an uninitialized read in the
  harness's adjacent scalar-negation case for
  `ecmult_multi/msan-defined-zero-scalar`; `secp256k1_scalar_clear` intentionally
  poisons memory under MSan and is therefore not a logical zero constructor.
  This is a fuzzer-domain defect, not a production finding: ASan/UBSan and the
  library's normal builds cannot classify the poisoned cleared value as
  uninitialized. The seed is retained so future oracle work cannot reintroduce
  the invalid harness state.
- **Low:** noncanonical MuSig nonce storage (`64250f7`) and overflowing opaque
  partial-signature serialization (`3c1d67b`) are local API-consistency and
  cross-build robustness failures. MuSig failed parse/aggregation/session and
  nonce-output cleanup (`5da4893`, `dd7030b`, `28c308d`, `50bb71f`, `ea0fff3`,
  `73de850`, `8780cdc`, `a8457e2`) is stale-state hygiene; public nonce cleanup
  remains non-critical when the nonce has no cryptographic meaning.
- **Low / informational hardening:** zero MuSig nonce scalars (`5b7ae25`) are
  cryptographically meaningful invalid state but require an approximately
  2^-255 reduced-model relation under the real order; infinity key aggregates
  (`ee7f44c`) likewise require an infeasible hash relation on normal master.
  Their reduced-order/exhaustive mutations prove the contracts without claiming
  a practical remote vulnerability.
- **Test-only / informational:** the MuSig exhaustive regression now uses
  `EXHAUSTIVE_TEST_ORDER` identical keys, so the weighted aggregate is infinity
  for every supported reduced model, not only order 13. It also uses the exact
  zero-nonce counters 5, 2, and 70 for orders 7, 13, and 199 respectively.
  This corrects an order-13-only test assumption; it does not change the
  master-relative severity or production behavior of the existing hardening.
- **Previously recorded oracle gaps now covered:** empty public-key aggregation
  (`c5c0afe`), ECDSA verification's invalid-opaque-key API boundary
  (`ef25d27`), and the public-key serializer's wrong flag-type boundary
  (`062f68d`) all have named seeds and mutation proofs. They remain
  informational master behavior, not production vulnerabilities.
  The existing `fuzz_ecmult_const` target also transitively covers
  `secp256k1_ge_table_set_globalz`: replacing the accumulated `zs` inverse with
  the per-entry `zr[i]` makes the `scalar-derived-xonly-fractions` seed abort,
  while the restored clean-master helper replays successfully. No duplicate
  helper-only assertion is needed; this is coverage evidence, not a new
  production finding.
  The three internal `ecmult_table_get_*` selectors are covered by separate
  independent paths as well. Temporarily negating only the positive lookup in
  `secp256k1_ecmult_table_get_ge` makes the existing `fuzz_ecmult_const`
  corpus abort; the same mutation in `secp256k1_ecmult_table_get_ge_lambda`
  aborts on `odd-multiples-table`, and the corresponding
  `secp256k1_ecmult_table_get_ge_storage` mutation aborts on
  `ecmult_multi/repeated-strauss-batches`. Restored clean default ASan/UBSan
  replays of the five `ecmult_const` and eleven `ecmult_multi` inputs pass in
  isolated directories. These are shared-helper mutation controls, so adding
  a duplicate selector-only oracle would not improve the current discovery
  surface; no clean-master production finding is claimed.
  It also compares the direct affine generator helper
  `secp256k1_ecmult_gen_ge` against the independently converted Jacobian result.
  Clean master passes; replacing its affine conversion with an infinity output
  makes the dedicated `generator-affine-agreement` seed abort. This is
  informational helper coverage, not a current-master production finding.
  The same target now directly compares `secp256k1_ecmult` with `ng == NULL`
  against an explicit zero generator scalar for finite and infinity bases,
  including zero and nonzero point scalars. Clean master passes; replacing the
  NULL term with a nonzero generator term makes `null-generator-equivalence`
  abort. This is informational coverage for the documented internal contract,
  not a current-master production finding; master already has a focused unit
  test, but its recent caller migration makes the fuzzer barrier useful for
  unusual scalar and base-state combinations.
  The static generator-context audit found that `ecmult_gen_context_build`,
  `ecmult_gen_blind`, and both `ecmult_gen_ge{j}` entry points are reached by
  context creation/randomization and the `ecmult_const`/group generator
  comparisons. Clean default and forced-int64 ASan/UBSan replays passed all
  five existing `ecmult_const` inputs, and the clean default context replay
  passed all six context inputs. As a mutation control, negating only
  `ctx->ge_offset` immediately after `secp256k1_ecmult_gen_blind` computes the
  randomized point made the existing `generator-affine-agreement` seed abort
  with libFuzzer exit 77 on both backends; the mutation was restored before the
  clean replay. This revalidates the existing direct generator oracle from
  `74170c1`; no duplicate fuzzer call was added. It is informational helper
  coverage, not a clean-master production finding. `ecmult_gen_context_clear` is intentionally not checked
  by reading the cleared bytes: `memclear_explicit` documents the post-clear
  contents as unspecified and VERIFY/MSan deliberately marks them undefined.
  `ecmult_compute_table` and `ecmult_gen_compute_table` are precomputation
  generators used while producing static tables, not input-dependent runtime
  state machines, so they remain covered by build/test generation rather than
  a duplicate fuzzer hook.
  The public API inventory on the same date found a call site in a fuzz target
  for every callable `SECP256K1_API` function except `secp256k1_selftest`.
  Selftest is a fixed SHA sanity check with no fuzz-input-dependent state, and
  context creation already invokes it; it remains unit-covered. The deprecated
  `context_no_precomp` symbol is a data alias, not an independent function, and
  is checked against `context_static` by the static-context unit tests. No
  missing public-operation oracle was identified.
  The matching `modinv32_var` and `modinv64_var` loops were also audited for
  assertion-side effects. Their `int i` counter is declared only under
  `VERIFY`, incremented only inside `VERIFY_CHECK(++i < limit)`, and never
  participates in termination or any arithmetic state; release builds stop
  only when `g == 0`. The release `noverify_tests` and `VERIFY` `tests` suites
  both passed their 16-iteration runs on the clean branch. This is deliberate
  debug-only progress accounting, not a release/VERIFY state divergence or a
  current-master production finding, so no duplicate inversion oracle was
  added for it.
  The scalar target now exercises a deterministic matrix of valid
  `secp256k1_scalar_get_bits_var` ranges and valid, non-crossing
  `secp256k1_scalar_get_bits_limb32` ranges at every 32-bit and 64-bit
  implementation boundary. The independent byte reference includes crossings
  that `get_bits_var` must accept but `get_bits_limb32` must reject, such as
  offset 24/count 25. Clean master passes the dedicated
  `scalar/get-bits-boundaries` seed on the default and forced-int64 field
  backends, and matching MSan replays pass as well. Routing only that
  offset/count pair through `get_bits_limb32` makes the new matrix abort at
  the production precondition; disabling only the new matrix lets the same
  mutation pass, proving that the older random-range checks do not provide the
  same deterministic barrier. Isolated `-workers=2 -jobs=2 -max_total_time=15`
  replays completed with exit code 0: default workers executed 851 and 854
  inputs at 1,727 edges, while forced-int64 workers executed 300 and 305 at
  3,488 edges. This is informational oracle hardening for the master scalar
  contract, not a current-master production finding.
  The context target also forces a multi-block custom SHA callback batch
  (`sha256-multiblock`): master passes the independent digest check, while a
  one-block production mutation aborts before it can hide a batching error.
  It also checks that `secp256k1_context_preallocated_size` rejects a
  compression-type flag in the external-default-callback build, where the
  default illegal callback can be counted without aborting. Clean master
  already returns zero; suppressing the invalid-type branch makes the focused
  seed abort. Ordinary builds intentionally skip this direct call because
  their default callback terminates the process.
  The constant-time multiplication target now also supplies
  `secp256k1_ecmult_const_xonly` with three valid representations of the same
  fraction: a magnitude-8 numerator (`x + 7p`), a magnitude-8 denominator
  (`1 + 7p`), and both raised together. The prior rational path normalized
  both operands before calling the helper. A temporary `VERIFY`-only mutation
  incremented `g.n[0]` after `secp256k1_fe_mul(&g, &g, n)` only when both raw
  operands were nonnormalized with magnitude 8; the focused seed aborted on
  both default and forced-int64 ASan/UBSan builds. Disabling only this helper
  left that seed and the three preexisting `ecmult_const` seeds green on both
  builds. Clean production code then passed all four fixed seeds, while
  two-worker/two-job 15-second campaigns completed with 285 and 172
  executions. The existing `sqr`, `sqrt`, `ecmult_gen_ge`, and
  `ecmult_const_tests` also passed in a fresh sanitizer build. This is
  informational internal-contract hardening, not a clean-master production
  finding; severity is unchanged.
  The field target also pins the largest valid `secp256k1_fe_add_int` input:
  magnitude 31 plus `0x7fff` must normalize identically to a low-magnitude
  reference. The production wrapper now asserts this precondition before the
  magnitude can exceed the documented 32 limit. Clean master passed the valid
  boundary; the invalid magnitude-32 call is a caller-domain violation, not a
  current production finding.
  It also compares `secp256k1_fe_set_b32_limit` against an independent
  big-endian `< p` reference and checks canonical round trips for accepted
  values. The focused seed pins `p-1`, `p`, `p+1`, zero, all-`FF`, and a derived
  input. Clean master already passes this informational parser oracle; inverting
  the wrapper's `secp256k1_fe_impl_set_b32_limit` branch makes the seed abort.
  The strict decoder is already used by public-key, Schnorr, recovery, and
  ECDSA paths, so this is a direct backend-boundary check rather than a current
  production finding.
  The field target now computes the exact canonical residue of the
  `get_bounds` magnitude-32 sum independently as bytes:
  `64 * (2^256 - 1) mod p = 64 * (2^32 + 976)`. This avoids deriving the
  expected value by normalizing the same production representation. With a
  temporary one-limb mutation changing
  `secp256k1_fe_impl_get_bounds` from `... * 2 * m` to `... * 2 * m - 1`,
  the older production-derived relational checks still passed the
  `magnitude32-normalize` corpus input, while the new byte reference aborted.
  The mutation was restored before the fixed-tree replay. This is
  informational oracle hardening, not a new clean-master finding; the existing
  10x26 magnitude-32 normalization issue remains rated Medium/latent as
  recorded above.
  The same target now pins a valid 10x26 magnitude-32 value whose raw residue
  is `2^58 + 2^32`, independently checks its canonical bytes, and requires
  both zero predicates to return false. The clean-master uint32 implementation
  returns true for both predicates after two carry wraps; reverting only the
  new zero-test carry patch makes `fe_normalize_max_magnitude` abort at the
  focused assertion with exit 134. The fixed forced-int64 unit and
  `zero-predicate-false-positive` corpus seed pass. This is a current-master
  production correctness finding with Medium/latent severity, distinct from
  the already-fixed normalize overflow: the earlier fork patch changed only
  normalize, normalize_var, and normalize_weak, so it did not remove this
  false-zero behavior.
  The field target now also exercises `fe_mul`, `fe_sqr`, `fe_inv`, and
  `fe_sqrt` with valid magnitude-8 representations of one and two (`1 + 7p`
  and `2 + 7p`). These operations accept nonnormalized inputs by contract,
  but the previous arithmetic checks normalized every operand before calling
  them. A temporary `VERIFY`-only mutation incremented `r->n[0]` after
  `secp256k1_fe_impl_sqr` only when the raw input was nonnormalized, magnitude
  8, and exactly `1 + 7p`; the focused seed aborted on both default and
  forced-int64 ASan/UBSan builds. Disabling only this helper left that seed and
  the four preexisting field seeds green on both builds. Clean production code
  then passed all five fixed seeds, while two-worker/two-job 15-second
  campaigns completed with 6,229 and 6,219 executions. This is informational
  internal-contract hardening, not a clean-master production finding; the
  master-relative severity ledger is unchanged.
  The field target now compares `fe_inv` and `fe_inv_var` for one canonical
  residue and the same residue represented at valid magnitude 32. The
  `maximum-magnitude-inverse` seed passes clean master on default and
  forced-int64 ASan/UBSan builds, and all seven field seeds replay cleanly on
  each backend. For mutation proof, a temporary `VERIFY`-only edit flipped
  `r->n[0]` after `secp256k1_fe_impl_inv` only when `x->magnitude == 32`, in
  both `src/field_10x26_impl.h` and `src/field_5x52_impl.h`. The focused seed
  aborted with status 134 on both builds; disabling only this new call left
  all six pre-existing field seeds green on both. The mutations were restored
  before the clean replay. This is informational internal-contract hardening,
  not a clean-master production finding; the master-relative severity ledger
  is unchanged. Existing field coverage exercised canonical and magnitude-8
  inversion, while maximum-magnitude tests covered normalization and zero
  predicates, so neither provided this representation-invariance check.
  The Schnorr target also checks that custom nonce callbacks receive the
  normalized secret key and matching x-only public key. A mutation that passes
  the secret-key buffer in place of the x-only key still produces signatures
  accepted by ordinary verification, so this callback-domain contract needs
  its own oracle (this commit). The ECDSA target independently checks the
  corresponding `key32` contract for its RFC6979 passthrough callback. Passing
  the same wrong buffer to both default and custom ECDSA nonce paths leaves
  their signature comparison green, so this check is likewise not redundant.
  It now checks the corresponding exact `msg32` contract as well: passing the
  same wrong message to both paths leaves their signatures, verification, and
  independent signing equation green, so a direct callback assertion is
  required for that domain too.
  The EllSwift target independently checks the corresponding exact `ell_a64`
  and `ell_b64` callback domain. Passing one encoded-party buffer to both
  custom callback arguments leaves the shared-X, point, symmetry, and output
  checks green, so this is an independent transcript oracle. It is
  informational and does not alter the clean-master severity ledger.
  The Schnorr callback now also checks the exact message bytes and length. A
  mutation that passes the normalized secret-key buffer as the 32-byte nonce
  message to both default and custom signing paths leaves their signatures and
  point equation consistent, so the direct callback-domain check is required.
  It also exercises the documented empty-message
  representation boundary: `sign_custom` and `verify` must accept both
  `(NULL, 0)` and a non-NULL zero-length pointer, and both signatures must be
  identical. Clean master already passes; this is an informational API oracle,
  not a current-master production finding.
  The MuSig target also completes signing after deterministic x-only tweaks,
  including the zero tweak and both final-key parities. This binds the cache's
  accumulated parity to partial-signature verification and final Schnorr
  verification. Negating the `secp256k1_extrakeys_ge_even_y` condition in
  `secp256k1_musig_pubkey_tweak_add_internal` makes the focused seed abort;
  the existing x-only implementation passes, so this is oracle hardening
  rather than a current-master finding.
  The x-only target also rejects a zeroed opaque keypair at
  `secp256k1_keypair_xonly_pub`, clears the x-only output, and preserves an
  initialized zero optional parity result. This is an informational local-state
  oracle: clean master already rejects the object, while skipping the
  `secp256k1_keypair_load` result check makes the focused seed abort.
  The `partial-keypair-projection` seed then zeroes the secret half and public
  half independently. Raw `keypair_sec` and `keypair_pub` must preserve their
  own halves, while `keypair_xonly_pub` must accept the valid public half and
  reject the invalid one. Clean master passes; changing the public extractor to
  copy the secret half makes the focused seed abort. This is informational
  invalid-state coverage, not a current-master production defect.
  The x-only target also passes a zeroed opaque `secp256k1_pubkey` through
  `secp256k1_xonly_pubkey_from_pubkey`. The conversion must propagate the
  existing invalid-object callback, clear its x-only output, and preserve the
  initialized zero optional parity result. Clean master already has this
  barrier. Temporarily returning success from the conversion's
  `secp256k1_pubkey_load` failure branch makes the dedicated
  `invalid-full-pubkey-xonly` seed abort with exit 134; removing only this new
  helper lets all nine x-only seeds pass under the same mutation. This is
  informational invalid-state oracle hardening, not a current-master
  production finding; the existing invalid opaque-key severity remains
  unchanged. The restored default and forced-int64 Clang ASan/UBSan builds
  each passed all nine x-only seeds. Native x86_64 GCC ASan/UBSan CTest passed
  110/110 selected tests (109 unit tests plus the x-only corpus). Two-worker,
  two-job, 15-second campaigns on both sanitizer configurations exited 0;
  each job executed 12 inputs without diagnostics or artifacts.
  A bounded `-workers=2 -jobs=2 -max_total_time=30` replay of the updated
  x-only corpus executed 1,334 and 1,361 inputs in its two jobs; both jobs
  exited 0, with no sanitizer diagnostics or crash artifacts.
  It also checks that `secp256k1_xonly_pubkey_cmp` maps an invalid opaque
  x-only key to the documented all-zero ordering sentinel: invalid is below a
  valid key, the reverse comparison is above, and two invalid keys compare
  equal. Clean master already has this behavior; changing the comparator's
  invalid-key fallback makes the focused seed abort.
  The `null-xonly-comparator` seed also exercises the documented NULL ordering
  fallback through an unannotated function pointer, so UBSan does not reject
  the deliberate runtime API-boundary input at the call site. Clean master
  passes; changing the comparator's invalid fallback memset from zero to
  `0xFF` makes the seed report a libFuzzer deadly signal. The existing x-only
  fuzzer and unit suite covered invalid opaque objects but not NULL operands.
  This is informational oracle hardening, not a current-master production
  defect; the production implementation is restored unchanged.
  The x-only parser now also compares both the x-only and compressed public-key
  parser results against a standalone byte-level curve-membership model. The
  model rejects x >= p, computes x^3 + 7 with modular double-and-add arithmetic,
  and verifies the (p+1)/4 square-root equation without using production field
  limbs or the production square-root addition chain. The master-derived
  parent at `36556b8` passes all eight tracked x-only inputs on default, forced-int64,
  MSan, and MSan-int64 builds. For the control proof, `secp256k1_ge_set_xo_var`
  was temporarily changed to reject only the valid X coordinate of `2G`
  (`C6047F9441ED7D6D3045406E95C07CD85C778E4B8CEF3CA7ABAC09B95C709EE5`). The
  pre-change target passed all seven pre-existing corpus inputs under that
  mutation on all four builds, while the new `independent-parse-reference` seed
  aborted on all four because the standalone model still accepted the valid
  coordinate. The mutation was restored before clean replay. This is
  informational oracle hardening; no current-master production vulnerability is
  claimed.
  The API target now also checks the complete SEC1 public-key wire boundary against
  a shared standalone byte-level model. For compressed inputs it independently
  checks the field bound and `x^3 + 7` square-root condition; for uncompressed and
  hybrid inputs it checks both field coordinates, `y^2 = x^3 + 7`, and hybrid-Y
  parity. A successful parse must also serialize back to the same compressed bytes
  or the same 64 coordinate bytes, rather than merely round-tripping the resulting
  opaque object. The model is shared with the x-only target through
  `pubkey_reference.h`, so the two fuzzers do not carry subtly different byte
  arithmetic. A fixed uncompressed `7G` vector forces the full-coordinate path on
  every API input. Clean master passes all 27 tracked API inputs and all 8 x-only
  inputs on the default ASan/UBSan build. For the control proof,
  `secp256k1_eckey_pubkey_parse` was temporarily changed to reject only that exact
  7G uncompressed encoding. With the new checks enabled, the existing
  `api_roundtrip/valid-ish-scalar` input aborts with `-handle_abrt=0` and exit 134;
  disabling only the new parser and wire-preservation checks lets the same input
  exit 0. The production mutation was restored before clean replay. This is
  informational oracle hardening, not a current-master production finding, and
  does not change any severity rating. The final fixed replay also passed on
  forced-int64 ASan/UBSan and MSan builds; default and forced-int64
  `-workers=2 -jobs=2 -max_total_time=30` runs for both changed targets exited 0
  after 360-362 executions per target, with no sanitizer diagnostics or crash
  artifacts.
  The MuSig target also calls `secp256k1_musig_pubkey_agg` with both optional
  output pointers set to `NULL`. Clean master accepts the valid input and
  returns success; requiring either the x-only aggregate or cache output makes
  the `aggregate-no-outputs` seed abort. This is informational optional-output
  API coverage, not a current-master production finding.
  It also compares the aggregate x-only output when only the cache output is
  omitted against the full-output call. Clean master accepts this independent
  optional-output combination; rejecting `(agg_pk != NULL, keyagg_cache ==
  NULL)` makes the `aggregate-xonly-without-cache` seed abort while the prior
  harness remains green. This is informational API coverage, not a
  current-master production finding; the deterministic MuSig tests already
  cover the same argument combination.
  The MuSig cache barrier now independently invalidates the 64-byte
  `second_pk` storage at offset 68 while leaving the aggregate point valid.
  `secp256k1_musig_pubkey_get`, ordinary tweak-add, and x-only tweak-add must
  all reject the cache, clear their public output, and preserve the opaque
  cache. The earlier barrier corrupted only the aggregate point at offset 4,
  so it did not prove this separate parser path. The dedicated
  `keyagg-second-pk-barrier` seed is 25 bytes. Clean default and forced-int64
  ASan/UBSan fixed replays passed all 39 MuSig corpus files (1,622 bytes
  total), the matching MSan replay passed the same corpus, and isolated
  two-worker/two-job campaigns completed with every job exiting 0 and no
  sanitizer diagnostics or artifacts. For the mutation proof, a temporary
  change replaced the `second_pk` non-infinity validation condition in
  `secp256k1_keyagg_cache_load` with `0 && ...`: the focused seed aborted with
  exit 134 with the new barrier enabled, while compiling out only that new
  block left the same mutation green with exit 0. The mutation and bypass
  were restored before replay. This is informational opaque-state oracle
  hardening, not a clean-master production finding, and does not change the
  severity ledger.
  The MuSig target also cross-checks `secp256k1_musig_nonce_gen_counter`
  against `secp256k1_musig_nonce_gen` and an independent transcript reference
  across all eight combinations of optional message, key-aggregation cache, and
  extra-input pointers. Each case uses a distinct counter, and successful calls
  must agree in secret nonce bytes, public nonce serialization, and the required
  random-input clearing. Clean master passes; deterministic tests cover the
  combinations individually, but the earlier fuzzer oracle covered only the
  all-present combination. This is informational oracle hardening, not a
  current-master production finding. A selective mutation that injects the
  secret key as `extra_input32` only for the all-NULL combination makes the
  dedicated `nonce-counter-optional-inputs` seed abort while the previous
  all-present oracle and scalar barrier remain green.
  The nonce-generation failure oracle now also snapshots `session_secrand32`:
  only a successful call may consume this caller-owned secret, while invalid
  seckeys and invalid public keys must leave it available for a corrected
  retry. Clean master passes; adding an unconditional wipe in the invalid
  seckey branch or in the invalid-public-key branch made the existing corpus
  pass before the new postcondition aborted on `state-output-failure-cleanup`
  or `nonce-invalid-pubkey-cleanup`, respectively. This is informational
  oracle hardening, not a clean-master production finding. A failure here
  would be a low-severity retry/availability regression, not a nonce-secret
  compromise; public nonce cleanup remains non-critical because that nonce has
  no cryptographic meaning.
  The MuSig target now forces the nonce hash callback to return an all-zero
  digest and exercises the documented zero-derived-scalar failure path. A
  failed `musig_nonce_gen` must return 0, preserve the caller's
  `session_secrand32` for a corrected retry, and leave both the secret and
  public nonce objects fully zeroed. The callback is installed before the
  forced state is enabled because the context validates custom SHA callbacks
  with its own self-test. The unmutated production implementation passes the
  dedicated `nonce-zero-scalar-failure` seed and the full MuSig corpus.
  Temporarily replacing the production `if (invalid_nonce)` guard with `if (0 &&
  invalid_nonce)` makes that seed abort at the existing nonzero-scalar
  invariant with exit 134. This is informational oracle hardening, not a
  current-master production finding; a failure would be a low-severity nonce
  failure/availability regression, not a cryptographic compromise.
  It also independently recomputes the one-key KeyAgg transcript. The absence
  of a second distinct key must not turn the sole key's coefficient into the
  identity scalar; the `keyagg-single-coefficient` seed compares the resulting
  full and x-only aggregates against the generic tagged-hash reference. Clean
  master passes; treating an infinite `second_pk` as the identity-coefficient
  branch makes the seed abort. This is informational oracle hardening, not a
  current-master production finding; the existing one-signer sign/verify path
  would otherwise share the same mistaken cache and fail to detect it.
  The group target also compares the constant-time and variable-time batch
  Jacobian-to-affine conversions on finite points. Clean master already agrees
  on this internal representation contract; changing the constant-time
  prefix-product initialization makes the focused seed abort. Infinity points
  remain in the variable-time-only path because the constant-time helper's
  caller contract requires finite inputs.
  The same target now independently converts a fixed `[finite, infinity,
  finite]` batch point-by-point and checks the variable-time helper's explicit
  infinity placement, while also calling both batch helpers with `(NULL,
  NULL, 0)`. Clean master passes; changing the infinity branch to leave a
  destination marked finite makes `batch-conversion-boundaries` abort on the
  per-point comparison. This is informational oracle hardening for an
  internal indexing/state-transition contract, not a current-master
  production finding; the deterministic unit suite already covers the empty
  range, but the fuzzer previously did not combine it with a mixed batch.
  The group target now extends both finite batch conversions to four entries
  and extends the mixed-infinity variable-time case to
  `[finite, infinity, finite, infinity]`, covering a trailing infinity as
  well as an interior one. The dedicated `four-entry-batch-conversion` seed
  brings the group corpus to 12 files. Clean default, forced-`int64`, and
  matching MSan fixed replays passed the seed and all 12 files plus the empty
  input; bounded two-worker/two-job campaigns also exited 0. For the first
  differential proof, a temporary production mutation replaced the
  constant-time prefix-product initialization with `secp256k1_fe_one` only
  when `len == 4`; the focused seed aborted with exit 134 on both field
  backends. Disabling only the two new finite four-entry calls let the same
  mutation pass all 12 corpus files plus empty on both backends. For the
  second proof, a temporary variable-time mutation replaced only the trailing
  infinity output at `len == 4` and `i == 3` with the valid generator point;
  the focused seed again aborted with exit 134 on both backends, while
  disabling only the newly extended mixed-infinity helper let the pristine
  corpus pass. All mutations and bypasses were restored before clean replay.
  This is informational internal batch-conversion oracle hardening, not a
  current-master production finding; no severity rating changes.
  The group target now constructs a projective generator representation and
  checks `secp256k1_ge_set_ge_zinv` against both the variable-time Jacobian
  conversion and the independent canonical generator. It also adds multiples
  of `p` to the inverse-Z value, exercising the valid nonnormalized magnitude-8
  representation. A temporary `VERIFY`-only mutation flipped one output limb
  in `secp256k1_ge_set_ge_zinv` only when the input was the generator and the
  inverse-Z magnitude was 8. The focused `ge-zinv-nonnormalized` seed aborted
  with status 134 on default and forced-int64 ASan/UBSan builds. Disabling only
  this new call left all 12 pre-existing group and `ecmult_const` seeds plus
  the focused seed green on both builds. The mutation was restored before the
  clean replay, which passed all nine group seeds. This is informational
  internal-contract hardening, not a clean-master production finding; the
  master-relative severity ledger is unchanged. Bounded two-worker libFuzzer
  runs also exited 0, completing 508 default and 326 forced-int64 executions.
  The group target now also constructs the same generator with the largest
  affine representations permitted by the internal contract: `x + 3p` and
  `y + 2p`, giving nonnormalized magnitudes 4 and 3. It compares storage
  bytes against the canonical generator, reloads the storage object, and
  compares both canonical and extended byte encodings. A temporary
  `VERIFY`-only mutation flipped `r->x.n[0]` in `secp256k1_ge_to_storage` only
  for this magnitude-4/magnitude-3 state. The focused
  `ge-storage-nonnormalized` seed emitted a libFuzzer deadly-signal failure on
  default and forced-int64 ASan/UBSan builds, while disabling only the new
  oracle call left all nine prior group seeds green on both backends. The
  mutation was restored before the clean replay, which passed all ten group
  seeds on both backends. Bounded two-worker/two-job campaigns against copied
  corpora also exited 0 after 988 default and 615 forced-int64 executions with
  no sanitizer diagnostics or artifacts. This is informational internal-
  contract hardening, not a clean-master production finding; the
  master-relative severity ledger is unchanged.
  The `ecmult_const` target now calls `secp256k1_ecmult_odd_multiples_table`
  directly with `n == 4`, reconstructs every omitted projective Z coordinate
  from the returned ratios, and compares the resulting `[a, 3a, 5a, 7a]`
  points against an independent repeated-double/addition sequence. The
  pre-existing `ecmult_const` corpus used only fixed-size production table
  callers, so it never exercised this generic four-entry contract directly.
  For the control proof, a temporary `VERIFY`-only mutation flipped one limb
  of `zr[1]` when `n == 4`; the four old corpus inputs plus the dedicated
  `odd-multiples-table` seed stayed green with only the new helper disabled,
  while enabling it made the focused seed emit a libFuzzer deadly-signal
  failure. The mutation was restored before the clean replay. Default and
  forced-`int64` ASan/UBSan focused and full-corpus replays passed, as did
  bounded `-workers=2 -jobs=2` campaigns with no sanitizer diagnostics or
  crash artifacts. This is informational/Low internal-contract oracle
  hardening, not a clean-master production bug; no public or cryptographic
  impact was demonstrated and the master-relative severity ledger is
  unchanged.
  The group target now constructs the exceptional `gej_add_ge` relation by
  adding a point to the negated lambda endomorphism image. This forces
  `y1 == -y2` while keeping `x1 != x2`, so the unified formula must use its
  alternate slope instead of taking the ordinary infinity path. An independent
  variable-time Jacobian addition supplies the expected point, while the
  existing addition helper checks the affine/Jacobian variants, canonical
  equality, and Z-ratio contract. The dedicated
  `lambda-degenerate-addition` seed and all 11 copied group inputs pass on
  clean default and forced-`int64` ASan/UBSan builds. For mutation isolation,
  a temporary `VERIFY`-only production mutation overwrote `rr_alt` with
  `rr` whenever `degenerate` was set in `secp256k1_gej_add_ge`; all 10
  pre-existing inputs plus the new seed stayed green with only the new helper
  disabled, while the enabled helper aborted on the focused seed on both
  backends. The mutation was restored before clean replay, and bounded
  `-workers=2 -jobs=2` campaigns exited 0 without sanitizer diagnostics.
  This is informational/Low internal arithmetic-oracle hardening, not a
  clean-master production bug; related lambda pairs were already covered by
  deterministic tests, no public or cryptographic impact was demonstrated,
  and the master-relative severity ledger is unchanged.
  It also compares the fractional X-coordinate curve predicate against an
  independently computed quotient and curve equation, with the generator as a
  deterministic on-curve case. Clean master passes this informational helper
  oracle; inverting the production square-test result makes its focused seed
  abort. The fraction predicate is already used by ElligatorSwift and
  deterministic tests, so this catches a regression in its rational-coordinate
  arithmetic without claiming a current-master vulnerability.
  The group target now supplements Jacobian result checks with a canonical
  affine-coordinate comparison built from `ge_set_gej_var` and canonical
  64-byte serialization, while retaining `gej_eq_var` as a consistency check.
  This prevents a group-addition-based equality helper from being the sole
  oracle for group operations. The `independent-equality-barrier` corpus also
  compares a finite generator point with its negation, so an equality helper
  that always returns true fails immediately. Clean master passes the focused
  corpus and all seven tracked group inputs on default, forced-int64, MSan,
  and MSan-int64 builds. Replacing `secp256k1_gej_eq_var`'s result with `1`
  made the new negative check abort; omitting only that new check made the
  same mutation pass. Default and forced-int64 isolated
  `-workers=2 -jobs=2 -max_total_time=15` campaigns also exited 0 without
  diagnostics. This is informational oracle hardening, not a current-master
  production finding, and does not change any severity rating.
  The same group target now feeds `secp256k1_gej_rescale` a magnitude-8
  nonnormalized field element whose value is exactly one, then compares it
  with the normalized scale path. The internal contract requires only
  a nonzero scale within the field precondition; the prior fuzzer supplied
  normalized scales and therefore did not exercise this representation. The
  existing group-equality oracle is affine, so it intentionally treats any
  nonzero projective rescale as equal; the new check compares normalized x/y/z
  coordinates as the rescale postcondition.
  Clean master passes the focused `rescale-nonnormalized-scale` seed and the
  complete group corpus on default and forced-int64 ASan/UBSan builds. A
  temporary production mutation increments `s_in.n[0]` only for a
  nonnormalized magnitude-8 scale that normalizes to one; the focused seed
  aborts, while disabling only this helper leaves that seed and all seven
  prior group seeds green. This is informational internal-contract hardening,
  not a clean-master production finding, and does not change any severity
  rating.
  The group target now sends a structurally valid storage object for an
  off-curve point through `secp256k1_ec_pubkey_tweak_mul` with a nonzero
  tweak. The operation must return zero, report exactly one illegal callback,
  and leave an opaque public key that `secp256k1_pubkey_load` still rejects.
  Existing malformed-key barriers covered serialization, combine, tweak-add,
  and negate, but not this distinct multiply/load failure path. The dedicated
  `invalid-pubkey-tweak-mul` seed is the ASCII input
  `invalid opaque pubkey tweak mul\n`. The oracle intentionally does not
  require the failed output to be byte-zeroed: the public API specifies an
  unspecified output value on this
  failure, while it does require the failure and invalid-state barrier.
  For the differential proof, `secp256k1_ec_pubkey_tweak_mul` was temporarily
  changed from
  `!overflow && secp256k1_pubkey_load(ctx, &p, pubkey)` to
  `secp256k1_pubkey_load(ctx, &p, pubkey) || !overflow`, making an invalid
  load appear successful. Bypassing only this new block left all 12
  pre-existing group seeds and the focused seed green under that mutation;
  restoring the block made the focused seed abort with exit 134. The
  production mutation and bypass were restored before clean replay. Default
  and forced-int64 Clang ASan/UBSan replays passed the focused seed and all 12
  prior group files. Two-worker/two-job 15-second campaigns on both builds
  completed with all four jobs exiting zero and no sanitizer diagnostics or
  artifacts. This is informational invalid-state oracle hardening, not a
  current-master production finding; the existing Medium malformed opaque
  public-key severity remains unchanged.
  The API target also pins an independent ECDSA signing equation with a fixed
  nonce: private key, message, and nonce are all one, so `r = x(G)` and
  `s = r + 1`. Clean master passes; changing the production signing addition
  to subtraction aborts on the focused seed. This is informational oracle
  hardening rather than a current-master finding: the previous default/custom
  nonce comparison delegated both paths to RFC6979 and could not independently
  pin the signing equation.
  It now also captures an input-derived valid scalar nonce and checks a
  variable-state equation without calling `secp256k1_ecdsa_verify`: byte
  arithmetic independently reduces the message and serialized nonce-point
  X coordinate, checks `r = x(kG) mod n`, and checks `s*k = z + r*d` or its
  negation for low-S normalization using the public scalar-tweak API. Clean
  master passes the dedicated `ecdsa-variable-nonce-equation` seed. A
  temporary production mutation that adds one to the signing numerator only
  when `secp256k1_scalar_get_bits_limb32(nonce, 0, 32) == 0xDFAD8E28u` makes
  that seed abort; compiling out the new check leaves the previous API checks
  green under the identical mutation. The production file was restored before
  replay. This proves an oracle gap, not a current-master production finding,
  and does not change any severity rating.
  It also verifies arbitrary parsed scalar-valid signatures without delegating
  to the internal verifier: it independently applies the public point equation
  `sR = zG + rQ`, reconstructs both parities for `x = r` and the `x = r+n`
  branch when it fits in 256 bits, and enforces the public low-S policy. Clean
  master passes `ecdsa-arbitrary-verification-equation`. Temporarily accepting
  every nonzero pair in `secp256k1_ecdsa_sig_verify` and disabling the public
  low-S gate makes that seed abort; after the existing high-S-specific check is
  bypassed, delegating the new reference to the production verifier makes the
  same mutation pass. This closes the prior gap for externally supplied
  signatures and is informational oracle hardening, not a current-master
  production finding.
  It also forces a valid scalar nonce to produce an invalid ECDSA equation
  (`s == 0`) and verifies that signing rejects that attempt, requests the next
  nonce, and returns a verified signature. Clean master passes; forcing
  `secp256k1_ecdsa_sig_sign` to report success makes the dedicated
  `ecdsa-valid-nonce-retry` seed fail verification. This is informational
  retry-state hardening, not a current-master production finding.
  The recovery target repeats this boundary through
  `secp256k1_ecdsa_sign_recoverable` and verifies that the retry's recovery ID
  still recovers the signer, not merely a valid ECDSA signature. Clean master
  passes; forcing `secp256k1_ecdsa_sig_sign` to report success makes the
  dedicated `recoverable-valid-nonce-retry` seed fail recovery. This is
  informational wrapper/state coverage, not a current-master production
  finding.
  It also exercises the valid zero-element `ec_pubkey_sort` boundary with a
  non-NULL array pointer. Clean master returns success; requiring at least one
  element in the production sort routine makes the focused seed abort. This is
  informational API-boundary coverage, not a current-master production bug.
  The `null-pubkey-sort` seed covers the complementary illegal boundary: a
  NULL array pointer must be rejected, and must invoke the illegal callback,
  for both zero and nonzero element counts without dereferencing the array.
  Clean master returns zero and reports exactly one callback per call; removing
  the production `ARG_CHECK(pubkeys != NULL)` makes the seed abort immediately.
  The existing unit test covered a NULL array only with `n_pubkeys == 2`, so
  this is informational oracle hardening, not a current-master production bug.
  The `null-pubkey-comparator` seed also exercises the documented NULL ordering
  fallback through an unannotated function pointer, so UBSan observes the
  deliberate runtime API-boundary input instead of the header nonnull
  attribute. Clean master passes; changing `secp256k1_ec_pubkey_cmp`'s invalid
  fallback memset from zero to `0xFF` makes the focused seed report a libFuzzer
  deadly signal. Invalid opaque objects were already covered by the fuzzer and
  NULL operands by the unit suite, but this closes the corresponding fuzzer
  boundary. This is informational oracle hardening, not a current-master
  production defect.
  A bounded `-workers=2 -jobs=2 -max_total_time=30` replay of the updated API
  corpus executed 543 and 550 inputs in its two jobs; both jobs exited 0, with
  no sanitizer diagnostics or crash artifacts.
  The API target now adds a gated eight-key `secp256k1_ec_pubkey_sort`
  oracle at the first length beyond its four-key stateful fixtures and the
  deterministic six-key vectors. It derives eight distinct public keys,
  computes the expected order with independent compressed-serialization byte
  comparisons, checks exact pointer order and sortedness, and repeats the sort
  to verify idempotence. The existing 136-byte `long-pubkey-combine` seed
  crosses the 128-byte gate. Clean default and forced-int64 ASan/UBSan fixed
  replays passed all 28 API files plus the empty input; bounded two-worker/
  two-job campaigns exited 0 without sanitizer diagnostics or artifacts.
  Matching default and forced-int64 MSan fixed replays passed the same corpus.
  For the differential proof, a temporary production mutation validated all
  eight pointers but decremented `n_pubkeys` to seven before calling heapsort.
  The extended seed aborted with exit 134 on both backends; bypassing only the
  new helper left all 28 files plus the empty input green with the mutation
  active. The mutation and bypass were restored before clean replay. This is
  informational oracle hardening, not a current-master production finding,
  and does not change any severity rating.
  The EllSwift target also replays a full-width BIP324 decode vector from the
  independently generated module test set and checks the serialized X coordinate
  and parity. Clean master passes; replacing the decode input's `t` half with
  its `u` half aborts on the dedicated seed. This is informational oracle
  hardening, not a current production finding. A separate temporary mutation of
  the EllSwift PRNG counter byte order passed the relational encode/create
  checks; because those public APIs explicitly do not guarantee stable output
  bytes across versions, that result is a stale/overbroad oracle and is not a
  production bug or a reason to pin unstable encodings.
  The same independently sourced BIP324 vector now exercises every successful
  `secp256k1_ellswift_xswiftec_inv_var` branch: each result must be nonzero,
  distinct, and map back through the forward decoder to the vector's X coordinate,
  with at least one valid branch. It deliberately does not require the inverse
  helper to reproduce this vector's exact `t`, because the API documents excluded
  encodings that may be valid for the forward decoder but omitted by the inverse.
  Clean master passes, and replacing only the inverse helper's final multiplication makes the new seed
  abort while the existing decode-vector assertion still passes with the oracle
  disabled. This is informational internal-helper coverage, not a current-master
  production finding. The inverse helper is now compiled as an internal fuzz target
  so the assertion observes the same implementation and verification contracts as
  the field/group fuzzers.
  The EllSwift target now also compares canonical `(u,t) = (1,1)` with three
  noncanonical encodings that add the field prime to `u`, `t`, or both, through
  both `ellswift_decode` and x-only XDH. This pins the documented modulo-p
  interpretation without pinning unstable encoder output bytes. The clean
  master-derived parent at `e4b782c` passes all nine tracked EllSwift inputs on
  default, forced-int64, MSan, and MSan-int64 builds. For the control proof,
  `secp256k1_ellswift_decode` was temporarily changed to return failure when
  the first 32 bytes were exactly `p+1`; the pre-change target remained green
  on all eight pre-existing corpus inputs under that mutation on all four
  builds, while the new `modulo-alias-encoding` seed aborted on all four.
  The mutation was restored before clean replay. This is informational oracle
  hardening; no current-master production vulnerability is claimed.
  The MuSig target now feeds `musig_nonce_process` three aggregate encodings
  that the extended parser deliberately accepts: `[infinity, P]`,
  `[P, infinity]`, and `[infinity, infinity]`. It independently recomputes the
  nonce coefficient and the effective point `R1 + b*R2` through public point
  operations, including the production fallback to the generator for
  infinity. The previous fuzzer only parsed and round-tripped these encodings;
  the deterministic suite checked the all-infinity acceptance but did not
  compare the mixed effective-nonce state. The `secp256k1_effective_nonce`
  implementation under test is unchanged from `origin/master`. A temporary
  mutation that adds `R2` instead of the identity when `R1` is infinity makes
  the `aggregate-no-outputs` replay abort on the generated `[infinity, P]`
  case; ordinary generated nonces do not enter that branch. The production
  file was restored and rebuilt. This is informational oracle hardening, not
  a current-master production finding; other branch hardening is not being
  presented as clean-master evidence.
  The Schnorr target also independently checks every generated signature's
  BIP340 point equation through public point operations: it parses the even-Y
  nonce point, derives the challenge with the generic tagged-hash reference,
  computes `sG - eP`, and compares the result with the serialized nonce. It
  runs for both the fixed 32-byte path and a variable-length message. Existing
  checks called the library verifier, whose challenge helper is shared with the
  signer; a temporary mutation that dropped the final byte only for non-32-byte
  challenges therefore let signer and verifier agree while the
  `sign32-custom` seed aborted on the independent equation. The production
  helper was restored and rebuilt. This is informational oracle hardening, not
  a current-master production finding.
  The target also compares each EllSwift encoding with a second encoding made from
  the same key and bitwise-complemented caller randomizer, requiring different
  encodings that both decode to the original key. Clean master passes; removing
  only the `rnd32` write in `secp256k1_ellswift_encode`, or only the optional
  `auxrnd32` write in `secp256k1_ellswift_create`, makes the focused
  `randomizer-effects` seed abort. Existing tests checked decode round trips but
  never checked that these entropy inputs affected the result. This is
  informational oracle hardening: no clean-master production defect is claimed,
  and no unstable encoding bytes are pinned.

  The MuSig target now independently computes the four-key `KeyAgg list` hash,
  the first-distinct-key coefficient rule, every remaining coefficient, and the
  four-term weighted public-point sum through public APIs. The earlier reference
  stopped at three keys, while the public aggregation API accepts arbitrary list
  lengths and the signing harness was capped at three signers. The
  dedicated `four-keyagg-reference` seed exercises callback index three. Clean
  default and forced-`int64` ASan/UBSan replays passed all 34 corpus files plus
  the new seed, and two-worker/two-job campaigns exited zero in both builds. For
  the control proof, replacing only the production callback coefficient at
  `idx == 3` with the identity scalar kept the pre-change target green on all 33
  existing corpus files, while the new seed aborted with `-handle_abrt=0` and
  exit 134. This is informational oracle hardening, not a current-master
  production finding, and does not change any severity rating.

  The stateful MuSig signing path now extends the independent coefficient,
  partial-signature, session, nonce-aggregation, and final-signature checks from
  three to four participants. The previous fuzzer selected only one through
  three public keys, so normal fuzzing and the existing corpus never entered a
  four-element nonce or partial-signature aggregation loop. The dedicated
  `four-signer-sign-roundtrip` seed is five bytes (`AACA` plus its newline) and
  deterministically selects `n_pubkeys == 4`. Clean default and forced-`int64`
  ASan/UBSan replays passed all 35 MuSig corpus files, including the new seed;
  two-worker/two-job campaigns returned zero on both backends, with each job
  executing all 35 files plus the empty input. For the differential proof,
  `secp256k1_musig_sum_pubnonces` was temporarily changed to skip only index 3
  when `n_pubnonces == 4`. The pre-extension target at `024a28b` remained green
  on all 34 pre-existing corpus files, while the new four-signer seed aborted
  with exit 134. A debugger backtrace placed the failure in the independent
  `secp256k1_fuzz_check_musig_final_sig_equation`, before the production
  Schnorr verifier. The mutation was restored before clean verification. This
  is informational oracle hardening, not a current-master production finding,
  and does not change any severity rating.

  The stateful MuSig signing path now extends that transcript and all
  independent partial- and final-signature checks from four to seven
  participants, matching the largest seven-key lists in the production MuSig
  vectors. The public APIs accept arbitrary list lengths, but the previous
  fuzzer stopped at four, so normal corpus runs never entered the seventh
  nonce or partial-signature aggregation transition. The dedicated
  `seven-signer-sign-roundtrip` seed is seven bytes (`AAAZAA` plus its newline);
  at selector offset 157, byte `Z` deterministically selects `n_pubkeys == 7`.
  Clean default and forced-`int64` ASan/UBSan replays passed all 36 MuSig
  corpus files, including both signer-count seeds; default and forced-int64
  two-worker/two-job campaigns returned zero, with each job executing all 36
  files plus the empty input. For the differential proof,
  `secp256k1_musig_sum_pubnonces` was temporarily changed to skip only index 6
  when `n_pubnonces == 7`. The pre-seven target at `042e1b2` remained green on
  all 35 pre-existing corpus files, while the new seven-signer seed aborted
  with exit 134. A debugger backtrace placed the failure in the independent
  `secp256k1_fuzz_check_musig_final_sig_equation`, before the production
  Schnorr verifier. The mutation was restored before clean verification. This
  is informational oracle hardening, not a current-master production finding,
  and does not change any severity rating.

  The field target now derives a canonical residue, computes `p - x` with a
  standalone byte-level borrow chain (special-casing zero), and compares that
  reference with `secp256k1_fe_negate` from both canonical and magnitude-8
  representations. The latter is built by adding seven copies of the field
  modulus, so the fuzzer checks both the value and the documented `m + 1`
  output budget rather than merely reusing production normalization. The
  dedicated `negation-byte-reference` seed and all eight copied field inputs
  pass on clean default and forced-`int64` ASan/UBSan builds. For isolation,
  a temporary `VERIFY`-only mutation flipped one low limb whenever
  `secp256k1_fe_negate` was called with `m == 8`; the old corpus plus the new
  seed stayed green with this helper disabled, while the enabled helper
  aborted on the focused seed on both backends. The mutation was restored
  before clean replay, and bounded `-workers=2 -jobs=2` campaigns exited 0
  without sanitizer diagnostics. This is informational/Low internal
  arithmetic-contract oracle hardening, not a clean-master production bug;
  no public or cryptographic impact was demonstrated and the master-relative
  severity ledger is unchanged.
  The field target also computes multiplication by five through repeated
  byte-level modular additions, then compares it with `secp256k1_fe_mul_int`
  and checks the documented magnitude transition from 1 to 5. This covers a
  production-used primitive that was previously used only to construct
  production-derived multiples of the modulus. The dedicated
  `small-multiplier-byte-reference` seed and all nine copied field inputs pass
  on clean default and forced-`int64` ASan/UBSan builds. A temporary
  `VERIFY`-only mutation flipped one low limb for multiplier 5 on a magnitude-1
  input; the old corpus plus the new seed stayed green with this helper
  disabled, while the enabled helper aborted on the focused seed on both
  backends. The mutation was restored before clean replay, and bounded
  `-workers=2 -jobs=2` campaigns exited 0 without sanitizer diagnostics. This
  is informational/Low internal arithmetic-contract oracle hardening, not a
  clean-master production bug; no public or cryptographic impact was
  demonstrated and the master-relative severity ledger is unchanged.
  The field target also computes the curve constant addition `x + 7` with a
  standalone byte-level carry and reduction model, then checks
  `secp256k1_fe_add_int` and its magnitude-2 nonnormalized output. The
  dedicated `add-int-byte-reference` seed and all 10 copied field inputs pass
  on clean default and forced-`int64` ASan/UBSan builds. A temporary
  `VERIFY`-only mutation flipped one low limb for addend 7 on a magnitude-1
  input; the old corpus plus the new seed stayed green with this helper
  disabled, while the enabled helper aborted on the focused seed on both
  backends. The mutation was restored before clean replay, and bounded
  `-workers=2 -jobs=2` campaigns exited 0 without sanitizer diagnostics. This
  is informational/Low internal arithmetic-contract oracle hardening, not a
  clean-master production bug; no public or cryptographic impact was
  demonstrated and the master-relative severity ledger is unchanged.

  The field target now computes inversion with a standalone 8x32-bit model:
  schoolbook multiplication, restoring binary reduction modulo p, and
  Fermat exponentiation by p - 2. It compares both `secp256k1_fe_inv` and
  `secp256k1_fe_inv_var` for canonical values, maximum-magnitude
  nonnormalized representations, and the defined zero residue. The
  dedicated `inverse-byte-reference` seed and all 11 copied field inputs pass
  on clean default and forced-`int64` ASan/UBSan builds. A temporary
  production mutation XORed the low limb by 2 for every nonzero result from
  both inverse wrappers. With the independent reference enabled, the focused
  seed aborted with exit 134 on both backends. With that reference and the
  pre-existing production-derived `x * inverse == 1` assertion both disabled,
  the same mutation passed; restoring only the reference made it abort. This
  demonstrates that the new oracle does not depend on inverse-path agreement
  or on the production multiplication implementation. The mutation was
  restored before clean replay, and bounded `-workers=2 -jobs=2` campaigns
  exited 0 without sanitizer diagnostics or failure artifacts. This is
  informational/Low internal arithmetic-oracle hardening, not a clean-master
  production bug; no public or cryptographic impact was demonstrated and the
  master-relative severity ledger is unchanged.
  The field target now models square roots with a standalone 8x32-bit
  exponentiation by `(p + 1) / 4` and independently squares the returned
  root with the byte-level reducer. It compares the success decision for a
  derived residue, its valid maximum-magnitude representation, and zero,
  while accepting either mathematical root sign. The dedicated
  `sqrt-byte-reference` seed and the complete 12-input field corpus pass on
  default and forced-`int64` ASan/UBSan builds; isolated two-worker/two-job
  campaigns exited 0 without sanitizer diagnostics. For the control proof,
  a temporary production mutation made `fe_sqrt(0)` report failure. The
  focused seed aborted with exit 134 on both backends; bypassing only the new
  byte-level helper let the identical mutation pass the legacy field checks
  on both backends. The mutation and temporary bypass were restored before
  clean replay. This is informational internal-contract oracle hardening, not
  a current-master production finding; the master-relative severity ledger is
  unchanged.

  The MuSig aggregation target now adds a gated eight-key reference at the
  first list length beyond the stateful seven-signer fixtures. It computes the
  complete `KeyAgg list` transcript independently, derives every coefficient
  with the standalone tagged-SHA model, reweights all eight public keys through
  public APIs, and compares the weighted point sum with both cached and
  cacheless `secp256k1_musig_pubkey_agg` results. It also checks the cached list
  hash. The dedicated `eight-keyagg-reference` seed is `AAA8AA` plus its
  newline; byte 157 selects the gated path and, for this seed, the eight-signer
  stateful path, while other selector bytes retain the ordinary one-through-
  seven range. Clean default and forced-`int64` ASan/UBSan fixed
  replays passed all 37 MuSig corpus files, and bounded two-worker/two-job
  campaigns exited 0 with no sanitizer diagnostics or artifacts. Matching
  default and forced-`int64` MSan fixed replays also passed all 37 files. For
  the proof, a temporary production mutation forced the aggregation callback's
  coefficient to zero only at index 7. The focused seed aborted with exit 134
  on both backends; with only the new helper bypassed, the identical mutation
  exited 0 through the legacy path. The mutation and bypass were restored
  before clean replay. This is informational arbitrary-list oracle hardening,
  not a clean-master production finding; no severity rating changes.

  The stateful MuSig signing path now extends its independent nonce,
  partial-signature, session-state, and final-signature checks from seven to
  eight participants. The existing `eight-keyagg-reference` seed is seven
  bytes (`AAA8AA` plus its newline); its ASCII `8` at selector offset 157
  selects eight signers without changing the one-through-seven mapping for
  other inputs. Clean default and forced-`int64` ASan/UBSan fixed replays
  passed the focused seed and all 37 MuSig corpus files; bounded
  two-worker/two-job campaigns exited 0. Matching default and forced-`int64`
  MSan fixed replays also passed the focused seed and all 37 files. For the
  differential proof, a temporary production mutation in
  `secp256k1_musig_sum_pubnonces` loaded but skipped only nonce index 7 when
  `n_pubnonces == 8`. The focused seed aborted with exit 134 on both field
  backends. A debugger backtrace placed the failure in the independent
  `secp256k1_fuzz_check_musig_final_sig_equation` at `musig.c:356`, before the
  production Schnorr verifier. With only the new stateful helper bypassed,
  the identical mutation passed the focused seed and all 37 corpus files on
  both backends. The mutation and bypass were restored before clean replay.
  This is informational stateful-list oracle hardening, not a clean-master
  production finding; no severity rating changes.

  The EllSwift raw XDH oracle now independently checks both `party` values. For
  `party == 0` it compares the hash input against `seckey_a * decode(ell_b)`;
  for `party == 1` it compares against `seckey_b * decode(ell_a)`. The prior
  raw check covered only party 0, while the separate symmetry check could agree
  when both sides selected the same wrong remote encoding. The dedicated
  `raw-party-both` seed is the 15-byte ASCII input `raw party both\n`; its
  derived raw A encoding starts with `0xB6` and its derived B secret is scalar
  one. Clean default and forced-`int64` ASan/UBSan fixed replays passed all 11
  EllSwift corpus files plus the empty input (12 executions), and both
  two-worker/two-job 15-second campaigns exited 0. The default workers each
  completed 94 executions; the forced-`int64` workers completed 55 and 57.
  Matching default and forced-`int64` MSan fixed replays also passed all 12
  executions. For differential proof, a temporary production mutation reloaded
  `ell_b64` instead of `ell_a64` only for `party == 1`, scalar one, and raw-A
  first byte `0xB6`; the focused seed aborted with exit 134 on both backends.
  Disabling only the new party-1 loop let that identical mutation pass the old
  harness and all 11 corpus files plus the empty input on both backends. The
  mutation and bypass were restored before the clean replay. This is
  informational oracle hardening, not a clean-master production finding; the
  `792f43f` decode/XDH history remains regression context and no severity rating
  changes.

  The MuSig target now checks the zero-length precondition of both
  `secp256k1_musig_pubkey_agg` and `secp256k1_musig_nonce_agg`. Each operation is
  called with a one-slot array and `n == 0`, then with a NULL array and `n == 0`;
  every call must invoke the illegal callback once, return zero, and leave its
  aggregate output cleared. The dedicated `empty-aggregation` seed is the
  24-byte ASCII input `empty MuSig aggregation\n`. Clean default and
  forced-`int64` ASan/UBSan fixed replays passed all 38 MuSig corpus files,
  including the dedicated empty-aggregation seed. For differential proof, a temporary
  production mutation replaced each `ARG_CHECK(n > 0)` with an early failure
  that skipped the illegal callback. The focused seed aborted with exit 134 on
  both backends for each mutation; disabling only the new helper let the
  identical mutations pass the same 38-file corpus; the corpus-directory
  runner reported 39 executions because it also executes its initial empty
  input. The mutations and bypass were restored before clean
  replay. This is informational precondition/output-oracle hardening, not a
  clean-master production finding; no severity rating changes.

  The context target now checks the documented equivalence of all four valid
  flag combinations: `SECP256K1_CONTEXT_NONE`, the deprecated `VERIFY` and
  `SIGN` flags, and their combination. For each flag it compares dynamic and
  preallocated creation, clone-size accounting, randomized public-key creation,
  and deterministic ECDSA serialization across the original and both clone
  paths. The dedicated `context/flag-matrix` seed is the 14-byte ASCII input
  `context flags\n`. Clean default and forced-`int64` ASan/UBSan fixed replays
  passed all seven context files; matching default and forced-`int64` MSan
  replays also passed. Isolated default and forced-`int64`
  `-workers=2 -jobs=2 -max_total_time=15` campaigns exited 0 after 228/231 and
  141/139 executions respectively, with no sanitizer diagnostics or artifacts.
  For the differential proof, `secp256k1_context_preallocated_size` was
  temporarily changed to return `ret - 1` whenever the deprecated `VERIFY`
  bit was present. The focused seed aborted at the new size-equivalence check
  on both ASan/UBSan backends; bypassing only the new helper left all seven
  corpus files green with the production mutation active. The mutation and
  bypass were restored before the fixed and MSan replays. This is
  informational API-oracle hardening, not a current-master production finding,
  and does not change any severity rating.

  The context target now checks the documented `secp256k1_context_randomize(ctx,
  NULL)` reset through deterministic ECDSA signing as well as public-key
  creation. After a randomized context is reset, the same message and secret
  key must serialize to the same signature produced before the reset. The
  dedicated `context/randomize-null-signature` seed is the 25-byte ASCII input
  `randomize null signature\n`. The earlier reset check compared only public
  keys, so a signing-only state regression could have passed every existing
  context oracle. For the differential proof, a temporary production
  mutation flipped one serialized signature byte only after the context's
  generator blinding had been reset to its initial state. The focused seed
  aborted with exit 134 at the new post-reset comparison; disabling only this
  helper left the pre-existing context corpus green under the mutation. The
  mutation and bypass were restored before clean replay. This is
  informational API state-transition hardening, not a current-master
  production finding, and does not change any severity rating. Clean replay
  passed all 8 context seeds under both default and forced-int64 Clang
  ASan/UBSan builds; four 15-second campaigns (2 jobs and 2 workers per
  backend) also exited cleanly with no artifacts.

  The same context target also checks the documented `NULL` reset through
  `secp256k1_schnorrsig_sign32`. It creates a keypair and deterministic
  BIP340 signature before the reset, then creates a fresh keypair and signs
  again afterward; the complete 64-byte signature must be unchanged. The
  dedicated `context/randomize-null-schnorr-signature` seed is the 33-byte
  ASCII input `randomize null schnorr signature\n`. This is an independent
  oracle because Schnorr signing has its own x-only parity and response path;
  the earlier ECDSA check did not execute it. For the differential proof,
  `secp256k1_schnorrsig_sign_internal` was temporarily changed to flip
  `sig64[0]` after `secp256k1_memczero` only when
  `ctx->ecmult_gen_ctx.proj_blind == secp256k1_fe_one`. The focused seed
  aborted with exit 134; bypassing only the new Schnorr helper left the
  pre-existing eight-file context corpus green under that mutation. The
  mutation and bypass were restored before clean replay. Default and
  forced-int64 Clang ASan/UBSan fixed replays passed the focused seed and all
  nine context files, and two 15-second campaigns (2 jobs and 2 workers per
  backend) exited 0 without sanitizer diagnostics or artifacts. This is
  informational API state-transition hardening, not a current-master
  production finding, and does not change any severity rating.

  The field target now combines the two dimensions of the documented
  `secp256k1_fe_mul` contract that were previously exercised separately: both
  operands use the largest accepted nonnormalized magnitude (8), while the
  output aliases the first operand (`r == a`). After normalization, the result
  is checked against an independent 256-bit schoolbook modular multiplication,
  not merely against a second production multiplication. The dedicated
  `field/nonnormalized-mul-alias` seed is the 24-byte ASCII input
  `nonnormalized mul alias\n`. Clean default and forced-`int64` ASan/UBSan
  replays passed all 13 tracked field files (509 bytes total), and matching
  default and forced-`int64` MSan replays also passed all 13 files. The
  two-worker/two-job ASan/UBSan campaigns exited 0: default workers completed
  341 and 348 executions, while forced-`int64` workers completed 335 and 334,
  with no sanitizer diagnostic, assertion, timeout, OOM, or crash artifact.
  For the differential proof, both backend `secp256k1_fe_impl_mul` functions
  were temporarily changed under `VERIFY` to flip one output limb only when
  `r == a` and both input magnitudes were 8. The exact seed then aborted with
  exit 134 on both ASan/UBSan builds using `-handle_abrt=0`; bypassing only the
  new alias block left the pre-existing 41-byte
  `field/nonnormalized-arithmetic` seed green on both backends with the
  mutation active. All temporary mutations and bypasses were restored before
  the fixed replays. This is informational internal-oracle hardening, not a
  current-master production finding, so it does not change any severity rating.

If a clean-master replay stops at an earlier known failure, isolate the later
contract with its dedicated seed or a minimal production mutation. Do not
claim the later behavior was tested merely because a follow-up fix lets the
full harness continue.

## 2026-07-14 Baseline Campaign

The pre-follow-up audit snapshot (`82b91c1`) was replayed against the
clean-master baseline `ebf594320dc838b9de1abb54d5ba98cef84f4297` after the
remaining target inventory found no independent contract that justified
another oracle. A fresh Clang
libFuzzer ASan/UBSan build passed every checked-in seed: `api_roundtrip` 28,
`context` 7, `ecdh` 5, `ecmult_const` 5, `ecmult_multi` 12, `ellswift` 11,
`field` 13, `group` 12, `hash` 8, `musig` 39, `recovery` 7, `scalar` 4,
`schnorrsig` 11, and `xonly_tweak` 8. The matching MSan build passed the same
14 target counts, with no sanitizer diagnostic, assertion, timeout, or crash.

Each target also completed a bounded `-workers=2 -jobs=2` campaign with
`-max_total_time=15` using disposable copies of the checked-in corpora. The
worker jobs exited 0 for all 14 targets, including the full 39-input MuSig
stateful corpus. Logs and generated minimization inputs were kept in private
temporary directories and are not part of the repository. The default
x86_64-wide-multiply sanitizer configuration could not compile under the
installed Clang 22 because the scalar inline assembly exhausted registers;
the passing sanitizer builds use the project's supported test-only
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` backend. This is a toolchain
limitation, not a production failure.

A separate GCC 16.1 ASan/UBSan build used the native x86_64 assembly and the
normal 4x64/5x52 field backend. It replayed the same 14 target corpora (170
files total) and each target's empty-input path with exit 0 and no sanitizer
diagnostic. Together, the two builds cover both the native backend and the
forced-int64 sanitizer configuration; neither produced a new clean-master
finding.

This campaign produced no new clean-master production finding and does not
change any severity rating. Existing findings remain classified against clean
master, independent of later fork fixes or optimization stacks.

## 2026-07-14 Direct Multi-Worker Replay

The current branch was rebuilt with Clang 22.1.7 using CMake's libFuzzer
runtime, ASan/UBSan, frame pointers, and the supported test-only
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` backend. All 14 fuzz binaries
linked successfully. Disposable copies of the current 183-file corpus were
then run from separate working directories with
`-workers=2 -max_total_time=30`; per-target logs and artifact directories were
isolated so libFuzzer worker output could not be confused between targets.

The exact checked-in corpus sizes and completed execution counts were:
`api_roundtrip` 30/342, `context` 9/402, `ecdh` 5/432,
`ecmult_const` 5/538, `ecmult_multi` 12/148, `ellswift` 12/179,
`field` 13/671, `group` 13/947, `hash` 9/45224, `musig` 43/44,
`recovery` 8/450, `scalar` 4/1644, `schnorrsig` 11/238, and
`xonly_tweak` 9/75. Every target exited 0. No sanitizer diagnostic,
assertion failure, timeout, crash, or artifact was produced.

This is a clean-master campaign result, not evidence that a fork patch makes
the baseline safe. It found no independent oracle gap and no production bug,
so no seed or implementation change was added. The earlier `-jobs` manager
experiment is not used as proof: its worker logs were shared by libFuzzer;
the direct-worker replay above is the authoritative result.

As a follow-up branch verification, a normal CMake build with all six optional
modules, tests, exhaustive tests, and the 14 non-libFuzzer harnesses completed
successfully. The resulting CTest run passed all 239 tests, including the full
exhaustive suite and every checked-in fuzz corpus. This confirms the recorded
sanitizer campaign was not relying on a sanitizer-only build configuration.

The audit continued after this snapshot with focused, separately verified
oracles: `64c0b7f` extended the RFC6979 stream, `1e8b152` covered a
zero-derived MuSig nonce, `a8ecc01` and `31046b6` covered ordinary and
recoverable ECDSA retry failure, and `f3b8034` covered consumed MuSig secret
nonce reuse. Those commits are part of the current branch (`f3b8034`); their
mutation proofs and sanitizer replays must not be retroactively attributed to
the snapshot above.

The MuSig follow-up then added an explicit empty partial-signature aggregation
oracle. It calls `secp256k1_musig_partial_sig_agg` with a valid session,
`n_sigs == 0`, and both a valid array pointer and `NULL`; each call must report
the documented illegal argument, invoke the callback once, and leave the
64-byte output zeroed. The focused `empty-aggregation` corpus input is the
24-byte ASCII string `empty MuSig aggregation\n`. The fixed Clang ASan/UBSan
replay passed all 42 current MuSig inputs, and two independent parallel full
corpus replays also exited 0. A separate Clang ASan/UBSan libFuzzer build ran
two managers with `-workers=2 -jobs=2 -max_total_time=15` over isolated copies;
all four worker jobs exited 0 without sanitizer diagnostics, assertion
failures, timeouts, or artifacts. A native GCC x86_64-assembly build also
replayed all 42 inputs successfully. For the causal proof,
`src/modules/musig/session_impl.h` was temporarily mutated from
`ARG_CHECK(n_sigs > 0)` to an always-true condition. The focused seed then
aborted at the new assertion with exit 134; bypassing only the new fuzzer call
let the same production mutation pass. The production guard and oracle were
restored before the fixed replay. This is informational API-oracle hardening,
not a clean-master production finding, and does not change any severity
rating.

The post-snapshot contract review also recorded three deliberate no-edit
decisions. `secp256k1_ecmult_const_xonly` has no documented output state on
failure and returns before writing its result for an invalid x-coordinate, so
requiring a sentinel value would be an overbroad oracle. A MuSig cache's
`pks_hash` is transcript state that cannot be validated after an opaque cache
is supplied without the original participant list; requiring rejection of an
arbitrary hash would likewise invent a contract. Finally, a separate zeroed
keypair Schnorr seed would exercise the same `keypair_load` rejection already
proved by the mismatched-keypair oracle. These cases remain documented audit
boundaries rather than duplicate corpus entries.

## 2026-07-14 EllSwift BIP324 Optional-Data Oracle

The EllSwift target now exercises the explicit `secp256k1_ellswift_xdh_hash_function_bip324`
contract that its `data` argument is ignored. The new
`bip324-ignored-data` seed passes a non-NULL 64-byte sentinel through both the
exported callback and the context-aware `secp256k1_ellswift_xdh` dispatch, and
compares both results with the independent BIP324 transcript reference. The
existing BIP324 checks used only `data == NULL`, so a regression in either
dispatch path could previously survive the reference, symmetry, and raw-X
checks.

This is informational API-contract oracle hardening, not a clean-master
production finding. For the differential proof, a temporary mutation after
BIP324 finalization XORed `output[0]` with the first byte of non-NULL `data`.
With the new assertions enabled, the focused `bip324-ignored-data` replay
aborted with exit 134. With only the new assertions disabled, all 11
pre-existing EllSwift seeds remained green under that identical mutation,
proving that the old BIP324 reference, symmetry, and raw-X checks did not
exercise the ignored-data contract. The mutation was restored before the clean
replay.

The restored forced-int64 Clang ASan/UBSan build passed all 12 seeds,
including `bip324-ignored-data`; a native GCC ASan/UBSan build passed all 12
seeds; and a two-manager, two-worker, 10-second Clang campaign exited 0 for
both jobs without sanitizer diagnostics, assertion failures, or artifacts.
This does not change any master-relative severity rating.

## 2026-07-14 MuSig Optional Secret-Key Nonce Oracle

The MuSig target now exercises the documented `secp256k1_musig_nonce_gen`
branch where `seckey == NULL`. The signer public key remains present, while
the independent nonce transcript is replayed across both secret-key states
and all eight combinations of optional message, key-aggregation cache, and
extra-input pointers. Each case uses a separately derived session random
value and checks the secret nonce bytes, public nonce serialization, and the
required successful-call clearing of `session_secrand32`.

The focused `nonce-gen-optional-seckey` seed is the 28-byte ASCII input
`MuSig nonce optional seckey\n`. This is informational cryptographic-domain
oracle hardening, not a clean-master production finding. For the differential
proof, the production branch that copies `session_secrand32` when `seckey ==
NULL` was temporarily changed to `memset(rand, 0, sizeof(rand))`. With the new
helper enabled, the focused replay aborted with exit 134. With only that
helper disabled, all 42 pre-existing MuSig seeds remained green under the
same mutation, proving that the prior corpus did not exercise the omitted-key
transcript. The mutation and bypass were restored before clean replay.

The restored forced-int64 Clang ASan/UBSan build passed all 43 MuSig seeds in
949 seconds; the focused native GCC ASan/UBSan replay passed; and a
two-manager, two-worker Clang campaign over the focused seed and one
pre-existing seed exited 0 for both jobs without sanitizer diagnostics,
assertion failures, or artifacts. Master correctly treats a missing secret
key as a distinct nonce derivation mode, so this does not change any
master-relative severity rating.

## l0rinc Fork Duplicate Audit

The l0rinc remote and all pull-request heads were refreshed against the same
`origin/master` baseline (`ebf594320dc838b9de1abb54d5ba98cef84f4297`) on
2026-07-13. The exact head mapping below is a replay ledger, not a claim that
the hashes must be ancestors of this branch: equivalent fixes were often
cherry-picked with new proof, and several heads are optimization stacks.

- PR #1 (`6e60f8d`) and PR #2 (`51e93c4`) repeat the existing MuSig, hash,
  EllSwift, ECDH, callback, opaque-key, scratch-size, and output-cleanup
  findings. The scratch-size guard (`d7e3b49`/`b827e0e`) remains an internal
  robustness fix, not a public High-severity vulnerability, as recorded above.
  The relevant production fixes and fuzzer barriers are already in this
  branch, including `f1e3eba`/`5ad8052`,
  `3618f4e`/`f9f1a6e`, `7075ed0`/`f48cd99`, and the MuSig cleanup series.
- PR #3 (`7ed2abc`) repeats invalid-secret MuSig nonce-output coverage. The
  public nonce has no cryptographic meaning here, so its clearing is Low
  severity stale-state hygiene, not critical secret erasure (`52c573b`).
- PR #4 (`b9a169b`) is an optimization stack, but it is **not behavior-neutral
  on top of this audit branch**. In addition to inlining and hash fast paths,
  it removes HMAC/RFC6979 and EllSwift temporary clearing, restores unchecked
  ECDH public-key loading and built-in failure-output behavior, and drops the
  EllSwift zero-`u` and callback-input guards. Applying it here would revert or
  mask already-rated clean-master findings (`5cfe7f7`, `35ffb87`, `119b407`,
  `067d4a3`, and the cleanup series), so it was deliberately not cherry-picked.
  Its optimizations may only be evaluated after those barriers are reapplied
  and independently replayed. PR #5 (`f06920c`) and PR #6 (`ac915c9`) are
  force-inline/field optimization subsets with no distinct production oracle;
  they must not be treated as proof that the affected master behavior was safe.
- PR #7 (`3f5fafa`) and PR #9 (`3f5fafa`, the same head) contain comments and
  test-maintenance changes. Their BER test encoding and aggregate-nonce
  assertions do not supersede the production findings or their dedicated
  mutations on this branch.
- PR #8 (`248be19`) combines the PR #4 stack with `104f53e` hardening and
  test/tool follow-ups. Its BER test change is covered by `d4d1519`/`0cf6f5d`,
  but the inherited PR #4 behavior changes still conflict with the cleanup and
  failure barriers above. Replaying that stack after the barriers would be a
  separate optimization experiment, not evidence that clean master was safe.
- PR #10 (`65d38b0`) contains two boundary fixes. The `fe_equal` bound change
  is already in master and is retained as regression coverage. The 10x26
  magnitude-32 normalization overflow was a real **Medium** latent
  current-master correctness bug, fixed separately in `cf5631f`, with the
  deterministic boundary proof in `7ef0714` and the fuzzer oracle in
  `139f6ab`. The commit message records why its impact can become High if
  valid maximum-magnitude internal state is reached; the current public
  trigger remains unproven. The fork patch did not touch
  `normalizes_to_zero{,_var}`: clean master separately accepted a valid
  magnitude-32 nonzero state as zero after two uint32 carry wraps. That is a
  distinct **Medium/latent** finding, now covered by the
  `zero-predicate-false-positive` seed and the current branch's uint64 carry
  repair; it must not be treated as fixed or severity-reduced by PR #10's
  normalize-only changes.
- PR #11 (`d1dca5c`) repeats the checked `pubkey_load` return paths already
  covered by `5ad8052`, `f9f1a6e`, and `7767442`.
- PR #12 (`944932c`, force-updated from `e153e26` on 2026-07-13) is exactly the
  behavior-preserving 5x52 word-serialization optimization already recorded as
  `91e4f02`; the force update did not change its source tree. Its commit
  intentionally follows the master-based findings and states that it is not
  security evidence.
- The l0rinc branch `l0rinc/l0rinc/field-5x52-serialize-word` was subsequently
  force-updated to `e217ead` on 2026-07-14. It adds the analogous 10x26
  word-serialization hunk beside the already represented 5x52 change. The
  commit was cherry-picked here because Git applies only that serializer hunk
  on top of the audit branch; the branch's existing `uint64_t` carry repairs in
  `normalizes_to_zero{,_var}` and magnitude-32 normalization remain intact.
  The hunk is byte-output equivalent and independently covered by the field
  reference oracle, so it is an optimization replay, not security evidence.
  Verification in an isolated autotools worktree used
  `CC=clang CFLAGS='-O1 -g -fsanitize=address,undefined
  -fno-omit-frame-pointer
  -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64'` with matching sanitizer
  `LDFLAGS`, then `make -j"$(nproc)" fuzz_field` and
  `./fuzz_field src/fuzz/corpora/field/*`; the replay returned 0 for every
  seed. `make check TESTS=fuzz_field` also reported `PASS fuzz_field` with no
  errors.

Apart from `e217ead`, no new l0rinc commit was cherry-picked in this refresh:
every other relevant commit is either already represented with stronger
current-master proof, already in master, or changes performance/comments
without adding a contract. This keeps the discovery order intact. Fork patches
are never used to prove that a current-master failure was absent; every
important barrier still has its own seed or minimal production mutation, and
severity is always assigned against the clean baseline before later fixes are
applied.

The additional fork refs were audited on 2026-07-13. The
`musig-cleanup-failures` branch (`bb02b1e`) is an older cleanup stack whose
production edits are already represented by the current MuSig cleanup commits
and the stronger opaque-state barriers here; replaying it would duplicate or
remove those barriers. `detached10` through `detached19` are alternative
force-inline or field-CMOV optimization snapshots. `detached20` through
`detached22` are broader snapshots containing the same optimization and
behavior-changing stacks already classified above. None adds a distinct
clean-master finding or a reason to change an existing severity rating.

The 2026-07-14 refetch also exposed three commits that are not useful
cherry-picks for this branch. `l0rinc/l0rinc/scratch-free-warning`
(`c0f32d4`) changes only `src/tests.c`: it moves the deliberately malformed
scratch object from stack storage to an already allocated heap object so a
GCC warning does not suggest that the error path can free stack memory. This
is a test-build warning fix, not a production contract or fuzz oracle. The
MuSig aggregate-nonce test merge (`8363a2d`) compares all 66 serialized bytes
instead of the first 33, which is deterministic test maintenance already
covered by the aggregate-nonce and full-signing checks here. Finally,
`detached4` (`3a5e9f3`) is a 94-line Strauss `no == 1` performance fast path;
it changes the control-flow implementation without adding a contract. It
must be benchmarked and differentially replayed as an optimization experiment,
not cherry-picked into the discovery stack or used to downgrade a master
finding. These decisions leave the current branch's barriers intact and do
not alter any clean-master severity rating.

For context, a temporary bounds-preserving port of the `no == 1` fast path
was applied on top of this branch because the original snapshot conflicted
with the current `bits_na_*` and generator-bit guards. That port passed
`tests`, `noverify_tests`, all 14 native Clang corpus replays, and all 14
forced-int64 Clang ASan/UBSan corpus replays. This differential result shows
no observed behavior or sanitizer regression, but it is still not evidence
that the optimization belongs in the oracle stack; promoting it would need a
separate upstream-quality patch, review, and benchmark record.

## 2026-07-14 Optional-Module CMake Matrix

The optional-module matrix found a branch-only fuzz-infrastructure defect
before runtime replay. With
`-DSECP256K1_BUILD_FUZZ=ON -DSECP256K1_FUZZ_USE_LIBFUZZER=OFF`, all six
configurations failed at the `bin/fuzz_hash` link step. The internal CMake
fuzz rule links only `secp256k1_precomputed` and `secp256k1_asm`, while
`fuzz/hash.c` included `fuzz.h`; its generic static helpers therefore emitted
unresolved references to public functions such as context creation, secret-key
verification, public-key parsing, and ECDSA signature serialization. The
failure is reproducible in the audit branch before this fix. `hash.c` is an
audit-branch addition and is absent from the clean `origin/master` baseline,
so this is **Informational / Low fuzzer-infrastructure**, not a production
finding and not a change to any master-relative severity.

The minimal repair makes `fuzz/hash.c` include `../secp256k1.c` before
`fuzz.h`, matching the existing internal `scalar`, `field`, and `group`
harnesses. This supplies the implementation used by the internal target
without changing the library or its public behavior. The six matrix options
were `ECDH=OFF`, `ELLSWIFT=OFF`, `MUSIG=OFF`, `SCHNORRSIG=OFF` with `MUSIG=OFF`,
`EXTRAKEYS=OFF` with `SCHNORRSIG=OFF` and `MUSIG=OFF`, and `RECOVERY=ON` with
all modules enabled. Every configuration rebuilt successfully with
`cmake --build ... --parallel 3`.

The post-fix CTest replay passed all 1,233 tests in the matrix: respectively
218, 208, 206, 189, 174, and 238 tests for those configurations. This
included 71 corpus suites and every enabled seed for `api_roundtrip`,
`context`, `hash`, `scalar`, `field`, `group`, `ecmult_const`,
`ecmult_multi`, `ecdh`, `ellswift`, `xonly_tweak`, `recovery`, `schnorrsig`,
and `musig`. No assertion, timeout, crash artifact, or nonzero test result
occurred. The initial link failure and the complete
post-fix matrix are the proof that the change restores coverage availability;
they do not establish or reduce a production vulnerability on clean master.

A separate all-module CMake build with Clang ASan/UBSan, frame pointers, and
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` also linked every target. Its
13 `secp256k1_fuzz` suites passed with no sanitizer diagnostic, assertion
failure, timeout, or artifact. This sanitizer replay is an independent check
of the repaired internal translation unit, not a production-bug claim.

## 2026-07-14 Tagged SHA-256 NULL-Input Oracle

The context target now exercises the fail-closed output contract for the
deliberate invalid-input calls to `secp256k1_tagged_sha256`. The public
declaration marks `tag` and `msg` non-NULL, and the API must reject both
through the illegal callback. Because the output size is fixed and known, the
prefilled 32-byte `hash32` must also be zeroed before the argument-check return.
The check is called through a function pointer so the fuzzer can intentionally
exercise the non-aborting callback boundary without letting the declaration's
compiler attribute suppress the invalid call. The dedicated
`tagged-sha256-null-inputs` seed is the 26-byte ASCII input
`tagged SHA256 null inputs\n`; it covers `(tag == NULL, msg != NULL)` and
`(tag != NULL, msg == NULL)`, with zero lengths in both cases.

This reiterates the existing clean-master fail-open output finding fixed by
`c02dc5e`: **Low/Medium API-state severity**, unchanged. It is not a new
production vulnerability or memory-corruption claim, and it does not change
the existing rating. Clean master's unit tests count the illegal callback for
these NULL cases; the earlier context fuzzer checked impossible lengths but
did not pass NULL tag/message pointers or assert output invalidation.

For differential proof, the production condition `tag == NULL || msg == NULL`
was temporarily removed from the pre-`ARG_CHECK` cleanup in
`secp256k1_tagged_sha256`. With only the new helper bypassed, all nine
pre-existing context seeds stayed green on both default and forced-int64 Clang
ASan/UBSan builds. Restoring the helper under the same mutation made the
focused seed exit 134 with `-handle_abrt=0` on both builds. The production
mutation and harness bypass were restored before the clean replay.

The restored default and forced-int64 Clang ASan/UBSan builds passed all ten
context corpus files, including the new seed. Two-worker/two-job bounded
campaigns on both backends completed with every job exiting 0 and no sanitizer
diagnostic or crash artifact. This is oracle hardening, not a clean-master
production fix; no severity rating changes.

## 2026-07-14 Core Parser NULL-Input Oracle

The API target now has one focused fail-closed matrix for the core parser
outputs: `secp256k1_ec_pubkey_parse`,
`secp256k1_ecdsa_signature_parse_der`, and
`secp256k1_ecdsa_signature_parse_compact`. Each call passes a NULL input
through a function pointer, so the fuzzer can install a counting illegal
callback without the public non-NULL attributes turning the probe into a
compile-time contract violation. A prefilled output must be zeroed, and each
invalid call must report exactly one illegal argument.

The `null-parser-inputs` seed is the 19-byte ASCII input
`NULL parser inputs\n`. This reiterates the existing clean-master fixed-output
finding for the DER and compact ECDSA parsers: **Low/Medium API-state
severity**, unchanged. Upstream `origin/master` already clears the public-key
output before its NULL-input check, so that case is a regression guard. The
upstream DER and compact parsers reject NULL without clearing a prefilled
signature; the earlier `c02dc5e` production fix in this audit branch adds that
cleanup and deterministic assertions. The gap was that the API fuzzer
exercised malformed buffers but not NULL input pointers for all three core
parsers. This is stale-output/fail-closed state, not a memory-corruption or
cryptographic claim.

For differential proof, each production condition was isolated separately: the
pubkey clear was temporarily guarded by `input != NULL`, while each ECDSA clear
was removed only on its NULL-input path. With the focused helper bypassed, all
29 pre-existing API corpus files remained green under each mutation on both
default and forced-int64 Clang ASan/UBSan builds. Restoring only the helper made
`null-parser-inputs` abort with exit 134 for each mutation on both backends. The
mutations and helper bypass were restored before the final clean replay.

The final default and forced-int64 Clang ASan/UBSan replays pass all API corpus
files, including this seed. Two-worker/two-job bounded libFuzzer campaigns on
both backends completed with every job exiting 0 and no sanitizer diagnostic or
crash artifact. This commit adds no production behavior change and does not
alter any master-relative severity rating.

## 2026-07-14 Jacobian/Affine Equality Negative Oracle

The group target now exercises the false branch of
`secp256k1_gej_eq_ge_var` with a fixed finite generator. It derives the affine
generator and its doubled point, verifies through the independent serialized
coordinate representation that they differ, and requires Jacobian/affine
equality to reject that pair. It also checks finite-vs-infinity and
infinity-vs-finite rejection, while retaining the explicit infinity-vs-infinity
true case. The focused `jacobian-affine-equality-negative` seed is the 34-byte
ASCII input `Jacobian affine equality negative\n`.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production bug: the unit-test matrix already covers direct negative equality
pairs, but the group fuzzer's equality calls were overwhelmingly true-only and
did not carry that independent serialized rejection oracle. The rating is
therefore relative to the clean master behavior and is not upgraded merely
because a hypothetical bad equality implementation could affect downstream
arithmetic.

The proof mutation replaces `secp256k1_gej_eq_ge_var` with an unconditional
true return. With the new helper bypassed, all pre-existing group corpus files
remain green, demonstrating that their positive-only equality assertions do
not detect this false-positive regression. Restoring the helper makes the
focused seed abort at the first finite-vs-doubled assertion under that same
mutation. The mutation and bypass were restored before the clean replay. The
final default and forced-int64 Clang ASan/UBSan replays each ran all 14 group
seeds once and exited 0 without a sanitizer diagnostic. A default campaign
with `-workers=2 -jobs=2 -max_total_time=15` completed both jobs with exit 0;
the forced-int64 campaign used the same two-worker/two-job shape with
`-max_total_time=10`, completed both jobs with exit 0, and reported zero
artifact files. Both campaigns used disposable corpus copies outside the
worktree, so no generated input or crash artifact was retained.

## 2026-07-14 Field Clear Oracle

The field target now exercises the documented internal cleanup primitive
`secp256k1_fe_clear` directly. It first creates a nonzero field object, clears
it, marks the result defined for memory-sanitizer-aware checking, and compares
the complete object representation against an independently zero-initialized
byte array. The `field-clear` corpus seed is the 12-byte ASCII input
`field clear\n`; the input does not select a special path, but records the exact
replay that proves the oracle is active.

This is **Informational / Low internal secret-state hygiene**, not a clean-master
production vulnerability. Field elements can carry secret intermediate values,
so the documented cleanup contract is worth preserving, but this check does not
establish that clean master leaks a cryptographically meaningful value. That is
also why it is materially different from public nonce cleanup: a nonce with no
cryptographic meaning does not require a Critical erasure rating.

The proof mutation temporarily changes `secp256k1_fe_clear` into a no-op. With
the new check bypassed, every pre-existing field corpus file remains green,
showing that the old field oracles did not observe this cleanup contract.
Restoring only the check makes `field-clear` abort with exit 134 under the same
mutation. The production mutation and helper bypass were restored before the
clean replay. Default and forced-int64 Clang ASan/UBSan replays then ran every
field seed once, and bounded two-worker/two-job campaigns completed with exit 0
and no sanitizer diagnostics or retained artifacts.

## 2026-07-14 Refreshed Fork Replay: `fe_equal` Bound

The refreshed l0rinc head `l0rinc/l0rinc/fe-equal-magnitude-bound`
(`994b350`) was reviewed against this branch. Its production hunk is the same
`secp256k1_fe_equal` maximum-`b`-magnitude correction already represented by
`161a39a`; the current field fuzzer also has deterministic magnitude-30 equal
and unequal checks. Its additional randomized unit-test wrapper is useful
context but does not add a distinct fuzz oracle, so it was not cherry-picked.
The commit describes a **Low / VERIFY-only internal precondition** issue, not a
new public or runtime security finding. This duplicate decision leaves the
existing master-relative severity and the stronger minimal-mutation proof
unchanged.

## 2026-07-14 Affine Group Cleanup Oracle

The group target now exercises the documented internal cleanup primitive
`secp256k1_ge_clear` directly. It starts from the nonzero affine generator,
clears the complete object, marks the result defined for memory-sanitizer-aware
checking, and checks the full representation with the existing independent
byte scanner. The focused `ge-clear` corpus seed is the 8-byte ASCII input
`ge clear\n`; the input does not select a special branch, but records the exact
replay that proves the oracle is active.

This is **Informational / Low internal secret-state hygiene**, not a clean-master
production vulnerability. Affine points can be sensitive intermediate state,
so preserving the cleanup contract is useful, but current master already
clears the object and this check does not establish a leak. It is also distinct
from public nonce cleanup: a nonce with no cryptographic meaning is not a
Critical erasure finding.

The proof mutation temporarily changes `secp256k1_ge_clear` into a no-op. With
the new helper bypassed, all pre-existing group corpus files remain green,
showing that the old group target did not observe this affine cleanup path.
Restoring only the helper makes `ge-clear` abort with exit 134 under the same
mutation. The production mutation and helper bypass were restored before the
clean replay. Default and forced-int64 Clang ASan/UBSan replays then cover the
full group corpus, and bounded two-worker/two-job campaigns cover the focused
seed without sanitizer diagnostics or retained artifacts.

## 2026-07-14 ecmult_multi Independent Equality Oracle

The multi-scalar target now checks successful results through canonical affine
coordinates and a 64-byte serialization, while retaining `secp256k1_gej_eq_var`
as a consistency check. This applies to the no-scratch/scratch comparison,
Pippenger one- and two-batch paths, Strauss batches, and the empty-input path.
It also adds a fixed finite-generator-versus-infinity negative barrier so a
false-positive `gej_eq_var` cannot silently make every positive result look
correct. The focused `equality-comparator-barrier` seed is the 30-byte ASCII
input `ecmult multi equality barrier\n`.

This reiterates an **Informational / Low internal-oracle gap**, not a
clean-master production finding. `secp256k1_gej_eq_var` is implemented with a
group-addition test; using it as the only result comparator coupled the target
to the same arithmetic family it was meant to check. Clean master produces
the expected results, and no public reachability, cryptographic impact, or
master-relative severity increase is claimed. The existing internal
multi-scalar result behavior remains unchanged.

The control mutation temporarily changed `secp256k1_gej_eq_var` to return
true unconditionally. The pre-change target loaded all 12 existing
`ecmult_multi` corpus files and stayed green under that mutation, showing that
its old positive-only equality checks and independent arithmetic reference did
not exercise the comparator's false branch. With the new barrier enabled, the
30-byte focused seed aborted with exit 134. Bypassing only that new barrier
made the identical mutation pass, proving that the barrier, rather than an
unrelated target assertion, detects the regression. The production mutation
and temporary harness bypass were restored before replay.

The restored default and forced-`int64` Clang ASan/UBSan builds passed all 13
tracked corpus files. Bounded `-workers=2 -jobs=2 -max_total_time=12`
campaigns on both backends completed with every manager and worker exiting 0,
without sanitizer diagnostics, assertion failures, timeouts, OOMs, or crash
artifacts. This is oracle hardening only and does not alter the master-relative
production severity ledger.

## 2026-07-14 MuSig `nonce_process` Cache-Failure Reiteration

The MuSig target now exercises the failure transition where the aggregate
nonce is valid but the opaque key-aggregation cache has an invalid magic
value. `secp256k1_musig_nonce_process` must report failure, invoke the illegal
argument callback once, preserve the rejected cache bytes, and leave its
caller-owned session fully zeroed. The focused
`nonce-process-invalid-cache-cleanup` seed is the 36-byte ASCII input
`nonce process invalid cache cleanup\n`.

This reiterates the existing clean-master finding fixed on this branch by
`ea0fff3` (`musig: clear failed nonce process sessions`): **Low to Medium
severity**. On `origin/master` at `ebf5943`, the invalid cache return preceded
the session clear, so a caller that ignored the return value could retain and
reuse an earlier MuSig transcript. That is stale signing-state authority, not
proven memory corruption, key disclosure, or signature forgery. The branch
already contains the production fix; this follow-up changes no production
behavior and adds the cache-specific fuzz oracle that was missing from the
earlier invalid-aggregate check. A nonce without cryptographic meaning would
not justify a Critical cleanup rating.

For the causal proof, the clean-master behavior was modeled by temporarily
guarding the branch's production clear so an invalid cache magic skipped it,
while all other `nonce_process` behavior remained unchanged. The pre-existing
43-input MuSig corpus was replayed with the new helper bypassed and remained
green under that mutation, demonstrating that its existing invalid-aggnonce
check did not cover this valid-aggregate/invalid-cache ordering. Restoring
only the new helper made the focused seed abort with exit 134; the mutated
cache bytes and callback count assertions identify the exact failing
transition. The production mutation and bypass were restored before final
clean replay. The final forced-int64 Clang ASan/UBSan replay and native GCC
x86_64-assembly ASan/UBSan replay each passed all 44 MuSig corpus inputs. A
disposable Clang libFuzzer campaign with
`-workers=2 -jobs=2 -max_total_time=12` ran both jobs to exit 0 over the same
44-input corpus, without sanitizer diagnostics, assertion failures, timeouts,
or crash artifacts. The existing `ea0fff3` production fix remains the fix for
the master-relative finding; this commit adds its missing fuzzer proof.

## 2026-07-14 MuSig `nonce_process` NULL-Input Reiteration

The MuSig target now covers all three exported NULL-input failure transitions
of `secp256k1_musig_nonce_process`: missing aggregate nonce, message, and
key-aggregation cache. Each call starts with a previously valid session, must
return zero, invoke the illegal-argument callback exactly once, clear the
caller-owned session, and leave the non-NULL opaque inputs byte-for-byte
unchanged. The focused `nonce-process-null-input-cleanup` seed is the
33-byte ASCII input `MuSig nonce_process NULL cleanup\n`.

This reiterates the same clean-master stale-session finding as `ea0fff3`
(`musig: clear failed nonce process sessions`): **Low to Medium severity**.
At `origin/master` `ebf5943`, `nonce_process` did not clear `session` before
the NULL-input argument checks, so an ignored failure could leave an earlier
signing transcript live. This is stale signing-state authority, not proven
memory corruption, key disclosure, or signature forgery. The branch already
contains the production clear; this commit changes no production behavior.
As with the prior cache-specific reiteration, cleanup of a nonce without
cryptographic meaning would not justify a Critical rating; the session is the
cryptographically meaningful state here.

For causal proof, a temporary production mutation made the session clear run
only when all three input objects were non-NULL, exactly modeling the old
master failure ordering while leaving successful and malformed-object paths
unchanged. With the new helper bypassed, all 44 pre-existing MuSig inputs
remained green under that mutation. Restoring only the helper made the focused
seed abort at the first NULL-input transition, while the helper covers all
three transitions in the clean run, proving that the old corpus did not
cover these argument-check exits. The mutation and bypass were restored before
the final clean replay. The final forced-int64 Clang ASan/UBSan and native GCC
x86_64-assembly ASan/UBSan replays pass all 45 MuSig corpus inputs, and a
disposable two-manager/two-worker Clang campaign has both managers exit 0
without sanitizer diagnostics, assertion failures, timeouts, or artifacts. The existing
`ea0fff3` production fix remains the fix for the master-relative finding.

## 2026-07-14 MuSig `partial_sign` NULL-Argument Reiteration

The MuSig target now covers the three post-load NULL-argument exits of
`secp256k1_musig_partial_sign`: missing keypair, key-aggregation cache, and
session. Each call starts with a valid secret nonce and a prefilled partial
signature, must report failure and invoke the illegal-argument callback once,
zero the partial-signature output, consume the secret nonce, and leave every
non-NULL opaque input unchanged. The focused
`partial-sign-null-argument-cleanup` seed is the 32-byte ASCII input
`MuSig partial sign NULL cleanup\n`.

Coverage review corrected the placement of this probe in the earlier commit:
the old call site ran after the normal signing loop had consumed and zeroed
`secnonce[0]`, so the helper stopped at the pre-load invalid-secnonce return
and did not actually reach the three post-load argument checks it described.
This commit runs it before that loop, preserving the valid nonce copy for the
named keypair, cache, and session transitions. The prior evidence should
therefore be read as a fuzzer-oracle claim that required this correction, not
as proof that those branches had already been exercised.

The correction was separately reachability-proven: a temporary mutation that
skipped the callback and cleanup only for `"keypair != NULL"` made the existing
`partial-sign-null-argument-cleanup` seed abort with status 134 on both the
default and forced-int64 sanitizer builds. The same mutation would have been
inert at the former post-loop call site, where the copied nonce failed before
the post-load checks.

This reiterates the clean-master stale-output and secret-state finding fixed by
`a8457e2` and `9f0e948`: **Low to Medium severity**. At `origin/master`
`ebf5943`, a caller that ignored a post-load argument failure could retain an
apparently usable partial signature, while the failure path also relied on
stack-local cleanup for loaded signing scalars. The public secret nonce was
already invalidated after load, so this oracle does not claim a nonce-reuse
bug on clean master. It also does not claim a direct key disclosure or
signature forgery; a nonce with no cryptographic meaning would not justify a
Critical cleanup rating, while the secret nonce and signing output remain
meaningful state. The branch already contains the production fixes; this
commit changes no production behavior.

For causal proof, a temporary production mutation changed the output clear to
run only when `partial_sig != NULL && keypair != NULL && keyagg_cache != NULL
&& session != NULL`, modeling the old NULL-argument ordering while preserving
the invalid-cache and invalid-secnonce paths. With only the new helper
bypassed, all 45 pre-existing MuSig inputs remained green. Restoring the
helper made the focused seed abort at the first missing-argument transition;
the clean helper covers all three transitions. The mutation and bypass were
restored before replay. The existing API tests cover this output contract, but
the prior fuzzer corpus did not couple a valid secret nonce to these three
post-load exits, so it could not serve as an independent discovery oracle.

The final forced-int64 Clang ASan/UBSan replay and native GCC x86_64-assembly
ASan/UBSan replay each pass all 46 MuSig corpus inputs. The disposable
two-manager/two-worker Clang campaign saw all 46 seeds in both jobs, completed
47 runs per job in 94 and 95 seconds, and both managers exited 0 without
sanitizer diagnostics, assertion failures, timeouts, or artifacts.

## 2026-07-14 ECDSA `sign` NULL-Input Oracle

The core API target now covers both NULL-input argument exits of
`secp256k1_ecdsa_sign`: a missing 32-byte message hash and a missing secret
key. Each call starts with a prefilled signature, installs a counting illegal
callback, supplies a nonce callback that must not be reached, and requires one
argument error, a zeroed signature, and no nonce callback invocation. The
focused `ecdsa-sign-null-input-cleanup` seed is the ASCII input
`ECDSA sign NULL cleanup\n`.

This is **Informational / Low oracle hardening**, not a new clean-master
production finding. At `origin/master` `ebf5943`, the API already clears the
caller-owned signature before rejecting either NULL argument. The existing
fuzzer covered invalid scalar and nonce-failure cleanup, but not these public
NULL transitions, so it could miss a regression that leaves stale signing
output live. The output is meaningful signature state; this does not claim a
cryptographic nonce erasure issue, disclosure, forgery, or Critical severity.
This commit changes no production behavior.

For causal proof, a temporary production mutation changed the precondition
cleanup to run only when the generator context was unavailable, preserving the
existing invalid-scalar and nonce-failure paths. With the new helper bypassed,
all 31 pre-existing API corpus inputs remained green. Restoring the helper
makes the focused seed abort with exit 134 at the first NULL-argument
transition; the mutation and bypass are restored before the clean replay.
Existing unit tests cover the argument checks and output clearing, but the old
fuzzer corpus did not assert the signing output contract at these two
entry-point exits.

The final forced-int64 Clang ASan/UBSan replay and native GCC x86_64-assembly
ASan/UBSan replay each pass all 32 API corpus inputs. The disposable
two-manager/two-worker Clang campaign saw all 32 seeds in both jobs, completed
105 runs per job in 16 seconds, and both managers exited 0 without sanitizer
diagnostics, assertion failures, timeouts, or artifacts. The native GCC build
also reports the pre-existing `-Wstringop-overread` in
`secp256k1_fuzz_scalar32_in_order` at `api_roundtrip.c:1254`; it is unrelated
to this helper and did not affect the sanitizer replay.

## 2026-07-14 Schnorr Signing Precondition Oracle

The Schnorr target now covers the two previously unmodeled public signing
precondition exits of `secp256k1_schnorrsig_sign_custom`: a nonzero-length
NULL message and a NULL keypair. Each call starts with a signature filled with
`0xA5`, installs a counting illegal-argument callback, supplies a custom nonce
callback that must not be reached, and requires one argument error, a zeroed
64-byte signature, and no nonce callback invocation. The focused
`sign-precondition-cleanup` seed is the 27-byte ASCII input
`odd-nonce-rejection oracle\n`.

This reiterates the existing clean-master stale-output finding fixed on this
branch by `c02dc5e` (`api: clear fixed outputs on failures`): **Low to Medium
severity** at `origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297`.
Before that fix, a caller that ignored the failed argument return could keep
using a previous Schnorr signature from the output buffer. This is fail-open
signing state, not proven memory corruption, key disclosure, signature
forgery, or nonce reuse. The output is cryptographically meaningful, but this
does not claim that clearing a nonce with no cryptographic meaning is Critical.
The branch already contains the production fix; this commit changes no
production behavior. The existing impossible-message-length helper remains
the oracle for that separate length guard; this helper closes only the NULL
message and NULL keypair gap.

For causal proof, a temporary production mutation changed the cleanup
condition from `!secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx)
|| (msg == NULL && msglen != 0) || msglen >= SECP256K1_SHA256_MAX_SIZE - 128
|| keypair == NULL` to
`!secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx) || msglen >=
SECP256K1_SHA256_MAX_SIZE - 128`, preserving the already-covered length
cleanup. With only the new helper bypassed, all 11 pre-existing Schnorr corpus
inputs remained green. Restoring the helper made the focused seed abort with
status 134. The mutation and bypass were restored before the final replay.
The final forced-int64 Clang ASan/UBSan and native GCC x86_64-assembly
ASan/UBSan replays each passed all 12 Schnorr corpus inputs. The disposable
Clang two-manager/two-worker campaign exited 0 for both jobs, completing 63
and 65 runs in 104 seconds, with no sanitizer diagnostics, assertion
failures, timeouts, or artifacts. The only build warning was the target's
pre-existing deprecation warning for the `secp256k1_schnorrsig_sign` alias.

## 2026-07-14 Core `ec_pubkey_combine` NULL-Member Oracle

The core API target now covers a non-NULL input array containing a NULL member
in both positions that matter for state: `[valid, NULL]`, where a valid point
has already been loaded, and `[NULL, valid]`, where validation fails before any
point is loaded. Calls go through the existing unannotated function-pointer
type so the intentional NULL member reaches the library argument callback.
Each case starts with a different nonzero output pattern, requires one illegal
argument callback and a failed return, and requires the complete output object
to be zeroed. The focused `null-combine-member` seed is the 28-byte ASCII input
`NULL combine member cleanup\n`.

This is **Informational / Low oracle hardening**, not a new clean-master
production finding. At `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`,
`secp256k1_ec_pubkey_combine` already clears `pubnonce` before inspecting the
member array. The unit suite already checks that NULL members are rejected at
each of three positions, but it does not prefill and verify the result object
on those callbacks. The older fuzzer checked empty arrays and invalid opaque
members, but not a NULL member after a valid partial aggregate. No memory
corruption, key disclosure, forgery, or cryptographic-state vulnerability is
claimed, and no production change is included.

For causal proof, a temporary production mutation retained the output clear
unless the first member was NULL or the second member was NULL. The focused
standalone Clang ASan/UBSan replay then exited 134 at the new postcondition.
With only the new helper bypassed, all 32 pre-existing API inputs remained
green under that mutation, showing that the old corpus did not detect this
contract regression. The mutation and bypass were restored before the clean
replay. The final Clang ASan/UBSan libFuzzer replay passed all 33 API inputs;
the native standalone replay and the disposable multi-worker campaign are
recorded with their exact commands and results in the commit message.

## 2026-07-14 MuSig Aggregation NULL-Member Oracle

The MuSig target now exercises both pointer-array positions for a NULL member
in `secp256k1_musig_nonce_agg` and `secp256k1_musig_partial_sig_agg`: a valid
object followed by NULL, and NULL followed by a valid object. Each call starts
with a distinct nonzero output pattern, requires one illegal-argument callback
and failure, requires the complete aggregate output to be zeroed, and checks
that every valid opaque input remains unchanged. The first layout is the
important prefix transition: the API has already encountered a valid member
before it rejects the array. The focused
`null-aggregation-member-cleanup` seed is the 80-byte ASCII input
`NULL aggregation member cleanup: valid prefix and suffix pointer checks: proof!\n`.

This is **Informational / Low oracle hardening**, not a new clean-master
production finding. At `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, both public aggregators clear
their fixed-size output before checking every array member. The MuSig unit
tests already cover NULL array members and output clearing; the existing
fuzzer checked malformed opaque members at the beginning and end of valid
arrays, but did not independently couple the NULL-member precondition to the
output postcondition. No memory corruption, key disclosure, signature
forgery, nonce-reuse, or Critical cleanup issue is claimed. In particular,
the public aggregate nonce is not a secret cryptographic nonce; the final
signature output is meaningful state, but callers must still check the return
value.

For causal proof, a temporary mutation guarded each production `memset` so it
was skipped only for the two tested NULL-member layouts while preserving the
other argument and malformed-object paths. The focused seed aborted with
exit 134 for the `nonce_agg` mutation and independently for the
`partial_sig_agg` mutation. With the new helper gated out, all 46
pre-existing MuSig inputs remained green under each mutation. Both mutations
were restored before replay. This proves that the old corpus did not detect
these two output-state regressions without implying that clean master has
them.

The final Clang ASan/UBSan libFuzzer replay passed all 47 MuSig corpus files.
Native GCC x86_64-assembly standalone and GCC ASan/UBSan standalone replays
also passed all 47 files. The disposable Clang two-worker/two-job campaign
saw all 47 seeds and completed 48 executions per job; both jobs exited 0
without sanitizer diagnostics, assertion failures, timeouts, or artifacts.
This commit changes no production behavior.

## 2026-07-14 Core Tweak Scalar Boundary Oracle

The core API target now independently recomputes scalar addition and
multiplication modulo the group order with byte arithmetic. A length-gated
boundary block uses `seckey = 1` and `tweak = order - 1`, checks the exact
success or failure status and zeroized output contract for both secret-key and
public-key tweak wrappers, compares the returned secret bytes with the
reference result, and reconstructs the expected public point from those bytes.
The double-and-add reference keeps every intermediate scalar canonical and
does not call the production scalar implementation. The focused
`independent-tweak-order-boundary` seed is a 285-byte ASCII input whose byte
11 deliberately selects the scalar-one path; it exceeds the 136-byte legacy
API corpus maximum so the new assertion is isolated from prior seeds.

This is **Informational oracle hardening**, not a clean-master production
finding. At `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the existing tweak checks compare
the secret-key and public-key wrappers and then compare the public result with
`ec_pubkey_create` from the production-returned secret bytes. Those relations
can agree if a shared boundary conversion is wrong. The unit coverage checks
zero, one, overflow, simple wrapping, and two-times behavior, but the fuzzer
had no independent byte-level product oracle for the exact `1 * (n - 1)`
state. No clean-master arithmetic error, key disclosure, signature forgery,
or availability issue was found, so no master-relative severity is raised.

For causal proof, a temporary production mutation rejected only
`secp256k1_ec_seckey_tweak_mul(seckey = 1, tweak = order - 1)` and zeroized its
output. With the new block bypassed, all 33 pre-existing API corpus inputs
passed. Restoring the block made the focused seed abort with status 134 at the
independent multiplication result; the mutation and bypass were restored
before clean replay. A broader add-boundary mutation was also tested and
dropped because an older combine oracle already detected it; the committed
block therefore records only the multiplication gap that the legacy corpus
did not cover.

The clean Clang ASan/UBSan replay passed all 34 API seeds. A disposable
15-second Clang ASan/UBSan campaign completed 168 executions without a
diagnostic, assertion, timeout, or artifact. Native GCC x86_64 standalone and
GCC ASan/UBSan standalone replays each passed all 34 seeds; GCC emitted only
the pre-existing `memcmp` bound warning in
`secp256k1_fuzz_scalar32_in_order`. The disposable Clang two-manager,
two-worker campaign saw all 34 seeds, completed 167 executions per job, and
both managers exited 0 without sanitizer diagnostics, assertion failures,
timeouts, or artifacts. This commit changes no production behavior.

## 2026-07-14 ECDSA Normalization Half-Order Oracle

The core API target now pins the exact low-S boundary with an independent
byte-level oracle. A length-gated block constructs compact signatures with
`r = 1`, `s = floor(n/2)`, and `s = floor(n/2) + 1` from constants rather than
calling production scalar helpers. It checks the public normalization return
value, the NULL-output query, canonical compact bytes, and in-place
normalization. The focused
`ecdsa-normalize-half-order-boundary` seed is 418 bytes and therefore exceeds
the 285-byte legacy API corpus maximum, keeping this deterministic boundary
check isolated from generated inputs.

This is **Informational oracle hardening**, not a clean-master production
finding. At `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the existing high-S check derives
its high-S signature through production `secp256k1_ec_seckey_negate`, while
the existing normalization checks compare other production-derived values.
Those relations can agree if a shared boundary conversion is wrong. The
fixed half-order seed is an independent assertion of the exact threshold and
canonical output. The public/unit coverage exercises normalization behavior,
but did not provide this fixed byte-level half-order boundary oracle. No
clean-master arithmetic defect, key disclosure, signature forgery, or
availability issue was found, so no master-relative severity is raised.

For causal proof, a temporary production mutation skipped the scalar
negation in `secp256k1_ecdsa_signature_normalize` only when the serialized
input `s` was exactly `floor(n/2) + 1`. With the new helper bypassed, all 34
pre-existing API corpus inputs remained green. Restoring the helper made the
focused seed abort with status 134 at the expected canonical output. The
mutation and bypass were restored before clean replay. This demonstrates the
legacy-corpus oracle gap without claiming that clean master contains the
mutated defect.

The clean Clang ASan/UBSan replay passed all 35 API seeds. A disposable
15-second Clang ASan/UBSan campaign completed 173 executions without a
diagnostic, assertion, timeout, or artifact. Native GCC x86_64 standalone and
GCC ASan/UBSan standalone replays each passed all 35 seeds; GCC emitted only
the pre-existing `memcmp` bound warning in
`secp256k1_fuzz_scalar32_in_order`. The disposable Clang two-manager,
two-worker campaign saw all 35 seeds, completed 167 and 171 executions in
the two jobs, and both managers exited 0 without sanitizer diagnostics,
assertion failures, timeouts, or artifacts. This commit changes no
production behavior.

## 2026-07-14 Clean Isolated Multi-Worker Corpus Campaign

After the focused oracle replays, all 14 targets were rebuilt from this
branch at `25f478b0c87c8a6a8a65b31d204d09985328bbb1` with Clang 22.1.7,
ASan, UBSan, and every optional module enabled. The tracked corpus tree was
copied outside the worktree so libFuzzer's generated corpus files could not
silently change the audit branch. Core targets used
`-workers=2 -jobs=2 -max_total_time=20`; module targets additionally used
`-timeout=10 -max_total_time=25`.

The initial corpus counts were `api_roundtrip` 35, `context` 10, `hash` 9,
`scalar` 4, `field` 14, `group` 15, `ecmult_const` 5, `ecmult_multi` 13,
`ecdh` 5, `ellswift` 12, `xonly_tweak` 9, `recovery` 8, `schnorrsig` 12,
and `musig` 47. The two workers completed 34,983 executions in aggregate;
the slowest target, MuSig, spent 69 seconds per worker loading and replaying
its staged corpus. Every manager and worker exited 0. No ASan or UBSan
diagnostic, assertion failure, timeout, or crash artifact was produced.

This is a **verification campaign**, not a new master-relative finding. It
does not change any severity in the finding ledger and does not claim that
the existing clean-master findings are absent. Its purpose is to show that
the current oracle set and all tracked seeds remain stable under isolated
multi-worker sanitizer execution without polluting the source corpus.

## 2026-07-14 Alternate `int128_struct` Representation Campaign

The full sanitizer build was repeated with CMake's test-only
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int128_struct` setting and
`SECP256K1_ASM=OFF`. This selects the emulated structure-backed 128-bit
arithmetic used by the 5x52 field backend, instead of the host compiler's
native `__int128` implementation. All six optional modules and recovery were
enabled. Clang 22.1.7 ASan/UBSan built the library, tests, and all 14 fuzz
targets successfully.

All 224 non-libFuzzer CTest cases passed. The copied corpus contained 198
files: `api_roundtrip` 35, `context` 10, `ecdh` 5, `ecmult_const` 5,
`ecmult_multi` 13, `ellswift` 12, `field` 14, `group` 15, `hash` 9,
`musig` 47, `recovery` 8, `scalar` 4, `schnorrsig` 12, and `xonly_tweak` 9.
The bounded `-workers=2 -jobs=2 -max_total_time=20` campaign completed with
status 0 for the other 13 targets and emitted no sanitizer diagnostic,
assertion failure, or artifact.

MuSig is substantially slower on this emulated backend: one fixed corpus
input took 2091 ms, so the initial 47-file load alone exceeded the 90-second
outer campaign limit. This was a harness-duration limitation, not a fuzzer
timeout or production hang. The MuSig corpus was therefore replayed in two
isolated deterministic batches with `-runs=1`; all 47 seed statuses were 0,
with no ASan/UBSan diagnostic, assertion failure, or artifact. The first
multi-worker attempt's shared libFuzzer worker log names are not used as
execution-count evidence.

This is **negative verification evidence**, not a new clean-master finding.
It found no representation-specific inconsistency, production bug, or
oracle gap and does not change any master-relative severity. Existing
findings remain rated against clean master before later fixes or fork
optimizations. Temporary corpus copies, worker logs, and artifact directories
were removed after the replay.

## 2026-07-14 Low-Window ECMULT Campaign

The complete sanitizer build was repeated with `SECP256K1_ECMULT_WINDOW_SIZE=2`
and `SECP256K1_ASM=OFF`. This changes the generator precomputation layout and
the verification multiplication window while retaining all six optional
modules and recovery. Clang 22.1.7 with ASan and UBSan built the library, all
tests, and all 14 fuzz targets. The complete 224-test CTest matrix passed.

The 198 tracked corpus files were copied outside the worktree. Each target was
run from its own disposable directory with
`-workers=2 -jobs=2 -timeout=10 -max_total_time=15`; the MuSig target required
59 seconds per worker to load and replay its 47 seeds. Across 28 worker jobs,
the campaign completed 15,755 executions. Every manager and worker exited 0.
The per-target worker logs had no ASan, UBSan, assertion, crash, or timeout
diagnostic, and no artifact file was produced.

The first parallel attempt used distinct artifact prefixes but a shared
working directory. Its `fuzz-0.log` and `fuzz-1.log` files were therefore
discarded as execution-count evidence and removed. The successful replay used
one working directory per target, so worker logs and generated artifacts were
isolated. A separate `clang -m32` link probe was unavailable because this
environment has no 32-bit C runtime; no 32-bit result is claimed here.

This is **negative verification evidence**, not a clean-master finding. It
found no low-window arithmetic inconsistency, production bug, or oracle gap,
and it does not change any master-relative severity. Existing findings remain
rated against clean `origin/master` before later fixes or fork optimizations.

## 2026-07-14 Field CMOV Metadata Oracle

`fuzz_field` now exercises the documented metadata transition of
`secp256k1_fe_cmov` with a canonical magnitude-1 representation of one and
the same residue represented as `1 + 7p` at magnitude 8. Both flag values are
checked with the nonnormalized operand in the destination and source
positions. The oracle requires the selected limbs and the nonnormalized
metadata contract: the output magnitude is 8 and `normalized` is 0 because
the maximum input magnitude is 8 and both inputs are not normalized.

This is **Informational / Low internal oracle hardening**, not a clean-master
production finding. At clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the `secp256k1_fe_cmov`
implementation already has the documented transition. The previous fuzzer
only used normalized magnitude-1 operands and therefore could not distinguish
a wrong metadata update from a correct one. No production fix or severity
change is claimed.

For causal proof, a temporary production-code mutation changed the update for
different magnitudes to the exact expression
`r->magnitude = (a->magnitude > r->magnitude ? a->magnitude : r->magnitude) + 1`.
The focused `cmov-magnitude-state` seed aborted with status 134 with the new
helper enabled. Bypassing only the new helper left all 14 pre-existing field
seeds green under the same mutation, proving that this assertion is the new
detection point. The mutation and bypass were restored before clean replay.

Clean default 5x52 and forced-int64/10x26 Clang ASan/UBSan replays passed all
15 tracked field inputs. Default and forced-int64 two-worker/two-job
15-second campaigns exited 0, completing 353/354 and 351/355 generated runs,
respectively, with no sanitizer diagnostic, assertion failure, timeout, OOM,
crash artifact, or nonzero worker result. The default build's 109 runnable
CTest cases passed; the optional `noverify_tests` binary also passed one
iteration. This oracle changes no production behavior.

## 2026-07-14 Direct ECMULT Allocation-Failure Oracle

`fuzz_ecmult_multi` now calls the internal single-batch Strauss and Pippenger
implementations directly with a zero-capacity scratch arena. The result is
prefilled with the finite generator, the scratch checkpoint is recorded, and
both a NULL generator scalar and a guaranteed valid non-NULL generator scalar
are tested. Because allocation fails before the callback can run, the oracle
requires return value 0, zero callback calls, an infinity result, and exact
scratch rollback. The focused
`ecmult direct allocation failure` corpus input is 33 bytes.

This is **Informational / Low internal oracle hardening**, not a clean-master
production finding. The public `secp256k1_ecmult_multi_var` path falls back to
its simple implementation when the supplied scratch arena cannot support a
batch, so the previous fuzzer did not reach the direct Strauss/Pippenger
allocation-failure branches. Clean master already clears the output before
those internal helpers allocate, and no public API vulnerability, arithmetic
defect, or availability issue was demonstrated. No production fix or
master-relative severity change is claimed.

For causal proof, each helper was mutation-tested independently by replacing
its reset with `if (n_points != 1) secp256k1_gej_set_infinity(r);`. With the
new oracle enabled, the focused seed exited 134 at the stale-result assertion
for both Strauss and Pippenger. The 13 pre-existing ecmult corpus inputs stayed
green under each mutation, and `-handle_abrt=0` made the result a direct
assertion-abort proof rather than a timeout or sanitizer artifact. Both
mutations were restored before rebuilding and replaying the clean target.

The clean Clang ASan/UBSan replay passed the focused seed and all 14 ecmult
corpus files (15 fixed runs), with no diagnostic, assertion failure, timeout,
or artifact. This oracle changes no production behavior.

## 2026-07-14 Built-In XDH NULL-Input Oracle

The ECDH and EllSwift-XDH harnesses now exercise the public argument checks
with each NULL input position, both through the exported library-owned hash
callbacks and through a custom callback. The built-in callbacks have a fixed
32-byte output contract, so the oracle requires an illegal-callback return of
0 and an all-zero output even though the failure occurs before hashing. The
custom callback has no fixed output size or cleanup contract; its sentinel
must remain unchanged and the callback must not run. The focused seeds are
`builtin-null-inputs` in both module corpora.

This is **Informational / Low API-oracle hardening**, not a new clean-master
production finding. The production guards already exist on this audit branch:
`bb15eb0` recognizes all library-owned ECDH hash callbacks, and `067d4a3`
adds the analogous EllSwift-XDH guard. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` did not clear fixed output for
these precondition failures, but the return value is an argument-error signal
and no key-recovery, cryptographic, or availability impact was demonstrated.
The severity is therefore below the earlier Medium invalid-state and callback
NULL-dereference findings. Public nonce cleanup remains non-critical where
the nonce has no cryptographic meaning; this XDH output is a caller-visible
shared-secret buffer and is documented separately.

For causal proof, the ECDH mutation removed `known_hashfp` from its
NULL-input guard. The EllSwift mutation removed only the `ell_a64 == NULL`
arm, leaving its existing `ell_b64`, secret-key, and prefix-data checks
intact; removing the entire guard would be caught by the older prefix-data
oracle and would not be an independent proof. With each narrow mutation and
the new helper active, the corresponding focused seed aborted at the first
custom/built-in output distinction. Bypassing only the new helper left every
pre-existing ECDH and EllSwift seed green under the matching mutation. The
mutations were restored before the clean replay. This proves that the new
checks exercise previously uncovered precondition branches and does not claim
a clean-master production bug beyond the already documented fail-closed output
gap.

Clean Clang ASan/UBSan focused and complete-corpus replays passed for both
targets, and isolated two-worker/two-job runs completed without diagnostics,
assertion failures, timeouts, OOMs, or artifacts. This commit changes no
production behavior.

## 2026-07-14 10x26 Zero-Predicate Reachability Audit

The 10x26 `normalizes_to_zero{,_var}` finding was replayed as a paired
reachability experiment. A forced-int64 Clang ASan/UBSan build of the fixed
tree replayed all 14 tracked target corpora with `-runs=1`, for 216 executed
inputs, with no sanitizer diagnostic, assertion failure, timeout, or artifact.

The two predicate implementations were then changed temporarily to the exact
clean-master uint32 carry chain, leaving every other source line unchanged.
The 13 non-field targets replayed 200 inputs, including the full 47-input
MuSig corpus and the public API, ECDH, EllSwift, recovery, Schnorr, and x-only
targets; every input still exited 0. The dedicated
`field/zero-predicate-false-positive` input independently aborted with a
libFuzzer deadly-signal exit under that mutation, while the repaired binary
passed the same input. The temporary mutation was restored before the fixed
source check.

This is stronger negative reachability evidence, not a severity reduction:
the defect is a real clean-master internal field correctness bug, but no
public or module corpus reached the exact maximum-magnitude state through the
tested paths. The master-relative rating therefore remains **Medium / latent**,
with potentially High arithmetic impact only if another valid production path
can construct that state. Because this screen ran on the audit tree containing
other independently repaired contracts, it does not claim that those earlier
clean-master failures could not mask a path; the exact predicate mutation and
dedicated field assertion remain the causal proof. This experiment changes no
production behavior.

## 2026-07-14 Field Square-Class Reference Oracle

`fuzz_field` now compares `secp256k1_fe_is_square_var` with the standalone
8x32 schoolbook/exponentiation model already used for the independent square
root oracle. The new check starts from an arbitrary canonical residue derived
with a separate salt, then adds `7p` without changing its value and repeats
the classification at the accepted nonnormalized magnitude-8 boundary. This
exercises nonsquares as well as squares; the earlier checks only classified a
production-generated square and its negation, so a Jacobi-symbol or fallback
regression could agree with the old oracle.

This is **Informational / Low internal oracle hardening**, not a clean-master
production finding. The dedicated
`field/is-square-independent-reference` input derives the canonical nonsquare
`cfbfd77d5f2a0ef8c0acbb9977427302e8cca2966470583d7ffbdce19a775621`.
Existing clean-master findings, including the latent 10x26 zero-predicate
defect, retain their severity relative to the unmodified `origin/master`;
this oracle does not claim a new arithmetic bug or change those ratings.

For causal proof, a temporary production mutation flipped the result in the
shared `secp256k1_fe_is_square_var` wrapper only for that residue's default
5x52 limbs, and placed the flip after the wrapper's internal `VERIFY_CHECK`.
All 15 pre-existing field inputs remained green under the final mutation. The
new seed aborted with exit 134 when the canonical-residue assertion was
enabled, while bypassing only that assertion let the same mutated seed pass;
the raised `+7p` assertion remained active. This isolates the new independent
reference from the older square/negation checks. The mutation was restored
before the clean replay.

Clean Clang ASan/UBSan replays passed all 16 field inputs on default 5x52 and
forced-int64/10x26 builds. Isolated `-workers=2 -jobs=2 -max_total_time=15`
campaigns completed 271 and 273 runs on the default backend and 262 and 265
runs on forced-int64, with every manager and worker exiting 0 and no
sanitizer diagnostic, assertion failure, timeout, OOM, or artifact. This
change modifies only the fuzzer and its corpus.

## 2026-07-14 MuSig Long Nonce Aggregation Oracle

`fuzz_musig` now has a gated 29-byte corpus case that parses one valid public
nonce, passes the same object 16 times to `secp256k1_musig_nonce_agg`, and
checks each aggregate component against an independently computed `16*P`
using public-key tweak multiplication. Repeating a public nonce is a loop
stress case, not a claim that a MuSig signer set may reuse one nonce. Public
nonces have no cryptographic secrecy requirement here, so this oracle does
not add or rate nonce clearing as a security issue.

This is **Informational / Low public-API oracle hardening**, not a new
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` accepts arbitrary positive
`n_pubnonces`, but the existing successful algebra check covered only one and
two entries; its failure-cleanup helper also deliberately capped its inputs
at two. The new case exercises the tail of a longer successful aggregation
without making a protocol-validity claim. Existing findings retain their
severity relative to unmodified master, including the latent 10x26 field
predicate defect and the Medium opaque-state findings.

For causal proof, a temporary production mutation changed the aggregation
loop in `src/modules/musig/session_impl.h` from
`i < n_pubnonces` to
`i < n_pubnonces - (n_pubnonces == 16)`, skipping only the final member of
the new 16-entry case. All 47 pre-existing MuSig corpus inputs stayed green;
the exact `long MuSig nonce aggregation` seed then aborted with status 134 at
the new aggregate comparison. Bypassing only the new helper made that seed
pass under the same mutation. The production mutation and bypass were
restored before fixed replay. The proof therefore identifies the new oracle
as the detection point without claiming a clean-master bug.

The fixed Clang ASan/UBSan target passed all 48 MuSig corpus files on both
default 5x52 and forced-int64/10x26 builds. Symbolizer-enabled exploratory
campaigns were excluded after GDB showed the ASan main thread spending the
outer timeout in `__sanitizer::SymbolizerProcess::ReadFromSymbolizer`; the
exact generated inputs independently replayed in about 1.3 seconds. Final
isolated campaigns used
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` with ASan/UBSan detection
and symbolization disabled: default jobs completed 13 and 15 executions,
and forced-int64 jobs completed 8 and 9. Every job and manager exited 0 with
no sanitizer diagnostic, assertion failure, timeout, OOM, or artifact. This
commit changes only the fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 MuSig Long Partial-Signature Aggregation Oracle

`fuzz_musig` now has a gated 41-byte corpus case that parses the canonical
scalar `order - 1` into a partial-signature object, passes that same object 16
times to `secp256k1_musig_partial_sig_agg`, and compares the complete 64-byte
result with an independent 257-bit byte-arithmetic model. The model preserves
the session's final nonce and computes `s_part + 16*(order - 1)` modulo the
group order, including the wraparound boundary. Repeated partial signatures
are an aggregation-loop stress case; the API does not verify signer identity
or the partial-signature equation in this function, so this is not a claim
that the repeated objects form a valid MuSig signer set.

This is **Informational / Low public-API oracle hardening**, not a new
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` accepts arbitrary positive
`n_sigs`, while the existing successful fuzzer path reaches at most eight
signers and checks its aggregate through the signing equations. It did not
independently exercise a longer scalar-aggregation tail or a 16-term modular
wrap. Partial signatures and nonce values here are serialized protocol state,
not secret buffers; no cleanup severity is claimed.

For causal proof, a temporary production mutation changed only the scalar
aggregation loop in `src/modules/musig/session_impl.h` from
`i < n_sigs` to `i < n_sigs - (n_sigs == 16)`, skipping the final term only
for the new case. All 48 pre-existing MuSig corpus inputs stayed green; the
exact `long MuSig partial signature aggregation` seed aborted with status 134
at the independent full-signature comparison. Bypassing only the new helper
made that seed pass under the same mutation. Both temporary changes were
restored before fixed replay.

The fixed Clang ASan/UBSan replay passed all 49 MuSig corpus files on default
5x52 and forced-int64/10x26 builds; the forced-int64 confirmation command
exited 0 explicitly. Isolated campaigns used
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` with sanitizer detection
active and symbolization disabled: default jobs completed 14 and 14 runs,
forced-int64 jobs 8 and 9, and every manager and worker exited 0. No
sanitizer diagnostic, assertion failure, timeout, OOM, or artifact occurred.
This commit changes only the fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 MuSig Long Key-Aggregation Oracle

`fuzz_musig` now has a gated 27-byte corpus case that constructs 16 distinct
public keys from scalar values one through 16. It recomputes the complete
`KeyAgg list` transcript and first-distinct-key rule with the standalone
tagged-SHA reference, derives every remaining coefficient independently,
scales the points through public-key tweak operations, and combines them with
the public point-combination API. The result is compared with the complete
MuSig cache output, the x-only output, the cacheless output, and the cached
list hash. This reaches the arbitrary-list callback at index 15 while keeping
the expected point equation separate from MuSig's `ecmult_multi` path.

This is **Informational / Low public-API oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` accepts arbitrary positive
`n_pubkeys`, but the existing independent coefficient helper was capped at
eight keys, the dedicated references stopped at eight, and the stateful
signing path also stopped at eight. Existing findings retain their severity
relative to unmodified master; this extension neither downgrades a severe
master finding nor treats a later fork optimization as evidence that master
was correct.

For causal proof, a temporary mutation changed only the production
`ecmult_multi` count in `src/modules/musig/keyagg_impl.h` from `n_pubkeys` to
`n_pubkeys - (n_pubkeys == 16)`, so the final callback term was omitted only
for this case while the 16-key transcript remained intact. All 49 pre-existing
MuSig corpus files completed 50 executions without a failure under the
mutation. The exact `long-keyagg-reference` seed then aborted with SIGABRT
exit 134 at the independent full-point comparison. Bypassing only the new
helper made the identical mutated seed pass with exit 0. Both temporary
changes were restored before fixed replay; no clean-master production bug is
claimed.

The restored Clang ASan/UBSan target passed all 50 MuSig corpus files on the
default 5x52 backend and on forced-`int64`/10x26, with 51 fixed executions and
exit 0 in each replay. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns ran both jobs
to completion on each backend; every manager and worker exited 0, with no
sanitizer diagnostic, assertion failure, timeout, OOM, or artifact. The
default jobs each completed 51 runs in 59 seconds; forced-`int64` jobs each
completed 51 runs in 103 seconds. This commit changes only the fuzzer, its
corpus, and this evidence ledger.

## 2026-07-14 MuSig Sixteen-Signer State-Machine Oracle

`fuzz_musig` now has a gated 29-byte corpus case that builds sixteen distinct
keypairs from scalar values one through 16 and traverses a valid multi-party
session. For every signer it independently checks MuSig nonce derivation and
serialization, then aggregates and processes all sixteen public nonces. It
independently recomputes every partial-signature equation, compares the
production partial verifier, checks session replay equivalence, aggregates all
partial signatures, and verifies the final signature with both the standalone
BIP340 point equation and the production Schnorr verifier. Ordinary fuzz
inputs retain the existing one-through-eight stateful path; only this fixed
case allocates and exercises the sixteen-entry transition.

This is **Informational / Low state-machine oracle hardening**, not a
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` accepts arbitrary positive list
lengths, but the stateful fuzzer stopped at eight participants. The existing
16-entry nonce and partial-signature cases deliberately repeat opaque objects
to stress count loops; they do not prove a valid sixteen-signer nonce,
partial-signature, and final-signature transcript. Existing findings retain
their severity relative to unmodified master.

For causal proof, a temporary production mutation in
`src/modules/musig/session_impl.h` made `secp256k1_musig_partial_sign` use the
identity scalar instead of the KeyAgg coefficient only for the loaded scalar
16 signer (including its parity-adjusted negation). All 50 pre-existing MuSig
corpus files completed 51 executions with exit 0 under that mutation. The
exact `sixteen-signer-sign-roundtrip` seed aborted with SIGABRT exit 134 at
the independent per-signer equation. Bypassing only the new helper made the
identical mutated seed pass with exit 0. The production mutation and bypass
were restored before fixed replay; no clean-master production bug is claimed.

The restored Clang ASan/UBSan target passed all 51 MuSig corpus files on the
default 5x52 backend and forced-`int64`/10x26, with 52 fixed executions and
exit 0 in each replay. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns ran both jobs
to completion on each backend; every manager and worker exited 0, with no
sanitizer diagnostic, assertion failure, timeout, OOM, or artifact. The
default jobs each completed 52 runs in 61 seconds; forced-`int64` jobs each
completed 52 runs in 107 seconds. This commit changes only the fuzzer, its
corpus, and this evidence ledger.

## 2026-07-14 ecmult_multi Sixteen-Point Direct-Batch Oracle

`fuzz_ecmult_multi` now has a gated 28-byte corpus case that fills sixteen
independent scalar/point entries and exercises the direct callback path with
`g_sc = 17`. Its expected result is built from sixteen separate
`secp256k1_ecmult_const` terms and checked both through canonical serialized
coordinates and the existing Jacobian equality helper. The callback trace
requires every index from 0 through 15 exactly once. Existing inputs retain
the prior 0-8 point distribution; the repeat-only Pippenger and Strauss cases
remain unchanged and continue to cover larger counts with their deliberate
repeated-point model.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` accepts larger direct batches, but
the independent direct callback model and its storage stopped at eight
entries. The existing large-count tests use one repeated point, so they do
not prove that a distinct callback value at a later index contributes to the
batch result. Existing findings retain their severity relative to unmodified
master; this extension does not treat a later optimization or a passing
repeat case as evidence that the direct path was fully modeled.

For causal proof, a temporary mutation in `src/ecmult_impl.h` made the direct
no-scratch implementation skip only the final arithmetic accumulation when
`n == 16` and the generator scalar was exactly 17. The callback still ran for
index 15, so the trace oracle passed and the independent point-result oracle
had to detect the missing term. All 14 pre-existing `ecmult_multi` corpus
files stayed green under the mutation on both backends. The exact
`sixteen-direct-batch` seed aborted with SIGABRT exit 134 on both backends;
bypassing only the new sixteen-point fixture made that same mutated seed pass
with exit 0 on both. The mutation and bypass were restored before fixed
replay; no clean-master production bug is claimed.

The restored Clang ASan/UBSan target passed all 15 corpus files on both the
default 5x52 and forced-int64/10x26 backends, with 16 fixed executions and
exit 0 in each replay. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns then ran both
jobs per backend; the default jobs completed 75 and 75 executions in 16
seconds, while forced-int64 jobs completed 40 and 41 executions in 16 and 17
seconds. Every manager and worker exited 0 with no sanitizer diagnostic,
assertion failure, timeout, OOM, or artifact. This commit changes only the
fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 MuSig Valid Duplicate-Key KeyAgg Oracle

`fuzz_musig` now has a gated 32-byte corpus case with sixteen valid public
keys whose scalar pattern is `[1, 1, 2, 3, 2, 4, ..., 14]`. The independent
reference derives the KeyAgg list hash from canonical serialized keys, selects
the first distinct key by serialized equality, assigns coefficient one to
both occurrences of that key, hashes every other coefficient independently,
and compares the weighted point sum with cached and cacheless MuSig
aggregation. The non-adjacent duplicate of the first distinct key and the
duplicate of the first list key are intentional: they distinguish the
first-distinct rule from a simple adjacent-pair check. The existing
noncanonical duplicate-key fixture remains a separate invalid-opaque-state
test.

This is **Informational / Low public-API oracle hardening**, not a new
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` implements the first-distinct
rule for arbitrary list lengths, but the existing independent long transcript
used sixteen distinct keys and the existing duplicate fixture used malformed
opaque state. This case proves the valid duplicate-key branch at the tail of
a longer list. It does not downgrade any previously recorded master-relative
Medium findings, and no severity is assigned to public-nonce cleanup here.

For causal proof, a temporary mutation in
`src/modules/musig/keyagg_impl.h` forced index 1 to be selected as the second
key whenever `n_pubkeys == 16`, even when it duplicated index 0. The 51
pre-existing MuSig corpus files remained green under the mutation; the exact
`duplicate-keyagg-reference` seed aborted with SIGABRT exit 134 on both
default 5x52 and forced-int64/10x26. Bypassing only this new helper made the
same mutated seed pass with exit 0 on both backends. The mutation and bypass
were restored before fixed replay, so this is a detection-point proof rather
than a claim that clean master is broken.

The restored Clang ASan/UBSan target passed all 52 MuSig corpus files on both
backends with exit 0. Isolated `-workers=2 -jobs=2 -max_total_time=15
-timeout=5` campaigns ran two jobs per backend, with 53 executions per job;
all managers and workers exited 0 and produced no sanitizer diagnostic,
assertion failure, timeout, OOM, or artifact. This commit changes only the
fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 ecmult_multi Distinct Pippenger Batch Oracle

`fuzz_ecmult_multi` now has a gated 34-byte corpus case with 264 distinct
callback entries, exactly three times `ECMULT_PIPPENGER_THRESHOLD` (88). Each
entry uses a different generator-derived point and scalar, while the generator
term is fixed at 17. A scratch space sized for one threshold batch forces the
production dispatcher through three Pippenger batches. The independent model
computes every point term with `secp256k1_ecmult_const`, checks the complete
serialized-coordinate result, requires every callback index exactly once, and
rejects at both sides of the first batch boundary while requiring an infinity
failure result.

This is **Informational / Low internal-oracle hardening**, not a new
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already exercises one- and
two-batch Pippenger behavior, but those large-count references repeat a single
point and derive one scalar total. The existing distinct direct case stops at
sixteen entries. This transcript checks that distinct callback state survives
multiple Pippenger batches; it does not change any prior master-relative
severity, and no cleanup issue is inferred from this public arithmetic state.

For causal proof, a temporary mutation in `src/ecmult_impl.h` skipped the
first accumulated batch only when the dispatcher received at least three
Pippenger-threshold batches. All 15 pre-existing `ecmult_multi` corpus files
remained green on default 5x52 and forced-int64/10x26. The exact
`distinct-pippenger-batches` seed aborted with SIGABRT exit 134 on both
backends. Bypassing only the new helper made the same mutated seed pass with
exit 0 on both. The mutation and bypass were restored before fixed replay;
the oracle identifies a missing-batch regression without claiming clean master
is currently defective.

The restored Clang ASan/UBSan target passed all 16 corpus files on both
backends with exit 0. Isolated campaigns used copied corpora with
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5`; both jobs on each backend
completed 64 executions in 16 seconds. Every manager and worker exited 0 with
no sanitizer diagnostic, assertion failure, timeout, OOM, or artifact. This
commit changes only the fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 Core `ec_pubkey_combine` Repeated-Infinity 16-Term Oracle

`fuzz_api_roundtrip` now has a gated 28-byte ASCII seed,
`sixteen-term-pubkey-combine`, that feeds the public combine API the fixed
scalar sequence `[1, n-1, 2, n-2, 3, n-3, 4, n-4, 5, 6, 7, 8, 9, 10, 11,
12]`, where `n` is the group order. The first eight terms cancel in four
separate pairs, so the accumulator reaches infinity repeatedly; the final
eight terms then produce `68G`. The oracle checks every zero-sum prefix,
checks that `5G` is accepted after the fourth cancellation, compares the
complete result with an independent byte-level scalar sum and generator
multiplication, and repeats the complete transcript in reverse order.

This is **Informational / Low public-API oracle hardening**, not a new
clean-master production finding. At clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the API loop already handles
arbitrary input counts, but the existing independent combine coverage stopped
at eight terms and the explicit cancellation helper stopped at four. This
transcript covers repeated infinity-to-finite transitions and a long tail
without changing the master-relative severity of any existing finding. No
public nonce cleanup or other non-cryptographic state is assigned a critical
severity here.

For causal proof, a temporary mutation in `src/secp256k1.c` skipped only the
last accumulator addition when `n == 16`. All 35 pre-existing API corpus files
remained green on both default 5x52 and forced-int64/10x26 Clang ASan/UBSan
builds, while the exact new seed aborted with SIGABRT exit 134 under
`-handle_abrt=0` on both backends. Bypassing only this new helper made the same
mutated seed pass with exit 0 on both backends. The production mutation and
helper bypass were restored before final replay, so this is detection-point
proof rather than a claim that clean master is currently defective.

The restored Clang ASan/UBSan targets replayed all 36 API corpus files with
exit 0 on both backends. Isolated campaigns used
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5`; the default backend ran
171 executions in 16 seconds and forced-int64 ran 113 executions in 17
seconds. Every manager and worker exited 0 with no sanitizer diagnostic,
assertion failure, timeout, OOM, or artifact. This commit changes only the
fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 Core `ec_pubkey_sort` Duplicate-Pointer 16-Key Oracle

`fuzz_api_roundtrip` now has a gated 24-byte ASCII seed,
`sixteen-key-pubkey-sort`, that constructs sixteen valid public-key objects
from scalar values 1 through 8, with each value represented by two separate
objects in a deliberately interleaved order. An independent insertion sort of
the 33-byte compressed encodings defines the expected byte sequence. The
oracle then checks that `secp256k1_ec_pubkey_sort` produces that sequence and
that every original pointer occurs exactly once. It repeats the checks after a
second sort without assuming an order among equal serialized keys.

This is **Informational / Low public-API oracle hardening**, not a new
clean-master production finding. At clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, the sort API already accepts
arbitrary pointer-array lengths, but the independent fuzzer oracle stopped at
eight distinct keys and its duplicate-pointer case stopped at four. The unit
suite has a separate six-key vector, so this transcript adds a longer
duplicate-heavy fuzzer boundary and preserves the existing master-relative
severity ledger. No public nonce cleanup or other non-cryptographic state is
assigned a critical severity here.

For causal proof, a temporary mutation in `src/secp256k1.c` overwrote the
last pointer with the first whenever `n_pubkeys == 16`, immediately before
`hsort`. All 36 pre-existing API corpus files remained green on default 5x52
and forced-int64/10x26 Clang ASan/UBSan builds, while the exact new seed
aborted with SIGABRT exit 134 under `-handle_abrt=0` on both backends.
Bypassing only this new helper made the same mutated seed pass with exit 0 on
both backends. The production mutation and helper bypass were restored before
final replay, so this is detection-point proof rather than a claim that clean
master is currently defective.

The restored Clang ASan/UBSan targets replayed all 37 API corpus files with
exit 0 on both backends. Isolated campaigns used
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5`; the default backend ran
174 executions in 16 seconds and forced-int64 ran 109 executions in 17
seconds. Every manager and worker exited 0 with no sanitizer diagnostic,
assertion failure, timeout, OOM, or artifact. This commit changes only the
fuzzer, its corpus, and this evidence ledger.

## 2026-07-14 Post-Rebase Full-Corpus Multi-Worker Replay

After fetching `origin/master` and rebasing, `codex/fuzz-oracles` remained
based directly on `ebf594320dc838b9de1abb54d5ba98cef84f4297`; the rebase was a
no-op because the branch already contained that commit. A fresh GCC
ASan/UBSan build with every optional module enabled passed all 14 CTest fuzz
targets and their tracked corpora. A separate Clang ASan/UBSan libFuzzer build
then replayed copied corpora with two workers and two jobs per target. The
worker execution counts were:

    api_roundtrip 170/171       context 209/209
    hash          22020/22064   scalar  845/848
    field         272/274       group   474/480
    ecmult_const  275/276       ecmult_multi 68/69
    ecdh          225/226       ellswift 91/94
    xonly_tweak   40/42         recovery 227/229
    schnorrsig    124/128       musig   53/53

Every manager and worker exited 0. No ASan/UBSan diagnostic, assertion
failure, timeout, OOM, or crash artifact was produced. MuSig's fixed corpus
required 63 seconds per job because its 52 stateful seeds are expensive; the
other targets completed in 16-17 seconds. This is negative evidence for the
current oracle set, not a claim that clean master or future fork patches are
bug-free.

The same pass audited a tempting alias case for
`secp256k1_ec_pubkey_combine`: the caller may be able to place `out` at the
same address as an element of `ins`, but the header specifies separate `Out`
and `In` roles and does not promise overlap. The implementation clears `out`
before loading inputs, so treating this as a supported alias would invent a
contract and create a false positive. No oracle or production change was
added. This deliberate no-edit prevents the existing combine findings from
being diluted by an undocumented caller-domain assumption. Public nonce state
without cryptographic meaning remains non-critical.

## 2026-07-15 MemorySanitizer-Flagged Post-Rebase Replay

A fresh Clang build enabled `-fsanitize=memory -fno-omit-frame-pointer -fPIE`
for compilation and `-fsanitize=memory -pie` for linking. The generated
compile and link rules were checked directly; the project's optional
`HAVE_MSAN` feature probe remained false in this toolchain, so this result is
reported as a sanitizer-instrumented fuzz run rather than a claim that the
probe-enabled CTest configuration was available. The fixed replay passed all
14 tracked fuzz corpora, including the stateful MuSig corpus.

The matching libFuzzer binaries replayed copied corpora with isolated working
directories, `-workers=2 -jobs=2 -max_total_time=15 -timeout=5`, and
`MSAN_OPTIONS=halt_on_error=1:exit_code=86:report_umrs=1`. The two job totals
were:

    api_roundtrip 117/135       context 150/148
    hash           6191/5448    scalar  387/402
    field          138/132      group   369/389
    ecmult_const   205/176      ecmult_multi 43/39
    ecdh           136/183      ellswift 62/60
    xonly_tweak    31/35        recovery 169/144
    schnorrsig     74/93        musig   56/56

Every manager and worker exited 0. The worker logs contained no
MemorySanitizer diagnostic, assertion failure, timeout, OOM, or crash
artifact. The stateful MuSig jobs
completed in about 38 seconds each; the other targets completed in about
16-17 seconds. This is negative evidence for the current oracle set, not a
new clean-master finding, and it does not change any existing severity rating.
The campaign also confirms that public nonce state without cryptographic
meaning remains non-critical.

## 2026-07-15 Partial Keypair X-Only Tweak Oracle

`fuzz_xonly_tweak` now has a gated 30-byte ASCII seed,
`partial-keypair-tweak-invalid`, that calls
`secp256k1_keypair_xonly_tweak_add` twice: once with an all-zero secret half
and valid public half, and once with a valid secret half and an all-zero
public half. Raw `keypair_sec` and `keypair_pub` projections intentionally
remain permissive, but this mutating operation must reject either partial
opaque state, invoke the illegal-argument callback once, and clear the
caller-owned keypair output.

This is **Informational / Low API-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already propagates
`keypair_load` failure through the tweak operation and clears the keypair.
The existing target covered raw partial projections and valid-but-mismatched
keypairs, but did not prove this distinct mutating transition. No public key
disclosure, signature forgery, or availability impact is claimed, and the
master-relative severity ledger is unchanged.

For causal proof, a temporary mutation in
`src/modules/extrakeys/main_impl.h` forced
`secp256k1_keypair_xonly_tweak_add` to continue after `keypair_load` rejected
either all-zero 32-byte half, using the loader's dummy state. All nine
pre-existing x-only corpus inputs stayed green under that mutation. The exact
new seed aborted with exit 134 at the new callback/output barrier; bypassing
only that helper made the identical mutated seed exit 0. The production
mutation and harness bypass were restored before fixed replay.

The restored Clang ASan/UBSan target passed all 10 x-only corpus files on both
the default 5x52 and forced-int64/10x26 backends. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns ran both jobs
per backend: default workers executed 40 and 39 inputs, while forced-int64
workers executed 38 and 39. Every manager and worker exited 0 with no ASan,
UBSan, assertion, timeout, OOM, or crash artifact. This commit changes only
the fuzzer, its focused corpus input, and this evidence ledger.

## 2026-07-15 Partial Keypair MuSig Counter-Nonce Oracle

`fuzz_musig` now has a gated 38-byte ASCII seed,
`partial-keypair-nonce-counter-invalid`, that calls
`secp256k1_musig_nonce_gen_counter` with an all-zero secret half/valid public
half and with a valid secret half/all-zero public half. Each rejection must
invoke the illegal-argument callback once and clear both caller-owned secret
and public nonce outputs before nonce derivation.

This is **Informational / Low API-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already propagates
`keypair_load` failure through `nonce_gen_counter` and clears both outputs.
The existing MuSig target covered invalid keyagg caches and valid-but-mismatched
keypairs, but not these two partial opaque states in the counter-nonce
consumer. No nonce disclosure, forgery, or availability impact is claimed;
public nonce cleanup alone is not Critical, and the master-relative ledger is
unchanged.

For causal proof, a temporary mutation in
`src/modules/musig/session_impl.h` forced `nonce_gen_counter` to continue
after `keypair_load` rejected either all-zero 32-byte half, using the loader's
dummy state. All 52 pre-existing MuSig corpus inputs stayed green under that
mutation on both default and forced-int64 builds. The exact new seed aborted
with exit 134 on both backends; bypassing only the new helper made the
identical mutated seed exit 0 on both. The production mutation and harness
bypass were restored before fixed replay, so this is detection-point proof,
not a claim that clean master is currently defective.

The restored Clang ASan/UBSan target passed all 53 MuSig corpus files on both
default 5x52 and forced-int64/10x26 backends: 54 runs including the empty
input, with exit 0 in 64 and 112 seconds respectively. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns executed 54 runs
per worker on both backends: 2x54 in 64 seconds per default worker and 2x54
in 111 and 112 seconds for forced-int64 workers. Every manager and worker
exited 0 with no ASan, UBSan, assertion, timeout, OOM, or crash artifact.
This commit changes only the fuzzer, its focused corpus input, and this
evidence ledger.

## 2026-07-15 MuSig NULL Session-Random Cleanup Oracle

`fuzz_musig` now has a gated 32-byte ASCII seed,
`nonce-gen-null-session-random`, that calls `secp256k1_musig_nonce_gen` through
a function pointer with its mandatory `session_secrand32` argument set to
`NULL`. The seckey, signer pubkey, message, keyagg cache, and extra input are
otherwise valid. Both caller-owned 132-byte nonce objects are prefilled with
different nonzero sentinels. The rejected call must invoke the illegal-argument
callback once and leave both the secret and public nonce objects all zero.

This is **Informational / Low API-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already initializes the two output
objects before rejecting the mandatory random-pointer precondition. The secret
nonce carries cryptographic state, so its invalidation is an important
fail-closed transition; the public nonce has no cryptographic meaning here, so
its cleanup alone is not Critical. No nonce disclosure, reuse exploit,
forgery, or availability impact is claimed, and the master-relative severity
ledger is unchanged.

For causal proof, the production clear was temporarily narrowed to
`if (session_secrand32 != NULL)`, leaving every non-NULL failure path intact.
All 53 pre-existing MuSig corpus files stayed green under that mutation in 54
runs. The exact new seed aborted with exit 134 at the stale-secret-output
assertion; bypassing only the new gated helper made the identical mutated seed
exit 0. The mutation and bypass were restored before fixed replay. This proves
that the old corpus did not already exercise this mandatory-pointer branch; it
does not claim that clean master currently contains the defect.

The restored Clang ASan/UBSan target passed all 54 MuSig corpus files plus the
empty-input path: 55 runs in 67 seconds on the default 5x52 backend and 55
runs in 116 seconds on the forced-int64/10x26 backend. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns ran both jobs on
each backend; every job completed 55 runs and exited 0. Log scans found no
ASan, UBSan, assertion, timeout, OOM, or crash artifact. The production change
is comment-only; the behavioral oracle, corpus seed, and this evidence ledger
are the substantive additions.

## 2026-07-15 Invalid Keypair-Creation Cleanup Oracle

`fuzz_xonly_tweak` now has a gated 31-byte ASCII seed,
`keypair-create-invalid-cleanup`, that calls
`secp256k1_keypair_create` with the two invalid secret-key boundaries:
all-zero and the group order. It pre-fills the 96-byte opaque keypair with
`0xA5`; each call must return 0 and leave every keypair byte zero.

This is **Informational / Low API-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already runs the invalid secret
through dummy generator state and `secp256k1_memczero` clears the saved
keypair. The existing x-only target exercised keypair creation only with
valid secrets and tested invalid keypair consumers, but did not observe this
distinct creation-failure transition. The secret half is cryptographic state,
so this is stronger than public nonce cleanup; no disclosure, signing
forgery, or availability impact is claimed, and no master-relative severity
change is made.

For causal proof, temporarily bypass only the
`secp256k1_memczero` in `secp256k1_keypair_create` by changing its condition
from `!ret` to `0`. The 10 pre-existing x-only corpus files each exited 0
under that mutation on both backends, while the exact new seed aborted with
status 134 at the full 96-byte zero-state assertion on both. Bypassing only
the new helper made that same mutated seed exit 0 on both. Both temporary
changes were restored before fixed replay. This proves an oracle gap, not a
clean-master defect.

The restored Clang ASan/UBSan target passed all 11 x-only corpus files plus
the empty-input path on both the default 5x52 and forced-int64/10x26 backends,
with exit 0 and no sanitizer or assertion diagnostic. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns also exited 0;
both reported jobs exited 0, with interleaved final-stat summaries of 39, 38,
and 43 runs on default and 38, 39, and 41 runs on forced-int64. No sanitizer,
assertion, timeout, OOM, or crash artifact was produced. This commit changes
only the fuzzer, corpus seed, and evidence ledger.

## 2026-07-15 Public-Key Serialization NULL-Output Oracle

`fuzz_api_roundtrip` now has a gated 29-byte ASCII seed,
`pubkey-serialize-null-output`, that passes a valid public key with a NULL
output pointer to `secp256k1_ec_pubkey_serialize` for both compressed and
uncompressed encodings. Each rejected call must invoke the illegal-argument
callback once and reset the caller-owned output length to zero.

This is **Informational / Low API-oracle hardening**, not a clean-master
production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already zeroes `*outputlen`
before rejecting the NULL output pointer, and the deterministic API test
checks this boundary. The existing round-trip fuzzer covered short buffers,
invalid flag types, and NULL parser inputs, but did not observe this distinct
serializer precondition. No disclosure, forgery, or availability impact is
claimed, and no master-relative severity change is made.

For causal proof, temporarily replace the production
`ARG_CHECK(output != NULL)` in `secp256k1_ec_pubkey_serialize` with an early
`return 0` after the output length has been reset. All 37 pre-existing
`fuzz_api_roundtrip` corpus files exited 0 under that mutation on both
backends; the exact new seed aborted with status 134 on both. Bypassing only
the new helper made the same mutated seed exit 0 on both. The production
mutation and helper bypass were restored before fixed replay. This proves an
oracle gap, not a clean-master defect.

The restored Clang ASan/UBSan target passed all 38 API corpus files plus the
empty-input path on both default 5x52 and forced-int64/10x26, with no
sanitizer or assertion diagnostic. Copied-corpus
`-workers=2 -jobs=2 -max_total_time=15 -timeout=5` campaigns reported job
exit 0 and final-stat summaries of 170 and 173 runs on default, and 107 and
107 runs on forced-int64. No sanitizer, assertion, timeout, OOM, or crash
artifact was produced. This commit changes only the fuzzer, corpus seed, and
evidence ledger.

## 2026-07-15 MuSig Partial-Signature Verification Invalid-State Oracle

`fuzz_musig` now has a gated 33-byte ASCII seed,
`partial-sig-verify-invalid-state`, that first builds a valid stateful MuSig
transcript and then calls `secp256k1_musig_partial_sig_verify` five times with
one malformed opaque input at a time: a magic-preserving all-`0xFF` partial
signature scalar, a corrupted pubnonce magic, an all-zero public key, a
corrupted key-aggregation-cache magic, and session parity `2`. Every call must
return `0`, invoke the illegal-argument callback exactly once, and leave the
valid neighboring objects byte-for-byte unchanged. The function-pointer call
keeps this harness contract independent of the public declaration's nonnull
attributes.

This reiterates the existing **Medium clean-master finding** from `0863a8b`,
`musig: reject overflowing partial signatures`. At clean
`origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297`,
`secp256k1_musig_partial_sig_load` still uses `VERIFY_CHECK(!overflow)`: a
magic-valid scalar at or above the group order can abort verification builds,
while a non-`VERIFY` build reduces it and continues as a different scalar.
That is malformed local opaque state or unsafe persistence, not a direct
wire-format or remote attack. The audit branch already contains the production
`ARG_CHECK(!overflow)` fix; this commit changes no production behavior. The
four neighboring malformed-object cases are consumer-boundary barriers for
the existing MuSig opaque-state ledger, not additional master findings. No
nonce-cleanup or public-nonce cryptographic-severity claim is made; a public
nonce with no cryptographic meaning is not Critical on that basis.

Coverage review showed that the prior fuzzer exercised valid verifier inputs,
valid-but-mismatched cache/session/key/pubnonce combinations, and independent
signature equations, but did not force the five opaque-object load failures
inside this verifier. Deterministic tests exercise the direct invalid cases,
but they do not provide the stateful consumer oracle that couples each failure
to a valid transcript and checks callback cardinality and input preservation.

For causal proof, a temporary mutation in
`src/modules/musig/session_impl.h` recognized exactly the all-`0xFF` scalar
used by the new seed, loaded its reduced value without invoking the partial
signature loader, and continued verification. All 54 pre-existing MuSig
corpus files stayed green under that mutation on both default and
forced-int64/10x26 builds. The exact new seed aborted with status 134 on both
backends; disabling only the new gated helper made the identical mutated seed
exit 0 on both. The production mutation and helper bypass were restored before
the final replay. This proves a missing verifier oracle and reiterates the
master-relative overflow bug; it does not claim that the already-fixed audit
branch remains vulnerable.

The restored Clang ASan/UBSan target passed all 55 MuSig corpus files and the
explicit empty-input path on both backends. A bounded targeted libFuzzer run
over five state-focused seeds used `-workers=2 -jobs=2 -runs=10` and
`-timeout=60`: both default jobs completed 10 and 11 runs, and both
forced-int64 jobs completed 10 and 11 runs, all with exit 0 and no sanitizer,
assertion, or crash diagnostics. An earlier all-corpus libFuzzer attempt with
`-timeout=5` hit the ordinary empty-input execution timeout before it could be
used as evidence; it was discarded rather than reported as a finding.

## 2026-07-15 MuSig `partial_sign` NULL-Output Oracle

The MuSig target adds a gated 33-byte ASCII seed,
`partial-sign-null-output-cleanup`, that builds a valid signing transcript and
passes `partial_sig == NULL` while keeping the secret nonce, keypair,
key-aggregation cache, and session valid. The call must fail with exactly one
illegal-argument callback, consume and zero the secret nonce, and leave every
valid neighboring opaque object byte-for-byte unchanged. There is no output
object to inspect in this case; the assertion is specifically about the
invalid state transition and secret-input consumption.

This is a **Low to Medium API-state/oracle finding**, not a new clean-master
production vulnerability. It extends the existing stale-output and secret
state finding fixed by `a8457e2` and `9f0e948`; a NULL output cannot retain a
stale partial signature, but the post-load rejection must still invalidate the
consumed secret nonce. A nonce with no cryptographic meaning is not assigned a
Critical cleanup severity. Clean master already rejects the NULL output; this
commit changes no production behavior.

For causal proof, a temporary mutation in
`secp256k1_musig_partial_sign_arg_check` recognized the exact
`"partial_sig != NULL"` failure and returned without invoking the callback or
clearing the loaded signing scalars. All pre-existing MuSig inputs stayed
green under that mutation, while the focused seed aborted on the new callback
count check. Bypassing only the new gated helper made the same mutated seed
exit 0. The mutation and bypass were restored before clean replay. This proves
the new NULL-output oracle is active and also records why the earlier
NULL-argument proof needed its call-site correction.

The restored Clang ASan/UBSan replays passed all 56 MuSig corpus files plus an
explicit empty input on both the default 5x52 and forced-int64/10x26 backends.
A bounded libFuzzer campaign over five state-focused seeds used
`-workers=2 -jobs=2 -runs=10 -timeout=60` with separate work directories and
artifact prefixes: default jobs completed 10 and 11 runs, and forced-int64
jobs completed 11 and 12 runs. Every manager and worker exited 0; the logs had
no sanitizer, assertion, timeout, OOM, or crash diagnostic, and both artifact
directories remained empty. An earlier concurrent attempt shared the audit
worktree's `fuzz-0.log` and `fuzz-1.log` paths, so those logs were discarded
and the campaign was repeated with isolated work directories before being
recorded here.

## 2026-07-15 MuSig Tweak-to-Infinity Rollback Oracle

The MuSig target adds a gated 30-byte ASCII seed,
`musig-tweak-infinity-rollback`, that builds a one-key cache for the fixed
generator and independently recomputes its nonzero KeyAgg coefficient `a`.
The EC API receives `-a mod n`. The x-only API receives `-a` when the cached
point already has even Y, or `+a` when x-only normalization first negates an
odd-Y point. This makes each API's cached point and tweak cancel exactly,
despite a one-key aggregate not being the generator itself. Both APIs must
return 0 without an illegal-argument callback, leave a supplied output fully
zeroed, and preserve the opaque cache byte-for-byte. The same postcondition is
checked with `output_pubkey == NULL`.

This is an **Informational to Low master-relative API-state/oracle finding**,
not a clean-master production vulnerability. Clean `origin/master`
`ebf5943` already rejects the infinity result and pre-clears a non-NULL output;
the audit branch's cleanup form also preserves the caller cache and only clears
the public tweak scalar. No cryptographic meaning is assigned to that public
tweak or to nonce cleanup, so no Critical severity is claimed. The point of the
oracle is to ensure a future failure-path change cannot report success, save an
infinity cache, or expose stale output when ordinary scalar arithmetic reaches
the identity.

Coverage showed the production `eckey_pubkey_tweak_add` failure branch was
unhit by the existing overflow, zero, random, and invalid-cache checks. The
deterministic MuSig tests cover overflow and malformed caches but do not create
a valid cache whose known discrete-log relation cancels at the tweak boundary.
The independent coefficient calculation, parity handling, and public-key
multiplication make the focused precondition auditable rather than relying on a
guessed coefficient or a raw `n-1` assumption.

For causal proof, temporarily replace only the production infinity-failure
`goto cleanup` with `return 1`. All 56 pre-existing MuSig corpus files plus
`/dev/null` stayed green on both default and forced-int64/10x26 Clang ASan/UBSan
builds; the exact new seed aborted with status 134 on both. Bypassing only the
new gated helper made the same mutated seed exit 0 on both. The mutation and
bypass were restored before fixed replay. The restored builds passed all 57
MuSig corpus files plus `/dev/null` on both backends. A bounded libFuzzer run
used `-workers=2 -jobs=2 -runs=10 -timeout=60`: default jobs completed 11 and
12 runs, and forced-int64 jobs completed 10 and 11 runs. Every manager and
worker exited 0; no sanitizer, assertion, timeout, OOM, or crash artifact was
produced. This proves a missing oracle, not a new clean-master defect; severity
is based on clean master behavior, not on whether a later minor cleanup masks
the path.

## 2026-07-15 Recoverable ECDSA Invalid-X Recovery Oracle

The recovery target adds a gated 30-byte ASCII seed,
`recovery-invalid-x-coordinate`, that parses `(r,s) = (5,1)` with recovery
IDs 0 and 1 and a zero message. The x coordinate is below the scalar order,
but `x^3 + 7 = 132` is a non-residue modulo the secp256k1 field prime, so
neither Y-parity candidate exists. Each recovery call must return 0, invoke no
illegal-argument callback, and clear the caller's prefilled public-key output.
This targets the internal `secp256k1_ge_set_xo_var` failure, not the separate
opaque-signature, `r+n`, or final-infinity failure paths already covered by the
recovery oracle set.

This is an **Informational to Low master-relative API-state/oracle finding**,
not a clean-master production vulnerability. Clean `origin/master`
`ebf5943` rejects the non-curve x coordinate and the public recovery wrapper
clears its output. The existing deterministic `(4,4)` cases and recovery
failure cleanup checks did not assert this fixed non-residue vector with both
parities and callback cardinality; all eight pre-existing corpus inputs also
survived the causal mutation. No forgery, disclosure, or availability impact
is claimed.

For causal proof, temporarily replace only the production
`if (!secp256k1_ge_set_xo_var(...)) return 0` branch with a fallback that sets
the candidate point to the generator and continues. The eight pre-existing
recovery corpus files plus `/dev/null` stayed green on default and
forced-int64/10x26 Clang ASan/UBSan builds; the exact new seed aborted with
status 134 on both. Bypassing only the new gated helper made the same mutated
seed exit 0 on both. The production mutation and harness bypass were restored
before fixed replay. The restored builds passed all nine recovery corpus files
plus `/dev/null` on both backends. Bounded libFuzzer campaigns over five
recovery seeds used `-workers=2 -jobs=2 -runs=10 -timeout=60`; both jobs exited
0 on each backend, with no sanitizer, assertion, timeout, OOM, or crash
artifact. This proves a missing branch-specific oracle, not a clean-master
production defect; severity is based on clean master behavior.

## 2026-07-15 Rebased l0rinc Pull-Head Reconciliation

The audit worktree was fetched and rebased onto `origin/master` at
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; Git reported that
`codex/fuzz-oracles` was already up to date. All currently published l0rinc
pull heads (#1 through #13), including the current PR #12 head `944932c` and
PR #13 head `87e57c8`, the fork branch `field-5x52-serialize-word`, and the
fork master head were refreshed and compared against this rebased branch.
No additional exact cherry-pick is justified: relevant behavior is already
represented by equivalent or stronger audit commits, while the remaining
changes are optimization, comment, or test maintenance.

- PRs 1, 2, and 3 repeat the MuSig cleanup, invalid-secret, and stale-output
  work already represented by the split cleanup series and matching fuzzer
  barriers. Clean-master severity remains **Medium** for malformed
  cryptographic opaque state and **Low/informational** for public nonce stale
  state. A public nonce without cryptographic meaning is not a Critical
  secret-clearing issue.
- PR 10's field-10x26 normalization behavior is represented by the existing
  overflow oracle and zero-predicate coverage. Both remain **Medium/latent**:
  arithmetic impact could become **High** only if the maximum magnitude state
  is reachable through a real caller; this is not a remote key or signature
  claim. The field equality bound is already in master.
- PR 11's `pubkey_load` checks are represented by the existing production
  checks and callback barriers. The clean-master rating is **Medium** for
  invalid opaque state reaching a non-aborting callback path.
- The PR #12 head (`944932c`)'s 5x52 serializer and the force-updated 10x26
  serializer (`e217ead`) are already represented by the adapted
  `91e4f02`/`0bf5669` commits. They are behavior-preserving consistency work,
  not evidence that clean master is safe without the associated round-trip
  assertions.
- PR 13 (`87e57c8`) repeats the scalar shift-width guard. The stronger
  `04bfcac` commit also guards rounded shifts, carries the deterministic
  `mul-shift-over-512` oracle, and records the clean-master sanitizer proof.
  The finding remains **Low/latent internal memory safety** because current
  in-tree callers use bounded shifts and no public caller controls this
  helper's shift. The exact fork head was therefore retained as comparison
  context rather than cherry-picked; it does not lower the master-relative
  severity or replace the causal replay.
- PRs 4, 5, 6, and inherited PR 8 were deliberately not applied as whole
  heads. Their optimization stack changes behavior relevant to this audit:
  they restore unchecked public-key loads, alter failure-output handling, or
  remove cleanup and callback barriers. Applying those commits would mask
  master findings and invalidate causal proof. PRs 7 and 9 are comment/test
  maintenance and have been imported where they affect a fuzzer contract.

The current master-relative severity ledger therefore remains: **Medium**
for opaque-state and callback failure paths; **Medium/latent** for the
10x26 arithmetic defects; **Low/informational** for internal scratch
robustness; and **Low/informational** for public nonce cleanup. Every claimed
fix is tied to clean-master reproduction, a deterministic regression or
corpus condition, and mutation/control evidence. A later fork fix never
proves master safe; when it changes a follow-up path, that interaction is
recorded with the finding rather than silently treated as a regression test.

The proposed Schnorr `noncefp == NULL` with non-NULL auxiliary-data seed was
reviewed and rejected as a duplicate: the existing target already exercises
that transcript and compares custom signing with the `sign32` result. No
duplicate seed or assertion was retained.

## 2026-07-15 Multi-worker Sanitizer Campaign

A disposable Clang ASan/UBSan RelWithDebInfo build was configured from this
rebased tree with the forced-int64/10x26 test backend. Existing corpus
directories were copied without source changes and exercised with two workers
per target: MuSig loaded 57 seeds and completed 58 runs in 118 seconds;
Schnorrsig loaded 12 seeds and completed 46 runs in 100 seconds; recovery
loaded 9 seeds and completed 520 runs in 61 seconds; and EllSwift loaded 13
seeds and completed 206 runs in 61 seconds. All managers and workers exited 0;
no sanitizer, assertion, timeout, OOM, or crash artifact was produced.

The Schnorrsig output contained one isolated symbol-address line during the
multi-worker run, so it was treated as suspicious rather than dismissed. A
single-worker replay with `ASAN_OPTIONS=detect_leaks=0:abort_on_error=1:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1` loaded the resulting 41-file disposable
corpus, completed 94 runs in 21 seconds, and produced no diagnostic. This was
libFuzzer worker output interleaving, not a production or sanitizer finding.

This campaign found no new oracle gap or clean-master production defect. The
master-relative severity ledger and the l0rinc reconciliation above are
unchanged. Generated corpus files and the disposable build were removed after
the replay; no fuzz process was left running.

## 2026-07-15 Clean-Master Differential Replays

To reiterate the existing findings against the actual baseline, a disposable
worktree was checked out at clean `origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297`.
Only the audit fuzzer sources and CMake wiring were overlaid; no production
source from `codex/fuzz-oracles` was copied into that worktree. The baseline
was built with Clang ASan/UBSan and forced-int64/10x26 arithmetic. The
`ecmult_multi` target was excluded because its audit harness uses the
branch-only `checked_size_mul` helper; that build limitation is not a
production result.

Three deterministic calls then failed on clean master, while the corresponding
fixed branch seeds completed with exit 0 under the same sanitizer settings:

- `context/sha256-impossible-lengths`: `secp256k1_tagged_sha256` accepted
  `taglen == 2^61` and attempted to hash from the fuzzer's one-byte fallback
  pointer. ASan reported a global-buffer-overflow in the SHA read path before
  the function could reject the unrepresentable length. This is the
  `ab36b78` fix and is **Medium, with low practical exploitability**: it is a
  real clean-master memory-safety/availability failure, but it needs a caller
  to provide an invalid pointer/length pair and does not establish a remote
  key or signature attack.
- `ecdh/explicit-builtin-invalid-scalar`: the order-plus-one scalar was
  replaced with one for the fallback multiplication, the explicit built-in
  callback wrote a valid digest, and clean master returned 0 while leaving
  that digest in the fixed output. This is the `bb15eb0` **Low** fail-closed
  API finding; the return value remains the authoritative error signal.
- `api_roundtrip/privkey-der-export-failure`: invalid DER export set the
  output length to zero but left all 300 sentinel bytes unchanged. This is the
  `36a009f` **Low** stale-output finding; only the documented 279-byte region
  is required to be cleared.

The first and third failures were also isolated at `-O0` with GDB: the SHA
case stopped in the ASan report, the ECDH case stopped at the order-plus-one
fixed-output assertion, and the DER case stopped at the exact sentinel
comparison. Replaying the three focused seeds on the repaired audit branch
completed one run each with no sanitizer or assertion diagnostic. These are
reiterations of existing master findings, not new defects introduced by the
current branch; the stronger baseline evidence is recorded here so a later
optimization or cherry-pick cannot be mistaken for proof that clean master
was safe.

## 2026-07-15 l0rinc Boundary-Branch Refresh

The l0rinc remote was refetched after the previous reconciliation. The new
heads are `7b47f1f` (`rfc6979-reject-max-counter`) and `87e57c8`
(`scalar-mul-shift-width`); `c0f32d4` (`scratch-free-warning`) and the
boundary-condition stack were also rechecked. No new cherry-pick is justified:
the two security-relevant heads are already superseded here by the stronger
fix-and-oracle commits `bc3f625` and `04bfcac`, while the scratch change is a
GCC test-warning adjustment and the boundary stack is already in master or
represented by the 10x26 field oracle. Applying the fork heads would duplicate
the fixes without preserving their current clean-master discovery order.

The direct clean-master replays below were run from `ebf5943` with only the
audit fuzzer and CMake sources overlaid, using Clang ASan/UBSan. The repaired
branch was replayed with the same forced-int64/10x26 configuration:

- `api_roundtrip/rfc6979-counter-max`: clean master did not return from the
  exported callback within a three-second hard timeout (libFuzzer exited via
  its interrupted-run path, status 77). The callback's `i <= counter` loop
  wraps when `counter == UINT_MAX`. The repaired seed completed in 150 ms with
  exit 0. This is **Medium** for a direct public callback caller that can
  forward an untrusted attempt, and **Low/edge-case** on ordinary signing
  retries; it is a denial-of-service issue, not a cryptographic compromise.
  Clearing a nonce buffer is fail-closed hygiene only and is not critical when
  the nonce has no cryptographic meaning.
- `scalar/mul-shift-over-512`: clean master reports a stack-buffer-overflow
  while reading `l[8]` in the native 5x52/4x64 implementation at
  `scalar_4x64_impl.h:910`, and `l[16]` in the forced 10x26/8x32
  implementation at `scalar_8x32_impl.h:707`, both for the shift-513 boundary.
  The repaired forced-int64 seed completed in 53 ms with exit 0. This remains
  **Low/latent internal memory safety**: the documented helper domain permits
  the shift, but current in-tree callers use 384 and no public caller controls
  it. The initial baseline runs also printed unrelated UBSan diagnostics from
  `ecmult_impl.h:201`; those were allowed to continue so the independent
  scalar ASan finding could be captured and are not a separate audit finding.

These replays strengthen, rather than replace, the mutation proofs already in
the two fix commits. The fork heads are therefore recorded as duplicate
coverage, not cherry-picked behavior that could hide a master failure.

## 2026-07-15 MuSig Partial-Sign Opaque-State Oracle

Coverage of the 57 pre-existing MuSig corpus inputs showed that
`secp256k1_musig_partial_sign` never reached either the invalid loaded-keypair
failure at `src/modules/musig/session_impl.h:737` or the invalid session-state
failure at `:761`. Existing inputs covered invalid secnonces, caches, NULL
arguments, and signer binding, but did not couple those two rejected state
transitions to the partial-sign output and consumed-secnonce postconditions.

The new gated input
`musig/partial-sign-opaque-state-cleanup` exercises both paths. It corrupts a
keypair's opaque public half while making the secnonce's embedded point equal
to the loader's documented dummy generator point, then supplies a session with
an invalid final-nonce parity byte. Each call must return 0, invoke the
illegal callback exactly once, clear the partial-signature output and consumed
secnonce, and preserve every caller-owned keypair, cache, and session object.

This is **Informational / Low master-relative API-state oracle hardening**, not
a clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects both states and
clears the temporary signing state. The existing malformed opaque-state ledger
therefore remains **Medium** where invalid state can reach a non-aborting
callback or consumer; this commit adds branch-specific regression detection and
does not change that severity. No public nonce secrecy or Critical cleanup
finding is implied: a public nonce without cryptographic meaning is not a
Critical secret-clearing issue.

For causal proof, two temporary production mutations ignored, rather than
skipped, the corresponding loader return values:

- Replacing the invalid-keypair branch with `(void)secp256k1_keypair_load(...)`
  left all 57 pre-existing inputs green (`control=0`), while the exact new
  seed aborted with status 134 when the oracle allowed the rejected state to
  continue.
- Replacing the invalid-session branch with
  `(void)secp256k1_musig_session_load(...)` likewise left all 57 pre-existing
  inputs green (`control=0`), while the same seed aborted with status 134.

The mutations were run with `-handle_abrt=0` to make the assertion failure
status deterministic and were restored before the fixed replay. The restored
forced-int64 coverage build passed all 58 MuSig inputs with status 0. A focused
Clang ASan/UBSan replay passed in 1.95 seconds; a two-worker, two-job ASan/UBSan
replay loaded all 58 inputs, each worker completed 59 runs in 115 seconds, and
both jobs exited 0 without sanitizer, assertion, timeout, OOM, or crash
artifacts. This proves that the new oracle distinguishes both rejected state
transitions from the prior corpus, not that clean master contained a new bug.

## 2026-07-15 Schnorr Infinity-Rejection Oracle

Branch coverage of the 12 pre-existing Schnorr corpus inputs left the explicit
`secp256k1_schnorrsig_verify` infinity rejection at
`src/modules/schnorrsig/main_impl.h:270-272` unexecuted. The existing target
checked valid signatures, overflowing `r`/`s`, odd reconstructed nonces, and
coordinate equations, but none of those cases made the verifier's reconstructed
nonce the identity.

The new gated input `schnorrsig/infinity-rejection` creates `P = G`, uses
`r = x(G)`, recomputes the BIP340 challenge independently, and sets `s` to that
challenge. Therefore the verifier's own equation is
`sG - eP = eG - eG = infinity`, while all wire scalars and the x-coordinate are
valid. The call must return 0 at the dedicated identity check. This is an
**Informational / Low master-relative API-oracle finding**, not a clean-master
production vulnerability: clean `origin/master` `ebf5943` already rejects the
identity before checking its Y parity or x-coordinate. It does not change the
existing severity of malformed opaque cryptographic state, and no nonce
clearing severity is implied by this public test vector.

For causal proof, the production infinity branch was temporarily changed from
`return 0` to `return 1`. All 12 pre-existing corpus files stayed green
(`control=0`) under that mutation, while the exact new seed aborted with status
134 under `-handle_abrt=0`. The mutation was restored before the fixed replay.
The restored forced-int64 Clang ASan/UBSan build passed all 13 Schnorr inputs.
The two-worker/two-job replay loaded all 13 inputs in each job and completed 14
runs per job without sanitizer, assertion, timeout, OOM, or crash artifacts.
This proves the new oracle reaches a previously untested identity transition;
it does not claim a clean-master bug.

## 2026-07-15 Group Z-Inverse Cancellation Oracle

The full group corpus covered ordinary Jacobian cancellation and the
`secp256k1_gej_add_zinv_var` infinity-input boundaries, but did not reach its
finite inverse-point branch at `src/group_impl.h:693-699`. That branch is
different from `secp256k1_gej_add_var`: it compares a Jacobian point with an
affine point whose Z inverse is supplied separately, then must represent
`G + (-G)` as the identity.

The gated input `group/zinv-inverse` fixes `a = G`, constructs the affine
negation `-G`, supplies `bzinv = 1`, and pre-fills the result with `0xA5`. The
oracle requires the infinity flag and all three projective coordinates to be
zero. This is **Informational / Low master-relative group-oracle hardening**,
not a clean-master production vulnerability: clean `origin/master` already
calls `secp256k1_gej_set_infinity` for this algebraic cancellation. It does not
change the existing severity ledger for malformed opaque state or arithmetic
memory safety.

For causal proof, the production `secp256k1_gej_set_infinity(r)` in this
z-inverse inverse branch was temporarily replaced with `*r = *a`. All 15
pre-existing group corpus files stayed green (`control=0`), while the exact
new seed aborted with status 134 under `-handle_abrt=0`. The mutation was
restored before replay. The fixed forced-int64 Clang ASan/UBSan corpus and the
two-worker/two-job replay passed all 16 inputs without sanitizer, assertion,
timeout, OOM, or crash artifacts. This proves a previously untested specialized
group transition, not a new defect on clean master.

## 2026-07-15 Group Affine-Infinity Validity Oracle

The group fuzzer's affine checker intentionally skipped `secp256k1_ge_is_valid_var`
when the converted point was infinity. The existing group corpus therefore did not
execute the direct rejection at `src/group_impl.h:450-452`, even though the unit
tests checked the same contract indirectly after Jacobian-to-affine conversion.

The gated input `group/infinity-validity` constructs an affine infinity with
`secp256k1_ge_set_infinity`, then requires `secp256k1_ge_is_infinity` to report
true and `secp256k1_ge_is_valid_var` to report false. This is **Informational /
Low master-relative fuzzer-oracle hardening**, not a clean-master production
vulnerability: clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects affine infinity.
The existing unit assertion at `src/tests.c:4783` remains useful, but it did not
make this fuzzer target exercise the branch. No public nonce secrecy or Critical
cleanup finding is implied by this arithmetic validity check.

For causal proof, the production infinity return in `secp256k1_ge_is_valid_var`
was temporarily changed from `return 0` to `return 1`. All 16 pre-existing group
corpus files stayed green (`mutated-control-status=0`, 17 total executions
including libFuzzer initialization), while the exact new seed aborted with status
134 under `-handle_abrt=0`. The mutation was restored before fixed replay. The
restored forced-int64 Clang ASan/UBSan build passed all 17 group inputs and the
empty-input path. A two-worker, two-job replay loaded all 17 inputs in each job,
completed 18 runs per job, and both jobs exited 0 without sanitizer, assertion,
timeout, OOM, or crash artifacts.

As part of the same audit, the defensive Strauss fallback at
`src/ecmult_impl.h:862-864` was coverage-checked. Current scratch-size ordering
does not produce the required state where Pippenger can handle the request while
Strauss cannot, so no unreachable-domain seed or speculative oracle was added.

## 2026-07-15 X-Only NULL Source-Key Output Oracle

The x-only target already supplied invalid opaque public-key bytes to
`secp256k1_xonly_pubkey_from_pubkey`, but that is a different state from a NULL
source pointer. The latter is rejected by the public argument contract before
the loader runs, and clean master explicitly zeroes the destination and resets
the optional parity output first (`src/modules/extrakeys/main_impl.h:108-115`).
The existing corpus did not reach that branch.

The gated input `xonly_tweak/pubkey-from-pubkey-null` passes a compiler-opaque
NULL source pointer while keeping a valid destination, then requires one illegal
callback, a zeroed `secp256k1_xonly_pubkey`, and parity `0`. This is
**Informational / Low master-relative API-oracle hardening**, not a clean-master
production vulnerability. A caller that passes NULL has already violated the
API precondition; no key disclosure, forgery, or cryptographic nonce-cleanup
impact is claimed. In particular, public nonce data without cryptographic
meaning is not a Critical secret-clearing finding.

For causal proof, only the exact NULL-source cleanup at
`src/modules/extrakeys/main_impl.h:112-114` was disabled. All pre-existing
x-only corpus inputs (11) stayed green, while the new seed aborted with status
134 on the zero-output assertion. The mutation was restored before fixed replay.
Coverage then recorded the NULL branch as `True: 1, False: 111` and the cleanup
`memset` as executed once. The restored forced-int64 Clang ASan/UBSan build
passed all 12 x-only inputs plus the empty-input path with no diagnostics. A
two-worker/two-job replay loaded all 12 inputs in each job, completed 13 runs
per job, and both jobs exited 0 without sanitizer, assertion, timeout, OOM, or
crash artifacts. The intentionally illegal call is made through an unannotated
function pointer so UBSan does not mistake the harness's deliberate API
precondition test for a production violation. This demonstrates a previously
untested public API state transition, not a new clean-master defect.

## 2026-07-15 EllSwift Inverse Degeneracy Oracle

The existing EllSwift inverse vector reached successful results and ordinary
rejections for all eight `c` branches, but its derived nonzero `u` never directly
exercised the two documented zero-state guards at
`src/modules/ellswift/main_impl.h:282-286`. Those guards are distinct: odd `c`
rejects `r == 0`, while every x3-formula branch rejects `s == 0` before inversion.

The gated input `ellswift/inverse-degenerate` calls the internal inverse with
`x = u = G.x`. This is a valid on-curve x-coordinate and a reachable field
state: `s = x-u = 0`, `q = 0`, and `r = sqrt(q) = 0`. The helper invokes
`c = 3` to reach the odd-`c`/zero-`r` rejection and `c = 2` to reach the
zero-`s` rejection. It checks only the documented return value; the failed `t`
output remains intentionally unconstrained.

This is **Informational / Low master-relative fuzzer-oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already contains both guards, and
the prior unit/inverse-vector coverage did not construct `u = x`. The finding
does not imply a cryptographic nonce-clearing issue; public nonce data without
cryptographic meaning is not a Critical secret-cleanup finding.

For causal proof, both exact production guard conditions were temporarily
changed to `if (0) return 0`. All 13 pre-existing EllSwift corpus files stayed
green (`control=0`), while the exact new seed aborted with status 134 under
`-handle_abrt=0`. Bypassing only the new helper made the same mutated binary
exit 0, and both guards were restored before fixed replay. The restored
forced-int64 Clang ASan/UBSan replay passed all 14 EllSwift inputs plus the
empty-input path. A two-worker/two-job replay loaded all 14 inputs in each job,
completed 15 runs per job, and both jobs exited 0 without sanitizer, assertion,
timeout, OOM, or crash artifacts.

## 2026-07-15 Affine Equality Infinity-Mismatch Oracle

The group target already checked positive affine equality and negative
Jacobian/affine equality, but its `secp256k1_ge_eq_var` calls compared points
with matching infinity flags. The direct mismatch guard at
`src/group_impl.h:403` therefore remained untested by the fuzzer. This is a
different contract from `secp256k1_gej_eq_ge_var`: it checks the affine
representation's infinity marker before touching coordinates.

The gated input `group/affine-equality-infinity` constructs affine infinity and
the generator, then requires infinity to equal itself and to differ from the
generator in both argument orders. This is **Informational / Low
master-relative internal-oracle hardening**, not a clean-master production
vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already has the correct mismatch
return, and the existing unit and group operations did not provide this direct
affine equality oracle. No cryptographic nonce or secret-cleanup impact is
claimed.

For causal proof, the exact mismatch return at `src/group_impl.h:403` was
temporarily changed from `return 0` to `return 1`. All 17 pre-existing group
corpus inputs stayed green, while the new seed aborted on its first mismatch
assertion with status 134. The mutation was restored before fixed replay.
Coverage recorded the mismatch branch as `True: 2, False: 511`, one hit for
each argument order. The restored forced-int64 Clang ASan/UBSan replay passed
all 18 group inputs and the empty-input path. A two-worker/two-job replay
loaded all 18 inputs in each job, completed 19 runs per job, and both jobs
exited 0 with no sanitizer, assertion, timeout, OOM, or crash artifacts. This
proves a missing branch-specific oracle, not a clean-master defect.

## 2026-07-15 MuSig Secret-Nonce Zero-Scalar Load Oracle

The MuSig opaque-nonce barrier already used a valid secret nonce with an
overflowing scalar, so it exercised `secnonce_load`'s `overflow0` rejection but
never reached either scalar-zero clause. Coverage of the 58 pre-existing MuSig
inputs showed zero hits for both `secp256k1_scalar_is_zero(&k[0])` and
`secp256k1_scalar_is_zero(&k[1])` at `src/modules/musig/session_impl.h:82-84`.

The gated input `musig/secnonce-zero-scalar-load` keeps the magic, signer
binding, and public point valid, then makes two independent copies of the
secret nonce: one with `k[0] = 0`, and one with `k[1] = 0`. Each
`secp256k1_musig_partial_sign` call must reject once, clear the partial-signature
output, and fully invalidate the consumed secret nonce. This is
**Informational / Low master-relative secret-state oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects both states. The
secret nonce is cryptographically meaningful, but this finding does not claim
that clean master permits nonce reuse, forgery, disclosure, or a Critical
cleanup failure.

For causal proof, the two production clauses were mutated independently: first
only the `k[0] == 0` rejection was removed, then only the `k[1] == 0` rejection
was removed. All 58 pre-existing MuSig inputs stayed green under each mutation;
the focused seed aborted with status 134 in each case. Fixed coverage recorded
one hit for each zero-scalar branch. The restored forced-int64 Clang ASan/UBSan
replay passed all 59 MuSig inputs plus empty input, and the native Clang
ASan/UBSan replay passed the same set. A two-worker/two-job forced-int64 replay
loaded all 59 inputs in each job, completed 60 runs per job, and both jobs
exited 0 without sanitizer, assertion, timeout, OOM, or crash artifacts. This
proves a missing secret-state oracle, not a clean-master defect.

## 2026-07-15 Recoverable ECDSA Zero-S Oracle

The recovery target already reached the first side of the internal
`secp256k1_scalar_is_zero(sigr) || secp256k1_scalar_is_zero(sigs)` guard with
all-zero signatures and an `(r, s) = (0, order)` vector. The second side was
not reachable from the existing compact parser corpus: `(order, 0)` is rejected
as an overflowing scalar before recovery. That left the valid-`r`, zero-`s`
state without a direct rejection oracle.

The gated input `recovery/zero-s-rejection` parses `(r, s) = (4, 0)` with
recovery id `0`, then recovers with a nonzero message. `r = 4` is a valid
recovery x-coordinate, so an incorrectly accepted `s = 0` cannot silently
produce the infinity result expected from this invalid signature. The helper
requires recovery to return `0` and independently checks that the destination
`secp256k1_pubkey` is zeroed.

This is **Informational / Low master-relative fuzzer-oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects both zero scalar
states; no signature-forgery, key-disclosure, or nonce-clearing impact is
claimed. In particular, public nonce data without cryptographic meaning is not
a Critical secret-cleanup finding.

For causal proof, only the production condition at
`src/modules/recovery/main_impl.h:122` was changed from
`scalar_is_zero(sigr) || scalar_is_zero(sigs)` to `scalar_is_zero(sigr)`. All
9 pre-existing recovery corpus inputs stayed green, while the exact new seed
aborted with status 134 on the zero-output assertion. The mutation was restored
before fixed replay. Coverage then recorded the first branch as `True: 12,
False: 126` and the previously unhit `sigs` branch as `True: 1, False: 125`.
The restored forced-int64 Clang ASan/UBSan replay passed all 10 recovery inputs
plus empty input; native-width Clang ASan/UBSan passed the same 11 inputs. A
two-worker/two-job forced-int64 replay loaded all 10 inputs in each job,
completed 11 runs per job, and both jobs exited 0 without sanitizer, assertion,
timeout, OOM, or crash artifacts. This proves a missing zero-`s` oracle, not a
clean-master defect.

## 2026-07-15 MuSig Secret-Nonce Second-Scalar Overflow Oracle

The existing MuSig opaque-nonce barrier used an overflowing first secret
scalar, so it reached `secnonce_load`'s `overflow0` short-circuit but never
forced evaluation of the independent `overflow1` condition. Coverage of the
59 pre-existing MuSig inputs recorded zero hits for the second overflow branch
at `src/modules/musig/session_impl.h:82`.

The gated input `musig/overflow1-secnonce-scalar` keeps the generated nonce
valid, then replaces only bytes 36 through 67 with the big-endian scalar order
plus one. `secp256k1_scalar_set_b32` therefore reports overflow for `k[1]` but
reduces it to the nonzero scalar one; `k[0]` remains valid, so neither zero
predicate can satisfy the rejection. `partial_sign` must reject once, clear
the prefilled partial signature, and invalidate the consumed secret nonce.
This is **Informational / Low master-relative secret-state oracle hardening**,
not a clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects both overflow
states. The secret nonce is cryptographically meaningful, but this finding
does not claim nonce reuse, forgery, disclosure, or a Critical cleanup impact.

For causal proof, only the production condition at
`src/modules/musig/session_impl.h:82-84` was changed from
`overflow0 || overflow1 || scalar_is_zero(k[0]) || scalar_is_zero(k[1])`
to the same expression without `overflow1`. All 59 pre-existing MuSig inputs
stayed green, while the exact new seed aborted with status 134. The mutation
must use order plus one: using exactly order reduces `k[1]` to zero and would
exercise the later zero predicate instead of isolating `overflow1`. Fixed
coverage recorded `overflow1` as `True: 1, False: 2,250`, with each zero
branch also true once. The restored forced-int64 Clang ASan/UBSan replay
passed all 60 MuSig inputs plus empty input; native-width Clang ASan/UBSan
passed the same 61 inputs. A two-worker/two-job forced-int64 replay loaded all
60 corpus files, completed 61 runs per job, and both jobs exited 0 after 119
seconds without sanitizer, assertion, timeout, OOM, or crash artifacts. This
proves a missing second-overflow oracle, not a clean-master defect.

## 2026-07-15 MuSig Second Derived-Nonce Scalar Zero Oracle

The existing zero-derived-nonce helper forced every SHA compression state to
zero, so both derived scalars became zero and short-circuit evaluation reached
only `secp256k1_scalar_is_zero(&k[0])`. Fresh coverage of the 60 pre-existing
MuSig inputs recorded no hits for the second branch at
`src/modules/musig/session_impl.h:449`.

The gated input `musig/nonce-second-zero-scalar-failure` keeps a valid nonce
transcript and installs a narrow SHA callback mode. It recognizes the current
nonce transcript's `i == 1` final padding block, zeros both compression states
for that scalar only, and requires exactly one match. The first derived scalar
therefore remains the ordinary nonzero hash result while the second is zero.
`musig_nonce_gen` must return 0, preserve the caller's session-random buffer,
and clear both the secret and public nonce outputs. This is **Informational /
Low master-relative nonce-state oracle hardening**, not a clean-master
production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects both derived-zero
states; no nonce-reuse, forgery, disclosure, or Critical cleanup impact is
claimed.

For causal proof, only the production condition at
`src/modules/musig/session_impl.h:449` was changed from
`scalar_is_zero(&k[0]) || scalar_is_zero(&k[1])` to
`scalar_is_zero(&k[0])`. All 60 pre-existing MuSig inputs stayed green, while
the exact new seed aborted with status 134. The mutation therefore proves the
first derived scalar stayed nonzero and isolates the second branch. Fixed
coverage recorded the first branch as `True: 65, False: 4,498` and the newly
reached second branch as `True: 1, False: 4,497`. The restored forced-int64
Clang ASan/UBSan replay passed all 61 MuSig inputs plus empty input; native
width Clang ASan/UBSan passed the same 62 inputs. A two-worker/two-job
forced-int64 replay loaded all 61 corpus files, completed 62 runs per job,
and both jobs exited 0 after 121 seconds without sanitizer, assertion,
timeout, OOM, or crash artifacts. This proves a missing second-zero oracle,
not a clean-master defect.
