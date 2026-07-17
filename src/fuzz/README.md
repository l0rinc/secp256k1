# secp256k1 Fuzz Oracles

Current upstream `master` did not contain tracked fuzz targets or seed corpora
before this directory was added. These targets are intentionally small public-API
oracles that exercise contract boundaries rather than only maximizing coverage.

Targets:

- `fuzz_api_roundtrip`: compressed/uncompressed/hybrid pubkey wire parsing, two-, three-, four-, eight-, and sixteen-term public-key combine with intermediate-infinity transitions, static-context public combine against fixed SEC1 vectors, NULL-member combine cleanup, four-, eight-, and sixteen-key public-key sorting with duplicate-pointer preservation, independent byte-level tweak arithmetic at the order-minus-one boundary including static-context public add/mul, secret-key tweak input/output overlap, independent ECDSA low-S half-order boundary, ECDSA input/output overlap, ECDSA compact, direct RFC6979 algorithm-domain transcripts, arbitrary-signature verification equation, fixed- and variable-nonce equations, fixed ECDSA verification-infinity and finite-x-mismatch transitions, valid- and invalid-secret nonce callback key- and message-domain checks, valid-nonce retry and post-retry failure cleanup, NULL-argument ECDSA signing cleanup, NULL-output public-key and compact-signature serialization cleanup, empty/NULL/invalid sort, DER, independently parsed private-key DER, signing, verification, normalization
- `fuzz_context`: context randomize, clone, reset, NULL-reset deterministic ECDSA and Schnorr signing, valid legacy-flag matrix, invalid-flag rejection, deterministic signing consistency, custom SHA compression equivalence through source and heap/preallocated clones during public-key creation and ECDSA/Schnorr signing, standalone tagged-SHA reference, and tagged-SHA output/tag and output/message overlap
- `fuzz_hash`: shared standalone SHA-256 reference, raw-SHA256 HMAC reference, arbitrary multi-block midstate reference, full-stream RFC6979 sequencing, chunking consistency, and finalized-state cleanup
- `fuzz_scalar`: scalar bit-extraction boundaries and rounded multiply-shift
  boundaries against independent byte/product references
- `fuzz_field`: internal field normalization, arithmetic, nonnormalized arithmetic, maximum-magnitude multiplication aliasing, strict input parsing, encoding, field cleanup, add-int boundaries, maximum-magnitude consistency and inversion representation invariance, canonical/raw-modulus zero-predicate slow-path checks, zero-predicate false-positive barriers, byte-level maximum-residue references, and independent byte-level negation, small-multiplier, add-int, and square-root references
- `fuzz_group`: Jacobian/affine group-operation agreement, independent canonical-coordinate equality, positive and negative Jacobian/affine equality including affine-infinity mismatches, fractional curve-membership, finite and mixed-infinity batch conversion, direct inverse-Z affine conversion, ordinary inverse-point cancellation with optional Z-ratio postconditions, nonnormalized affine-to-storage conversion, normalized and nonnormalized rescale scales, rescale aliasing, invalid opaque public-key operation barriers, lambda-degenerate alternate-slope addition, affine-point cleanup, and state cleanup
- `fuzz_ecmult_const`: constant-time multiplication, a fixed generator-times-two known-answer vector, affine generator conversion, NULL-generator equivalence, direct odd-multiples-table omitted-Z reconstruction, and normalized/non-normalized rational x-only fractions
- `fuzz_ecmult_multi`: internal scratch/no-scratch multi multiplication consistency, independent serialized-coordinate result equality, false-positive equality barriers, callback batching/failure barriers including a fixed sixteen-point direct batch and distinct three-batch Pippenger transcripts, scratch accounting and checkpoint-prefix preservation, checked allocation multiplication, and defined scalar-state transitions
- `fuzz_ecdh`: ECDH symmetry with a standalone default-SHA reference, a fixed generator-times-two byte-equation oracle, coordinate passthrough hashers, built-in callback NULL-input output cleanup, and invalid-scalar callback-point postconditions
- `fuzz_ellswift`: EllSwift encode/decode, modulo-alias wire encodings, randomizer influence, inverse-branch round trips and degenerate rejection guards, an independent BIP324 decode vector and SHA transcript, a fixed decoded-point scalar-one XDH vector, both-party raw XDH point consistency, XDH symmetry, built-in hash cleanup, built-in callback NULL-input output cleanup, invalid-secret callback-X postconditions, and custom hash callback encoded-party domain checks
- `fuzz_xonly_tweak`: x-only serialization, standalone byte-level curve-membership parsing, parity, tweak, static-context public tweaking, keypair equivalence, invalid keypair-creation cleanup, partial keypair projections and tweak rejection, invalid and NULL full-pubkey conversion, invalid comparator ordering, and complete in/out tweak alias coverage
- `fuzz_recovery`: recoverable ECDSA round trips, recoverable signing input/output overlap, arbitrary parsed-signature recovery, a fixed generator recovery vector, independent recovery point equations, static-context parse/serialize/convert/recover/verify, zero-`s` recovery rejection, no-curve-point recovery failure cleanup, nonce callback key- and message-domain checks, valid-nonce retry, and post-retry failure cleanup when recovery is enabled
- `fuzz_schnorrsig`: Schnorr sign/verify, standalone BIP340 tagged-SHA reference, arbitrary-signature BIP340 verification equation, empty-message pointer equivalence, `sign32`/`sign_custom` equivalence, nonce callback message-domain checks, signing precondition cleanup, an independent BIP340 point-equation model, and a fixed generator algebraic-equation oracle that also checks static-context verification
- `fuzz_musig`: MuSig key aggregation, zero-length key/nonce/partial-signature aggregation boundaries, one- through sixteen-key independent coefficient transcripts, valid duplicate-key first-distinct coefficient transcripts, zero-coefficient and weighted-key-cancellation aggregate-infinity rejection, optional aggregate outputs, opaque cache curve/state barriers, tweak equivalence, x-only-tweak signing, standalone tagged-SHA transcripts, an authoritative BIP327 nonce-generation known-answer vector, one- through sixteen-signer nonce/signature round trips, consumed-secnonce reuse rejection, failure-path secnonce invalidation, zero secret-nonce scalar load rejection, second secret-nonce scalar overflow rejection, NULL-argument partial-sign cleanup, NULL-member nonce/final-signature aggregation cleanup, counter-nonce optional-input equivalence, partial-keypair counter-nonce rejection, optional-secret-key nonce-input equivalence, session-random aliases with optional inputs and the aggregate cache, deterministic zero-derived-nonce failure, second derived-nonce scalar zero rejection, mixed-infinity effective-nonce modeling, NULL-input and invalid-cache nonce-process cleanup, arbitrary parseable partial-signature verification equations, invalid opaque partial-signature verification state, and independent partial- and final-signature point equations

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

The entries below preserve the severities recorded against their historical
clean-master snapshots. For current decisions, the authoritative baseline is
`origin/master` at `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, which is also the
current `l0rinc/master` as of 2026-07-17. The current-master replays and
follow-up sections below reassess findings against that revision. A later
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

- **Medium / confirmed internal memory safety, low current reachability:**
  `ecmult_multi/scratch-wrap-create` (`b827e0e`). A clean-origin/master
  ASan/UBSan replay at `11dad6d` requests `SIZE_MAX`; `base_alloc + size`
  wraps to 31 bytes, and the constructor then clears the 32-byte scratch
  header. Native 5x52 and forced-int64/10x26 builds both report the resulting
  heap-buffer-overflow. The constructor is static/internal in this baseline,
  the removed public scratch-space symbols are not an entry point, and current
  production MuSig aggregation uses the no-scratch path, so no remotely
  reachable public-API exploit was demonstrated. That reachability limit does
  not make the confirmed memory corruption informational. The guard and
  `SIZE_MAX` regression test are in `cac07e8`; the earlier Low label
  understated impact, while the original High label overstated current public
  reachability.
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
- **Informational oracle hardening:** `fuzz_musig` now checks exact aliases of
  the documented `In/Out session_secrand32` buffer with each optional 32-byte
  nonce input: `seckey`, `msg32`, and `extra_input32`. The independent MuSig
  transcript is evaluated from pre-call bytes, while the postcondition still
  requires successful calls to zero the shared session-random storage and
  produce the expected secret and public nonces. The focused
  `session-random-input-overlap` seed and the complete 65-file MuSig corpus
  pass on the restored Clang ASan/UBSan build. A temporary production
  clear-before-read mutation leaves all 64 pre-existing files green but makes
  the new seed abort, proving this is an independent alias oracle rather than
  duplicate coverage. This is negative oracle hardening only: no clean-master
  production defect or severity change is claimed, and invalidation of a
  cryptographically non-meaningful public nonce is not rated Critical.
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
- **Low:** input/output tweak aliasing in
  `secp256k1_ec_pubkey_tweak_add` and
  `secp256k1_keypair_xonly_tweak_add`. On clean master, both functions clear
  their in/out object before consuming `tweak32`. A caller that deliberately
  places a valid tweak in `pubkey.data` or the keypair's secret half therefore
  receives success but applies a zero tweak. The public contracts document the
  output object and tweak input but do not prohibit this overlap. This is API
  aliasing/state correctness, not memory corruption or a cryptographic key
  compromise; the exact clean-master and production-mutation proofs are
  recorded in the detailed entry below.
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

## 2026-07-15 l0rinc PR #14 DER Duplicate

The refreshed l0rinc PR #14 head `b5e6108` was compared with the rebased
branch. Its `src/ecdsa_impl.h` change is byte-for-byte equivalent to the
production part of `52cb1af`: both replace the `sig + inputlen` end-pointer
construction with offset checks before indexing or advancing. The fork test
snapshot is different, however: it removes existing audit assertions for
scratch, HMAC/RFC6979 state, impossible SHA lengths, scalar boundaries, and
field/group edge cases. Cherry-picking the whole head would therefore add no
production behavior and would weaken the evidence carried by the current
follow-up stack.

No PR #14 source was cherry-picked. The existing `52cb1af` commit retains the
stronger clean-master proof: Clang pointer-overflow UBSan reports the
one-byte `{0x00}`, `SIZE_MAX` DER parse on clean master before the parser can
reject it, while the offset implementation returns failure without forming an
out-of-range pointer. This remains **Low parser/API robustness** against clean
master because the public API requires an `inputlen`-byte array; it is not a
remote signature forgery or memory-corruption claim. PR #14 is recorded as
duplicate context rather than as a new finding.

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

## 2026-07-15 MuSig Key-Aggregation Zero-Coefficient Infinity Oracle

The existing MuSig key-aggregation corpus covered successful aggregation and
invalid input cleanup, but did not reach the explicit aggregate-infinity
rejection at `src/modules/musig/keyagg_impl.h:216`. The gated seed
`musig/keyagg-zero-coefficient` supplies one valid fixed public key and
temporarily installs the existing SHA compression hook in an all-zero mode.
That makes both the one-key `KeyAgg list` hash and its `KeyAgg coefficient`
hash zero, so the weighted aggregate is the point at infinity. The helper
requires both hash domains to execute, requires `secp256k1_musig_pubkey_agg`
to return 0, and checks that the aggregate x-only key and cache remain fully
zeroed after rejection.

This is **Informational / Low master-relative oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already contains the infinity
guard. The zero hash is a test-only callback mutation of the internal hash
transcript, not a claim that an attacker can choose MuSig's cryptographic
hash output; no forgery, disclosure, or availability impact is claimed.

For causal proof, coverage of the 61 pre-existing MuSig inputs recorded zero
true hits for the condition at line 216 and zero executions of its rejection
return. With the new seed added, the same coverage build recorded one true
hit and one rejection return. Changing only that production condition from
`if (secp256k1_gej_is_infinity(&pkj))` to
`if (0 && secp256k1_gej_is_infinity(&pkj))` left all 61 pre-existing inputs
green, while the focused seed exited with status 134 at the established
post-infinity `VERIFY_CHECK` on line 221. Restoring the guard made all 62
tracked inputs pass. This proves the seed reaches the intended production
state transition rather than merely duplicating a successful aggregation
case.

Final-source Clang ASan/UBSan deterministic replays passed all 62 MuSig
inputs on both native and forced-int64/10x26 arithmetic. The bounded
libFuzzer replay used two jobs with two workers per backend; every job
completed 65 runs and exited 0 without an ASan/UBSan diagnostic, assertion,
timeout, OOM, or artifact. The shared temporary corpora grew by two ordinary
libFuzzer-generated inputs during the replay; those files were not retained.
This remains a missing oracle, not a new clean-master defect.

## 2026-07-15 MuSig Weighted-Key-Cancellation Infinity Oracle

The zero-coefficient seed covered one route to the aggregate-infinity guard,
but the same production contract also names cancellation of weighted public
keys. The gated seed `musig/keyagg-weighted-cancellation` uses the fixed
points `G` and `-G`; an independent `secp256k1_ec_pubkey_combine` check
confirms that the two inputs cancel. During `secp256k1_musig_pubkey_agg`, the
SHA hook leaves the `KeyAgg list` hash and all ordinary hashing unchanged, but
replaces exactly the final compression state of the first 65-byte `KeyAgg
coefficient` transcript with scalar one. The second distinct key naturally
uses its identity coefficient, so the production multiscalar operation is
`1*G + 1*(-G)` rather than a zero-coefficient shortcut. The oracle requires
one and only one coefficient override, rejection, and zeroed aggregate/cache
outputs.

This is **Informational / Low master-relative oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects the cancellation
state at line 216. The coefficient-one state is a test-only transcript
mutation; no forged-signature, disclosure, or availability impact is claimed.

For causal proof, the 62-input control corpus already contained the separate
zero-coefficient infinity seed and recorded one true hit at line 216. Adding
the cancellation seed raised the same guard to two true hits and its helper
executed once with exactly one coefficient-one match. For the isolated
production mutation, only the line-216 condition was changed to
`if (0 && secp256k1_gej_is_infinity(&pkj))`; the 61 controls excluding both
focused infinity seeds stayed green, while the cancellation seed exited 134
at the established post-infinity `VERIFY_CHECK` on line 221. Restoring the
guard made all 63 tracked inputs pass. This demonstrates the second
infinity cause independently of the earlier zero-coefficient oracle.

Final-source Clang ASan/UBSan deterministic replays passed all 63 MuSig
inputs on native and forced-int64/10x26 arithmetic. The bounded libFuzzer
replay used two jobs with two workers per backend; every job completed 66 runs
and exited 0 without sanitizer, assertion, timeout, OOM, or artifact. The
shared temporary corpora began with 63 tracked files and grew to 65 through
ordinary libFuzzer additions during the replay; those files were not retained.
This remains a missing oracle, not a new clean-master defect.

## 2026-07-15 Group Inverse-Point Z-Ratio Oracle

Coverage of the 18 pre-existing group corpus inputs reached the ordinary
Jacobian and affine inverse-point cancellation paths, but never requested the
optional `rzr` result in either specialized branch. The assignments at
`src/group_impl.h:558` and `:621` therefore remained unexecuted even though
their contract differs from cancellation with `rzr == NULL`: both paths must
return the identity and explicitly set the caller's ratio output to zero.

The gated input `group/inverse-rzr` constructs `a = G` and `b = -G`, then runs
both `secp256k1_gej_add_var` with a Jacobian `b` and
`secp256k1_gej_add_ge_var` with an affine `b`. Each result starts as `0xA5`,
as does `rzr`; the oracle requires the infinity flag, all three identity
coordinates, and the requested ratio to be zero. This is **Informational /
Low master-relative internal-oracle hardening**, not a clean-master
production vulnerability: clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already contains both zero
assignments. No cryptographic or nonce-secrecy impact is claimed.

For causal proof, the first assignment was temporarily changed from
`secp256k1_fe_set_int(rzr, 0)` to `...1`. All 18 pre-existing inputs stayed
green, while the focused seed aborted with status 134 at the new ratio
assertion. The source was restored, and the second assignment was mutated in
the same way: the same 18 controls exited 0 and the focused seed exited 134.
Coverage after restoration recorded execution of both assignments, including
the live `rzr != NULL` inverse branches. The fixed coverage replay passed all
19 tracked inputs. A Clang ASan/UBSan standalone replay also passed all 19
inputs. Finally, a bounded libFuzzer replay over a copied 19-file corpus used
`-workers=2 -jobs=2 -max_total_time=20`; both jobs exited 0 after 630 and 633
runs, with no sanitizer, assertion, timeout, OOM, or crash artifact. The
temporary libFuzzer corpus grew through normal mutations and was removed.
This proves two previously untested ratio-output transitions, not a new bug
on clean master.

## 2026-07-15 Field Zero-Predicate Slow-Path Oracle

The field corpus compared `secp256k1_fe_normalizes_to_zero_var` with a
production-derived normalized value, but never forced the 5x52 variable-time
implementation past its fast nonzero return. The missing path matters because
zero can arrive both as the canonical zero representation and as the raw field
modulus before normalization.

The gated input `field/zero-predicate-slow-path` constructs those two
representations and requires both constant-time and variable-time predicates
to report the independently known zero residue. It then normalizes the raw
modulus and checks the public field-zero postcondition. This is **Informational /
Low master-relative arithmetic-oracle hardening**, not a clean-master
production vulnerability: clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already returns the correct value.
No cryptographic or nonce-secrecy impact is claimed.

For causal proof, the production variable-time return at
`src/field_5x52_impl.h:198` was temporarily changed to `return 0`. All 16
pre-existing field inputs stayed green, while the exact focused seed exited
with status 134 on its independent zero-residue assertion. The mutation was
restored before fixed replay. Fresh coverage recorded six executions of the
previously untouched slow-path lines 183-198. The restored Clang ASan/UBSan
replay passed all 17 field inputs on both native 5x52 and forced-int64/10x26
arithmetic. A bounded libFuzzer replay used two workers and two jobs over a
copied 17-file corpus; both jobs exited 0 after 362 and 361 executions with no
sanitizer, assertion, timeout, OOM, or crash artifact. This proves a previously
untested representation transition, not a new bug on clean master.

## 2026-07-15 RFC6979 Algorithm-Domain Transcript Oracle

The API target exercised the exported RFC6979 callback only with the ECDSA
compatibility form where `algo16 == NULL`. That left the public callback's
optional algorithm domain and its interaction with the 32-byte extra-data
domain untested, even though the production transcript has separate fixed-size
append operations for both. The existing unit test checks that a few calls with
and without algorithm data differ, but does not independently reconstruct the
expected transcript or cover retry counters beyond the ordinary signing path.

The gated seed `api_roundtrip/rfc6979-algorithm-domain` derives a valid key,
message, and extra-data value from the input, then calls
`secp256k1_nonce_function_rfc6979` with a fixed 16-byte algorithm tag. A small
raw-SHA256 HMAC model independently implements RFC6979 initialization and
generates the expected nonce for counters 0, 1, and 2. The oracle compares all
three outputs against the public callback, including the reduced message,
32-byte extra-data field, and 16-byte algorithm field in their documented
order. The raw-SHA256 reference is shared with `fuzz_hash` through
`src/fuzz/rfc6979_reference.h`, and sensitive reference state is explicitly
cleared after each check.

This is **Informational / Low master-relative oracle hardening**, not a
clean-master production vulnerability. The relevant `algo16` append at
`src/secp256k1.c:613` is unchanged in clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; no cryptographic, disclosure, or
availability impact is claimed. The prior compact NULL-signature candidate was
discarded because it duplicated an existing deterministic unit test and a
branch-only output-cleanup fix, so it is not counted as a separate finding.

For causal proof, the production `buffer_append(keydata, &offset, algo16, 16)`
was temporarily changed to append zero bytes. All 38 pre-existing API corpus
inputs stayed green, while the focused algorithm-domain seed exited with status
134 at the independent transcript comparison. Restoring the append made all 39
tracked inputs pass; final coverage took the algorithm branch 3 times and the
NULL branch 468 times, with line 613 executed 3 times. Clang ASan/UBSan
deterministic replays covered all 39 seeds plus an empty input on native and
forced-int64/10x26 builds. Fresh two-worker, two-job libFuzzer replays over
independent copied corpora loaded all 39 seeds and exited cleanly: native jobs
completed 65 and 66 runs, while forced-int64 jobs completed 40 and 40. No
sanitizer, assertion, timeout, OOM, or crash artifacts were produced. This
proves a missing independent oracle for a reachable clean-master contract, not
a claim that clean master currently has a nonce bug.

## 2026-07-15 Compressed SEC1 Non-Residue Boundary Oracle

The shared SEC1 byte-level model already handled compressed encodings, but the
fixed API corpus only forced the independently checked uncompressed 7G vector.
That left the compressed square-root success and failure transitions dependent
on incidental fuzzing. The gated seed
`api_roundtrip/compressed-pubkey-parse-boundaries` now supplies both parity
encodings of the valid 7G x-coordinate and both parity encodings of `x = 5`.
For the latter, `x^3 + 7 = 132` is a non-residue modulo the field prime. Each
valid encoding must serialize back byte-for-byte; each invalid encoding must
return 0, leave no illegal-callback event, and clear the opaque public-key
output. The expected decision comes from the standalone byte-level parser,
not from the production field or square-root implementation.

This is **Informational / Low master-relative parser-oracle hardening**, not a
clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already rejects the non-residue and
preserves both valid parity encodings. The existing generic public-key
round-trip checks already detect an ordinary parity-selection regression; this
seed adds the distinct non-residue failure state and makes both compressed
branches explicit. No forgery, disclosure, availability, or nonce-secrecy
impact is claimed.

For causal proof, the first temporary mutation forced every compressed key to
use the even root and immediately failed old generated-key round trips; that
control demonstrates the generic parity behavior was already covered and was
not counted as new evidence. The narrowed mutation changed only the
`secp256k1_eckey_pubkey_parse` compressed failure path for the `x` prefix and
trailer matching the fixed `x = 5` input, returning the generator instead of
rejecting the non-residue. All 39 pre-existing API inputs stayed green, while
the exact new seed exited with status 134 at the independent decision and
zero-output checks. Restoring the source made all 40 tracked inputs pass.
Fresh coverage recorded both `secp256k1_ge_set_xo_var` success and failure
outcomes at `src/eckey_impl.h:21`.

Clang ASan/UBSan deterministic replays passed all 40 API seeds plus an empty
input on both native 5x52 and forced-int64/10x26 builds. Two-worker,
two-job libFuzzer replays over independent 40-file corpora exited 0 for every
manager and worker on both backends; each backend completed 101 and 102 runs
per job with no sanitizer, assertion, timeout, OOM, or crash artifact. This
proves a previously unforced compressed failure transition, not a new defect
on clean master.

## 2026-07-15 Scratch Invalid-Checkpoint State Oracle

`fuzz_ecmult_multi` already exercised an invalid scratch object whose allocation
cursor exceeded its capacity, but it did not pass an invalid checkpoint to a
valid scratch object. The gated corpus input
`ecmult_multi/scratch-invalid-checkpoint-boundaries` now allocates 16 bytes,
attempts checkpoints at `alloc_size + 1` and `SIZE_MAX`, and independently
checks that each call reports one error, leaves `alloc_size` unchanged, and
does not alter the allocated bytes. It then applies the valid zero checkpoint
and checks that destruction remains quiet. This reaches the distinct
`checkpoint > scratch->alloc_size` branch in `src/scratch_impl.h:62` and keeps
the existing invalid-object oracle separate.

This is **Informational / Low master-relative scratch-state oracle hardening**,
not a clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` contains the same callback-and-
preserve contract. The state is internal allocation bookkeeping and has no
cryptographic or nonce-secrecy meaning; no Critical severity is claimed.

For causal proof, a temporary production mutation replaced the guard with
`if (0)`. All 16 pre-existing `ecmult_multi` corpus inputs remained green,
while the exact new seed aborted with exit 134 on the cursor-preservation
assertion. Restoring the guard made all 17 tracked inputs pass. Clean coverage
reached the new helper once and the production invalid-checkpoint callback
branch twice at `src/scratch_impl.h:62-65`.

Clang ASan/UBSan deterministic replays passed all 17 inputs on native 5x52 and
forced-int64/10x26 builds. Isolated libFuzzer replays used
`-workers=2 -jobs=2 -max_total_time=20` over copied corpora. Native jobs
completed 91 and 96 runs; forced-int64 jobs completed 50 and 54 runs. Every
manager and worker exited 0 with no sanitizer diagnostic, assertion, timeout,
OOM, or crash artifact. This commit adds no production behavior change and
does not alter any existing master-relative severity rating.

## 2026-07-15 Scratch Allocation-Overflow Boundary Oracle

The scratch harness previously covered invalid scratch metadata and ordinary
capacity exhaustion, but it did not exercise the two overflow guards on a
valid scratch object. The gated input
`ecmult_multi/scratch-allocation-overflow-boundaries` now checks
`secp256k1_scratch_max_allocation` with
`SIZE_MAX / (ALIGNMENT - 1) + 1` and `secp256k1_scratch_alloc` with
`SIZE_MAX`. Both calls must return failure without invoking the error callback,
moving the allocation cursor, or changing an existing allocation. The helper
then rolls back and destroys the scratch object normally.

This is **Informational / Low master-relative scratch-overflow oracle
hardening**, not a clean-master production vulnerability. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already contains both guards at
`src/scratch_impl.h:74-90`. The state is internal allocation bookkeeping with
no cryptographic or nonce-secrecy meaning; no Critical severity is claimed.

For causal proof, replacing only the alignment-product guard with `if (0)`
left all 17 pre-existing `ecmult_multi` inputs green and made the new seed
abort with exit 134. Restoring it and replacing only the `ROUND_TO_ALIGN`
wraparound guard with `if (0)` produced the same result. Restoring both guards
made all 18 tracked inputs pass. Clean coverage reached each production guard
once and the gated helper once; the two mutation controls show that the
assertions are independent rather than duplicate capacity checks.

Clang ASan/UBSan deterministic replays passed all 18 inputs on native 5x52 and
forced-int64/10x26 builds. Isolated libFuzzer replays used
`-workers=2 -jobs=2 -max_total_time=20` over copied corpora. Native jobs
completed 86 and 87 runs; forced-int64 jobs completed 43 and 45 runs. Every
manager and worker exited 0 with no sanitizer diagnostic, assertion, timeout,
OOM, or crash artifact. This commit adds no production behavior change and
does not alter any existing master-relative severity rating.

## 2026-07-15 Tweak Input/Output Overlap

On clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`,
`secp256k1_ec_pubkey_tweak_add` and
`secp256k1_keypair_xonly_tweak_add` loaded their in/out object, cleared it,
and only then read `tweak32`. The public declarations describe an in/out
public key or keypair and a 32-byte tweak, but do not impose a non-overlap
precondition. A caller can therefore place a valid tweak inside the object
being updated. With secret key one, `pubkey.data[0..31]` is a valid scalar
encoding of the generated public-key X coordinate, and `keypair.data[0..31]`
is a valid secret scalar. Clean master returns success for both overlapping
calls, but the clear makes each helper consume zero and leaves the original
key unchanged instead of applying the nonzero tweak.

This is **Low master-relative API aliasing/state correctness**. It requires a
deliberate overlap by a caller and provides no memory corruption, disclosure,
forgery, or key-compromise primitive. The severity is therefore below the
existing output-state findings, even though the silent successful result can
violate a caller's key-transition contract.

The deterministic proof uses a copied tweak as the reference and compares it
with the same valid bytes supplied from the output object. A sanitized clean-
master standalone replay printed
`pubkey_tweak_alias=0 keypair_tweak_alias=0` and exited 1; the fixed source
printed both values as 1 and exited 0. For causal proof, only the two
production clear-before-helper
orderings were restored to their clean-master form. All 12 pre-existing
`xonly_tweak` inputs stayed green, while the exact new
`tweak-input-output-overlap` seed aborted with status 134. Restoring both
orderings made all 13 inputs pass. The fixed Clang ASan/UBSan replay passed
the seed and complete corpus, `tests -t=ec`, and `tests -t=extrakeys`; a
two-worker/two-job replay completed 22 and 25 runs with both jobs exiting 0
and no sanitizer, assertion, timeout, OOM, or crash artifact. Generated
libFuzzer corpus extensions were discarded. This proves a clean-master
production behavior bug, not merely a stronger fuzzer oracle.

## 2026-07-15 Complete Native Multi-Worker Sanitizer Campaign

The rebased audit tree at `cda68e3d0a865cf6a7fb5453cb51d9030278afaa` was
built with Clang 22.1.7, ASan/UBSan, `VERIFY`, all six optional modules, and
recovery enabled. Each of the 14 tracked corpora was copied to a disposable
directory and run with `-workers=2 -jobs=2 -max_total_time=8 -timeout=20`.
The loaded seed counts were: `api_roundtrip` 40, `context` 10, `hash` 9,
`scalar` 4, `field` 17, `group` 19, `ecmult_const` 5, `ecmult_multi` 18,
`ecdh` 6, `ellswift` 14, `xonly_tweak` 12, `recovery` 10, `schnorrsig`
13, and `musig` 63.

Both jobs for every target exited 0. The jobs completed 25-12,579 runs per
worker; the state-heavy MuSig workers completed 64 corpus/fuzz runs each in
76 seconds, and the other targets completed in 9-10 seconds each. No ASan or
UBSan diagnostic, assertion failure, timeout, OOM, or crash artifact was
produced. LibFuzzer-generated corpus extensions were discarded after review.
The same build also passed all 224 CTest cases, including both verify modes
and every enabled module test.

This campaign found no new oracle gap and no new clean-master production
finding. It is negative evidence for the current branch only; it does not
weaken the clean-master findings and severities already recorded above, and
no nonce-cleanup severity is inferred from this run.

## 2026-07-15 Follow-up Alias Boundary and Corpus Replay

The follow-up Clang 22.1.7 ASan/UBSan libFuzzer build replayed the current
rebased tree, including the new `tweak-input-output-overlap` seed, with
`-jobs=2 -workers=2 -runs=1` for every enabled target. The two workers loaded
the complete tracked corpora and completed these runs per job:

    api_roundtrip 41/41       context 11/11       hash 10/10
    scalar 5/5                field 18/18         group 20/20
    ecmult_const 6/6          ecmult_multi 19/19  ecdh 7/7
    ellswift 15/15       xonly_tweak 14/14  recovery 11/11
    schnorrsig 14/14           musig 64/64

Every manager and worker exited 0. There was no ASan/UBSan diagnostic,
assertion failure, timeout, OOM, nonzero worker result, or crash artifact.
The replay did not expose a new oracle gap or a new clean-master production
finding.

The same configured Clang build completed all 224 CTest cases, covering both
`noverify_tests` and `VERIFY` `tests` modes plus every enabled module. The
CTest log ended normally with no failed-test log; the expensive internal
targets were allowed to complete rather than being replaced by a focused
subset.

The same review deliberately excludes three tempting aliases. The headers
describe `xonly_pubkey_tweak_add` and both MuSig public-key tweak wrappers as
separate `Out output_pubkey` and `In` inputs, and describe
`ec_pubkey_combine` as separate `Out` and `In` roles. They do not promise
overlap, so requiring those undocumented aliases would turn implementation
behavior into a false-positive oracle. MuSig's `In/Out keyagg_cache` is loaded
before its final save, so a tweak pointer into that cache is not the same
clear-before-read defect. These cases remain documented no-edits rather than
additional production changes. Public nonce state without cryptographic
meaning remains non-critical.

## 2026-07-15 Coverage and Static Audit Recheck

After the preceding replay, `origin/master` was fetched again and remained
`ebf594320dc838b9de1abb54d5ba98cef84f4297`. The l0rinc remote was also
refreshed. The newly visible `boundary-condition-bugs` head `65d38b0` still
contains the `fe_equal` bound and 10x26 normalization repairs already
represented by `994b350` and `cf5631f`; the
`musig-clear-invalid-seckey-pubnonce` head `7ed2abc` still contains the public
nonce cleanup already represented by `b4de762`. Their parent patches
(`161a39a` and `fde940f`) were compared directly. No exact cherry-pick is
needed: applying them would duplicate behavior already present here and would
discard the stronger master-relative mutation and fuzzer evidence carried by
the audit commits. The public nonce has no cryptographic meaning, so its
cleanup remains Low/informational stale-state hygiene rather than Critical
secret erasure.

A disposable Clang profile build replayed every tracked corpus once with
`-runs=1`. All 14 targets loaded their complete current seed sets and exited
0: `api_roundtrip` 41, `context` 11, `hash` 10, `scalar` 5, `field` 18,
`group` 20, `ecmult_const` 6, `ecmult_multi` 19, `ecdh` 7, `ellswift` 15,
`xonly_tweak` 14, `recovery` 11, `schnorrsig` 14, and `musig` 64. The
profile report was inspected per target because LLVM 22 hangs when one report
combines raw profiles from different fuzz executables. No replay diagnostic,
assertion, timeout, or sanitizer-like failure occurred.

A separate production-only `scan-build-22 --status-bugs --keep-going` build
covered the core library and all six enabled modules with no analyzer reports.
The remaining zero-coverage regions are known boundaries rather than new
findings: native builds do not execute the alternate 10x26 scalar/field
serialization branches, the cofactor-one curve has no valid non-subgroup
public-key input, and proper-context guards require an invalid opaque context
outside the API domain. The forced-int64 campaigns and invalid-state barriers
already cover the meaningful counterparts. This pass therefore adds no new
oracle or production fix and changes no severity rating; all findings remain
rated against clean master before any fork patch or later repair.

## 2026-07-16 Post-Rebase Stateful Sanitizer Recheck

The audit branch was fetched and explicitly rebased onto unchanged
`origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297`. The l0rinc refs
were refreshed before this run; the relevant PR tips remain represented by
existing commits in this branch, and no fork-only optimization or repair was
used to mask a master-relative result.

A fresh Clang 22 ASan/UBSan libFuzzer build enabled all six optional modules
and built all 14 fuzz targets. Four state-heavy targets then replayed their
complete tracked corpora in isolated directories with two workers and two
independent job managers per target, using a 45-second bounded campaign and a
30-second per-input timeout:

    api_roundtrip 460/457   ecmult_multi 176/181
    ellswift       265/268  musig        66/66

Every manager and worker exited 0. The logs contained no ASan/UBSan
diagnostic, assertion failure, timeout, OOM, or crash report, and all artifact
directories remained empty. This is negative regression evidence for the
current oracle set, not a claim that clean master is safe and not a mutation
proof for a new finding. No production behavior, severity rating, or existing
master-relative finding is changed. Public nonce state without cryptographic
meaning remains non-critical.

## 2026-07-16 MuSig Invalid-SecnNonce Cleanup Recheck

The suspected `secp256k1_musig_partial_sign` cleanup path was reviewed against
the actual control flow. An invalid secnonce magic or point makes
`secp256k1_musig_secnonce_load` return before either secret scalar is read;
`partial_sign` then zeroes the caller's secnonce and calls
`secp256k1_scalar_clear` on the local scalar storage. The clear helper is an
explicit write-only memory wipe, so this is not an uninitialized scalar read or
an observable state transition defect. The public nonce buffer has no
cryptographic meaning and is not part of this severity assessment.

A fresh Clang 22 MemorySanitizer build with origin tracking and the forced
`int64`/10x26 backend replayed all 63 tracked MuSig inputs plus the empty input
(`64` executions) with `-runs=1`. It exited 0 after 60 seconds with no MSan,
UBSan, assertion, timeout, or artifact. The existing invalid-magic,
invalid-point, overflowing-scalar, failure-cleanup, and nonce-reuse oracles
therefore remain the stronger evidence; this review adds no production fix,
new oracle, clean-master finding, or severity change.

## 2026-07-16 Release-Mode MuSig/Extrakeys Recheck

The audit tree was also built as a disposable CMake `RelWithDebInfo` release
library with all enabled modules and `SECP256K1_BUILD_FUZZ=OFF`. The
`noverify_tests` target therefore exercised the production library without the
`VERIFY` definition. The focused `musig` and `extrakeys` suites passed, then
the complete no-`VERIFY` test dispatch passed at the normal 16 iterations with
two worker processes. This includes the opaque cache, keypair, nonce, and
session-state barriers reviewed above.

This is negative release-mode evidence for the rebased audit tree, not a claim
that assertions are a substitute for production validation and not a clean-
master mutation proof. It found no release-only transition bug, adds no
production change or fuzzer oracle, and does not change any master-relative
finding or severity. In particular, public nonce cleanup is not treated as
cryptographic secret erasure.

## 2026-07-16 Complete In/Out Tweak Alias Oracle

The existing `tweak-input-output-overlap` seed already proves the clean-master
Low aliasing finding for `secp256k1_ec_pubkey_tweak_add` and
`secp256k1_keypair_xonly_tweak_add`. It now also supplies valid overlapping
cases for `secp256k1_ec_pubkey_tweak_mul` (`tweak32 == pubkey.data`) and both
secret-key in/out helpers (`tweak32 == seckey`). The declarations expose an
In/Out object plus an In tweak and do not impose a non-overlap precondition, so
the fixed implementations must snapshot all tweak bytes before writing the
object.

These three additions are negative regression oracles, not new clean-master
findings. The current source passed the exact trigger and all 13 tracked
`xonly_tweak` inputs under Clang 22 ASan/UBSan with two jobs and two workers.
They do not change the existing Low severity rating, which remains limited to
the two clean-master add-path findings.

The trigger now also checks shifted 32-byte windows at 16-byte offsets inside
the 64-byte public-key object and the 96-byte keypair object. Each window is
copied into an independent reference call, then supplied directly from the
in/out object for the comparison call; non-scalar windows are skipped rather
than being treated as valid API inputs. This closes the oracle gap where an
implementation could snapshot only an object-base alias while still clearing
a valid shifted overlap. It remains negative oracle evidence: no new
clean-master defect or severity change is claimed.

Causal proof used three disposable production mutations. Moving factor parsing
in `secp256k1_ec_pubkey_tweak_mul` below its output `memset`, inserting
`memset(seckey, 0, 32)` before `ec_seckey_tweak_add_helper`, and inserting the
same read-after-clear mutation before factor parsing in
`ec_seckey_tweak_mul` each made `tweak-input-output-overlap` terminate with
`ERROR: libFuzzer: deadly signal`. Restoring the source made the exact seed and
the two-worker corpus replay pass. No mutation was committed, no production
behavior changed, and no new severity is claimed.

## 2026-07-16 MuSig Cache/Tweak Alias Oracle

The MuSig target now has a gated 32-byte seed,
`tweak-input-cache-overlap`. It first applies an independent `+1` tweak so the
cache's serialized accumulated tweak is nonzero, then compares both
`secp256k1_musig_pubkey_ec_tweak_add` and
`secp256k1_musig_pubkey_xonly_tweak_add` with an independent tweak copy versus
the same 32-byte field through `keyagg_cache->data`. The output public key,
return value, and complete updated cache must agree. This is a distinct
In/Out-cache plus In-tweak boundary; it does not broaden the deliberately
excluded pure Out-plus-In aliases such as `ec_pubkey_combine`.

This is **Informational / Low master-relative oracle hardening**, not a new
clean-master production bug. The public header labels `keyagg_cache` In/Out and
does not impose a separate non-overlap precondition. Clean master already
loads the complete cache before parsing the tweak and saves it only afterward,
so the current implementation passes the alias. No production fix or severity
change is claimed; the existing Low clean-master alias finding remains limited
to the two core add paths. Public nonce cleanup is unrelated and remains
non-critical when the nonce carries no cryptographic meaning.

For causal proof, a disposable production mutation cleared the cache after
`secp256k1_keyagg_cache_load` but only when `tweak32` was the exact serialized
tweak address. All 63 pre-existing MuSig inputs stayed green and exited 0;
the exact new seed aborted with `ERROR: libFuzzer: deadly signal` and exit 134.
After restoring the source, the seed exited 0, two independent jobs with two
workers replayed all 64 tracked inputs (65 executions per job including the
empty input) with no ASan/UBSan diagnostic or artifact, and sanitized `tests`
and `noverify_tests` passed their `musig` and `extrakeys` suites. The mutation
was never committed.

## 2026-07-16 ECDSA Compact Serializer NULL-Output Oracle

The API target now has a gated `ecdsa-compact-null-output` seed. It calls
`secp256k1_ecdsa_signature_serialize_compact` through a function pointer with
an initialized context illegal callback, a valid 64-byte output buffer
prefilled with `0xA5`, and a NULL signature input. The call must return zero,
invoke the illegal callback exactly once, and clear all 64 output bytes.

This reiterates the existing **Low to Medium master-relative API-state
finding** fixed by `a5aa4ce`: clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` rejects the NULL signature after
checking the output pointer but leaves a caller's previous compact signature
in place. The unit suite already covered the behavior, but the API fuzzer did
not reach the serializer's `sig == NULL` branch; its opaque-signature and DER
output checks therefore could not independently detect this stale-output
transition. This is fail-closed state correctness, not memory corruption,
signature forgery, disclosure, or nonce-cleanup severity.

For causal proof, a disposable production mutation removed only the
`if (sig == NULL) memset(output64, 0, 64)` block from
`src/secp256k1.c`. All pre-existing API corpus inputs stayed green, while the
new seed stopped at the exact 64-byte zero assertion with exit 134. Disabling
only the new helper made the same mutation pass; restoring the cleanup made
the focused seed and the complete API corpus pass. The fixed Clang ASan/UBSan
replay, two-worker replay, and the deterministic ECDSA unit suite passed with
no sanitizer diagnostic, assertion, timeout, or artifact. The production
mutation was never committed, and the existing master-relative severity is
unchanged.

## 2026-07-16 ecmult_multi Pippenger Window-9 Boundary Oracle

The `ecmult_multi` target now has a gated `pippenger-window-1261` seed. It
constructs 1,261 distinct generator-derived points and nonzero scalar terms
on the heap, uses a generator scalar of 17, and forces the dispatcher to use a
single Pippenger batch. The point result is checked against an independent
model that calls `secp256k1_ecmult_const` for every term and compares
serialized affine coordinates. The callback transcript requires every input
index exactly once, the scratch checkpoint must be restored, and a second run
rejecting the final callback must return an infinity result rather than expose
a partial sum.

This is **Informational / Low internal-oracle hardening**, not a new
clean-master production finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` intentionally jumps from bucket
window 7 at 1,260 points to window 9 at 1,261 points; window 8 is not used
with the endomorphism. The earlier fuzzer transcripts reached at most 264
points, and the unit batching test reached only two Pippenger thresholds.
The selector's inverse-boundary test therefore did not execute the high-window
arithmetic, bucket allocation, or callback rollback at the first window-9
point count. No public nonce or cryptographic secret-erasure severity is
assigned here.

For causal proof, a disposable mutation in `src/ecmult_impl.h` skipped bucket
index 1 from the running sum only when `bucket_window == 9`. All 18
pre-existing `ecmult_multi` corpus files remained green; the exact new seed
aborted with `-handle_abrt=0` and exit 134. Disabling only the new helper made
that same mutated seed pass with exit 0. Restoring the loop and the helper
produced a clean exit for the new seed and all 19 tracked corpus files. The
mutation was never committed, so this proves the fixture detects a high-window
regression without claiming that clean master is currently defective.

The restored Clang 22 ASan/UBSan builds passed the deterministic seed and all
19 tracked corpus files on both the default backend and forced-int64/10x26;
each fixed corpus replay completed 20 executions including the empty input.
Isolated two-worker/two-job campaigns completed 78 executions per job on each
backend with every manager and worker exiting 0. No sanitizer diagnostic,
assertion failure, timeout, OOM, or crash artifact occurred. This commit
changes only the fuzzer, its corpus, and this evidence ledger.

## 2026-07-16 SHA256 Buffered-Block Cleanup Oracle

The `hash` target now has a gated `sha256-write-buffer-clear` seed. It writes
one byte and then the remaining 63 bytes of a fixed 64-byte message, forcing
`secp256k1_sha256_write` to consume a previously buffered block. Before
finalization, the oracle requires the reusable `hash->buf` storage to be zero
and independently checks the final digest against standalone SHA256. The
postcondition is about the consumed block, not about clearing the live hash
state before its caller finishes using it.

This is a **Medium master-relative memory-hygiene finding**, extending the
existing HMAC/RFC6979 secret-state retention finding. Clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297` documents this path as wiping the
buffer but leaves the consumed 64-byte block in place after compression. A
buffered HMAC/RFC6979 block can contain key- or nonce-derived material while
the computation continues. There is no standalone memory-read primitive, so
this is not rated as a disclosure or Critical cryptographic compromise; the
rating matches the existing secret-state lifetime issue. It is distinct from
public nonce cleanup, which carries no cryptographic meaning here and remains
non-critical.

The old hash corpus and hash unit tests compare digests and only inspect
state after an explicit full-object clear, so they did not observe this
intermediate state. For causal proof, removing only the new
`secp256k1_memclear_explicit(hash->buf, sizeof(hash->buf))` made all nine
pre-existing hash corpus inputs pass, while the exact new seed aborted with
standalone ASan/UBSan exit 134. Restoring the wipe made the seed, all ten
tracked inputs, and `tests -t=hash` pass. The digest assertion prevents the
oracle from being reduced to a cleanup-only check that could miss a corrupted
state transition.

The fixed Clang 22 ASan/UBSan `SECP256K1_ASM=OFF` build replayed all ten
tracked hash inputs and the hash unit suite without a diagnostic. A separate
Clang 22 ASan/UBSan `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` build also
replayed all ten inputs without a diagnostic. A separate
libFuzzer build replayed the seed and completed an isolated two-worker/two-job
campaign on a disposable corpus copy: job 0 completed 1,323 executions and
job 1 completed 1,337, both with exit 0 and no assertion, sanitizer
diagnostic, timeout, OOM, or artifact. The production wipe is the only
non-fuzzer behavior change in this finding; no unrelated optimization-stack
commit was cherry-picked over it.

## 2026-07-16 Full Isolated Multi-Worker Recheck

The current branch was already based on clean `origin/master`
`ebf594320dc838b9de1abb54d5ba98cef84f4297`, so no rebase was required. A
fresh Clang 22.1.7 `RelWithDebInfo` build used ASan, UBSan, no assembly, all
optional modules, and the libFuzzer runtime. Every target was run with two
jobs and two workers, `-max_total_time=20`, `-timeout=10`, and
`-rss_limit_mb=2048`. Each target received a copied corpus in a private
temporary directory; this matters because libFuzzer writes newly discovered
inputs into a directory passed as a corpus argument.

The fixed build replayed `api_roundtrip`, `context`, `ecdh`, `ecmult_const`,
`ecmult_multi`, `ellswift`, `field`, `group`, `hash`, `musig`, `recovery`,
`scalar`, `schnorrsig`, and `xonly_tweak`. Both workers for every target
exited 0. The arithmetic and API workers completed 54--17,393 executions
each; the stateful MuSig and Schnorr workers completed 65 and 35 executions
each because their protocol seeds are deliberately expensive. There was no
sanitizer diagnostic, assertion failure, timeout, OOM, or crash artifact.

This is negative verification evidence, not a new clean-master finding and
does not reduce the severity of any existing finding. The source review also
rejected undocumented output/input overlap, public MuSig nonce clearing, and
internal helper failure outputs as findings because the public contracts do
not promise those behaviors. In particular, a nonce with no cryptographic
meaning is not a Critical cleanup issue. Existing master-relative findings
remain rated against clean master before later audit fixes or l0rinc fork
patches; no fork optimization commit was applied over this recheck.

## 2026-07-16 l0rinc PR #15 Duplicate Reconciliation

The complete public pull-head scan found l0rinc PR #15 at `a2a0ac2`, in
addition to PRs #1--#14 already recorded above. PR #15 moves the clearing of
the in/out keypair in `secp256k1_keypair_xonly_tweak_add` until after both
helpers consume `tweak32`, and adds a copied-versus-aliased regression. This
is the keypair half of the existing `ba8d379` production fix, not a new
behavioral change.

The branch already contains stronger evidence: `ba8d379` covers both the
core public-key and extrakeys keypair paths, records **Low** severity against
clean `origin/master` `ebf594320dc838b9de1abb54d5ba98cef84f4297`, and includes
the clean-master reproducer, exact production mutation, deterministic tests,
gated corpus seed, sanitized replay, and multi-worker verification. The
current xonly_tweak oracle also covers the complete in/out alias surface,
including shifted valid windows. PR #15's production patch therefore has
already been applied and its test does not add independent proof.

PR #15 was deliberately not cherry-picked: doing so would duplicate the
production hunk and weaken the master-relative audit history by presenting a
follow-up fork commit as a second fix. No commit needed amendment because the
existing `ba8d379` message already states the affected API, clean-master
failure, severity, proof, and boundary decision. The other fork heads remain
reconciled in the earlier ledger; no optimization or later fix was applied
over the master-relative alias finding.

## 2026-07-16 Unsupported X-Only Output/Tweak Alias Retraction

The audit briefly tested the exact 33-byte trigger whose text is
`xonly tweak input-output overlap` followed by a line-feed, placing a valid
tweak inside `output_pubkey.data` for
`secp256k1_xonly_pubkey_tweak_add`. Clean master clears that Out-only object
before reading the In-only `tweak32`, so the trigger reliably demonstrates a
different result for an undocumented alias.

That behavior is deliberately not a master-relative finding. The public
header documents `output_pubkey` as `Out` and both `internal_pubkey` and
`tweak32` as `In`; unlike the core pubkey and keypair tweak APIs, it does not
promise an In/Out object or aliasing. The earlier `ba8d379` boundary decision
therefore excludes this path, along with the pure Out-plus-In MuSig tweak
wrappers and `ec_pubkey_combine`. `c7ee9d4` reverted the temporary production
change, unit test, fuzzer oracle, corpus input, and severity entry. The
supported Low core/keypair alias findings and the MuSig keyagg-cache/tweak
oracle are unchanged.

## 2026-07-16 MuSig Session-Random Input Alias Oracle

The public MuSig declaration gives `session_secrand32` an explicit `In/Out`
role: it is consumed as nonce-derivation input and must be invalidated after a
successful call. The optional `seckey`, `msg32`, and `extra_input32` arguments
are 32-byte `In` inputs, and the header does not impose a non-overlap
precondition. The new gated
`session-random-input-overlap` seed therefore exercises one exact alias for
each of those three inputs.

Each case snapshots the pre-call bytes, computes the nonce transcript with
the independent MuSig reference, invokes `secp256k1_musig_nonce_gen` with the
shared pointer, and checks the return value, both secret nonce scalars, public
nonce serialization, and the required zeroing of the shared session-random
buffer. The oracle intentionally does not require an aliased `const` input
view to remain readable after the documented `In/Out` invalidation; that would
test a different and unsupported preservation contract.

This is informational/negative oracle hardening, not a clean-master finding.
The restored Clang 22.1.7 ASan/UBSan build passed the focused seed and all 65
tracked MuSig files (66 executions including the empty input). For causal
proof, a disposable production mutation cleared `session_secrand32` before
nonce derivation whenever it exactly aliased `seckey`, `msg32`, or
`extra_input32`. The new seed aborted at the independent postcondition, while
all 64 pre-existing files remained green in a 65-execution replay. The
mutation was restored before the fixed replay. No production behavior or
severity rating changes; a public nonce without cryptographic meaning is not
a Critical cleanup issue.

## 2026-07-16 Callback and Output-Lifecycle Recheck

The context, ECDH, and recoverable-signature callback paths were re-reviewed
against their public contracts and replayed without finding a new clean-master
bug. Context cloning copies callback state by value and reset restores the
default backend; ECDH keeps callback coordinates in local storage and clears
its built-in output on callback failure; recovery clears malformed outputs and
rejects invalid recovered points. The existing oracles also cover callback
domain separation, failure cleanup, clone routing, and post-retry state.

The Clang 22.1.7 libFuzzer build replayed the complete context corpus (10
inputs), ECDH corpus (6), and recovery corpus (10) with `-workers=2 -jobs=2`;
both workers for every target exited 0. The deterministic `tests -t=ecdsa
-i=1` and `noverify_tests -t=ecdsa -i=1` runs also passed. This is negative
evidence only: no production patch or severity change is justified by this
pass, and pure undocumented output/input aliases remain outside the supported
oracle boundary.

## 2026-07-16 Custom SHA Secret-Operation Oracle

The context target now has a gated `sha256-secret-operations` seed. It keeps a
valid replacement SHA-256 compression callback installed while comparing
compressed public-key creation, deterministic ECDSA compact signatures, and
BIP340 Schnorr signatures with a fresh default context randomized from the same
seed. The callback counter must also increase during the secret-dependent
operation. The comparison is made through public encodings and verification,
not by treating opaque context or blinding state as portable.

The public callback contract requires the exact SHA-256 compression effect, so
secret-dependent API results must remain unchanged even when generator
blinding state is represented differently. The previous context oracle checked
tagged hashing and callback routing, but restored the default backend before
its ordinary signing checks; the unit probe intentionally uses a non-SHA
compression function and therefore cannot prove this equivalence.

This is informational negative oracle hardening, not a clean-master finding;
no severity is assigned. For causal proof, a disposable mutation in
`src/ecmult_gen_impl.h` added one to `scalar_offset` after custom-backend
randomization only when the derived seed began `6a 58 69 09`, the exact prefix
from the new corpus input. The new seed then stopped with libFuzzer's deadly
signal (exit 77), while all 10 pre-existing context inputs stayed green in
both two-worker jobs. Restoring the source made the focused seed and all 11
tracked inputs pass in both two-worker jobs (12 executions per job), and the
deterministic `tests -t=ecdsa -i=1` and `noverify_tests -t=ecdsa -i=1` runs
also passed. The mutation was never committed; no production behavior or
master-relative severity rating changed.

## 2026-07-16 Secret-Key Tweak Input/Output Alias Oracle

The API target now has a gated `secret-tweak-input-output-overlap` seed. It
passes the same 32-byte storage as both the documented `In/Out` secret key and
the `In` tweak to `secp256k1_ec_seckey_tweak_add` and
`secp256k1_ec_seckey_tweak_mul`. Independent byte-level modular addition and
double-and-add multiplication determine the expected return values and output
bytes. This is the secret-key counterpart to the existing public-key and
keypair alias checks; the headers impose no non-overlap precondition.

Clean master currently passes both aliases because it parses the scalar inputs
before writing the `In/Out` buffer. This is informational negative oracle
hardening, not a clean-master production finding, so no severity is assigned.
For causal proof, two disposable mutations were tested separately: each
cleared `seckey` when `tweak32 == seckey` before operand parsing, once in the
add wrapper and once in the multiply wrapper. The focused seed aborted under
each mutation while all 41 pre-existing `api_roundtrip` inputs remained green;
restoring the source made the focused seed and the complete 42-input corpus
pass. The fixed Clang 22.1.7 ASan/UBSan native build and the assembly-off
forced-int64 build each passed both two-worker jobs (43 executions per job,
including the empty input), the focused seed, and the deterministic `ec` and
`ecdsa` suites. The mutations were never committed, and no production
behavior or master-relative severity rating changed.

## 2026-07-16 Pippenger Boundary Replay Budget Clarification

The `pippenger-window-1261` fixture is intentionally expensive: it builds
1,261 points and performs two complete independent 1,261-term scalar-sum
models. On this host, an initial all-target run used `-workers=2 -jobs=2
-max_total_time=20 -timeout=10`; both `ecmult_multi` worker logs timed out
while executing `ecmult_multi.c:735`, the exact `pippenger-window-1261` seed.
The timeout was caused by concurrent sanitizer workers sharing the CPU, not by
an infinite loop, memory error, or oracle failure. The isolated seed completed
successfully in 4,381 ms, and a corrected two-worker/two-job replay with
`-max_total_time=30 -timeout=60` completed 118 and 124 executions with both
jobs exiting 0 and no sanitizer diagnostic, assertion, timeout, OOM, or
artifact. The deterministic ECDSA and MuSig suites also passed in both verify
modes.

This is a campaign-budget correction only. It is not a clean-master
production finding, does not change the Informational/Low oracle-hardening
rating of `5e5491e`, and does not justify treating the internal Pippenger
boundary as a denial-of-service bug. Future multi-worker replays of this target
must give this gated fixture a per-input timeout of at least 60 seconds.

## 2026-07-16 Public API Contract Audit Negative Pass

The audit was repeated against clean-master baseline `ebf594320dc838b9de1abb54d5ba98cef84f4297`.
The source review covered the remaining public stateful surfaces in
`src/modules/ellswift/main_impl.h`, `src/secp256k1.c`, ECDH, recovery,
extrakeys, Schnorr, and MuSig key aggregation/session loading. EllSwift
already models canonical aliases, inverse branches, both party selections,
invalid secrets, callback transcripts, cleanup, and BIP324's ignored data.
Context reset, callback routing, recovery output cleanup, opaque loaders, and
MuSig cleanup likewise already have focused postconditions. The only direct
public entry point absent from the call inventory is `secp256k1_selftest`,
which context creation already executes; the other omissions are internal
helpers or undocumented pure output/input aliases. Adding assertions for
those cases would invent contracts rather than discover master bugs.

The existing master-relative dispositions remain: Medium for SHA state
retention (`55f98a8`), impossible SHA length handling (`ee2e591`), and the
10x26 field magnitude-32 normalization issue (`cf5631f`); Low for the
documented public/keypair tweak-input overlap fixes (`ba8d379` and
`c0c6948`); and Informational for the session-random, secret-key tweak, and
Pippenger oracle hardening. A public nonce without cryptographic meaning is
not a Critical cleanup finding. The existing README entries contain the
reproductions and mutation proofs; this pass found no severity downgrade or
hidden severe master bug.

The initial standalone Clang 22.1.7 ASan/UBSan build enabled all modules and
passed all 14 tracked fuzz suites. After reconfiguring the disposable build
with `SECP256K1_FUZZ_USE_LIBFUZZER=ON`, the deterministic commands
`tests -t=ecdsa -i=1`, `noverify_tests -t=ecdsa -i=1`,
`tests -t=musig -i=1`, and `noverify_tests -t=musig -i=1` also passed. A
copied set of 248 corpus files was then replayed with every target using
`-workers=2 -jobs=2 -max_total_time=15 -timeout=60`; all 14 targets exited
zero, including the expensive Pippenger boundary, with no sanitizer,
assertion, timeout, OOM, or artifact output. The disposable build used
the libFuzzer runtime; the earlier standalone runner's rejection of worker
flags was a harness-mode mismatch, not a fuzz result.

This is negative evidence only. No new seed, oracle, production patch, or
severity change is justified, and no production bug is claimed without a
reproduction on master or a minimal production mutation.

## 2026-07-16 Custom SHA Clone Secret-Operation Oracle

The existing gated `sha256-secret-operations` seed now exercises the valid
custom SHA-256 compression backend through the source context, a heap clone,
and a preallocated clone. The source and default reference contexts are
randomized from the same seed; the clones retain the state copied at creation,
as required for context cloning. Each context must produce the same compressed
public key and deterministic ECDSA compact signature as the default reference,
and each custom context must invoke the installed compression callback during
ECDSA signing. With the optional modules enabled, the same per-context checks
also cover BIP340 Schnorr signatures and callback use.

The earlier clone oracle checked illegal-callback inheritance and tagged-SHA
routing, while the existing custom-SHA seed checked secret operations only on
the source context. Those checks would not prove that generator blinding and
custom hashing remain coherent when secret APIs run through a cloned context.
This extension adds no undocumented alias or opaque-state comparison.

This is Informational oracle hardening, not a clean-master production finding;
the public clone and SHA callback contracts are already respected. For causal
proof, a disposable production mutation in
`secp256k1_context_preallocated_clone` added one to the cloned generator's
`scalar_offset` only when the source used a non-default SHA backend. The
callback pointer and tagged-SHA behavior remained intact: the focused existing
seed aborted with libFuzzer status 77, while all 10 other pre-existing context
corpus files exited 0 under the mutation. Restoring the source made the focused
seed pass and all 11 copied context inputs pass; no production mutation was
committed and no master-relative severity rating changed.

## 2026-07-16 Pippenger All-Filtered Identity Oracle

The `ecmult_multi` target now has a gated `pippenger-all-filtered` seed with
exactly `ECMULT_PIPPENGER_THRESHOLD` (88) callback entries. It runs the
Pippenger dispatcher twice: once with finite points and zero scalars, and once
with infinity points and nonzero scalars. Both cases must enumerate every
callback index, return success, restore the scratch checkpoint, and produce
the canonical all-zero Jacobian infinity representation. The generator term is
also absent in the first run and explicitly zero in the second, so the result
cannot depend on an untested generator-only contribution.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding. The existing random inputs can filter individual terms,
and the larger fixtures use nonzero finite terms, but coverage showed that the
Pippenger `no == 0` identity return had never executed. The public dispatcher
already initializes this result correctly; no production defect or severity
change is claimed.

For causal proof, a disposable mutation inserted `r->infinity = 0` in the
all-filtered branch immediately before its successful return. All 19
pre-existing `ecmult_multi` corpus files remained green, while the focused
seed recorded a noncanonical result for both filter configurations and exited
134 under `-handle_abrt=0` at the final identity assertion. Restoring the
source made the focused seed and the complete 20-file corpus pass. A separate
mutation that changed the internal return value was rejected as non-probative:
the dispatcher currently ignores that helper status. The accepted mutation
was never committed. This fixture complements, rather than duplicates, the
existing batch, callback, zero-scalar, and infinity-point checks by proving
their conjunction reaches the early identity state.

## 2026-07-16 MuSig Stateful Coverage Negative Pass

The current clean source was rebuilt with Clang 22.1.7 ASan/UBSan instrumentation
and the `fuzz_musig` target. All 65 existing MuSig corpus files passed the
standalone replay (`66` executions including the empty input), and two independent
libFuzzer jobs with two workers each also exited 0. Neither run produced an
assertion, sanitizer, timeout, OOM, or artifact diagnostic. The deterministic
`tests -t=musig -i=1` and `noverify_tests -t=musig -i=1` suites passed as well.

A separate coverage build replayed the same corpus as individual inputs. It
executed 97.06% of the fuzzer source, 99.41% of MuSig key aggregation, and
99.81% of MuSig session production code. The only missed production lines are
the callback-failure return in `keyagg_impl.h` (the current aggregation callback
cannot fail) and the subgroup rejection in `session_impl.h` (secp256k1 has
cofactor one). These are negative coverage explanations, not missing security
oracles; adding forced callers would invent unsupported domains.

The existing master-relative ratings therefore remain unchanged: Medium for
SHA state retention, impossible SHA length handling, and the 10x26 magnitude-32
normalization issue; Low for documented tweak-input overlap; and Informational
for callback/session-random, secret-key tweak, and Pippenger oracle hardening.
As recorded elsewhere, clearing a public nonce has no cryptographic meaning and
is not a Critical cleanup finding. No production bug, severity change, or new
MuSig fixture is claimed without a master reproduction or a minimal mutation.

## 2026-07-16 Pippenger Window-10 Boundary Oracle

The `ecmult_multi` target now has a gated `pippenger-window-4421` seed. It
constructs 4,421 repeated generator terms, forcing the first Pippenger batch
with `bucket_window == 10`; the adjacent 4,420-point selector remains window
9, and the window-10 inverse boundary is checked as 7,880. Each callback index
must be visited exactly once, scratch allocation must return to its checkpoint,
and a callback failure at the final index must return failure with a canonical
infinity output after visiting the complete prefix.

The result oracle is independent of the Pippenger implementation: every term
uses the same scalar `2^60 + 1` and point G, the generator term is 17G, and the
expected point is computed as one constant-time multiplication by
`4,421 * (2^60 + 1) + 17` modulo the group order. The actual and expected
points are compared through serialized affine coordinates as well as the
existing Jacobian equality helper. The high scalar component is deliberate: an
initial scalar-1 draft was rejected because all nonzero work stayed in the
lowest window while the accumulator was infinity, so a skipped high-window
doubling did not change the result.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding. The previous 20-file corpus did not exercise the first
window-10 batch, so normal build and test coverage could not distinguish a
window-10 arithmetic regression from an untested path. For causal proof, a
disposable mutation changed the production loop in `src/ecmult_impl.h` from
`j < bucket_window` to
`j < bucket_window - (bucket_window == 10)`. The old 20-file corpus stayed
green; the exact new seed failed at the independent result assertion with
libFuzzer status 77 and `-handle_abrt=1`. Bypassing only the new helper made
the same mutated seed pass with status 0. Restoring both source and helper
made the focused seed and all 21 tracked inputs pass. The mutation was never
committed, and no master-relative severity change or production fix is claimed.

The fixed Clang 22.1.7 ASan/UBSan libFuzzer build passed the focused seed, the
complete 21-input corpus, and two independent workers in two job managers with
`-max_total_time=45 -timeout=60`; both managers exited 0 with no sanitizer,
assertion, timeout, OOM, or crash artifact. The deterministic
`tests -t=ec -i=1` and `noverify_tests -t=ec -i=1` suites also passed. The
verification build used `SECP256K1_ASM=OFF` and all optional modules. Existing
ratings remain: Medium for the SHA and 10x26 production findings, Low for
documented tweak-input overlap and related state correctness, and
Informational for oracle-only hardening. Public nonce cleanup remains
non-Critical because the nonce carries no cryptographic meaning.

## 2026-07-16 ECDSA DER Parser Boundary Oracle

The `api_roundtrip` target now has a gated `ecdsa-der-parser-boundaries` seed
and an extension of its existing DER round-trip oracle. The focused table
exercises an absent length octet, forbidden `0xFF` and indefinite lengths,
truncated and over-wide long-form lengths, long-form lengths exceeding the
input, non-shortest encodings, an invalid integer tag, and excessive `0xFF`
integer padding. Every malformed input must return failure and zero the
public signature object. A positive case with `r = 0x80` checks that one
required leading zero is accepted and that compact serialization returns
exactly `r = 0x80, s = 1`.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding or a production fix. The existing DER checks already
covered ordinary round trips, trailing data, zero padding, short long-form
lengths, empty integers, and negative values, but coverage showed that these
specific `der_read_len` rejection paths and the invalid integer tag path were
not reached. The coverage replay reached 100% of the `ecdsa_impl.h` branches
and 95.56% of its lines; the only remaining non-executed parser line is the
successful long-form return, which requires an encoded sequence of at least
128 bytes and is outside ordinary ECDSA signature encoding.

For causal proof, a disposable production mutation changed only the
excessive-`0xFF` guard in `src/ecdsa_impl.h` to `if (0)`. All 42 pre-existing
`api_roundtrip` corpus files stayed green, while the exact new seed aborted
at the strict rejection assertion with status 134. Disabling only the new
helper call made that mutated seed pass with status 0. Restoring the guard
and helper made the focused seed, all 43 API corpus files, and empty input
pass. The mutation was never committed. The failure models acceptance of a
noncanonical signature integer that the master parser must reject; it is not
claimed as a master vulnerability.

Verification used the Clang 22.1.7 ASan/UBSan libFuzzer build with every
optional module enabled and `SECP256K1_ASM=OFF`: all 14 targets passed empty
input and all 251 tracked corpus files in the non-libFuzzer replay; the new
seed passed directly; and two job managers with two workers each completed
479 API fuzzing runs in 46 seconds with no sanitizer, assertion, timeout, OOM,
or crash artifact. The ASan/UBSan `tests -t=ec -i=1` and
`noverify_tests -t=ec -i=1` suites passed. Existing master-relative ratings
remain unchanged: Medium for the established SHA, impossible-length, and
10x26 findings; Low for documented state-correctness overlaps; and
Informational for oracle-only hardening. Clearing a public nonce remains
non-Critical because it carries no cryptographic meaning.

## 2026-07-16 ecmult_const Canonical Infinity Oracle

The internal `ecmult_const` target now has a gated
`ecmult-const-canonical-infinity` seed. It drives the infinity-base early
return with `q = 1`, prefills the output with a sentinel, and requires the
result to have the infinity flag plus the exact canonical field-zero storage
written by `secp256k1_gej_set_infinity`. This complements the existing
zero-scalar and infinity-base checks, which previously observed only the
infinity flag.

The oracle intentionally does **not** assert cleared coordinates for
`0 * G`: clean master returns the infinity flag there with noncanonical
intermediate coordinates, and treating that representation as a documented
contract would create a false positive. The new assertion is limited to the
branch whose production code explicitly calls `secp256k1_gej_set_infinity`.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding or fix. For causal proof, a disposable mutation changed
only the infinity-base branch in `src/ecmult_const_impl.h` from
`secp256k1_gej_set_infinity(r)` to `r->infinity = 1`. All five pre-existing
`ecmult_const` corpus inputs stayed green, while the exact new seed aborted at
the coordinate assertion with status 134. Disabling only the new helper call
made the mutated seed pass with status 0. Restoring the production line and
helper made all six inputs and empty input pass; the mutation was never
committed. Coverage recorded the infinity-base branch 10 times.

The fixed Clang 22.1.7 ASan/UBSan libFuzzer build passed the focused seed and
all six inputs, then completed a two-worker/two-job campaign in 31 seconds
with 521 runs and no sanitizer, assertion, timeout, OOM, or crash artifact.
The supported `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` / 10x26 build
also passed all six inputs plus `tests -t=ec -i=1` and
`noverify_tests -t=ec -i=1`. Existing master-relative findings remain
unchanged, and public nonce cleanup remains non-Critical because the nonce
has no cryptographic meaning.

## 2026-07-16 Group Canonical Affine-Infinity Storage Oracle

The internal `group` target now has a separate gated
`canonical-infinity-storage` seed. It poisons an affine point, calls
`secp256k1_ge_set_infinity`, and independently requires the infinity marker
plus the exact field-zero representation in both coordinates. The existing
`group/infinity-validity` seed intentionally remains a control: it checks the
semantic infinity and invalidity results, but does not observe the raw
coordinates. The new fixture therefore covers the affine constructor's
explicit zero writes without changing the older validity oracle.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding or a production fix. Clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already writes both affine
coordinates to zero. This is a distinct affine storage contract from the
Jacobian infinity output covered by the preceding `ecmult_const` oracle; no
cryptographic nonce or secret-state impact is involved.

For causal proof, a disposable production mutation changed both explicit
`secp256k1_fe_set_int(..., 0)` coordinate writes in `secp256k1_ge_set_infinity`
to `1`. All 19 pre-existing group corpus files stayed green, while the exact
new seed aborted with status 134. Disabling only the new gated helper made the
same mutated seed pass with status 0. An initial sentinel-preservation
mutation was rejected because it triggered an unrelated field-metadata
`VERIFY` failure in old controls; the accepted nonzero-field mutation models
a valid but noncanonical infinity representation. Restoring the constructor
and helper made all 20 inputs and empty input pass. The mutation was never
committed, and no master-relative severity change is claimed.

The Clang 22.1.7 ASan/UBSan builds with `SECP256K1_ASM=OFF` passed the focused
seed, the complete 20-input group corpus, and empty input on both the default
and forced-`int64`/10x26 backends. Two workers in two job managers completed
bounded campaigns in 16 seconds with 464 default and 457 forced-int64 runs,
respectively, without sanitizer diagnostics, assertions, timeouts, OOMs, or
crash artifacts. The default and forced-int64 `tests` and `noverify_tests`
group slices (`ge`, `gej`, `gej_rescale_alias`, `gej_zinv_in_place`, and
`group_decompress`, one iteration) also passed.

## 2026-07-16 Pippenger Second-Allocation-Failure Oracle

The internal `ecmult_multi` target now has a gated
`pippenger-second-allocation-failure` seed. It calls the single-batch
Pippenger helper with one point and a scratch arena sized to admit the
`points`, `scalars`, and `state_space` allocations but reject the subsequent
`state_space->ps` allocation. Both NULL and non-NULL generator-scalar cases
are exercised. The result starts as a valid finite generator, so the oracle
requires failure to produce canonical Jacobian infinity, zero callback calls,
an untouched callback trace, and complete scratch rollback.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding or fix. Clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297` already resets the result and
rolls back the scratch checkpoint on this allocation failure. The previous
21-input corpus reached only the first Pippenger allocation-failure block;
coverage showed that the later `state_space->ps`/scratch-object failure block
was absent. No public API vulnerability or availability claim is made, and
no cryptographic nonce or secret-state issue is involved.

For causal proof, a disposable mutation changed only the initial
`secp256k1_gej_set_infinity(r)` in `secp256k1_ecmult_pippenger_batch` to
`r->infinity = 1`. The 21 pre-existing inputs stayed green on both default
and forced-int64/10x26 ASan/UBSan builds, while the new seed aborted with
status 134 at the canonical-coordinate assertion. Disabling only the new
helper made the same mutated seed pass with status 0 on both backends. The
mutation and bypass were restored before the clean replay. This proves the
new postcondition observes a failure-state representation that the old
infinity-bit oracle did not.

The Clang 22.1.7 ASan/UBSan builds with `SECP256K1_ASM=OFF` replayed all 22
inputs plus empty input on both backends. The default and forced-int64
`tests` and `noverify_tests` `ecmult_multi_tests` targets passed one
iteration. Isolated `-workers=2 -jobs=2 -max_total_time=15 -timeout=10`
campaigns used private corpus copies; every manager and worker exited 0
without sanitizer diagnostics, assertions, timeouts, OOMs, or artifacts.
Existing master-relative findings and nonce-cleanup severity remain
unchanged.

## 2026-07-16 Direct Empty-Batch Oracle

The internal `ecmult_multi` target now has a gated
`direct-empty-batch` seed (`ecmult direct empty batch\n`). It calls both
single-batch implementations directly with `inp_g_sc = NULL`, no callback
points, and a zero-capacity scratch arena. The result is prefixed with a
finite generator to make stale output observable. The oracle requires a
successful return, canonical Jacobian infinity storage, zero callback calls,
an untouched callback trace, and unchanged scratch allocation state for both
Strauss and Pippenger.

This is **Informational / Low internal-oracle hardening** relative to clean
master `ebf594320dc838b9de1abb54d5ba98cef84f4297`, not a production finding or
fix. The public empty-input API paths were already covered, but the corpus
never invoked the internal single-batch `n_points == 0` exits directly. These
entry points are internal implementation contracts and the new assertion does
not imply a remotely triggerable availability, memory-safety, or cryptographic
issue. No nonce or secret-state finding is involved.

For causal proof, two disposable production mutations were tested separately:
after the existing `secp256k1_gej_set_infinity(r)` in
`secp256k1_ecmult_strauss_batch` or
`secp256k1_ecmult_pippenger_batch`, the empty branch wrote
`secp256k1_fe_set_int(&r->x, 1)` before returning success. This models a
noncanonical identity transition without changing the surrounding arithmetic.
The 22 pre-existing `ecmult_multi` controls stayed green on both default and
forced-int64/10x26 ASan/UBSan builds, while the exact new seed reached the
canonical-coordinate assertion and exited with libFuzzer status 77 on each
backend. Disabling only the new helper made the same mutated seed pass with
status 0 on both backends. Both mutations and both bypasses were restored; no
production mutation is committed.

Coverage replay of all 23 inputs reached both direct empty-batch branches; it
also preserved coverage of the prior Pippenger allocation-failure oracle. The
clean Clang 22.1.7 ASan/UBSan builds with `SECP256K1_ASM=OFF` replayed the
focused seed and complete corpus on both backends. The default and forced-
int64 `tests` and `noverify_tests` `ecmult_multi_tests` slices passed one
iteration. Isolated `-workers=2 -jobs=2 -max_total_time=15 -timeout=10`
campaigns used private corpus copies, completed with zero manager/worker
failures, and produced no sanitizer, assertion, timeout, OOM, or crash
artifacts. The master-relative severity ledger and public-nonce assessment
remain unchanged.

## 2026-07-16 Zero-Point Batch-Size Oracle

The internal `ecmult_multi` target now has a gated `batch-size-zero` seed
(`ecmult batch size zero\n`). It calls
`secp256k1_ecmult_multi_batch_size_helper` with `n == 0` and four capacity
cases: zero, one, one above `ECMULT_MAX_POINTS_PER_BATCH`, and `SIZE_MAX`.
The zero-capacity failure must leave both output counters at their sentinels;
every valid capacity must return success and set both `n_batches` and
`n_batch_points` to zero.

This is **Informational / Low internal-oracle hardening** relative to clean
master `ebf594320dc838b9de1abb54d5ba98cef84f4297`, not a production finding or
fix. The helper is an internal batch-planning contract. Public
`ecmult_multi_var` returns before calling it when `n == 0`. The unit suite
already directly covers the `max == 1, n == 0` boundary; this seed is a
corpus-driven reiteration of that internal contract, not a claim that unit
coverage was missing. The older input-derived helper checks and tracked fuzz
corpus did not select zero, so fuzzer replay lacked this explicit oracle. No
public availability, memory-safety, cryptographic, or nonce-state issue is
claimed.

For causal proof, a disposable production mutation changed only
`*n_batches = 0` in the `n == 0` branch to `*n_batches = 1`. All 23
pre-existing `ecmult_multi` inputs stayed green on both default and
forced-int64/10x26 ASan/UBSan builds, while the exact new seed reached the
zero-count assertion and exited with libFuzzer status 77 on both backends.
Disabling only the new helper made the same mutated seed pass with status 0.
The mutation and bypass were restored; no production mutation is committed.

Clean Clang 22.1.7 ASan/UBSan builds with `SECP256K1_ASM=OFF` replayed all 24
inputs on both backends, and coverage recorded three executions of the
production `n == 0` branch, including the clamped and ordinary valid-capacity
cases. The default and forced-int64 `tests` and `noverify_tests`
`ecmult_multi_tests` slices passed one iteration. Isolated
`-workers=2 -jobs=2 -max_total_time=15 -timeout=10` campaigns used private
24-file corpus copies; every manager and worker exited 0 without sanitizer,
assertion, timeout, OOM, or crash artifacts. Existing master-relative ratings
and the non-critical public-nonce assessment remain unchanged.

## 2026-07-16 Refreshed Reachability and Isolated Worker Recheck

After amending the zero-point batch-size context, the complete 256-file
corpus was replayed through the instrumented build. All 14 CTest fuzz suites
passed. The refreshed reachability review found no independent next oracle:
EllSwift's implementation paths were reached by its corpus; the remaining
MuSig subgroup rejection requires a non-subgroup point, which cannot occur on
the cofactor-one secp256k1 curve after a valid point load; and the MuSig
KeyAgg callback-failure return is unreachable with the current non-failing
internal callback. The remaining ecmult gaps are the zero-scalar WNAF path,
bucket-window inverse boundaries, and constrained-scratch simple fallback,
all directly exercised by `test_fixed_wnaf_small`,
`test_secp256k1_pippenger_bucket_window_inv`, or
`test_ecmult_multi_batching` in `src/tests.c`. No duplicate seed, production
change, or severity downgrade was justified.

The rebuilt Clang ASan/UBSan tree passed all 224 CTest cases, including the
verified ecmult-multi, MuSig, Schnorr, Recovery, EllSwift, field, and group
slices. Correctly isolated `-workers=2 -jobs=2 -max_total_time=8` campaigns
then passed with no sanitizer, assertion, timeout, OOM, or artifact result,
using private copies of every corpus: `api_roundtrip` completed 44 and 92
runs; `context` 84 and 76; `ecmult_multi` 25 and 25; `ellswift` 50 and 36;
`recovery` 76 and 71; `schnorrsig` 51 and 45; and `musig` 66 and 66. A
preliminary wrapper was discarded because its managers inherited shared
`fuzz-0.log` and `fuzz-1.log` paths and wrote 253 generated inputs under the
source corpus directories. Those generated files were removed before this
evidence run; the recorded counts are only from private per-target corpus
copies and temporary work directories, all removed after completion.

This remains a negative reachability pass, not a new clean-master finding.
Severity is still assessed against clean master before later fork or audit
fixes: Medium for malformed opaque state and public callback failure paths,
Medium/latent for the 10x26 arithmetic defects, Low/informational for
internal scratch robustness, and non-Critical for clearing a public nonce
that carries no cryptographic meaning.

The only additional field-side miss worth checking was the loop in
`secp256k1_jacobi64_maybe_var` that inspects the non-low limbs after
`f.v[0] == 1`. The 17 tracked field inputs and an isolated two-worker,
two-job libFuzzer search produced 935 and 931 executions, 32 and 36 new
units, and private corpora of 73 and 74 inputs; replaying the resulting 80
unique private inputs through the coverage binary passed without a diagnostic,
but still did not reach that line. The generic arbitrary-modulus Jacobi and
inversion model remains covered by `run_modinv_tests` in `src/tests.c`.
Because the fuzzer's field domain fixes the secp256k1 field modulus and no
independent public state transition or production failure was found, adding a
synthetic trigger here would only duplicate deterministic arithmetic coverage.
This is another negative reachability result, with no production fix and no
change to the clean-master severity ledger.

## 2026-07-16 Struct-Backed Wide-Multiply Recheck

The branch was refreshed against unchanged clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; no rebase was needed. A disposable
Clang 22.1.7 ASan/UBSan build selected the previously unrecorded
struct-backed 5x52 backend with `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=
int128_struct`, `SECP256K1_ASM=OFF`, all six optional modules, and the
libFuzzer runtime. The build command was:

```sh
cmake -S . -B /tmp/secp256k1-next-int128struct -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DSECP256K1_ASM=OFF -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int128_struct \
  -DSECP256K1_BUILD_BENCHMARK=OFF -DSECP256K1_BUILD_TESTS=ON \
  -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DSECP256K1_BUILD_CTIME_TESTS=OFF \
  -DSECP256K1_BUILD_FUZZ=ON -DSECP256K1_FUZZ_USE_LIBFUZZER=ON \
  -DSECP256K1_ENABLE_MODULE_ECDH=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_ENABLE_MODULE_EXTRAKEYS=ON \
  -DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON \
  -DSECP256K1_ENABLE_MODULE_MUSIG=ON \
  -DSECP256K1_ENABLE_MODULE_ELLSWIFT=ON
cmake --build /tmp/secp256k1-next-int128struct -j2
```

Independent replay passed all 256 tracked corpus files and an empty input for
each of the 14 targets: `api_roundtrip` 43, `context` 11, `ecdh` 6,
`ecmult_const` 6, `ecmult_multi` 24, `ellswift` 14, `field` 17, `group` 20,
`hash` 10, `musig` 65, `recovery` 10, `scalar` 4, `schnorrsig` 13, and
`xonly_tweak` 13. The same private corpus copies passed isolated
`-workers=2 -jobs=2 -max_total_time=8 -timeout=60` campaigns for every target;
all managers and workers exited 0, with no sanitizer, assertion, timeout, OOM,
or artifact output. The longest run was MuSig: 69 worker executions in 138
seconds.

The exact deterministic slices `tests` and `noverify_tests` for `ecdsa`,
`musig`, `ecmult_multi_tests`, `fe_normalize_max_magnitude`, and `group`
passed. The complete `ctest --test-dir /tmp/secp256k1-next-int128struct
--output-on-failure -j1` matrix passed all 224 tests in 615.85 seconds. This
backend pass found no cross-configuration inconsistency, but it cannot
downgrade findings that are specific to clean master's 10x26 implementation,
malformed opaque state, or public callback contracts. It is negative backend
evidence only: no new seed, production fix, or severity change is justified.
The existing findings remain rated against clean master, and clearing a public
nonce remains non-Critical because that nonce carries no cryptographic
meaning.

## 2026-07-16 10x26 Backend Corpus and Worker Recheck

The branch was checked against unchanged clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; `origin/master` was already an
ancestor, so no rebase was needed. A disposable Clang 22.1.7 Debug
ASan/UBSan build selected the alternate `int64` wide-multiply backend with
`SECP256K1_ASM=OFF`, all six optional modules, exhaustive tests, and the
libFuzzer runtime:

```sh
cmake -S . -B /tmp/secp256k1-next-int64-audit -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
  -DSECP256K1_ASM=OFF -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64 \
  -DSECP256K1_BUILD_TESTS=ON -DSECP256K1_BUILD_BENCHMARK=OFF \
  -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=ON \
  -DSECP256K1_BUILD_FUZZ=ON -DSECP256K1_FUZZ_USE_LIBFUZZER=ON \
  -DSECP256K1_ENABLE_MODULE_ECDH=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_ENABLE_MODULE_EXTRAKEYS=ON \
  -DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON \
  -DSECP256K1_ENABLE_MODULE_MUSIG=ON \
  -DSECP256K1_ENABLE_MODULE_ELLSWIFT=ON \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/secp256k1-next-int64-audit -j2
```

Fixed-input replay passed all 256 tracked corpus files and one temporary
zero-byte input for each of the 14 targets: `api_roundtrip` 43, `context` 11,
`ecdh` 6, `ecmult_const` 6, `ecmult_multi` 24, `ellswift` 14, `field` 17,
`group` 20, `hash` 10, `musig` 65, `recovery` 10, `scalar` 4,
`schnorrsig` 13, and `xonly_tweak` 13. The complete `bin/noverify_tests`
and `bin/tests` suites also passed, taking 564.813 and 1542.775 seconds,
respectively.

For a bounded parallel campaign after the complete replay, each target used
the first four existing seeds in sorted order plus an empty input in a
private corpus copy. `-workers=2 -jobs=2 -max_total_time=8 -timeout=60`
campaigns passed for all 14 targets; every manager and worker exited 0 with
no target assertion, sanitizer report, timeout, OOM, or crash artifact. An
earlier full-seed worker attempt was stopped solely because libFuzzer replayed
the entire corpus independently in every worker. Its ASan trace ended in
libFuzzer's `InterruptExitCode` during that external interruption, not in
production or harness code; it is excluded from the pass counts and is not a
secp256k1 finding.

This is negative cross-backend evidence, not a new clean-master finding. The
existing magnitude-32 10x26 normalization defects remain **Medium/latent**
when rated against clean master, even though this branch carries their fix;
malformed opaque state and public callback failure paths remain **Medium**,
and clearing a public nonce remains **non-Critical** because it has no
cryptographic meaning. No production fix, new corpus seed, cherry-pick, or
severity change is justified by this recheck.

## 2026-07-16 Fresh MuSig Coverage Recheck

The audit branch was checked against unchanged clean master
`ebf594320dc838b9de1abb54d5ba98cef84f4297`; it was already an ancestor of
`HEAD`, so no rebase was needed. This pass used GCC 16.1.0, CMake 3.31.6,
the `Coverage` build type, and a fresh out-of-tree build of the non-libFuzzer
`fuzz_musig` binary with all modules enabled:

```sh
cmake -S . -B /tmp/secp256k1-coverage-musig-next \
  -DCMAKE_BUILD_TYPE=Coverage -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_FUZZ_USE_LIBFUZZER=OFF -DSECP256K1_BUILD_TESTS=OFF \
  -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DSECP256K1_BUILD_BENCHMARK=OFF \
  -DSECP256K1_BUILD_CTIME_TESTS=OFF -DSECP256K1_INSTALL=OFF \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON
cmake --build /tmp/secp256k1-coverage-musig-next --target fuzz_musig -j2
cd /tmp/secp256k1-coverage-musig-next
./bin/fuzz_musig /tmp/secp256k1-oracles-next/src/fuzz/corpora/musig/*
```

The replay covered all 65 tracked MuSig corpus inputs and exited 0 with an
empty diagnostic log. `gcov -b -f` recorded 99.79% of the 468 production
lines and 100% of the 310 branches in
`src/modules/musig/session_impl.h`. The sole missed production line is the
subgroup rejection after a successful compressed-point parse. It is not a
fuzzer gap on this curve: secp256k1 has cofactor one, so every successfully
parsed finite curve point is in the subgroup. The fuzzer-side checks already
exercise malformed encodings and the full failure cleanup around that parser.

`src/modules/musig/keyagg_impl.h` reached 99.35% of 154 lines and 100% of
all 100 branches. Its only missed line is the `ecmult_multi_var` callback
failure fallback at line 212, which the source itself documents as unreachable
with the current non-failing internal callback. The fuzzer covers the
callback's real invalid-input barriers, aggregate-infinity result, duplicate
and cancellation cases, and output cleanup. The harness reached 97.55% of
its 2,407 lines and 97.92% of its 2,114 branches; the misses are helper
reference edge cases or deliberately synthetic failure hooks, not an
unasserted production state transition.

This is a fresh negative reachability and oracle-strength pass, not a new
clean-master finding. No production mutation, new corpus seed, cherry-pick,
or severity change is justified. Existing findings remain rated against clean
master before later audit or fork repairs: Medium for malformed opaque state
and public callback failure paths, Medium/latent for the clean-master 10x26
arithmetic defects, Low/informational for internal scratch robustness, and
non-Critical for clearing public nonce state because it carries no
cryptographic meaning.

## 2026-07-16 Recovery Message-Order Reduction Oracle

The recovery fuzzer already used an independent byte-level reduction model for
the ECDSA message scalar, but random 32-byte derivation almost never reaches
the `message >= n` branch. The gated
`recovery/message-order-reduction` seed now constructs the exact 32-byte value
`n + 1`, signs it and the reduced message `1` with the same fixed nonce, and
requires identical compact `(r, s, recid)` output. It then recovers both public
keys and checks the `n + 1` result with the independent point equation. This
tests the full sign/recover transition rather than merely marking a reference
branch covered.

This is **Informational oracle hardening**, not a clean-master production
finding. Clean master already reduces the message scalar in both signing and
recovery. No production fix or severity change is justified; existing
master-relative findings remain rated against clean master, and clearing a
public nonce remains **non-Critical** because it carries no cryptographic
meaning.

For causal proof, a disposable mutation in
`src/modules/recovery/main_impl.h` matched only the exact `n + 1` transcript
after message loading and replaced its message scalar with zero. All ten
pre-existing recovery inputs passed under the mutation, while the new seed
aborted with status 134 at the signature/recovery equivalence assertion.
Disabling only the new gated helper made that same mutated seed pass with
status 0. The production and harness mutations were restored before fixed
replay; the mutation was never committed.

The restored GCC coverage binary passed all 11 recovery inputs and reached
100% of the 109 production lines and all 98 production branches in
`src/modules/recovery/main_impl.h`. The restored Clang 22.1.7 ASan/UBSan
binary passed the focused seed and all 11 inputs. Both `tests -t=recovery -i=1`
and `noverify_tests -t=recovery -i=1` passed. A private-corpus
`-workers=2 -jobs=2 -max_total_time=8 -timeout=60` campaign completed 134 and
136 executions with both managers and workers exiting 0, and produced no
sanitizer, assertion, timeout, OOM, or crash artifact.

## 2026-07-16 Schnorr Custom-Tag Boundary Oracle

The Schnorr fuzzer already independently modeled the BIP340 nonce equation,
but its custom-algorithm check only generated tags of lengths 14 through 32.
That skipped the API's accepted short-tag range and the production dispatch
boundary: the optimized BIP340 midstate is selected at exactly 13 bytes, while
other non-NULL tags use the general tagged-SHA initializer. A gated
`schnorrsig-custom-nonce-tag-boundaries` seed now invokes the independent
reference for lengths 0 through 16, with and without auxiliary randomness.
Lengths 13 and 14 are deliberately adjacent so an optimization boundary
cannot silently become an oracle blind spot; length 0 also proves that a
non-NULL zero-length tag remains a valid callback-domain input.

The clean GCC 16.1.0 non-libFuzzer replay ran each of the 14 Schnorr corpus
files individually and exited 0 with no diagnostics. The non-libFuzzer driver
must receive file paths rather than the corpus directory itself; the equivalent
replay command is:

```sh
./bin/fuzz_schnorrsig /tmp/secp256k1-oracles-next/src/fuzz/corpora/schnorrsig/*
```

For causal proof, a disposable production-only mutation changed
`secp256k1_sha256_initialize_tagged(..., algolen)` to
`secp256k1_sha256_initialize_tagged(..., algolen + (algolen < 14))`. All 13
pre-existing Schnorr inputs stayed green under that mutation, while the exact
new boundary seed aborted with status 134. Removing only the new boundary call
made the mutated seed pass with status 0, isolating the failure to the new
oracle. The production mutation was restored; it is not a product fix.

A disposable Clang 22.1.7 ASan/UBSan libFuzzer build with assembly disabled and
all six optional modules enabled replayed the 14 tracked seeds plus an empty
input with `-runs=1`; it exited 0 after 15 runs. A private-copy campaign with
`-workers=2 -jobs=2 -max_total_time=8 -timeout=60` ran both jobs to exit 0
after 34 and 34 executions in 94 seconds. No sanitizer report, assertion,
timeout, OOM, or crash artifact was produced, and generated inputs stayed out
of the repository.

This is **Informational oracle hardening**, not a clean-master production
finding: master already computes the short-tag paths consistently. No
production fix or severity change is justified. Existing findings continue to
be rated against clean master before later audit or fork repairs; in
particular, malformed opaque state and public callback failure paths remain
**Medium**, clean-master 10x26 arithmetic defects remain **Medium/latent**, and
clearing a public nonce remains **non-Critical** because that nonce carries no
cryptographic meaning.

## 2026-07-16 ECDSA DER Scalar-Overflow Oracle

The API-roundtrip fuzzer now has a gated
`ecdsa-der-scalar-overflow` seed for the parser branch that handles a positive
33-byte DER INTEGER. The encoding is syntactically valid DER but out of range
for a secp256k1 scalar. The helper tests both an overflowing `r` and an
overflowing `s`, requires the public parser to return success and initialize a
serializable signature, and independently requires verification to fail. It
therefore checks the documented distinction between parseability and signature
validity instead of treating an accepted input as a valid signature.

This is **Informational / Low internal-oracle hardening**, not a clean-master
production finding. Clean master deliberately accepts valid DER with
out-of-range numbers and produces a signature that cannot verify. The prior
43-file API corpus exercised malformed DER, leading-zero and order-overflow
integers, but never reached the separate positive 33-byte `rlen > 32` branch;
ordinary build/tests therefore did not provide a deterministic fuzzer oracle
for this postcondition. No production fix or severity change is justified.

For causal proof, a disposable mutation changed the `rlen > 32` overflow guard
in `src/ecdsa_impl.h` to allow the 33-byte copy. All 43 pre-existing API inputs
passed under that mutation, while the exact new seed triggered an ASan
stack-buffer-overflow on the 33-byte write. Disabling only the new helper made
the same mutated seed pass with status 0. The production and harness mutations
were restored before fixed replay and are not committed.

The restored GCC coverage replay passed all 44 API inputs and reached 96.30%
of the 135 `ecdsa_impl.h` lines and all 106 production branches; the remaining
misses are the cryptographically unreachable successful long-form return, the
recovery-only recid overflow/high-S assignments, and invalid-point branches
outside this target's generated verification domain. The restored Clang
22.1.7 ASan/UBSan binary passed the focused seed and all 44 inputs. Both
`tests -t=ec -i=1` and `noverify_tests -t=ec -i=1` passed. A private-copy
`-workers=2 -jobs=2 -max_total_time=8 -timeout=60` campaign completed 92 and
93 executions with both managers and workers exiting 0 and no sanitizer,
assertion, timeout, OOM, or crash artifact. Existing master-relative findings
remain rated against clean master, and clearing a public nonce remains
**non-Critical** because it carries no cryptographic meaning.

## 2026-07-16 Hash and EllSwift Reachability Recheck

After refreshing `origin/master`, the audit branch still has clean master as
an ancestor (`origin/master`=`ebf594320dc838b9de1abb54d5ba98cef84f4297`), so
no rebase was required. The hash target replayed all ten tracked inputs in a
Clang 22.1.7 ASan/UBSan build, including the buffered-block cleanup seed. The
`hash` and `noverify_tests` slices both passed at one iteration, and an
isolated `-workers=2 -jobs=2 -max_total_time=8 -timeout=60` campaign completed
with both jobs exiting 0 and no sanitizer, assertion, timeout, OOM, or crash
artifact. This independently rechecks the Medium clean-master secret-state
retention finding and the production wipe carried by `55f98a8`; it does not
raise the finding to a disclosure or Critical cryptographic issue.

A fresh GCC 16.1.0 Coverage build replayed all 14 EllSwift corpus inputs. The
EllSwift implementation reached 100% of its 264 production lines and 100% of
its 156 production branches. The remaining audit misses are outside this
module's valid public state domain or belong to unrelated targets; adding a
synthetic callback or impossible point would weaken the oracle rather than
discover a master bug. This is negative reachability evidence, not a new
production finding or fix.

The severity ledger is unchanged when evaluated against clean master before
later audit or fork repairs: malformed opaque state and public callback
failure paths remain **Medium**, the reachable-status 10x26 arithmetic issue
remains **Medium/latent**, documented tweak alias behavior remains **Low**,
and the remaining cleanup/oracle checks are **Informational** unless they
demonstrate a production failure. A nonce without cryptographic meaning is
not a Critical erasure finding.

## 2026-07-16 Exact-Commit Corpus and Worker Recheck

The audit tree was rebuilt from commit `8d6eb1f` after refreshing both
`origin/master` and the l0rinc fork. `origin/master` remained
`ebf594320dc838b9de1abb54d5ba98cef84f4297` and an ancestor of the audit tree,
so no rebase was required. The exact-commit Clang 22.1.7 ASan/UBSan
libFuzzer build has all six optional modules enabled. A `-runs=1` replay of
the complete tracked corpus passed with these execution counts:

```
api_roundtrip 45   context 12       ecdh 7          ecmult_const 7
ecmult_multi  25   ellswift 15     field 18        group 21
hash          11   musig 66        recovery 12     scalar 5
schnorrsig    15   xonly_tweak 14
```

The stateful targets then ran in private working directories with
`-workers=2 -jobs=2 -max_total_time=15` (MuSig used 5 seconds after its
long corpus replay): `fuzz_api_roundtrip` completed 168 and 169 executions,
`fuzz_ecmult_multi` completed 45 and 45, `fuzz_recovery` completed 227 and
230, and `fuzz_musig` completed 66 and 66. Every job exited 0. The remaining
targets ran with `-workers=2 -max_total_time=10` and completed 138, 152, 63,
87, 312, 14,488, 416, 83, and 14 executions for `context`, `ecdh`,
`ellswift`, `field`, `group`, `hash`, `scalar`, `schnorrsig`, and
`xonly_tweak`, respectively. No ASan/UBSan diagnostic, assertion failure,
OOM, timeout, or crash artifact was produced. Generated mutation files were
removed from the tracked corpus directories after the run.

`clang-tidy -p /tmp/secp256k1-tidy src/secp256k1.c
-checks='-*,clang-analyzer-*' --quiet` emitted only the existing release-build
dead-store warning for the EllSwift square-root result consumed by
`VERIFY_CHECK`; verification builds use the result and the preceding
square-class test establishes the precondition. It is not a production bug or
a reason to add a weaker oracle.

The same `clang-analyzer-core` and `clang-analyzer-security` checks against
the `-DVERIFY` production translation unit completed with no diagnostics.
GCC 16.1.0 `-fanalyzer` completed the equivalent module-expanded syntax pass
with no diagnostics as well.

The matching `clang-analyzer-core` and `clang-analyzer-security` pass over all
14 fuzz translation units emitted no memory or arithmetic defect. Its only
diagnostics were deliberate NULL calls made through function-pointer typedefs
so the harness can test illegal-argument callbacks despite public non-NULL
attributes, and stack-address-escape warnings for callback counters whose
callbacks are cleared before each helper returns. These are harness-modeling
warnings, not production findings, so no suppression or weaker oracle was
added.

This is negative verification evidence, not a new clean-master finding. The
severity ledger is unchanged when evaluated before later audit or fork
repairs: malformed opaque state and public callback failure paths remain
**Medium**, the reachable-status 10x26 arithmetic issue remains
**Medium/latent**, documented tweak alias behavior remains **Low**, and
cleanup/oracle-only checks remain **Informational**. A nonce without
cryptographic meaning is not a Critical erasure finding.

The latest relevant l0rinc fork tips were also reconciled before this
recheck. `b5e6108` (DER lengths with offsets) is the same parser and boundary
test as `52cb1af`; `87e57c8` (scalar shifts above the product width) is the
same guard and regression as `04bfcac`; and `c0f32d4` (scratch test pointer
provenance) is already represented by `527770c` and its stronger scratch
oracles. The BER generator, compact-signature bounds, MuSig cleanup, and
magnitude-32 field fixes are likewise already present. No duplicate fork
commit was cherry-picked, and no optimization-only fork commit was applied
because it would change the clean-master behavior being audited.

## 2026-07-16 Rebase and Exact-Master Worker Recheck

After the previous audit replay, `origin/master` advanced from
`ebf594320dc838b9de1abb54d5ba98cef84f4297` to `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`.
The new upstream commits only update `SECURITY.md`. The audit branch was
rebased with `git rebase origin/master`; it completed without conflicts and
left the production and fuzzer sources unchanged.

The rebased Clang 22.1.7 ASan/UBSan build, with assembly disabled and all six
optional modules enabled, replayed every tracked input once. All 14 targets
exited 0: `api_roundtrip`, `context`, `ecdh`, `ecmult_const`, `ecmult_multi`,
`ellswift`, `field`, `group`, `hash`, `musig`, `recovery`, `scalar`,
`schnorrsig`, and `xonly_tweak`. There was no sanitizer diagnostic, assertion,
timeout, OOM, or crash artifact. The exact recovery corpus still reaches its
malformed signature, callback-failure, invalid-point, message-reduction, and
output-cleanup oracles; a separate review found no untested recovery contract
that justified another duplicate seed.

Private copies of the API-roundtrip, ecmult-multi, recovery, and MuSig corpora
then ran with `-workers=2 -jobs=2 -max_total_time=10 -timeout=60`. Every
manager and worker exited 0. Fuzzing generated additional inputs only in the
private copies; those files were removed after the run, and the tracked
corpora remained unchanged. No sanitizer, assertion, timeout, OOM, or crash
artifact was produced.

This is negative post-rebase verification, not a new clean-master finding.
Severity remains evaluated against clean master before later audit or fork
repairs: malformed opaque state and public callback failure paths remain
**Medium**, the reachable-status 10x26 arithmetic issue remains
**Medium/latent**, documented tweak alias behavior remains **Low**, and
cleanup/oracle-only checks remain **Informational**. Clearing a public nonce
is not a Critical erasure finding because that nonce carries no cryptographic
meaning. No production fix or duplicate l0rinc cherry-pick is justified by
this replay.

## 2026-07-16 Private Multi-Worker Campaign and PR 15 Reconciliation

The exact latest fork pull head for l0rinc PR #15 is `a2a0ac2`. Its
production change moves the keypair clear in `secp256k1_keypair_xonly_tweak_add`
until after the tweak has been read, preserving the documented keypair/tweak
overlap. That behavior is already present in `ba8d379`, with the related
x-only projection coverage in `dc14cb7`; those commits also add stronger
copied-versus-aliased comparisons, deterministic tests, a focused corpus
input, and allocation-failure/mutation proof. The fork commit therefore adds
no distinct clean-master behavior or evidence and remains intentionally
uncherry-picked.

Against `origin/master` at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, private copies of the
`api_roundtrip`, `ecmult_multi`, `recovery`, and `musig` corpora ran under the
Clang 22.1.7 ASan/UBSan libFuzzer build with all six optional modules enabled,
assembly disabled, and:

```
-workers=4 -jobs=4 -max_total_time=120 -timeout=60 -rss_limit_mb=4096
```

Each manager and worker exited 0. The artifact directories stayed empty, and
there was no ASan/UBSan diagnostic, assertion failure, timeout, OOM, or crash.
The generated corpus files and worker logs were kept outside the audit tree;
the tracked corpus remains unchanged. This is stronger negative evidence for
the stateful paths, not a new production finding. Severity is still assessed
against clean master before later audit/fork repairs: malformed opaque state
and public callback failure paths are **Medium**, the reachable-status 10x26
arithmetic issue is **Medium/latent**, documented tweak alias behavior is
**Low**, and cleanup/oracle-only checks are **Informational**. A public nonce
without cryptographic meaning is not a Critical erasure finding.

## 2026-07-16 External-Callback Corpus and Worker Recheck

The context target has several illegal-argument and null-preallocation paths
that are compiled only with `SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS=ON`.
Those paths were verified separately instead of being treated as covered by
the normal aborting-callback build. A disposable Clang 22.1.7 ASan/UBSan
libFuzzer build used `SECP256K1_ASM=OFF`, all six optional modules, tests, and
the external default callbacks. It replayed all 259 tracked corpus files once;
the extra empty-input execution made the per-target totals:

```
api_roundtrip 45   context 12       ecdh 7          ecmult_const 7
ecmult_multi  25   ellswift 15     field 18        group 21
hash          11   musig 66        recovery 12     scalar 5
schnorrsig    15   xonly_tweak 14
```

The command was run as `bin/fuzz_<target> -runs=1 -timeout=10
src/fuzz/corpora/<target>` for every target. All 14 processes exited 0 with no
sanitizer diagnostic, assertion failure, timeout, OOM, or crash artifact. The
API, context, ecmult-multi, and MuSig targets were then replayed from private
corpus copies with two workers and two jobs using:

```
-workers=2 -jobs=2 -max_total_time=10 -timeout=5 -rss_limit_mb=4096
```

Both workers for each target exited 0. Generated inputs stayed outside the
audit tree, and the tracked corpora were unchanged. This is distinct negative
verification for the external callback paths, not a new clean-master finding.
The severity ledger therefore remains evaluated against clean master before
later audit or fork repairs: malformed opaque state and public callback
failure paths are **Medium**, the reachable-status 10x26 arithmetic issue is
**Medium/latent**, documented tweak alias behavior is **Low**, and
cleanup/oracle-only checks are **Informational**. Clearing a public nonce is
not a Critical erasure finding because that nonce carries no cryptographic
meaning.

## 2026-07-16 ECDH Explicit Built-In SHA Context Dispatch

The clean-master source at `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` had a
reachable dispatch inconsistency in `secp256k1_ecdh`. The runtime SHA-256
compression setter documents a context backend for library operations, and the
`hashfp == NULL` ECDH path already used that backend. Passing either exported
library-owned callback, `secp256k1_ecdh_hash_function_sha256` or
`secp256k1_ecdh_hash_function_default`, instead called the callback wrapper,
which is bound to `secp256k1_context_static`. A caller selecting the explicit
built-in therefore received the right digest but silently bypassed the custom
hardware or platform compressor installed on the supplied context.

This is a **Low master-relative production finding**: it is a public API
contract and performance/dispatch failure, not a cryptographic result change,
secret disclosure, or signature/ECDH weakness. A valid replacement compressor
must be SHA-256-equivalent, so the strongest observable proof is callback
invocation rather than output inequality. The fuzzer now resets its callback
counter around all four explicit built-in ECDH calls and requires each call to
use the configured context backend. Previously the same six-file corpus only
compared explicit and default outputs; that comparison passed because both
paths still computed the standard digest.

For causal proof, the new deterministic ECDH test installs an exact
SHA-256-equivalent compressor through the public setter, resets its counter
after setter self-tests, and invokes both exported built-in callback aliases.
A disposable production mutation changing the repaired `if (known_hashfp)`
dispatch back to `if (hashfp == NULL)` made `bin/tests -t=ecdh -i=1` abort at
`sha256_ecdh_valid_calls != 0` (exit 134). Before the mutation, the enhanced
fuzzer likewise aborted on the first existing ECDH corpus input (exit 134),
while the prior unasserted corpus replay remained green. Restoring the
context-aware dispatch made the six corpus files plus empty input pass, along
with `tests -t=ecdh -i=1` and `noverify_tests -t=ecdh -i=1`, under Clang
22.1.7 ASan/UBSan. The mutation was restored and is not itself a fix.

The master-relative severity ledger remains explicit: malformed opaque state,
public callback failure paths, SHA state retention, impossible SHA lengths,
and the reachable 10x26 magnitude-32 arithmetic issue remain **Medium** or
**Medium/latent** as previously recorded; documented tweak-input overlap is
**Low**; oracle-only hardening is **Informational**. Clearing a public nonce
does not become Critical because that nonce carries no cryptographic meaning.

## 2026-07-16 ECDSA Explicit Built-In RFC6979 Context Dispatch

The clean-master source at `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` had a
second reachable instance of the context-dispatch contract already examined
for ECDH. `secp256k1_ecdsa_sign_inner` used the caller's SHA-256 context when
`noncefp == NULL`, but passing either exported library-owned nonce pointer,
`secp256k1_nonce_function_rfc6979` or `secp256k1_nonce_function_default`,
called the public wrapper bound to `secp256k1_context_static`. The shared
helper made this affect both ordinary and recoverable ECDSA signing.

This is a **Low master-relative production finding**: a caller selecting an
explicit built-in nonce function silently bypassed a configured hardware or
platform SHA compressor. The generated signature, recovery id, and RFC6979
transcript remain correct because the compression callback is required to be
SHA-256-equivalent; this is a public API dispatch/performance failure, not a
forgery, nonce-reuse, key-disclosure, or cryptographic-result finding. The
severity is assessed against clean master, not reduced because later audit
changes or unrelated fork fixes happen to touch nonce handling.

This reiterates and tightens the earlier `63c9bd4` recoverable-signing oracle:
that check proved `NULL` routing and compared explicit/default signatures, but
output equality cannot detect a valid compressor being bypassed. The new
oracles clear a counted exact-SHA callback after setter self-tests and require
the callback to run for both explicit aliases in `fuzz_api_roundtrip` and
`fuzz_recovery`; `ecdsa_ctx_sha256` provides the deterministic unit regression.
Arbitrary caller callbacks remain dispatched directly and retain their data
semantics. The analogous ECDH production fix is recorded separately in
`955daba`; no l0rinc fork pull request duplicated this ECDSA dispatch behavior.

For causal proof, a temporary production mutation changed only
`if (known_noncefp)` back to `if (noncefp == NULL)`. With the new assertions
present, `bin/tests -t=ecdsa -i=1`, all 44 existing API-roundtrip corpus inputs,
and all 11 existing recovery corpus inputs each aborted with exit 134 at the
callback-use oracle. Restoring the shared context-aware branch made the unit,
API corpus, and recovery corpus pass under Clang 22.1.7 ASan/UBSan; the
mutation was temporary and is not part of the fix.

## 2026-07-16 SHA-Sensitive Cross-Backend Multi-Worker Recheck

After the ECDSA dispatch fix, the clean-master audit was refreshed at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` and the l0rinc pull-request refs
were fetched again. `origin/master` was already an ancestor of the audit
branch, so no rebase was required. The source review checked the remaining
uses of `secp256k1_context_static`, the public keypair projection accessors,
and the serialization/state-machine paths most likely to hide a weak oracle.
The keypair accessors intentionally return raw halves and document success
even for malformed opaque state; treating that behavior as a validity oracle
would be a stale fuzzer contract, not a production bug.

The following exact targets were rebuilt with Clang 22.1.7 ASan/UBSan and
replayed against their existing corpora on the native backend and with
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`:

```
context api_roundtrip ecdh ellswift musig recovery schnorrsig
```

Each target used two workers and two jobs with
`-workers=2 -jobs=2 -rss_limit_mb=4096`; the native pass used
`-max_total_time=15 -timeout=60` and the forced-int64 pass used
`-max_total_time=10 -timeout=60`. Every worker/job exited 0. There was no
sanitizer diagnostic, assertion failure, timeout, OOM, crash artifact, or
cross-backend result mismatch. The expensive MuSig state corpus completed in
both configurations, and the explicit built-in SHA callback paths were
exercised again after the ECDSA/ECDH fixes. Generated mutations were kept in
disposable corpus copies or removed after verification; tracked corpora and
production sources remained unchanged.

This is a distinct negative verification pass, not a new clean-master
finding. The severity ledger is therefore unchanged: the previously proven
master-relative production findings retain their recorded ratings, while
the rejected accessor interpretation remains an invalid oracle. Clearing a
public nonce is not a Critical erasure finding because that nonce carries no
cryptographic meaning.

## 2026-07-16 Scalar Inversion Worker Recheck

The scalar target was rechecked separately after the SHA-sensitive campaign.
The audit tree was clean at `48f1a5a`, with `origin/master` at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` already an ancestor, so no rebase
was required. The four tracked scalar inputs were copied to disposable corpus
directories and run with two libFuzzer workers and two jobs:

```
-workers=2 -jobs=2 -max_total_time=20 -timeout=60 -rss_limit_mb=4096
```

The native Clang 22.1.7 ASan/UBSan build and the forced
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` Clang 22.1.7 ASan/UBSan build
both completed all manager jobs with exit 0. The runs generated additional
mutations, and neither backend produced a sanitizer diagnostic, assertion
failure, timeout, OOM, or crash artifact. Temporary corpora and libFuzzer
logs were removed; the tracked scalar corpus was unchanged.

This recheck confirms the existing scalar oracle rather than adding a weaker
duplicate: canonical decoding is independently reduced, multiplication is
checked against a base-2^16 product and long-division model, constant-time and
variable-time inverses are compared, and the inverse product is checked as
zero or one. No clean-master scalar defect was reproduced, so no production
fix, new seed, or severity change is justified. Existing findings remain
rated against clean master; clearing a nonce without cryptographic meaning is
not a Critical erasure finding.

## 2026-07-16 Latest l0rinc Ref Reconciliation and ecmult Clone Oracle Recheck

The audit branch is based on `origin/master` at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`; that ref is already an ancestor,
so no rebase was needed after the latest fetch. The current fork refs were
checked against the complete audit history before another `ecmult_multi`
campaign. The exact `6f2828a` cloned-error-callback oracle is already in this
branch, including `error-callback-clone`: it installs distinct non-NULL error
callback data, clones both heap and preallocated contexts, forces each clone
through invalid scratch destruction, and verifies that changing the original
context does not redirect either clone.

The remaining detached refs do not justify duplicate cherry-picks:
`248be19` fixes the BER test helper and is represented by `4104f54` plus the
stronger DER parser oracle; `8363a2d` checks all 66 MuSig aggregate-nonce
bytes and is already covered by the current unit and independent fuzzer
checks; `994b350` is in upstream through `5a8a411`; and `65d38b0` is covered
by `0d03dda` and the magnitude-32 field corpus. The older MuSig cleanup refs
(`bb02b1e` and `7ed2abc`) are represented by the current cleanup and invalid
state barriers. The force-inline, xor-mask-CMOV, hash-stack, EllSwift
unchecked-square-root, and Strauss fast-path refs are optimization snapshots
or behavior-changing stacks. They remain excluded from the discovery branch
so they cannot mask a clean-master failure or be used to lower its severity.

The cloned-error-callback seed and all 25 tracked `ecmult_multi` inputs were
replayed after rebuilding the exact source with Clang 22.1.7 ASan/UBSan,
assembly disabled, both natively and with
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`. Private copies of each corpus
then ran with `-workers=2 -jobs=2 -max_total_time=12 -timeout=60
-rss_limit_mb=4096`; every manager and worker exited 0. No sanitizer report,
assertion, timeout, OOM, crash artifact, or generated file remained.

This is negative fork reconciliation and oracle verification, not a new
clean-master production finding. The existing ledger is unchanged: malformed
opaque state and public callback failure paths remain **Medium**, the
reachable-status 10x26 magnitude-32 arithmetic issue remains
**Medium/latent**, documented tweak-input overlap remains **Low**, and
cleanup-only or oracle-only checks remain **Informational**. Severity is still
assigned against clean master before later fixes or fork patches; a nonce
without cryptographic meaning is not a Critical erasure finding.

## 2026-07-16 Arithmetic Worker Recheck

The native and forced-int64 Clang 22.1.7 ASan/UBSan builds were rebuilt from
the same clean audit source and replayed over the tracked `field`, `group`,
and `ecmult_const` corpora. Each target used private corpus copies and

```
-workers=2 -jobs=2 -max_total_time=20 -timeout=60 -rss_limit_mb=4096
```

All manager and worker processes exited 0 in both wide-multiply
configurations. No sanitizer diagnostic, assertion failure, timeout, OOM,
crash artifact, or cross-backend mismatch occurred; temporary corpora and
logs were removed and no tracked input changed. The first forced-int64
attempt was incomplete because `fuzz_field` had not been built; it was built
before the successful replay, so the pass covers all three targets.

This is a negative arithmetic oracle recheck, not evidence of a new clean
master defect. The existing reachable-status 10x26 magnitude-32 issue stays
**Medium/latent**, and no severity is reduced because later fork fixes or
optimization snapshots were not used to mask it. A nonce without
cryptographic meaning is not a Critical erasure finding.

## 2026-07-16 Complete Refreshed l0rinc Detached-Ref Reconciliation

After the latest `git fetch l0rinc --prune`, the additional detached refs were
compared against the complete audit history and current source. No rebase was
needed: `origin/master` remains `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`
and is an ancestor of `HEAD`.

The following behavior-bearing refs are already represented by equal or
stronger commits here, so cherry-picking them would duplicate history rather
than test clean master independently:

```
6e60f8d  -> d8f4bdc   clear keypair on NULL x-only tweak
13308e3  -> 6920d56   clear failed MuSig aggregate signature output
51e93c4  -> 6920d56   same MuSig fix from the alternate detached parent
b5e6108  -> cad8e5b   parse DER lengths with offsets
e217ead  -> 6f602e7   serialize field elements by word
a2a0ac2  -> 45d05f7,
             dc14cb7  preserve documented overlapping tweak inputs
d1dca5c  -> c51b255  reject invalid loaded public keys, with stronger
                       ECDH and combine failure barriers
7b47f1f  -> 6fa1dbc  reject RFC6979's maximum retry counter
87e57c8  -> 96e21ad  guard scalar rounded shifts above 512 bits
```

The current versions add independent fuzz assertions, broader output cleanup,
or cross-backend mutation proofs where applicable. The detached
`3f5fafa` ref changes only a modinv32 comment and has no runtime effect. These
decisions therefore do not alter clean-master behavior or lower any existing
finding: malformed opaque state and callback failure paths remain **Medium**,
the reachable-status 10x26 arithmetic issue remains **Medium/latent**,
documented tweak overlap remains **Low**, and cleanup/comment/optimization-only
changes remain **Informational**. A nonce without cryptographic meaning is not
a Critical erasure finding.

## 2026-07-16 Complete Current-Master Corpus Recheck

The audit tree was rechecked after the latest fetch at clean `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`. The ref is already an ancestor of
`codex/fuzz-oracles`, so no rebase was required. All 14 tracked targets were
run from private copies of their existing corpora, keeping libFuzzer's
generated mutations and job logs outside the repository:

```
api_roundtrip context ecdh ecmult_const ecmult_multi ellswift field
group hash musig recovery scalar schnorrsig xonly_tweak
```

The native Clang 22.1.7 ASan/UBSan build used two workers and two jobs per
target with `-max_total_time=12 -timeout=60 -rss_limit_mb=4096`. Every manager
and worker exited 0. The forced-int64/10x26 Clang 22.1.7 ASan/UBSan build used
the same private-corpus layout, two workers and two jobs, with
`-max_total_time=10`; every manager and worker also exited 0. There was no
sanitizer report, assertion failure, timeout, OOM, crash artifact, or tracked
corpus change. The expensive 65-input MuSig corpus and the high-window
Pippenger inputs completed on both backends.

This is a negative verification pass, not a new clean-master finding. It
reiterates the existing ratings against the unmodified master baseline:
malformed opaque state and public callback failure paths remain **Medium**;
secret hash-state lifetime, impossible SHA lengths, and the reachable-status
10x26 magnitude-32 arithmetic issue remain **Medium** or **Medium/latent**;
documented tweak-input overlap remains **Low**; and cleanup-only or
oracle-only checks remain **Informational**. A nonce with no cryptographic
meaning is not a Critical erasure finding. No fork patch or later audit fix
was used to downgrade a master finding, and no production change is claimed
without a master reproduction or a minimal mutation proof.

## 2026-07-16 Extended Stateful Multi-Worker Campaign

The audit branch was still a clean descendant of
`origin/master 11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, so no rebase was
needed before this pass. The four highest-state targets were copied from the
tracked corpora into disposable directories: `musig` (65 inputs),
`ecmult_multi` (24), `api_roundtrip` (44), and `ellswift` (14). Eight workers
and eight jobs per target shared only those private directories, allowing
libFuzzer to mutate stateful inputs without changing the repository:

```
-workers=8 -jobs=8 -max_total_time=60 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1
```

The native Clang 22.1.7 ASan/UBSan binaries and the forced
`SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64` Clang 22.1.7 ASan/UBSan
binaries each ran all four targets. All 64 manager jobs and their workers
exited 0. The workers generated additional private mutations; the expensive
Pippenger-window and MuSig inputs took substantially longer than ordinary
corpus replay, but produced only four libFuzzer `slow-unit-*` records and no
crash artifact. There was no sanitizer diagnostic, assertion failure, timeout,
OOM, or runtime-error marker, and no fuzz process remained after polling.

This is a longer backend-agreement recheck, not a new clean-master finding.
It reinforces the existing oracle and severity ledger: malformed opaque state
and public callback failure paths remain **Medium**; the reachable-status
10x26 magnitude-32 arithmetic issue remains **Medium/latent**; documented
tweak-input overlap remains **Low**; and cleanup-only or oracle-only checks
remain **Informational**. Severity is still assigned against clean master
before later fork fixes or audit repairs. A nonce without cryptographic
meaning is not a Critical erasure finding. No production fix is claimed
without a master reproduction or a minimal production mutation proof.

## 2026-07-16 MuSig `s_part` Mutation Proof

The serialized session field at `session->data + 101` is the tweak-dependent
`s_part` that `secp256k1_musig_partial_sig_agg` adds before emitting the final
signature. To test whether the existing oracle independently binds that field,
a disposable production mutation changed the clean-master condition in
`src/modules/musig/session_impl.h` from
`if (!secp256k1_scalar_is_zero(&cache_i.tweak))` to
`if (0 && !secp256k1_scalar_is_zero(&cache_i.tweak))`. This removes only the
`e * tweak` contribution; no fuzzer code or corpus file was changed.

The exact input `src/fuzz/corpora/musig/tweaked-signing-parity` (23 bytes)
reached the tweaked signing path and aborted at the independent
`secp256k1_fuzz_check_musig_final_sig_equation` assertion at
`src/fuzz/musig.c:459`, after `secp256k1_fuzz_check_musig_tweaked_sign_case`
at line 1038. Native Clang 22.1.7 ASan/UBSan and forced-int64/10x26
Clang 22.1.7 ASan/UBSan builds both reproduced the same SIGABRT under GDB.
After restoring the one-line mutation, both builds replayed the same input
once with exit status 0 and no sanitizer output.

This is an oracle-strength proof, not a production bug: clean master already
contains the `s_part` contribution and the independent final equation catches
its removal. It does not change the existing severity ledger. Master-relative
ratings remain **Medium** for malformed opaque state and callback failure,
**Medium/latent** for the reachable-status 10x26 magnitude-32 arithmetic
issue, **Low** for documented tweak-input overlap, and **Informational** for
this negative oracle result. A nonce without cryptographic meaning is not a
Critical erasure finding. No fork fix or later audit commit was used to lower
any master-relative severity.

## 2026-07-16 Four-Target Cross-Backend Worker Campaign

The audit branch remained a clean descendant of
`origin/master 11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, so no rebase was
needed. Private copies of the `api_roundtrip` (44 inputs), `ecmult_multi`
(24), `ellswift` (14), and `musig` (65) corpora were replayed with the
Clang 22.1.7 ASan/UBSan native 5x52 build and the forced-int64/10x26 build.
Each target used four workers and four jobs:

```
-workers=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1
```

All 32 manager jobs exited 0. The stateful MuSig jobs took up to 140 seconds
to finish their complete corpus because each input performs a long protocol
trace; the slower run was not an input timeout. No sanitizer diagnostic,
fuzzer assertion, nonzero worker result, OOM, timeout, or crash artifact was
observed, and the private artifact directories remained empty. Generated
mutations stayed outside the repository and were removed with the temporary
campaign directories after inspection.

This is negative regression evidence for the committed audit oracles, not
proof that clean master has no additional defect: the branch contains prior
production fixes whose behavior could mask a later mutation. It therefore
does not lower any master-relative rating or replace a clean-master or
minimal-mutation proof. The existing ledger remains **Medium** for malformed
opaque state, callback failure, and secret SHA-state lifetime; **Medium/latent**
for the reachable 10x26 magnitude-32 arithmetic issue; **Low** for documented
tweak-input overlap; and **Informational** for cleanup/oracle-only checks. A
nonce without cryptographic meaning remains non-Critical.

## 2026-07-16 Signing and Context Worker Recheck

After refreshing both remotes, the audit branch remained a clean descendant of
`origin/master 11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`; no rebase was
needed. The l0rinc comparison had no fork-side commits to cherry-pick: the
only three left-side commits were the upstream security-contact merge and its
`SECURITY.md` follow-ups.

Private copies of the tracked `context` (11 files), `ecdh` (6), `recovery`
(11), `schnorrsig` (14), and `xonly_tweak` (13) corpora were replayed on
native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan builds. Each of
the ten target/backend invocations used:

```
-workers=4 -jobs=4 -max_total_time=60 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1
```

All 40 worker jobs exited 0. One forced-int64 Schnorr manager took longer than
the nominal manager budget while draining its complete stateful corpus, but
finished successfully. No sanitizer diagnostic, fuzzer assertion, nonzero
worker, timeout, OOM, or crash artifact was observed; no fuzz process remained
after polling. Generated mutations and artifacts stayed in disposable
directories outside the repository.

This is negative regression evidence for the committed branch oracles, not a
clean-master discovery proof: prior branch production fixes can mask a
clean-master mutation. The existing findings therefore remain rated against
unmodified master: **Medium** for malformed opaque state, public callback
failure, and secret SHA-state lifetime; **Medium/latent** for the reachable
10x26 magnitude-32 arithmetic defect; **Low** for documented tweak-input
overlap; and **Informational** for cleanup/oracle-only checks. A nonce without
cryptographic meaning is not a Critical erasure finding. No production fix or
severity downgrade is claimed without a clean-master reproduction or a
minimal production mutation proof.

## 2026-07-17 Value-Profiled Arithmetic and Scratch Recheck

The remotes were refreshed before this pass. `origin/master` remained
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, the audit branch was already its
descendant, and the l0rinc fork still had no commit ahead of that baseline
that warranted a new cherry-pick. No rebase was needed.

To search for state combinations that ordinary corpus replay may not generate,
the native 5x52 Clang 22.1.7 ASan/UBSan build and the forced-int64/10x26
Clang 22.1.7 ASan/UBSan build each ran `field`, `group`, `scalar`,
`ecmult_const`, `ecmult_multi`, and `hash` with private copies of the tracked
corpora. Each invocation used:

```
-use_value_profile=1 -entropic=1 -reduce_inputs=0
-workers=2 -jobs=2 -max_total_time=45 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1
```

The 12 invocations requested 24 isolated libFuzzer jobs. All jobs exited 0;
there was no ASan/UBSan diagnostic, fuzzer assertion, timeout, OOM, or crash
artifact. The native backend executed 685/365 field, 627/618 group,
1710/1707 scalar, 371/376 ecmult-const, 163/73 ecmult-multi, and
30847/30999 hash units across its two jobs. The forced-int64 backend executed
638/352, 375/376, 758/718, 227/261, 33/33, and 59417/30900 respectively.
The differing rates reflect target cost and backend speed, not a behavioral
disagreement.

The forced-int64 `ecmult_multi` run emitted one `slow-unit` artifact for the
existing 22-byte input `pippenger window 1261\n`, corresponding to the tracked
`pippenger-window-1261` boundary seed. Replaying that exact artifact took
7.565 seconds, returned 0, and produced no diagnostic; it is an intentional
Pippenger-window stress case, not a new failure. All artifact directories were
otherwise empty, and no fuzz process remained after polling.

The first attempt at this pass was discarded because libFuzzer wrote its
`-jobs` child logs in the shared repository working directory, interleaving
streams from different targets. The corrected rerun changed only the runner's
working directory and retained isolated corpora and artifact prefixes; its
per-target logs show the expected 17, 20, 4, 6, 24, and 10 seed counts.

A separate 32-bit CMake configuration could not start: the host lacks the
32-bit C runtime objects (`Scrt1.o`, `crti.o`, `libc`) and headers
(`bits/libc-header-start.h`). This is an environment limitation, not a
32-bit code result. The value-profiled cross-backend pass found no new
clean-master production bug and does not change severity. Existing findings
remain **Medium** for malformed opaque state, callback failure, and secret
SHA-state lifetime; **Medium/latent** for the reachable 10x26 magnitude-32
arithmetic defect; **Low** for documented tweak-input overlap; and
**Informational** for cleanup/oracle-only checks. A nonce without cryptographic
meaning is not a Critical erasure finding. No production fix is claimed
without a clean-master reproduction or a minimal production mutation proof.

## 2026-07-17 Independent Arbitrary Field Product Oracle

The field fuzzer now checks arbitrary products derived with salts `307` and
`311` against the standalone 8x32-bit schoolbook-and-long-division model in
`src/fuzz/field.c`. The model does not call field multiplication, inversion,
or normalization helpers from the library under test. The production result
is checked for canonical operands, for the documented `r == a` aliasing case,
and again after both operands are raised by `7p` to magnitude 8 without
changing their field values. The focused corpus input is the exact ASCII
string `field product independent reference\n` in
`src/fuzz/corpora/field/product-independent-reference`.

Clean replay of the focused input and all 18 field corpus files passed on
native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan builds. A
two-worker, two-job value-profiled campaign over the same corpus used:

```
-use_value_profile=1 -entropic=1 -reduce_inputs=0
-workers=2 -jobs=2 -max_total_time=30 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1
```

Both jobs exited 0 on each backend, with no sanitizer diagnostic, assertion,
timeout, OOM, crash, or artifact. The first wrapper attempt stopped before
launching the target because its disposable artifact directory had not been
created; the corrected replay created it explicitly and passed.

The oracle's value was proven with a temporary production mutation, compiled
under `SECP256K1_FUZZ_MUTATE_PRODUCT` in `src/field_impl.h`: immediately after
`secp256k1_fe_impl_mul`, it flipped `r->n[0] ^= 1` only when the canonical
operand limbs exactly matched the focused seed's derived `x` and `y` values
(with backend-specific 5x52 and 10x26 constants). All 17 pre-existing field
inputs stayed green on both backends, while the focused input deterministically
exited through the fuzzer assertion on both. The mutation was removed before
the clean replay and is not a production finding; it proves that the new
independent product oracle detects a plausible arithmetic corruption that the
previous corpus/oracles did not bind.

The native sanitizer CTest suite then passed all 224 tests; the forced-int64
suite passed all 222 tests. No new clean-master production bug was found, so
this change is **Informational / oracle hardening**, not a severity finding.
Existing findings remain rated against unmodified master: **Medium** for
malformed opaque state, public callback failure, and secret SHA-state
lifetime; **Medium/latent** for the reachable 10x26 magnitude-32 arithmetic
defect; **Low** for documented tweak-input overlap; and **Informational** for
cleanup/oracle-only checks. A nonce without cryptographic meaning is not a
Critical erasure finding. No production fix or severity downgrade is claimed
without a clean-master reproduction or a minimal production mutation proof.

## 2026-07-17 Clean-Master Recheck After Master Refresh

After refreshing both remotes, `origin/master` is
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`; `codex/fuzz-oracles` is already a
descendant, so no rebase was required. The current `l0rinc/master` has no
commit ahead of this baseline that is not already represented in the branch,
so no additional cherry-pick was applied.

A disposable worktree at that exact origin baseline received only
`src/CMakeLists.txt` and `src/fuzz` from this branch. No production source or
fix commit was copied into it. The native Clang 22.1.7 ASan/UBSan build
replayed all 18 field inputs and all 6 `ecmult_const` inputs successfully.
The clean-master harness transplant has two known compatibility limits:
`context` and related targets refer to the newer branch-only
`SECP256K1_SHA256_MAX_SIZE` macro, and `ecmult_multi` refers to the newer
`checked_size_mul` helper. Those compile/link limits are not production
results and were excluded from the baseline classification. A disposable
forced-include header defining only the absent size macro then compiled the
remaining API/module targets without copying production fixes.

The following clean-master failures reiterate existing findings and were
verified against the repaired branch with the same focused inputs:

- `scalar/mul-shift-over-512` exits 134 on both backends. Native 5x52/4x64
  reports UBSan index 8 and an ASan stack-buffer-overflow at
  `scalar_4x64_impl.h:910`, reading past local `l[8]`; forced-int64/10x26
  reports the matching index 16 and overflow at `scalar_8x32_impl.h:707`.
  Both use the shift-513 boundary. This remains **Low/latent internal memory
  safety** because current in-tree callers use shift 384 and no public caller
  controls this helper domain. The unrelated clean-master WNAF diagnostics at
  `ecmult_impl.h:201` are not a second finding. The branch fix and both
  focused replays exit 0.
- The forced-int64 field replay exits 134 on its deterministic magnitude-32
  check before input-dependent product work, including for
  `field/product-independent-reference`. The old 10x26 `uint32_t` carry chain
  loses the valid maximum-magnitude contributions during normalization. This
  preserves the existing **Medium/latent 10x26 correctness** rating: the
  state is valid at the internal contract boundary, but no public path making
  this exact representation reachable has been demonstrated. The repaired
  forced-int64 `magnitude32-normalize` replay exits 0.
- `group/off-curve-opaque-pubkey` exits 134 at `src/fuzz/group.c:124` because
  clean-master `secp256k1_pubkey_load` accepts the storage encoding of
  `x = 1, y = 1`, even though the point is off curve. This is the existing
  **Medium opaque-state** finding: it requires corrupted or directly
  misused local opaque state, not a serialized wire input, but can otherwise
  let invalid group state cross API boundaries. The repaired branch rejects
  it through the illegal callback and zeroes failure outputs; its focused
  replay exits 0.
- `hash/hmac-independent-reference` reaches the independent output check on
  clean master, then exits 134 at `src/fuzz/hash.c:298` because
  `secp256k1_hmac_sha256_finalize` leaves the internal HMAC state live. This
  reiterates the **Medium secret-state lifetime** finding, not a demonstrated
  disclosure. A nonce or other public buffer without cryptographic meaning is
  not a Critical erasure finding. The repaired branch's focused replay exits
  0.
- The macro-compatible `context/sha256-impossible-lengths` replay produces an
  ASan heap-buffer-overflow while clean master hashes a `2^61`-byte tag from
  the fuzzer's short fallback pointer. This is the existing **Medium, low
  practical exploitability** impossible-length finding, not a new oracle
  failure. `ellswift/xdh-overflow-plus-one` separately reaches clean master's
  `main_impl.h:362` zero-`u` VERIFY guard after the fuzzer deliberately
  replaces a SHA callback state with zero. In a non-VERIFY build the same
  condition is the existing **Low** wrong-encoding edge case; the production
  fix maps zero `u` to one and retries.
- The remaining macro-compatible first stops are also previously classified:
  `schnorrsig/opaque-keypair-consistency` reaches the intentional
  `nonce_function_bip340(NULL, ...)` contract probe and ASan reports the
  existing **Medium** callback NULL write; `musig/off-curve-keyagg-cache`
  reaches the existing **Medium** noncanonical opaque-cache state; and the
  API, ECDH, recovery, and x-only seeds stop at their existing stale-output or
  inconsistent-opaque-state assertions. None is a new clean-master category.

These are baseline confirmations, not new defects introduced by the current
oracle work. The production fixes remain independently justified by their
named mutation proofs and deterministic tests; no severity is reduced merely
because a later or unrelated fix makes a replay pass.

## 2026-07-17 Affine X-Only Multiplication Oracle

`src/fuzz/ecmult_const.c` now includes a bounded independent affine model for
the x-only multiplication contract. It implements the ordinary secp256k1
slopes for doubling and addition directly with field operations, including
infinity and opposite-point transitions. It does not call `ecmult_const`,
`ecmult`, `ecmult_gen`, projective group-addition helpers, scalar recoding, or
precomputed tables. For every input it derives a scalar in `[1, 16]` with
salt `149`, computes the affine reference point, and checks
`secp256k1_ecmult_const_xonly` for both `known_on_curve` values with both the
direct `n` form and the equivalent `n/d` form. The existing arbitrary-scalar
cross-implementation checks remain in place; the small scalar bounds the
reference's variable-time inversions without weakening the random production
path coverage.

The exact six tracked `ecmult_const` corpus inputs passed in single-process
replays on native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan builds.
The native replay then ran two workers in two jobs, with value profiling,
entropic scheduling, and `-reduce_inputs=0`; both jobs requested 100 runs and
exited 0. The forced-int64 replay used the same settings with 50 runs per job;
both jobs exited 0. No sanitizer diagnostic, fuzzer assertion, timeout, OOM,
or crash artifact was produced. The normal `ecmult_const_tests` and
`noverify_tests.ecmult_const_tests` CTest entries also passed in both builds.
All worker corpora and artifacts were disposable copies; no generated corpus
files were added to the repository.

As a clean-master control, the same current harness was transplanted into a
disposable worktree at `origin/master` `11dad6d`. Its native 5x52 ASan/UBSan
two-worker, two-job replay requested 100 runs per job and exited 0 in both
jobs. The forced-int64/10x26 ASan/UBSan control requested 50 runs per job and
also exited 0 in both jobs. This control copied no production fix from the
audit branch, so it does not downgrade or mask any of the previously recorded
clean-master findings.

This is **Informational / oracle hardening**, not a production finding. The
new model provides a separate affine group-law path for future mutations of
projective multiplication and x-only denominator handling, but this campaign
did not reproduce a clean-master defect and therefore claims no production
fix or severity change. Existing findings remain rated against unmodified
master, including the Medium malformed-opaque-state and secret-SHA-state
issues, the Medium/latent 10x26 magnitude-32 defect, and the Low documented
tweak-input overlap. A nonce or other public buffer without cryptographic
meaning is not a Critical erasure finding.

## 2026-07-17 Independent Affine Ecmult-Multi Oracle

`src/fuzz/ecmult_multi.c` now checks the direct multi-scalar result against a
separate affine double-and-add model. The model implements secp256k1 point
addition and doubling with direct field slopes, and handles infinity,
doubling with `y == 0`, equal points, and opposite points explicitly. It does
not call `ecmult_multi`, `ecmult`, projective group-addition helpers, scalar
recoding, or precomputed multiplication tables. The point inputs retain the
existing generator-derived construction, but the accumulation path is now
independent from the production batch implementation; both no-scratch/simple
and 64 KiB/Strauss results are checked when the model is enabled.

The exact `ecmult_multi` corpus was replayed one input at a time on native
5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan builds. The six direct
batch and repeated-batch seeds, plus the full tracked corpus, passed after
the final 64 KiB gate change. Short value-profiled native and forced-int64
two-worker/two-job campaigns also exited 0. No sanitizer diagnostic, fuzzer
assertion, timeout, OOM, or crash artifact was produced. The existing
`ecmult_multi` unit tests and the dedicated scratch-wrap seed passed with the
branch guard enabled.

The clean-master control was checked separately at `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, with only the audit fuzzer overlay
and a disposable source-compatibility helper for the branch-only
`checked_size_mul` symbol. Both ASan/UBSan backends stopped before the affine
comparison on `ecmult_multi/sixteen-direct-batch`: the harness's unconditional
`SIZE_MAX` scratch-constructor probe triggered the existing
`base_alloc + size` wrap in clean `scratch_impl.h`. The native and int64
reports both show a 32-byte `memset` immediately past a 31-byte allocation.
This is a re-confirmation of the existing **Medium** clean-master finding,
fixed by `cac07e8`, not a defect in this oracle. The control therefore does
not downgrade the finding merely because the branch guard makes the replay
pass. A nonce or other public buffer without cryptographic meaning remains
non-Critical for erasure severity.

## 2026-07-17 MuSig Native and Int64 Worker Recheck

After the master and fork refresh, `fuzz_musig` was run against all 65 tracked
MuSig corpus inputs with `-jobs=2 -workers=2`, value profiling, entropic
scheduling, `-reduce_inputs=0`, `-rss_limit_mb=0`, and a 45-second per-job
budget. Native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan managers
completed cleanly; each worker replayed the 65-file corpus plus the
empty-input path and reported `Done 66 runs`. No sanitizer diagnostic,
assertion, timeout, OOM, crash artifact, or nonzero worker result was
observed.

This was a recheck of the existing state-machine and opaque-object oracles,
not a new production finding. The current-master severity ledger is
unchanged: existing findings remain rated against unmodified master, and a
public or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Independent Affine Group-Law Oracle

`src/fuzz/group.c` now checks generated Jacobian addition and doubling results
against a direct affine model using only field slopes. The reference handles
infinity, equal points, opposite points, and the `y == 0` doubling contract;
it does not use generator multiplication, Jacobian equality, or the
production addition/doubling routines to construct its expected coordinates.
The production `gej_add_var`, `gej_add_ge_var`, constant-time affine add, and
both doubling variants are each checked against the serialized affine
postcondition. The existing generator-derived points remain the input domain,
so the model tests actual valid state transitions rather than synthetic
off-curve objects.

The dedicated `group/affine-addition-reference` seed exercises `G + G`,
`G + (-G)`, `infinity + G`, and `G + infinity`. The complete 21-file group
corpus passed single-input native 5x52 and forced-int64/10x26 Clang 22.1.7
ASan/UBSan replays, including the new seed. For differential proof, a
disposable mutation normalized the ordinary Jacobian-addition x coordinate
and added one before producing the y coordinate. With only the older
production-equality/addition assertions bypassed for isolation, the focused
seed aborted with `run_rc=134` on both backends; the mutation and bypasses were
restored before fixed replay. This demonstrates that the new postcondition
rejects a representation-valid but mathematically wrong group result.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with the same fuzzer overlay and
no branch production fixes. Clean master first stopped at the existing
malformed opaque-pubkey finding and, after that control-only bypass, at the
existing `gej_rescale` scale-alias assertion. Bypassing those two known
stops in the disposable harness allowed the focused seed and all 21 group
inputs to pass on native and forced-int64 ASan/UBSan. No new clean-master
production defect or severity change was found. Existing findings remain
rated against unmodified master, and a nonce without cryptographic meaning is
not a Critical erasure finding.

## 2026-07-17 Public-Module Worker Recheck

The refreshed native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan
binaries ran `fuzz_api_roundtrip`, `fuzz_ecdh`, `fuzz_recovery`, and
`fuzz_schnorrsig` with two workers, value profiling, entropic scheduling,
`-reduce_inputs=0`, `-rss_limit_mb=0`, and a 25-second manager budget. Each
manager used a private copy of its tracked corpus: 44 API, 6 ECDH, 11
recovery, and 14 Schnorr inputs per backend. All eight managers exited 0 with
no sanitizer diagnostic, fuzzer assertion, timeout, OOM, or crash artifact;
the resulting corpus growth stayed outside the repository. The native and
forced-int64 `tests -t=ecdh -t=recovery -t=schnorrsig -i=4` subsets also
passed.

This is a negative recheck of the existing independent ECDSA, recovery,
ECDH-hash, Schnorr-equation, callback, parser, and state-cleanup oracles, not
a new production finding. The clean-master ledger remains authoritative:
severity is rated against unmodified master and is not lowered by a later
branch fix or by this campaign. A public or non-cryptographic nonce buffer is
not a Critical erasure finding.

## 2026-07-17 Independent Scalar WNAF Oracles

`src/fuzz/scalar.c` now computes generic WNAF directly from canonical big-endian
bytes, including the `n - scalar` high-bit normalization, window extraction,
carry, signed digit, and returned-length rules. It does not use production
scalar bit access, scalar multiplication, scalar addition, or another WNAF
implementation to construct the expected array. The same byte model checks the
129-entry `int8_t` Strauss wrapper after the low-128 split. A second model
recomputes fixed WNAF from independent low-128 byte windows, including skew,
the final short window, signed carry adjustment, and the adjacent `+/-1`
normalization rule. The existing production-arithmetic reconstruction and
digit-shape checks remain after these reference comparisons.

The new `scalar/wnaf-independent-reference` seed is 27 bytes and is replayed
alongside the four existing scalar seeds. Native 5x52 and forced-int64/10x26
Clang 22.1.7 ASan/UBSan builds each passed all five tracked files plus the
empty-input path (`Done 6 runs`). The final private-corpus worker campaign
used value profiling, entropic scheduling, `-reduce_inputs=0`, two workers,
two jobs, and a 10-second manager budget. Native jobs completed 320 and 321
units; forced-int64 jobs completed 174 and 178 units. All four managers and
workers exited 0 with no sanitizer diagnostic, assertion, timeout, OOM, or
artifact. Native and forced-int64 `tests -t=wnaf -i=4` and their no-VERIFY
counterparts also passed.

Two disposable production mutations prove that the new assertions matter.
First, `secp256k1_ecmult_wnaf` negated `wnaf[0]` for window 2 whenever a digit
was present. This preserves oddness, range, and spacing, while changing the
represented scalar. The old scalar-reconstruction equality was disabled only
for isolation; the focused seed aborted with raw `run_rc=134` on both
backends. Second, `secp256k1_wnaf_fixed` negated its first window for `w=2`,
and the old fixed-WNAF reconstruction equality was similarly disabled. The
same focused seed again aborted with `run_rc=134` on native and forced-int64.
All mutations and isolation bypasses were restored before the passing replay.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with the current harness and no
audit production fixes. The unmodified control first reproduced the existing
generic-WNAF signed-shift UB at `ecmult_impl.h:201` and the existing scalar
rounded-shift stack overflow at `scalar_4x64_impl.h:910` /
`scalar_8x32_impl.h:707`. These remain the previously recorded **Low**
generic-WNAF width issue and **Low/latent internal memory-safety** shift issue;
they are not new findings from this oracle. A second disposable control skipped
only WNAF window 31 and shifts above 512, allowing the five-file corpus to
complete 6 runs on both backends with no additional failure. No clean-master
production inconsistency or severity change was found. The existing findings
remain rated against unmodified master, and a public or non-cryptographic
nonce buffer is not a Critical erasure finding.

## 2026-07-17 Independent Field Addition Oracle

`src/fuzz/field.c` now checks arbitrary `fe_add` results against an independent
8x32-bit byte model. The model reduces two fuzz-derived 32-byte values with
standalone byte arithmetic, adds their little-endian words with carry, performs
the conditional modulus subtraction, and compares the normalized production
result. It also checks first-operand aliasing and adds two maximum-valid
magnitude-8 operands (representing `x + 7p` and `y + 7p`) before normalization,
so the postcondition covers the real `fe_add` magnitude contract rather than
only canonical inputs or production-derived equivalences.

The dedicated `field/add-independent-reference` seed is 32 bytes containing
`field add independent reference` followed by a newline. Native 5x52 and
forced-int64/10x26 Clang 22.1.7 ASan/UBSan replays passed all 19 tracked field
files plus the empty-input path (`Done 20` on each backend). Two-worker,
two-job value-profiled campaigns used private corpus copies; native workers
completed 139 and 141 units, while forced-int64 workers completed 131 and 139.
The field `tests` and `noverify_tests` slices both passed with `-i=4` in both
backends. No sanitizer diagnostic, assertion, timeout, OOM, or crash artifact
was produced.

The oracle's differential proof used a temporary production mutation directly
after `secp256k1_fe_impl_add`: `r->n[0] ^= 1`. The focused 32-byte seed aborted
with raw status 134 on both backends. Because the new reference is the first
operation in the harness, this proves the independent postcondition detects a
representation-valid but wrong field residue before later checks can mask it.
The mutation was removed and both binaries were rebuilt before the passing
replay. Existing tests did not provide this proof because their arbitrary-add
expectations were production-derived or metadata-focused; no separate modular
addition model bound the result to the field characteristic.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with only the current fuzzer
overlay. The focused seed passed the new add oracle on native 5x52. On clean
forced-int64/10x26, execution then stopped at the already-recorded
`secp256k1_fuzz_fe_check_magnitude32_reference` assertion in `field.c:83`,
called by the unconditional `(16, 16)` magnitude-boundary check. This is a
reconfirmation of the existing **Medium/latent internal field-correctness**
bug in clean-master 10x26 normalization, fixed on the audit branch by
`0d03dda`; it is not a new `fe_add` finding and does not lower its master-side
severity. No public key or signature trigger has been demonstrated. This
commit claims **Informational / oracle hardening**, no production fix, and no
severity change. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Independent Field Square Oracle

`src/fuzz/field.c` now checks direct `fe_sqr` output against the existing
standalone 8x32 modular multiplication model with both operands set to the
same independently reduced byte value. The oracle checks canonical input,
`r == a` aliasing, and a magnitude-8 input obtained by adding `7p`; expected
bytes never come from `fe_mul`, `fe_sqr`, square-root, or another production
field operation. This closes the gap where the old square checks only compared
implementation equivalences or checked a production-generated square.

The dedicated `field/square-independent-reference` seed is 35 bytes containing
`field square independent reference` followed by a newline. Native 5x52 and
forced-int64/10x26 Clang 22.1.7 ASan/UBSan replays passed all 20 tracked field
files plus the empty-input path (`Done 21` on each backend). Two-worker,
two-job value-profiled campaigns used private corpus copies; native workers
completed 144 and 138 units, while forced-int64 workers completed 136 and 131.
The VERIFY and no-VERIFY `-t=field -i=4` slices passed in both backends. No
sanitizer diagnostic, assertion, timeout, OOM, or crash artifact was produced.

The differential proof used a temporary mutation immediately after
`secp256k1_fe_impl_sqr`: `r->n[0] ^= 1`. The focused 35-byte seed aborted with
raw status 134 on native and forced-int64. The mutation was removed and both
binaries were rebuilt before the passing replay. Existing tests did not prove
this independently because their square checks compared production `fe_sqr`
to production multiplication or a production-derived square; they could not
bind a corrupted square to a separately computed field residue.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with only the current fuzzer
overlay. The focused square seed passed on native 5x52. Clean forced-int64/
10x26 then stopped at the already-recorded magnitude-32 normalization
assertion in `secp256k1_fuzz_fe_check_magnitude32_reference` (`field.c:83`),
after the new square check and the new addition check had completed. This is
the existing **Medium/latent internal field-correctness** master finding fixed
on the audit branch by `0d03dda`, not a new square defect and not a severity
downgrade. This commit claims **Informational / oracle hardening**, no
production fix, and no severity change. A public or non-cryptographic nonce
buffer is not a Critical erasure finding.

## 2026-07-17 Independent Scalar Inverse Oracle

`src/fuzz/scalar.c` now computes the scalar inverse independently as
`a^(n-2) mod n` using its standalone base-2^16 product and binary long-division
reduction model. Zero is explicitly mapped to zero, matching the internal
contract. The result is compared independently with both constant-time and
variable-time production inverse implementations before the existing
inverse-product and cross-implementation checks run, so a shared inversion
mistake cannot satisfy the old `a * inverse == 1` oracle.

The dedicated `scalar/inverse-independent-reference` seed is 37 bytes
containing `scalar inverse independent reference` followed by a newline. Native
5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan replays passed all six
tracked scalar files plus the empty-input path (`Done 7` on each backend).
Two-worker, two-job value-profiled campaigns used private corpus copies; native
workers completed 123 and 118 units, while forced-int64 workers completed 90
and 89. VERIFY and no-VERIFY `-t=wnaf -i=4` slices passed in both backends.
No sanitizer diagnostic, assertion, timeout, OOM, or crash artifact was
produced.

The differential proof applied the same temporary low-limb mutation after
`secp256k1_scalar_from_signed62`/`from_signed30` in both
`secp256k1_scalar_inverse` and `_var`: `r->d[0] ^= 1`. The focused 37-byte
seed aborted with raw status 134 on native and forced-int64. Mutating both
implementations together deliberately leaves their old equality check
consistent; the new exponentiation comparison rejects the shared wrong value
before the existing product identity check. All mutations were removed and
the restored binaries passed the corpus. Existing tests did not provide this
proof because they compared the two inverse implementations and checked only
the product identity, allowing a common algorithmic error to agree with both.

The unmodified clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with the current scalar harness.
Both native and forced-int64 runs stopped later at the existing generic-WNAF
signed-width UB (`ecmult_impl.h:201`) and scalar rounded-shift stack
out-of-bounds (`scalar_4x64_impl.h:910` / `scalar_8x32_impl.h:707`). These
remain the previously recorded **Low** generic-WNAF issue and **Low/latent
internal-memory-safety** shift issue, not inverse findings. A disposable
harness-only control skipped WNAF window 31 and shifts above 512; all six
corpus files plus the empty path then passed on both backends, confirming the
new inverse oracle completed without a clean-master mismatch. This commit
claims **Informational / oracle hardening**, no production fix, and no severity
change. A public or non-cryptographic nonce buffer is not a Critical erasure
finding.

## 2026-07-17 Independent GLV Scalar-Split Oracle

`src/fuzz/scalar.c` now recomputes `secp256k1_scalar_split_lambda` from the
documented GLV constants rather than deriving only the relation
`split1 + lambda * split2 == k`. The reference independently computes both
rounded products at shift 384, multiplies by `minus_b1` and `minus_b2`, adds
the two residues for `split2`, then computes `split1 = k - lambda * split2`
with the standalone base-2^16 product and binary-reduction model. Both
production outputs are compared to this reference before the existing
relation and signed-128-bit size checks. This prevents a shared wrong split,
wrong rounding boundary, swapped constant, or incorrect recombination from
agreeing with its own algebraic checks.

The dedicated `scalar/split-lambda-independent-reference` seed is 42 bytes
containing `scalar split lambda independent reference` followed by a newline.
Native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan replays passed the
focused seed and all seven tracked scalar files plus the empty-input path
(`Done 8` for each corpus directory, and `Done 2` for each empty run). Two
worker, two-job value-profiled campaigns used private corpus copies. Native
workers completed 117 and 118 runs; forced-int64 workers completed 89 and 89.
The full VERIFY and no-VERIFY test suites also passed in both backends: native
VERIFY 283.047 seconds, native no-VERIFY 90.097 seconds, forced-int64 VERIFY
405.141 seconds, and forced-int64 no-VERIFY 193.475 seconds. No sanitizer
diagnostic, assertion, timeout, OOM, or crash artifact was produced.

The differential proof inserted `r1->d[0] ^= 1` in the production
`secp256k1_scalar_split_lambda` path after its internal VERIFY relation check
and immediately before return. This models a corrupted returned split while
leaving the production self-check, old relation check, and signed-128-bit
size checks intact. The exact 42-byte seed aborted at the new exact-output
comparison with raw status 134 on both native and forced-int64. A first
placement before the production VERIFY check was rejected by that internal
check, so it was discarded and is not the recorded proof. The mutation was
removed and both branch binaries were rebuilt before the passing replays.
Existing tests did not provide this proof because they checked the split
relation and bounds, but did not independently bind the rounded GLV constants
or either exact output; a shared wrong split could satisfy those checks.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with the current scalar harness.
The focused seed reached the new split checks and then reconfirmed the
existing generic-WNAF signed-width UB at `ecmult_impl.h:201` and scalar
rounded-shift stack reads at `scalar_4x64_impl.h:910` /
`scalar_8x32_impl.h:707`. A disposable harness-only control skipped WNAF
window 31 and shifts above 512; all seven scalar files plus the empty path
then passed on both backends, proving no clean-master split mismatch was
hidden behind those later failures. These remain the previously recorded
**Low** generic-WNAF issue and **Low/latent internal-memory-safety** scalar
shift issue; they are not GLV findings and their severity is unchanged. This
commit claims **Informational / oracle hardening**, no production fix, and no
severity change. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Independent Arbitrary-Scalar `ecmult_const` Oracle

`src/fuzz/ecmult_const.c` now feeds the affine reference model the complete
canonical 256-bit scalar encoding and extracts bits directly from its bytes.
The model remains independent of the production projective formulas, GLV
split, signed-digit recoding, and x-only isomorphism code; it uses only the
existing affine group formulas and field operations to compute a point by
double-and-add. Both x-only input forms, the normal affine base and the
`numerator/denominator` fraction, are checked against this reference before
the older production-derived x-only comparisons. This replaces the former
independent model coverage limited to scalars 1 through 16.

The dedicated `ecmult_const/affine-arbitrary-scalar-reference` seed is 47
bytes containing `ecmult const affine arbitrary scalar reference` followed by
a newline. Native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan
replays passed all seven tracked `ecmult_const` files plus empty-input
execution (`Done 8` for each corpus directory and `Done 2` for each empty
run). Two-worker, two-job value-profiled campaigns used private corpus copies:
native workers completed 107 and 117 runs; forced-int64 workers completed 74
and 78. No sanitizer diagnostic, assertion, timeout, OOM, or crash artifact
was produced.

The differential proof temporarily inserted `r->n[0] ^= 1` immediately after
the final affine-map multiplication in production
`secp256k1_ecmult_const_xonly`. The focused 47-byte seed aborted at the new
independent comparison with raw status 134 on both backends; the old
production-derived x-only checks were later in the harness and did not have a
chance to be the first failure. The mutation was removed and both binaries
were rebuilt before the passing replays. Existing fuzz coverage did not prove
this because its independent affine check used only 1 through 16, while its
arbitrary-scalar x-only checks derived the expected point from production
implementations.

The clean-master control used `origin/master`
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` with the same harness overlay.
The focused seed and all seven corpus files plus empty input passed on native
and forced-int64 clean master. This is **Informational / oracle hardening**:
no production mismatch was found, no fix is claimed, and master-side severity
does not change. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Independent Distinct-Batch Pippenger Affine Oracle

The dedicated `ecmult_multi/distinct-pippenger-batches` fixture already forced
three distinct 88-point Pippenger batches, but its expected result still
multiplied each term with production `secp256k1_ecmult_const` and accumulated
with production Jacobian addition. The affine double-and-add model is now
reused for the same 264 scalar/point pairs and the complete result is compared
as serialized affine coordinates before that older expected-result check. The
reference therefore covers the batch dispatcher, callback offsets, Pippenger
bucket accumulation, and term multiplication with a separate affine path.

The existing seed is 34 bytes containing `distinct ecmult pippenger batches`
followed by a newline. Native 5x52 and forced-int64/10x26 Clang 22.1.7
ASan/UBSan replays passed all 24 tracked `ecmult_multi` files plus empty input:
25 runs completed in 13 seconds on native and 22 seconds on forced-int64.
Two-worker, two-job value-profiled campaigns used private corpus copies; all
four jobs returned zero, with no sanitizer diagnostic, assertion, timeout, OOM,
or artifact.

For the differential proof, `secp256k1_ecmult_pippenger_batch` was temporarily
changed to flip `r->x.n[0]` only after the third 88-point batch (`cb_offset ==
176`). The existing repeated-batch and direct checks remained unaffected, and
the new affine comparison precedes the production-derived comparison in the
distinct fixture. The exact 34-byte seed aborted with raw status 134 on both
backends. The mutation was removed and both branch binaries were rebuilt before
the restored replays. Existing tests and the prior fuzzer oracle did not prove
this because they either covered repeated points or used production
`ecmult_const` for the expected terms.

The raw clean-master replay reached the existing `SIZE_MAX` scratch-constructor
heap overflow on both backends. A disposable compatibility/control harness
then skipped that **Medium confirmed internal memory-safety** finding and the
already-ledgered callback partial-result and audit-boundary checks; the new
affine comparison, successful three-batch transcript, and callback stop trace
passed on clean native and forced-int64. No clean-master Pippenger arithmetic
mismatch was found, so this is **Informational / oracle hardening**, with no
production fix and no severity change. A public or non-cryptographic nonce
buffer is not a Critical erasure finding.

## 2026-07-17 Independent Affine Oracle for the Pippenger Window-9 Boundary

The existing gated `ecmult_multi/pippenger-window-1261` fixture reaches the
first 1,261-point window-9 Pippenger batch, but its expected value still sums
1,261 terms through production `secp256k1_ecmult_const` and Jacobian addition.
It now computes the same generator-derived points and scalars with the
independent affine double-and-add model and checks serialized affine
coordinates before the older result oracle. This binds the high-window bucket
accumulator and term path to a separate implementation while retaining the
existing callback transcript, scratch rollback, and failure-state checks.

The existing seed is 22 bytes containing `pippenger window 1261` followed by a
newline. Restored native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan
replays passed all 24 `ecmult_multi` corpus files plus empty input: 25 runs in
16 seconds and 26 seconds. Two-worker, two-job value-profiled campaigns used
private corpora. Native jobs completed 84 runs in 31 seconds and 83 in 121
seconds; forced-int64 jobs completed 36 and 35 in 32 and 31 seconds. The
longer native job was dominated by the explicit 1,261/4,421-point stress
inputs. No sanitizer diagnostic, assertion, timeout, OOM, or artifact remained.

For the differential proof, `secp256k1_ecmult_pippenger_batch` was temporarily
changed to flip `r->x.n[0]` only when `bucket_window == 9` and
`n_points == 1261`, immediately after the Pippenger result was formed. The
exact 22-byte seed aborted with raw status 134 on native and forced-int64; the
new affine comparison precedes the production-derived check. The mutation was
removed and both binaries were rebuilt before the restored corpus replay.
Existing tests and the prior window oracle did not prove this because their
expected point was derived through production multiplication.

The raw clean-master replay again reproduced the existing **Medium confirmed
internal memory-safety** scratch-constructor overflow on both backends. A
disposable clean control skipped that and stopped after the successful window-9
result, isolating the new comparison from the already-ledgered callback-failure
partial-output state. The affine result and callback success transcript passed
on clean native and forced-int64. No clean-master arithmetic mismatch was
found, so this is **Informational / oracle hardening**, with no production fix
or severity change. A public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Independent Byte-Total and Affine Oracle for the Window-10 Boundary

The existing gated `ecmult_multi/pippenger-window-4421` fixture uses 4,421
copies of G with scalar `2^60 + 1` and generator scalar 17. Its prior expected
point reduced the repeated sum with production scalar multiplication/addition
and one production `ecmult_const` call. The oracle now recomputes
`4421 * (2^60 + 1) + 17` with a standalone base-256 multiply-and-add, checks
that byte result against the production scalar result, and multiplies G by the
reference scalar with the independent affine model. The actual window-10
Pippenger result is compared to those serialized affine coordinates before the
older Jacobian/production-derived check, without performing 4,421 affine term
multiplications.

The existing seed is 22 bytes containing `pippenger window 4421` followed by a
newline. Restored native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan
replays passed all 24 `ecmult_multi` corpus files plus empty input: 25 runs in
16 seconds and 27 seconds. Two-worker, two-job value-profiled campaigns used
private corpora; all four jobs completed 25 runs and returned zero. No
sanitizer diagnostic, assertion, timeout, OOM, or artifact was produced; the
forced-int64 private logs only recorded the existing slow-unit marker for the
window-9 seed.

For the differential proof, `secp256k1_ecmult_pippenger_batch` was temporarily
changed to flip `r->x.n[0]` only when `bucket_window == 10` and
`n_points == 4421`, immediately after Pippenger accumulation. The exact 22-byte
seed aborted with raw status 134 on native and forced-int64. The new byte-total
and affine checks precede the old expected-result check. The mutation was
removed and both binaries were rebuilt before the restored corpus replay.
Existing tests and the prior window-10 oracle did not independently bind the
repeated-term total or the curve result.

The raw clean-master replay reproduced the existing **Medium confirmed internal
memory-safety** scratch-constructor overflow on both backends. A disposable
clean control skipped that and stopped after the successful window-10 result,
isolating the new checks from the already-ledgered callback-failure
partial-output state. The base-256 total, affine result, and callback-success
transcript passed on clean native and forced-int64. No clean-master Pippenger
or scalar-total mismatch was found, so this is **Informational / oracle
hardening**, with no production fix or severity change. A public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Independent Affine Oracle for X-Only Tweak-by-2

The `xonly_tweak/affine-reference` fixture adds a 29-byte gated transcript,
`xonly tweak affine reference` followed by a newline. It reconstructs the
even-Y point represented by the fuzzed x-only key with byte-level field
multiply, reduction, and square-root helpers, doubles an explicit byte-level
generator, and computes `P_even + 2G` with affine group addition. The
compressed result is compared with both `secp256k1_xonly_pubkey_tweak_add` and
`secp256k1_keypair_xonly_tweak_add` followed by public-key extraction.

The byte subtraction, multiplication, square root, point doubling, and point
addition are separate from the production group representation and bind the
result to an explicit curve equation. To keep this one dedicated trigger
bounded, its modular inverse uses `secp256k1_fe_inv_var`; this is an explicit
model boundary, not a claim of an independently implemented field inverse.
Routine inputs do not pay this cost. The oracle is therefore strongest against
wrong-but-valid tweak results and cross-API inconsistencies, while sanitizer
instrumentation remains responsible for memory and undefined-behavior bugs.

The differential proof used a temporary mutation in
`src/modules/extrakeys/main_impl.h`: for the exact 32-byte tweak `02`,
`secp256k1_xonly_pubkey_tweak_add` was made to pass scalar `01` to its existing
production helper. The exact command
`fuzz_xonly_tweak -runs=1 -detect_leaks=0 -rss_limit_mb=0 -timeout=30
src/fuzz/corpora/xonly_tweak/affine-reference` aborted with raw status 77 at
the new `FUZZ_CHECK`, without an illegal-callback diagnostic. Thus the oracle
detects a valid, serialized `P + G` result where the contract requires
`P + 2G`; the mutation was removed before the restored replay. Existing tests
and the prior fuzzer checks did not prove this because they did not bind this
fixed scalar to an independent group equation and could share the production
path.

Restored Clang 22.1.7 ASan/UBSan replays passed the 14 existing x-only corpus
files plus this fixture in 9.88 seconds on native 5x52 and 10.16 seconds on
forced-int64/10x26. The focused native replay took 0.84 seconds. Branch
`tests` and `noverify_tests` passed; clean-master `tests` also passed.

The clean `origin/master` snapshot was `11dad6d`, but it predates the CMake
fuzz-target wiring and has no `fuzz_xonly_tweak` binary; the fuzz options were
reported unused, so a raw clean-master replay of this new harness is not
possible. The clean test suite is the available control and found no
production mismatch. This is **Informational / oracle hardening**, with no
production fix and no severity finding. A public or non-cryptographic nonce
buffer is not a Critical erasure finding.

## 2026-07-17 Independent Affine Oracle for MuSig Key Aggregation

The `musig/keyagg-affine-reference` fixture adds a 30-byte transcript,
`MuSig keyagg affine reference` followed by a newline. It aggregates the
explicit points G and 2G. The harness hook forces the hashed first KeyAgg
coefficient to one, while MuSig's second-distinct-key rule supplies one for
2G, making the expected weighted sum exactly `G + 2G`. A separate byte-level
affine model computes that addition and compares both the full compressed
point returned by `secp256k1_musig_pubkey_get` and the x-only aggregate.

The byte subtraction, multiplication, and group equation are independent of
the production Jacobian and `ecmult_multi` representations. As with the
other bounded affine references, the one modular inverse uses
`secp256k1_fe_inv_var` to keep the dedicated trigger fast; this is documented
as a model boundary rather than an independently implemented inverse.

The prior MuSig key-aggregation references independently recomputed the
transcript and coefficient hashes, but built their expected weighted point
with production `secp256k1_ec_pubkey_tweak_mul` and
`secp256k1_ec_pubkey_combine`. They could therefore agree with a shared group
or batch-accumulation error. The new postcondition binds the callback result
to an independently computed affine point.

For causal proof, a temporary mutation in
`src/modules/musig/keyagg_impl.h` set `sc` to zero after the callback computed
the coefficient when `idx == 1`. On the dedicated trigger this changes the
aggregate from `G + 2G` to `G`. A temporary trigger-only early return skipped
the older production-derived checks so the failure was attributable to the
new oracle. Native 5x52 and forced-int64/10x26 Clang 22.1.7 ASan/UBSan
replays both exited with raw libFuzzer status 77 at the new assertion, with
no illegal-callback diagnostic. The mutation and isolation return were
removed before restored replay.

Restored native and forced-int64 focused replays passed in 1.15 and 1.95
seconds. The 66-file corpus plus empty input completed 67 runs in 81.5 and
140.7 seconds. Private two-job/two-worker campaigns completed two 67-run jobs
per backend in about 82 and 140 seconds; all workers returned zero with no
sanitizer diagnostic, assertion, timeout, OOM, or artifact. Clean-master
MuSig tests (`tests -t=musig -i=1`) passed, but clean `origin/master`
`11dad6d` has no `fuzz_musig` target because its fuzz CMake wiring predates
this harness, so a raw clean-master fuzzer replay is unavailable.

No clean-master production mismatch was confirmed. This is **Informational /
oracle hardening**, with no production fix or severity finding. Severity
continues to be rated against unmodified master; a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Fixed Generator Algebraic Oracle for Schnorr Verification

The `schnorrsig/generator-equation` fixture adds a 30-byte transcript,
`schnorrsig-generator-equation` followed by a newline. It uses the known
compressed-even generator x-coordinate as both the signature's `r_x` and the
x-only public key, with a fixed 32-byte zero message. The standalone BIP340
tagged-hash reference computes `e = H_challenge(r_x || P_x || m) mod n`, and a
byte-level scalar add-one reference constructs `s = (e + 1) mod n`.

This is a valid signature by the algebraic identity `sG - eP = (e + 1)G -
eG = G`: the reconstructed nonce is exactly the even-Y generator, so its
x-coordinate is `r_x`. The expected validity therefore does not come from
the signing implementation, a production scalar tweak, or a production
point-combine result. The x-only parse remains a public API setup boundary;
the oracle's independent contract is the challenge reduction, scalar
addition, and equation. It complements the existing arbitrary and generated
signature references, whose expected public points still use production
public-key operations.

For causal proof, a temporary trigger-only early return skipped the older
production-derived checks. A temporary mutation in
`src/modules/schnorrsig/main_impl.h` then returned zero after argument checks
only when `sig64[0..7]` equaled `79be667ef9dcbbac`, `msglen` was 32, and all
32 message bytes were zero. With `-handle_abrt=0`, the exact new seed aborted
with status 134 on native 5x52 and forced-int64/10x26, while a control corpus
containing all 14 pre-existing Schnorr files completed 15 executions with
status 0 on both backends. An initial two-byte prefix mutation also matched
the pre-existing fixed-nonce path; it was discarded before this final
measurement. No illegal-callback diagnostic was produced. The mutation and
isolation return were removed before the restored replay.

Restored Clang 22.1.7 ASan/UBSan replays passed all 15 corpus files plus the
empty execution: 16 runs in 2.17 seconds on native 5x52 and 3.73 seconds on
forced-int64/10x26. Four private `-fork=2` jobs, two per backend, loaded the
15 corpus files and exited 0 in about 5.5 seconds per native job and 6.8
seconds per forced-int64 job. No sanitizer diagnostic, assertion, timeout,
OOM, or artifact was produced. Branch `tests` and `noverify_tests` passed the
Schnorr subset, as did clean-master `tests`.

Clean `origin/master` was `11dad6d`; it predates the CMake fuzz-target wiring,
so the disposable clean build had no `fuzz_schnorrsig` target and could not
replay this harness. No clean-master production mismatch was confirmed. This
is **Informational / oracle hardening**, with no production fix or severity
change. Severity remains master-relative; clearing a public or
non-cryptographic nonce buffer is not a Critical finding.

## 2026-07-17 Independent ECDH Generator-Two Oracle

The `ecdh/generator-2g` fixture adds an 18-byte transcript,
`ecdh-generator-2g` followed by a newline. It creates the public generator
with scalar one, invokes ECDH with scalar two and a coordinate passthrough
callback, and checks the returned x-coordinate against the fixed canonical
`2G` x-coordinate. It then checks the returned y-coordinate's even parity and
the independent byte-field equation `y^2 = x^3 + 7`. The default ECDH output
is separately checked against a standalone SHA-256 reference over the fixed
compressed-even `2G` encoding.

The prior ECDH oracle compared the arbitrary shared point against
`secp256k1_ec_pubkey_tweak_mul`, so a shared constant-time multiplication bug
could have made both sides agree. The new fixed postcondition does not obtain
the expected point from that API or from a production group representation.
Its byte-field model uses the existing standalone modular add/multiply helper;
the fixed x-coordinate and even root contract make the expected point
unambiguous without copying the production y-coordinate.

For causal proof, a temporary trigger-only return skipped the older random
ECDH checks. A temporary mutation in `src/modules/ecdh/main_impl.h` replaced
the exact scalar-two state with scalar one after scalar parsing. With
`-handle_abrt=0`, the dedicated seed aborted with status 134 on native 5x52
and forced-int64/10x26, while all six pre-existing ECDH corpus files completed
seven executions with status 0 on both backends. The mutation and isolation
return were removed before restored replay.

Restored Clang 22.1.7 ASan/UBSan replays passed all seven corpus files plus
the empty execution: eight runs in 0.61 seconds on native 5x52 and 1.06
seconds on forced-int64/10x26. Four private `-fork=2` jobs, two per backend,
completed with status 0 in about 4.3 seconds per native job and 4.6 seconds
per forced-int64 job. No sanitizer diagnostic, assertion, timeout, OOM, or
artifact was produced. Branch `tests` and `noverify_tests` passed the ECDH
subset, as did clean-master `tests`.

Clean `origin/master` was `11dad6d`; its older CMake fuzz configuration has
no `fuzz_ecdh` target, so a raw clean-master fuzzer replay was unavailable.
No clean-master production mismatch was confirmed. This is **Informational /
oracle hardening**, with no production fix or severity change. Severity remains
master-relative; clearing a public or non-cryptographic nonce buffer is not a
Critical finding.

## 2026-07-17 Fixed Generator-Two Known-Answer Vector for `ecmult_const`

The `ecmult_const/generator-2g` fixture adds the 26-byte transcript,
`ecmult-const-generator-2g` followed by a newline. It invokes the direct
internal `secp256k1_ecmult_const` entry point on the fixed generator with
scalar two and compares the normalized result with the fixed canonical
`2G` x-coordinate
`c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5` and its
even-Y parity. The expected point is fixed known-answer data; it is not
constructed through `ecmult_gen`, `ecmult`, a public tweak operation, or the
fuzzer's affine model.

This is intentionally a separate internal-path check from the ECDH
generator-two vector. The ECDH fixture validates the public ECDH callback and
hash boundary; this fixture binds the constant-time multiplication
implementation itself. The existing affine oracle uses a production
generator-derived base for its arbitrary scalar, so a shared generator-output
mistake could otherwise make that model agree with the multiplication under
test.

For causal proof, a temporary mutation in `src/ecmult_const_impl.h` replaced
the result with valid point `G` only when the exact base pointer was
`&secp256k1_ge_const_g` and the scalar equaled two. A trigger-only return
skipped the older random checks. The new seed aborted with status 134 on
native 5x52 and forced-int64/10x26, while all seven pre-existing
`ecmult_const` seeds completed with status 0 on both backends. Because the
replacement remains a valid curve point, the failure is attributable to the
fixed coordinate/parity postcondition rather than an internal validity check.
The mutation and isolation return were removed before restored replay. An
initial replay also caught and fixed a harness-side missing normalization
before calling `secp256k1_fe_is_odd`; that precondition error was not used as
proof.

Restored Clang 22.1.7 ASan/UBSan replays passed all eight corpus files plus
the empty execution: nine runs on each backend. Private `-fork=2` campaigns
completed three jobs in about 11 seconds on native and forced-int64, with zero
OOM/timeout/crash counts and no artifact. Branch `tests`, `noverify_tests`,
and the forced-int64 EC test slice passed; clean-master `tests -t=ec -i=1`
also passed.

Clean `origin/master` was `11dad6d`; its older CMake fuzz configuration has
no `fuzz_ecmult_const` target, so a raw clean-master fuzzer replay was
unavailable. No clean-master production mismatch was confirmed. This is
**Informational / oracle hardening**, with no production fix or severity
change. Severity remains master-relative; clearing a public or
non-cryptographic nonce buffer is not a Critical finding.

## 2026-07-17 Fixed Generator Vector for Recoverable ECDSA

The `recovery/generator-vector` fixture adds a 26-byte transcript,
`recovery-generator-vector` followed by a newline. It encodes the fixed
recoverable signature `(r, s, recid) = (x(G), x(G), 0)` and a zero message
hash, then requires recovery to serialize the known even-Y generator. The
compact signature round trip is checked separately so the expected point
comparison cannot be satisfied by a serialization-only mutation.

The vector is valid by the recovery identity
`Q = r^-1 (sR - zG) = x(G)^-1 (x(G)G - 0G) = G`: recid zero selects the
generator's even-Y point, and `x(G)` is below the group order. The expected
point is fixed compressed wire data; no production tweak multiplication,
public-key combine, or recovery equation helper constructs it. This is
complementary to the existing arbitrary recovery equation, whose expected
terms still use production public-key operations.

For causal proof, a temporary trigger-only return skipped the older random
recovery checks. A temporary mutation in
`src/modules/recovery/main_impl.h` set the recovered Jacobian point to
infinity after `ecmult` only when `recid == 0`, the message scalar was zero,
and `r == s`. With `-handle_abrt=0`, the exact seed aborted with status 134
on native 5x52 and forced-int64/10x26, while all 11 pre-existing recovery
corpus files completed 12 executions with status 0 on both backends. The
mutation and isolation return were removed before restored replay.

Restored Clang 22.1.7 ASan/UBSan replays passed all 12 corpus files plus the
empty execution: 13 runs in 0.96 seconds on native 5x52 and 1.62 seconds on
forced-int64/10x26. Four private `-fork=2` jobs, two per backend, completed
with status 0 in about 4.6 seconds per native job and 5.3 seconds per
forced-int64 job. No sanitizer diagnostic, assertion, timeout, OOM, or
artifact was produced. Branch `tests` and `noverify_tests` passed the
recovery subset.

Clean `origin/master` was `11dad6d`; its older CMake fuzz configuration has
no `fuzz_recovery` target, and the disposable clean build has no `recovery`
test target because that module is disabled there. A raw clean-master fuzz or
recovery-test replay was therefore unavailable. No clean-master production
mismatch was confirmed. This is **Informational / oracle hardening**, with no
production fix or severity change. Severity remains master-relative; clearing
a public or non-cryptographic nonce buffer is not a Critical finding.

## 2026-07-17 Fixed Decoded-Point XDH Vector for EllSwift

The `ellswift/xdh-fixed-decode` fixture adds the 26-byte transcript,
`ellswift-xdh-fixed-decode` followed by a newline. It reuses the independently
known BIP324 ElligatorSwift wire value already checked by the fixed decode
vector, passes that value as both parties' encoding, and uses scalar one with
the passthrough X callback. The callback output must equal the fixed decoded
point x-coordinate `948b40e7181713bc018ec1702d3d054d15746c59a7020730dd13ecf985a010d7`.
The expected bytes come from fixed compressed wire data, not from
`ec_pubkey_tweak_mul`, `ecmult`, or a production public-key comparison.

This adds a distinct postcondition for `xswiftec_frac_var` plus the x-only
multiplication path. The earlier raw-party oracle compared arbitrary XDH
output with production public-key multiplication, and the earlier fixed
vector checked decode serialization only; both could miss an XDH-only
regression that agreed with a shared production reference or affected only
the fraction loader.

For causal proof, a temporary mutation flipped the final byte of the XDH
coordinate after multiplication only when `party == 0`, the effective scalar
was one, and the remote fixed encoding had prefix
`c5981bae27fd8440` and final byte `78`. A trigger-only return skipped the
older random checks. With `-handle_abrt=0`, the new seed aborted with status
134 on native 5x52 and forced-int64/10x26, while the 14 pre-existing EllSwift
seeds completed with status 0 on both backends. The mutation and isolation
return were removed before restored replay.

Restored Clang 22.1.7 ASan/UBSan replays passed all 15 EllSwift corpus files
plus the empty execution: 16 runs in about 2 seconds on native 5x52 and 4
seconds on forced-int64/10x26. Private `-fork=2` campaigns completed three
jobs in about 13 seconds native and 14 seconds forced-int64, with zero
OOM/timeout/crash counts and no artifact. Branch `tests`, `noverify_tests`,
and the forced-int64 EllSwift test subset passed; clean-master EllSwift tests
also passed.

Clean `origin/master` was `11dad6d`; its older CMake fuzz configuration has
no `fuzz_ellswift` target, so a raw clean-master fuzzer replay was unavailable.
No clean-master production mismatch was confirmed. This is **Informational /
oracle hardening**, with no production fix or severity change. Severity remains
master-relative; clearing a public or non-cryptographic nonce buffer is not a
Critical finding.

## 2026-07-17 BIP327 MuSig Nonce-Generation Known-Answer Vector

The `musig/bip327-nonce-gen-vector` fixture adds the exact 30-byte transcript
`MuSig BIP327 nonce-gen vector` followed by a newline. It uses the authoritative
BIP327 nonce-generation case with a session secret of 32 bytes of `0x0f`, no
secret key, no message, no key-aggregation cache, no extra input, and the fixed
compressed public key `2G`:
`02f9308a019258c31049344f859d5229b531c845836f99b08601f113bce036f9`.
The oracle checks the fixed secret nonce scalars
`89bdd787d0284e5e4d5fc572e49e316bab7e21e3b1830de37dfe80156fa41a6d` and
`0b17ae8d024c53679699a6fd7944d9c4a366b514baf43088e0708b1023dd2897`,
and the serialized public nonce
`02c96e7cb1e8aa5dac64d872947914198f607d90ecde5200de52978ad5ded63c00` ||
`0299ec5117c2d29edee8a2092587c3909be694d5cff0667d6c02ea4059f7cd9786`.
The oracle also checks that the consumed session-random buffer is zeroed. The
fixed public nonce is compared as wire data rather than being regenerated by
the fuzzer's existing public-key reference. The scalar
slots of the opaque secret-nonce object are checked at the same fixed-vector
boundary used by the module's existing test, and the harness clears that
cryptographic secret object after checking it.

This is distinct from the existing optional-secret-key and standalone nonce
reference checks: those derive expected public nonce points through production
public-key creation, while this fixture binds the complete BIP327 transcript to
known answers. It also exercises the public API path where the signer supplies
the public key without a secret key.

For causal proof, a temporary mutation in `src/modules/musig/session_impl.h`
flipped the final byte of the first derived hash buffer before scalar reduction
only when the fixed `0x0f` session-random, no-optional-input, `2G` transcript was
present. A temporary trigger-only return isolated the new check from the older
random checks. With `-handle_abrt=0`, the exact seed aborted with status 134 on
native 5x52 and forced-int64/10x26; all 66 pre-existing MuSig corpus files
completed with status 0 on both backends. The mutation and isolation return
were removed before the restored replay.

The restored Clang 22.1.7 ASan/UBSan replay passed all 67 MuSig corpus inputs
on both backends. Private `-fork=2 -max_total_time=10` campaigns exited 0 in
about 92 seconds on native and 159 seconds on forced-int64, with zero
OOM/timeout/crash counts and no artifacts. Branch `tests`, `noverify_tests`,
and forced-int64 `tests` passed the MuSig subset; clean `origin/master` at
`11dad6d` passed its MuSig test subset. The clean-master build exposes no raw
MuSig fuzzer target. No clean-master production mismatch was confirmed. This
is **Informational / oracle hardening**, with no production fix or severity
change. Severity remains master-relative; a public or otherwise
non-cryptographic nonce buffer does not make a clearing issue Critical.

## 2026-07-17 Full Corpus and Worker Recheck

After the master refresh, `origin/master` remained
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, and `codex/fuzz-oracles` remained
its descendant. The fetched `l0rinc/master` and pull-request heads contain no
additional commit that is not already represented by this branch, so no rebase
or extra cherry-pick was needed for this pass.

All 275 tracked corpus files were replayed from disposable copies, one target
at a time, with Clang 22.1.7 ASan/UBSan builds in both native 5x52 and forced
int64/10x26 configurations. The target/file counts were: `api_roundtrip` 44,
`context` 11, `hash` 10, `scalar` 7, `field` 20, `group` 21,
`ecmult_const` 8, `ecmult_multi` 24, `ecdh` 7, `ellswift` 15,
`xonly_tweak` 14, `recovery` 12, `schnorrsig` 15, and `musig` 67. Every
replay exited 0; no assertion, sanitizer diagnostic, timeout, OOM, or
artifact was produced. A second pass ran each target with
`-fork=2 -max_total_time=5 -timeout=15 -rss_limit_mb=0` on both backends.
All 28 worker jobs exited 0 and produced no artifacts. The worker pass is
evidence that the current oracles remain stable under parallel execution; it
is not treated as proof that random exploration found every state.

This recheck found no new production defect and no severity change. The
existing findings are still rated against unmodified master, even where a
later branch fix or unrelated change causes a replay to pass:

- **Medium**: confirmed internal scratch-allocation wraparound; malformed or
  off-curve opaque group/key-aggregation state; public callback failure paths
  that can expose invalid or stale state; and secret SHA/HMAC state lifetime
  until the relevant state is explicitly cleared. These are not promoted to
  Critical without a master-reachable security impact.
- **Medium/latent**: forced-int64/10x26 magnitude-32 normalization overflow
  and impossible SHA-length handling. The first is a real internal arithmetic
  contract failure; the second has low practical reachability. Neither is
  downgraded because the audit branch contains a fix.
- **Low/latent**: scalar shift-above-width reads and generic WNAF signed-width
  behavior; EllSwift zero-`u` wrong-encoding behavior in non-VERIFY builds;
  documented tweak-input overlap; and partial results after a later
  `ecmult_multi` callback failure where the API contract permits the caller to
  observe them.
- **Informational**: fixed vectors, affine/product equations, cleanup checks,
  and other oracle-only hardening whose clean-master mutation controls did not
  demonstrate a production mismatch. Clearing a public or otherwise
  non-cryptographic nonce buffer is not a Critical erasure finding.

No production mutation was used to claim a new issue in this recheck. The
previous mutation-backed findings and deterministic tests remain the required
proof for the production fixes; no finding is considered closed merely because
a minor follow-up commit masks the triggering state.

## 2026-07-17 ECDSA Verification-Infinity Oracle

Coverage of the 44 pre-existing `api_roundtrip` corpus inputs did not reach
the explicit `secp256k1_ecdsa_sig_verify` identity rejection at
`src/ecdsa_impl.h:215`. The new gated input
`api_roundtrip/ecdsa-verification-infinity` is the exact ASCII string
`ecdsa verification infinity` followed by a newline. It constructs the
generator `Q = G`, compact scalars `r = 1` and `s = 1`, and the valid message
scalar `z = n - 1`. Verification therefore computes
`u1*G + u2*Q = (-1)*G + 1*G = infinity`; the signature is parseable and low-S,
so this reaches the identity transition rather than an earlier scalar guard.
The assertion requires the public verifier to return zero.

For causal proof, the production guard was temporarily changed from
`if (secp256k1_gej_is_infinity(&pr))` to `if (0 &&
secp256k1_gej_is_infinity(&pr))`. The exact seed aborted with status 134 at
`src/group_impl.h:421` on native 5x52 and forced-int64/10x26 Clang 22
ASan/UBSan builds. All 44 pre-existing API inputs stayed green under the same
mutation on both backends. The mutation was restored before the fixed replay.

The restored target passed all 45 API corpus inputs, including the new seed,
on both sanitizer backends; the exact focused seed also passed independently.
The coverage replay recorded the infinity branch as taken twice (and the
non-infinity path 996 times) in `ecdsa_impl.h`, making the new state transition
visible in the report rather than merely relying on the mutation result.
Clean `origin/master` at `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` already has
the production rejection, so this is **Informational / Low master-relative
oracle hardening**, not a new production vulnerability or fix. It is not
downgraded or upgraded by later audit commits, and no nonce-erasure claim is
involved: a public or otherwise non-cryptographic nonce buffer is not a
Critical finding.

## 2026-07-17 l0rinc PR 16 and RFC6979 Ref Reconciliation

The latest `git fetch --all --prune` exposed
`l0rinc/l0rinc/field-10x26-normalize-overflow` at `b938a5d` (PR 16) and
`l0rinc/l0rinc/rfc6979-reject-max-counter` at `7b47f1f`. Neither requires a
rebase or an additional cherry-pick on `codex/fuzz-oracles`:

- PR 16 repairs 64-bit carry propagation in 10x26 normalization. Its patch
  is not byte-identical to this branch's `0d03dda` normalization repair and
  `0346c09` zero-predicate repair, but those commits cover the same boundary
  with a broader independent byte/reference oracle and a mutation-backed
  zero-predicate seed. The clean-master issue remains **Medium/latent** and
  is not considered absent merely because either patch makes a later replay
  pass.
- The RFC6979 branch adds the maximum-counter rejection already represented by
  `6fa1dbc`, which also clears failure output and has the dedicated
  `api_roundtrip/rfc6979-counter-max` corpus input. The public callback's
  output is not cryptographic secret state; its clearing is stale-output
  hygiene and is not Critical severity.

The exact patch IDs differ because the audit commits retain the current-master
behavioral context and add stronger proofs. These fork refs were compared as
discovery-order evidence, not used to claim that clean master was safe. Any
future fork fix that changes the triggering behavior must be recorded beside
the affected finding and replayed with the original master-relative mutation.

## 2026-07-17 ECDSA Finite X-Mismatch Oracle

The new gated input `api_roundtrip/ecdsa-verification-x-mismatch` is the exact
ASCII phrase `ecdsa verification x mismatch` followed by one LF byte. It uses
the same parseable low-S compact signature `r = s = 1` as the identity vector,
but sets `Q = G` and `z = 1`. The verifier consequently computes `R = 2G`.
The fixed generator vector has `x(2G) =
c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5`, which is
neither `r` nor `r + n`; the public verifier must return zero after both
finite x-coordinate comparisons fail. This is independent of the fuzzer's
arbitrary-signature reference and reaches a different path from the
infinity-rejection vector.

For causal proof, the final `return 0` in `secp256k1_ecdsa_sig_verify` was
temporarily changed to `return 1` only after the `x` and `x + n` comparisons.
The exact seed aborted with status 134 on native 5x52 and forced-int64/10x26
Clang 22 ASan/UBSan builds, while all 45 earlier API inputs stayed green under
the same mutation on both backends. The mutation was restored before replay.
The restored 46-input API corpus passed on both sanitizer backends, and the
coverage report recorded the final mismatch return once.

Clean `origin/master` already contains the same final rejection, so this is
**Informational / Low master-relative oracle hardening**, not a production
bug or fix. The existing severity ledger remains unchanged; a public or
otherwise non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Full 277-Input Corpus and Worker Recheck

At `f275f89b824c17bf11c047205299efd49d1dfd7e`, `origin/master` was still
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`; `git rev-list --left-right
--count origin/master...HEAD` was `0 583`, so no rebase was needed. The latest
fetched `l0rinc/master` and pull-request heads still contain no unrepresented
commit requiring another cherry-pick.

The current Clang 22.1.7 ASan/UBSan builds replayed every tracked corpus file
once on both native 5x52 and forced-int64/10x26. The 277-file inventory was:
`api_roundtrip` 46, `context` 11, `hash` 10, `scalar` 7, `field` 20,
`group` 21, `ecmult_const` 8, `ecmult_multi` 24, `ecdh` 7, `ellswift` 15,
`xonly_tweak` 14, `recovery` 12, `schnorrsig` 15, and `musig` 67. Every
file-level replay exited 0 without an assertion, sanitizer diagnostic, OOM,
or crash artifact.

The worker campaign used private corpus copies and
`-fork=2 -jobs=2 -max_total_time=5 -timeout=15 -rss_limit_mb=0` for all 14
targets on both backends. Twenty-seven managers completed in the concurrent
matrix. Forced-int64 MuSig was rerun alone after the first 180-second outer
wall bound expired under CPU contention; both workers then exited 0 after 158
and 160 seconds, with `oom/timeout/crash: 0/0/0` and no sanitizer diagnostic
or artifact. The completed matrix plus that isolated rerun is 28 managers
and 56 workers with zero worker failures.

The concurrent run's default artifact path was shared by all managers and
left one `timeout-*` file containing the existing `pippenger window 1261`
seed. Direct `-runs=1` replays of that exact seed exited 0 in 7.456 seconds
native and 11.740 seconds forced-int64 under the same 15-second input limit,
with no artifact. The file was therefore a run-directory collision, not a
reproducible library timeout or production finding, and was removed before
committing this record.

Native and forced-int64 `tests` and `noverify_tests` all passed the `-t=ec
-i=1` and `-t=musig -i=1` slices. This recheck found no new production defect
and no master-relative severity change. Existing mutation-backed findings
remain rated against unmodified master even where a later branch change masks
their trigger. A public or otherwise non-cryptographic nonce buffer does not
make a clearing issue Critical.

## 2026-07-17 ECDSA DER Long-Form Success Oracle

The `api_roundtrip` target now has the gated
`api_roundtrip/ecdsa-der-long-form-success` input, whose exact bytes are the
ASCII phrase `ecdsa DER long-form success` followed by one LF byte. The helper
constructs a 132-byte DER sequence `30 81 81`: its 129-byte payload contains a
positive 124-byte `r` INTEGER and the one-byte `s = 1` INTEGER. The outer
length encoding is therefore canonical long form and reaches the successful
return at `src/ecdsa_impl.h:88`; the oversized positive integer is deliberately
accepted by the parser and mapped to scalar zero. The oracle independently
requires parse success, compact `r = 0, s = 1`, and verification failure.

The earlier 46-file API corpus exercised malformed, truncated, non-shortest,
and short long-form lengths, but never a valid long-form length of at least
128 bytes. Normal builds and tests consequently did not bind this public
parser postcondition. This is **Informational / Low oracle hardening**, not a
clean-master production vulnerability or fix: the parser's existing behavior
is intentionally preserved and no severity rating changes.

For causal proof, a temporary clean-master-equivalent mutation changed only
`if (*len < 128)` to `if (*len < 128 || *len == 129)`. All 46 pre-existing API
inputs remained green on native 5x52 and forced-int64/10x26 Clang 22.1.7
ASan/UBSan builds, while the exact new seed aborted with status 134 on both.
The broader mutation that disabled the entire `< 128` check was rejected as
too coarse because it also changes the already-covered BER rejection control;
the length-129 mutation isolates the new successful-return contract. The
mutation was restored before replay and was never committed.

The restored 47-file API replay exited 0 on both sanitizer backends. GCC
coverage recorded the successful long-form return once, with `ecdsa_impl.h`
at 98.52% of lines and 100% of production branches. Private worker copies
ran with `-fork=2 -jobs=2 -max_total_time=10 -timeout=60 -rss_limit_mb=0`;
both backend managers and every logged job exited 0 with
`oom/timeout/crash: 0/0/0` and no sanitizer or crash artifact. Native and
forced-int64 EC VERIFY and no-VERIFY slices also passed. Existing findings
remain rated against unmodified master; a public or otherwise
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Exact Jacobi Reachability Probe and Full Corpus Recheck

The remote refresh still resolves both `origin/master` and `l0rinc/master` to
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`. That commit is an ancestor of
`codex/fuzz-oracles`, so no rebase was required. The restored audit source was
replayed through all 278 tracked inputs across the 14 current fuzz targets:
`api_roundtrip` 47, `context` 11, `ecdh` 7, `ecmult_const` 8,
`ecmult_multi` 24, `ellswift` 15, `field` 20, `group` 21, `hash` 10,
`musig` 67, `recovery` 12, `scalar` 7, `schnorrsig` 15, and `xonly_tweak`
14. Every non-libFuzzer replay returned zero. The merged public-library
coverage reached 94.69% of `secp256k1.c` lines and 97.83% of its branches;
ECDSA reached 100% of its 135 lines and 106 branches, while the remaining
public-library misses were invalid-argument/error paths or internal code
covered by the dedicated targets.

The field-only coverage build reached 100% of the 265 `field_impl.h` lines
and 295 `field_5x52_impl.h` lines. In
`secp256k1_jacobi64_maybe_var`, the 20 tracked seeds caused 1,680 update
iterations and reached `f.v[0] == 1` 44 times. Every one had already reduced
`len` to 1, so the `for (j = 1; j < len; ++j)` body remained unexecuted;
`modinv64_impl.h` nevertheless reached 98.07% of its lines and all 294
instrumented branches.

To test the exact gate rather than infer it from line coverage, two temporary
abort probes were applied and restored. First, aborting when a low-limb-one
state had a nonzero higher limb ran four native and four forced-int64 workers
for 45 seconds each, with every job exiting 0 and no artifact. Second,
aborting only when `f.v[0] == 1 && len > 1` ran four workers per backend for
30 seconds each; all jobs again exited 0, with no sanitizer, assertion,
timeout, OOM, or crash result. The 20 tracked seeds passed before each
campaign. Neither probe, its generated corpus, nor a production mutation was
committed.

This is negative reachability evidence, not a new clean-master defect. The
inner loop is an internal arithmetic convergence check with no independent
public state transition, so adding a synthetic seed would duplicate the
deterministic arbitrary-modulus Jacobi/inversion tests without strengthening
bug discovery. Existing findings are reiterated and still rated against the
unmodified master before later fork or audit changes: **Medium** for malformed
opaque public/key-aggregation state and secret HMAC state lifetime; **Medium,
low practical exploitability** for impossible SHA length handling;
**Medium/latent** for valid-status 10x26 magnitude-32 arithmetic; and
**Low/latent** for the internal scalar shift-over-width memory-safety edge,
EllSwift zero-`u` encoding edge, and documented tweak-input overlap. Oracle-only
and cleanup-only checks remain **Informational/Low**. A nonce or other public
buffer without cryptographic meaning is not a Critical erasure finding.

## 2026-07-17 Fresh Clean-Master Finding Reiteration

The focused baseline recheck used clean production master
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, with only the current fuzzer
sources and CMake wiring overlaid. No audit production fix was copied into the
baseline. The group barrier's disposable skip was removed only for its
focused baseline input and restored immediately afterward; the scalar target
was rebuilt after its current shift-boundary source was installed. The repaired
branch then replayed the same inputs on native 5x52 and forced-int64/10x26
Clang ASan/UBSan builds. Its `tests` and `noverify_tests` `ec`, `ecdsa`, and
`musig` slices all returned zero on both backends.

The exact baseline conditions and outcomes were:

- `group/off-curve-opaque-pubkey` (`off-curve opaque pubkey barrier\n`) exits
  134 on both backends. Clean master loads the opaque storage encoding of
  `x = 1, y = 1` because `secp256k1_pubkey_load` checks only nonzero `x`, even
  though the point is off curve. The repaired branch invokes the illegal
  callback, clears the failed output, and returns zero. This remains
  **Medium opaque-state integrity**: it requires malformed local opaque state
  or direct API misuse, not a serialized wire point.
- `field/magnitude32-normalize` (`field normalize magnitude32 bounds split
  zero raise seed\n`) passes native but exits 134 on clean forced-int64/10x26
  at the independent magnitude-32 reference. The old carry chain loses valid
  maximum-magnitude contributions during normalization. Both repaired
  backends pass. This remains **Medium/latent correctness** because a valid
  internal representation is affected, but no public path making that exact
  10x26 state reachable has been demonstrated.
- `scalar/mul-shift-over-512` (`scalar multiply shift 513 and UINT_MAX\n`)
  exits 134 on both clean backends. The shift-513 boundary reports the
  unrelated WNAF signed-integer diagnostics and then reads `l[8]` in native
  `scalar_4x64_impl.h:910` or `l[16]` in forced-int64
  `scalar_8x32_impl.h:707`; ASan classifies both as stack-buffer-overflow.
  Both repaired backends pass. This remains **Low/latent internal memory
  safety**, since current production callers use shift 384 and no public
  caller controls this helper domain.
- `hash/hmac-independent-reference` (`abc1234hij0\n`) reaches the clean
  independent output check and exits 134 because
  `secp256k1_hmac_sha256_finalize` leaves the consumed HMAC state live. Both
  repaired backends pass. This remains **Medium secret-state lifetime**; it
  does not claim a disclosure, and a public or non-cryptographic nonce is not
  a Critical erasure issue.
- `ecmult_multi/callback-failure-output-state`
  (`ecmult multi callback failure output state\n`) produces a clean-master
  ASan heap-buffer-overflow while a callback failure writes the 32-byte
  result through an undersized output state. Both repaired backends pass.
  This remains **Medium callback-failure memory safety** for the internal
  failure-state contract; its public reachability is separately limited by
  the callback and scratch domain.
- `context/sha256-impossible-lengths`
  (`context tagged sha256 impossible length seed\n`) in the
  macro-compatible clean build produces an ASan heap-buffer-overflow while
  hashing a `2^61`-byte tag from the fuzzer's short fallback pointer. Both
  repaired backends pass. This remains **Medium, low practical
  exploitability**: it is an invalid pointer/length pair, not a demonstrated
  remote cryptographic attack.
- `musig/off-curve-keyagg-cache`
  (`off-curve MuSig key aggregation cache\n`) reaches clean master's
  noncanonical field state and aborts its verification barrier. Both repaired
  backends pass. This remains **Medium opaque MuSig state**.
- `schnorrsig/opaque-keypair-consistency`
  (`opaque keypair secret and public state consistency\n`) reaches clean
  master's NULL-data nonce callback, with UBSan's null-pointer offset report
  followed by an ASan write fault. Both repaired backends pass. This remains
  **Medium inconsistent opaque keypair/callback state**, not a wire-format
  signature finding.

These are reiterated clean-master findings, not new defects and not severity
downgrades. The existing mutation-backed fix commits remain the strongest
causal proof; a later minor fix or cherry-pick making a replay pass does not
erase the baseline failure. No production mutation from this recheck was
committed, and no fuzz process remained running.

## 2026-07-17 State-Heavy Worker and Full-Test Recheck

The remote refs were refreshed after the preceding audit commits. Both
`origin/master` and `l0rinc/master` remain at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, which is an ancestor of
`codex/fuzz-oracles`; no rebase was required. The audit worktree was clean
before and after the campaign.

The native ASan/UBSan build ran `fuzz_musig` and `fuzz_ecmult_multi` with
`-fork=2 -jobs=2 -max_total_time=30 -timeout=60 -rss_limit_mb=0` against
private copies of their complete tracked corpora. All four workers exited 0;
the managers reported `oom/timeout/crash: 0/0/0`, and neither target created an
artifact. The forced-int64/10x26 ASan/UBSan build ran `fuzz_ecmult_multi` with
the same two-worker settings and the complete 88-input corpus; both workers
exited 0 with no artifact. Its isolated forced-int64 Musig replay used
`-fork=2 -jobs=2 -max_total_time=20` against 66 corpus inputs. Both workers
also exited 0 after the approximately 158-second merge/fuzz interval, with
no sanitizer report, timeout, OOM, crash, or artifact. The earlier 100-second
wrapper was therefore an infrastructure budget miss during corpus merging,
not a finding; the longer isolated run is the authoritative result.

The production test matrix was also checked. Native `tests` (default 16
iterations), native `noverify_tests`, and forced-int64 `noverify_tests` all
returned 0. Forced-int64 `tests -i=1` returned 0 in 90 seconds. A default
16-iteration forced-int64 `tests` invocation exceeded its 300-second wrapper
without diagnostics; this is recorded as a runtime boundary rather than a
pass claim, while the one-iteration run covered the same test registry with
the suite's expected low-iteration skips. No new clean-master mismatch or
master-relative severity change was found. Existing findings remain rated
against unmodified master, and a public or non-cryptographic nonce buffer is
not a Critical erasure finding.

## 2026-07-17 Public API Error-State Worker Recheck

The current native and forced-int64 ASan/UBSan builds replayed private copies
of the complete `api_roundtrip` corpus (47 inputs) and `context` corpus (11
inputs) with four libFuzzer workers per target:
`-fork=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=0`.
All 16 workers exited 0. The API workers retained their parser, serializer,
combine, sort, tweak, signature-state, callback, and output-cleanup oracles;
the context workers retained their clone, SHA backend, impossible-length,
callback, and deterministic-signing equivalence checks. Every logged worker
reported `oom/timeout/crash: 0/0/0`; no sanitizer diagnostic or artifact was
created on either backend. The forced-int64 run is an independent arithmetic
and state-transition check, not merely a repeat of native execution.

This campaign found no new clean-master defect and no master-relative severity
change. Existing findings continue to be rated against the unmodified master,
even where current production fixes make the repaired replay pass. A public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Signature-Domain Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds replayed the
complete `recovery` corpus (12 inputs) and `schnorrsig` corpus (15 inputs) with
four workers per target using
`-fork=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=0`.
All 16 workers exited 0. Recovery exercised recoverable-signature parsing,
recid separation, high-S behavior, nonce retries, verification, and failure
cleanup. Schnorr exercised x-only parity, nonce normalization and rejection,
custom/default nonce routing, equation verification, invalid opaque states,
and failure cleanup. Every worker reported `oom/timeout/crash: 0/0/0`; no
sanitizer diagnostic or artifact was produced on either backend.

The refreshed fork heads still contain no relevant production or oracle change
not already represented by this branch; `origin/master` and `l0rinc/master`
remain the same ancestor, so no rebase or additional cherry-pick is required.
This campaign found no new clean-master defect or master-relative severity
change. Existing findings remain rated against unmodified master, and a public
or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 ECDH and EllSwift Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds replayed the
complete `ecdh` corpus (7 inputs) and `ellswift` corpus (15 inputs) with four
workers per target using
`-fork=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=0`.
All 16 workers exited 0. ECDH exercised shared-point symmetry, built-in and
custom hash callbacks, invalid scalar handling, and output cleanup. EllSwift
exercised encoding/decoding, modulo aliases, randomizer influence, inverse
branches, XDH callbacks, and zero-`u`/failure cleanup. Every worker reported
`oom/timeout/crash: 0/0/0`; no sanitizer diagnostic or artifact was produced
on either backend.

This campaign found no new clean-master defect or master-relative severity
change. Existing findings remain rated against unmodified master, and a public
or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 X-only Tweak Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds replayed the
complete `xonly_tweak` corpus (14 inputs) with four workers using
`-fork=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=0` on each
backend. All 16 workers exited 0. The campaign exercised the independent
affine/group reference for two-G tweaking, x-only parse equivalence, parity
negation, invalid and partial opaque keypairs, input/output alias windows,
null-tweak cleanup, and keypair/public-key consistency. Every worker reported
`oom/timeout/crash: 0/0/0`; no sanitizer diagnostic or artifact was produced.

This campaign found no new clean-master defect and no master-relative severity
change. Existing findings remain rated against unmodified master, and a public
or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Core Arithmetic Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds replayed the
complete `ecmult_const` (8 inputs), `field` (20), `group` (21), `hash` (10),
and `scalar` (7) corpora. Each target/backend pair used four workers with
`-fork=4 -jobs=4 -max_total_time=30 -timeout=60 -rss_limit_mb=0`, for 40
worker jobs total. All 40 jobs exited 0. The run retained the independent
affine multiplication, field normalization/Jacobi, group equation, hash
state, scalar arithmetic, infinity, aliasing, and failure-state oracles
already present in the targets. Every worker reported
`oom/timeout/crash: 0/0/0`; no sanitizer diagnostic or artifact was produced.

This campaign found no new clean-master defect and no master-relative severity
change. Existing findings remain rated against unmodified master, and a public
or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long MuSig Stateful Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds ran the MuSig
target for three minutes per backend with the complete 67-file corpus and
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`. All eight
worker processes exited 0. Native workers accepted 67 seed inputs; the
forced-int64 workers accepted 66 from the same 67 tracked files, with no
corpus rejection or diagnostic. The workers reached the existing key
aggregation, nonce, session, partial-signature, callback, rollback, cleanup,
and independent equation oracles. Every worker reported
`oom/timeout/crash: 0/0/0`; no sanitizer diagnostic or artifact was produced.

This longer stateful campaign found no new clean-master defect and no
master-relative severity change. Existing findings remain rated against
unmodified master, and a public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Long Ecmult-Multi Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds ran
`ecmult_multi` for three minutes per backend against the complete 24-file
corpus using `-fork=4 -jobs=4 -max_total_time=180 -timeout=60
-rss_limit_mb=0`. All eight worker processes exited 0 and every worker
reported `oom/timeout/crash: 0/0/0`. Both backends reached additional coverage
in the existing `secp256k1_fuzz_ecmult_multi_affine_double` oracle. Each
backend emitted one libFuzzer `artifact-slow-unit-*` marker containing
`pippenger window 1261\n`, the existing 22-byte slow seed; it was not a crash
artifact, timeout, sanitizer report, or nonzero worker result. No other
artifact was produced.

This longer callback, scratch, and allocation campaign found no new
clean-master defect and no master-relative severity change. Existing findings
remain rated against unmodified master, and a public or non-cryptographic
nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Public API Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds ran
`api_roundtrip` for 180 seconds per backend against the complete 47-file
corpus using `-fork=4 -jobs=4 -max_total_time=180 -timeout=60
-rss_limit_mb=0`. All eight worker processes exited 0, loaded all 47 seed
inputs, and reported `oom/timeout/crash: 0/0/0`. The run expanded existing
public error-state, callback-context, parser, tweak alias, sort/compare,
ECDSA nonce-retry, and independent pubkey-equation coverage on both
representations. No sanitizer diagnostic or artifact was produced.

This longer public-API campaign found no new clean-master defect and no
master-relative severity change. Existing findings remain rated against
unmodified master, and a public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Long Schnorr Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds ran
`schnorrsig` for 180 seconds per backend against the complete 15-file corpus
using `-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight worker processes exited 0, loaded all 15 seed inputs, and reported
`oom/timeout/crash: 0/0/0`. The run exercised hardened custom nonce routing,
BIP340 nonce/challenge hashing, x-only parity, infinity and overflow
rejection, generator and signature equations, invalid opaque keypairs, and
failure-output cleanup on both representations. No sanitizer diagnostic or
artifact was produced.

This longer Schnorr campaign found no new clean-master defect and no
master-relative severity change. Existing findings remain rated against
unmodified master; clearing a public or non-cryptographic nonce buffer is not
a Critical erasure finding.

## 2026-07-17 Schnorr Context-Routing Negative Control

The `sign32-custom` corpus input (`schnorr sign32 sign_custom equivalence with
fixed aux\n`, 54 bytes) was replayed against a detached build of this branch and
against a one-line production mutation. The mutation changed the built-in
Schnorr signing path in `src/modules/schnorrsig/main_impl.h` to call
`nonce_function_bip340_impl` with `secp256k1_context_static` instead of the
caller context. It therefore models a regression that silently ignores a
caller-installed SHA compression backend while leaving the rest of signing
functional.

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan mutant builds were
configured with all modules and libFuzzer enabled, rebuilt from the detached
worktree, and run with `-runs=1` on that exact seed. Both exited `77` after
`FUZZ_CHECK` at `src/fuzz/schnorrsig.c:1073`, where the context hook must observe
the BIP340 nonce hash. The unchanged audit binary accepted the identical seed
with exit `0`; the native replay completed in 140 ms. This proves the assertion
distinguishes the regression from ordinary signing success and works across
both field representations. The mutation was restored outside the audit
branch before this entry was recorded.

This is **Informational oracle hardening**, not a clean-master production bug:
the production mutation is required to create the failure, and the clean
master branch already routes the nonce through the caller context. It does not
change any master-relative severity rating. A nonce buffer with no standalone
cryptographic meaning is not a Critical erasure finding; severity remains tied
to the actual master-branch impact of a confirmed defect.

## 2026-07-17 Long Context Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 11-file `context` corpus for 180 seconds per backend with four forked
jobs and four workers:
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 11 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 3074` with a maximum
feature count of 6,607; forced-int64 workers reached `cov: 5028` with a
maximum feature count of 13,143. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised context randomization and clone/reset transitions, source
and heap/preallocated clone paths, NULL-reset deterministic ECDSA and Schnorr
signing, valid and invalid context flags, custom SHA compression routing, the
standalone tagged-SHA reference, impossible SHA lengths, and secret-operation
cleanup. This adds negative evidence for the current oracles only: it found no
new clean-master defect and no master-relative severity change. Existing
findings remain rated against unmodified master; a public or non-cryptographic
nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Recovery Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 12-file `recovery` corpus for 180 seconds per backend with four forked
jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 12 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 3111` with a maximum
feature count of 7,115; forced-int64 workers reached `cov: 5073` with a
maximum feature count of 14,898. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised recoverable-signature parsing and serialization, recid
separation, high-S and zero-S rejection, recovery-point equations, verification
infinity and finite-x mismatch barriers, valid and invalid nonce callback
domains, nonce retries, and failure-output cleanup. This extends the prior
30-second signature-domain check without finding a new clean-master defect or
changing any master-relative severity. Existing findings remain rated against
unmodified master; a public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Long EllSwift Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 15-file `ellswift` corpus for 180 seconds per backend with four forked
jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 15 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 3000` with a maximum
feature count of 7,651; forced-int64 workers reached `cov: 4876` with a
maximum feature count of 14,873. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised EllSwift encode/decode round trips, modulo-alias wire
encodings, randomizer influence, inverse branches and degenerate rejection,
BIP324 decode and transcript equations, fixed scalar-one XDH, raw XDH symmetry,
built-in hash cleanup, NULL-input output cleanup, invalid-secret callback
postconditions, and custom-hasher party-domain checks. This extends the prior
30-second ECDH/EllSwift module check without finding a new clean-master defect
or changing any master-relative severity. Existing findings remain rated
against unmodified master; a public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Long ECDH Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 7-file `ecdh` corpus for 180 seconds per backend with four forked jobs
and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 7 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 2601` with a maximum
feature count of 5,140; forced-int64 workers reached `cov: 4430` with a
maximum feature count of 11,950. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised shared-point symmetry, the fixed generator-times-two
byte-equation reference, built-in and coordinate-passthrough hash callbacks,
built-in NULL-input output cleanup, invalid-scalar callback point
postconditions, and callback output-state transitions. This extends the prior
30-second ECDH/EllSwift module check without finding a new clean-master defect
or changing any master-relative severity. Existing findings remain rated
against unmodified master; a public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Long X-only Tweak Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 13-file `xonly_tweak` corpus for 180 seconds per backend with four
forked jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 13 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 3037` with a maximum
feature count of 7,575; forced-int64 workers reached `cov: 5011` with a
maximum feature count of 14,602. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised x-only serialization and parity, independent curve
membership parsing, ordinary and x-only tweaks, complete in/out overlap
windows, invalid and partial opaque keypairs, invalid full-pubkey conversion,
comparator ordering, tweak rejection, and keypair/public-key consistency.
This extends the prior 30-second x-only module check without finding a new
clean-master defect or changing any master-relative severity. Existing
findings remain rated against unmodified master; a public or non-cryptographic
nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Hash Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 7-file `hash` corpus for 180 seconds per backend with four forked jobs
and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 7 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 511` and a maximum
feature count of 1,376 after roughly 459k to 461k executions per worker;
forced-int64 workers reached the same `cov: 511` and feature count after
roughly 458k to 459k executions per worker. No sanitizer, assertion, or
runtime-error diagnostic was emitted, and neither backend produced an
artifact.

The run exercised the standalone SHA-256 reference, raw-SHA256 HMAC, arbitrary
multi-block midstates, full RFC6979 sequencing, chunking consistency, tagged
hashing, and finalized-state cleanup. This extends the prior core-arithmetic
recheck without finding a new clean-master defect or changing any
master-relative severity. The existing clean-master consumed-HMAC state
finding remains **Medium** for secret-state lifetime; a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Field Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete tracked 20-file `field` corpus for 180 seconds per backend with four
forked jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers exited `0` and reported `oom/timeout/crash: 0/0/0`.
Native workers reached `cov: 1596` with a maximum feature count of 2,253;
forced-int64 workers reached `cov: 2806` with a maximum feature count of
4,044. No sanitizer, assertion, or runtime-error diagnostic was emitted, and
neither backend produced an artifact. All 20 tracked seed filenames remained
present in both disposable corpus directories; the fork-manager startup lines
reported 19 native and 17 forced-int64 unique seed entries while the shared
disposable corpora were being managed, so those manager counts are not treated
as tracked-input counts.

The run exercised field normalization and magnitude bounds, nonnormalized and
aliased arithmetic, byte-level add/negate/product/square-root references,
maximum-magnitude inversion, strict parsing limits, zero-predicate slow and
false-positive barriers, and cleanup. This extends the prior arithmetic
recheck without finding a new clean-master defect or changing any
master-relative severity. The existing clean-master forced-int64 magnitude-32
issue remains **Medium/latent internal correctness**; a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Group Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 21-file `group` corpus for 180 seconds per backend with four forked
jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 21 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 2868` with a maximum
feature count of 5,278; forced-int64 workers reached `cov: 4642` with a
maximum feature count of 9,365. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised Jacobian/affine agreement, canonical-coordinate equality,
positive and negative addition, affine infinity, fractional curve
membership, finite and mixed-infinity batch conversion, inverse-Z conversion,
inverse-point cancellation, nonnormalized storage conversion, rescaling and
aliasing, lambda-degenerate addition, cleanup, and invalid opaque public-key
barriers. It extends the prior group campaign without finding a new
clean-master defect or changing any master-relative severity. The existing
clean-master off-curve opaque-public-key barrier remains **Medium/opaque
state integrity**: it protects malformed internal opaque state and direct
misuse, not a wire-format point accepted by the public parser. A public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Scalar Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 7-file `scalar` corpus for 180 seconds per backend with four forked
jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 7 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 2085` with a maximum
feature count of 5,957; forced-int64 workers reached `cov: 3716` with a
maximum feature count of 13,918. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised independent scalar reduction, addition and negation,
variable bit extraction boundaries, inversion, GLV lambda splitting, WNAF,
carry/zero boundaries, and product-shift references. This extends the prior
scalar campaign without finding a new clean-master defect or changing any
master-relative severity. The existing clean-master over-512-bit shift
finding remains **Low/latent internal memory safety** because current
production callers use the bounded 384-bit shift; a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Long Ecmult-Const Worker Recheck

The native 5x52 and forced-int64/10x26 Clang ASan/UBSan binaries replayed the
complete 8-file `ecmult_const` corpus for 180 seconds per backend with four
forked jobs and four workers using
`-fork=4 -jobs=4 -max_total_time=180 -timeout=60 -rss_limit_mb=0`.
All eight workers loaded all 8 seed inputs, exited `0`, and reported
`oom/timeout/crash: 0/0/0`. Native workers reached `cov: 2642` with a maximum
feature count of 7,400; forced-int64 workers reached `cov: 4525` with a
maximum feature count of 14,406. No sanitizer, assertion, or runtime-error
diagnostic was emitted, and neither backend produced an artifact.

The run exercised independent affine double/addition and scalar-multiply
references, NULL-generator equivalence, canonical infinity, generator
agreement across generic/constant-time/precomputed paths, x-only numerator /
denominator forms, invalid x rejection, nonnormalized fractions, odd-multiple
table reconstruction, and the fixed generator-times-two equation. This
extends the prior constant-multiplication campaign without finding a new
clean-master defect or changing any master-relative severity. Existing
findings remain rated against unmodified master; a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Final ASan Build and Test Verification

The current audit worktree rebuilt the Clang ASan/UBSan library, tests, and all
14 fuzz binaries with `cmake --build /tmp/secp256k1-oracles-clang -j2`;
all 34 build steps completed successfully. The only compiler diagnostic was
the pre-existing deprecation warning for the compatibility
`secp256k1_schnorrsig_sign` API.

`ctest --test-dir /tmp/secp256k1-oracles-clang --output-on-failure` then passed
all 224 tests with zero failures, including both VERIFY and no-VERIFY test
variants, arithmetic boundaries, public API error states, ECDSA/recovery,
Schnorr, MuSig, EllSwift, and XDH. No sanitizer diagnostic or test artifact
was produced. This verifies the complete build after the latest oracle
ledger commits; it does not change any master-relative finding severity.

## 2026-07-17 Validator Return Audit

A source-level inventory reviewed every production call site of
`secp256k1_pubkey_load`, `secp256k1_keypair_load`, the x-only public-key
loader, `secp256k1_ge_set_xo_var`, and `secp256k1_ecmult_const_xonly`. The
public API, ECDH, EllSwift, extrakeys, recovery, and MuSig paths either branch
on the validator result, propagate it, or establish a documented internal
invariant before using the result. The MuSig aggregation callback records the
result and asserts the already-loaded-key invariant under VERIFY. The
remaining intentionally ignored loader calls are in test/exhaustive-test
code, not production transitions.

This audit also rechecked l0rinc PR #11 (`d1dca5c`, `ecdh, ec: check
pubkey_load return`). Its two relevant checks are already present at
`src/modules/ecdh/main_impl.h` and `src/secp256k1.c`; cherry-picking the PR
again would be redundant and would add no independent master-relative proof.
No new clean-master finding or severity change resulted. The existing
malformed opaque-state, callback-failure, and arithmetic findings remain
rated against unmodified master. A public or non-cryptographic nonce buffer
is not a Critical erasure finding.

## 2026-07-17 Exhaustive Model and Oracle Check

The exhaustive-test surface was rebuilt in isolated Clang ASan/UBSan
configurations from unmodified master `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`
and from this audit branch. The order-13 binary was run with the same seed and
`./bin/exhaustive_tests 2 0x5eed1234`; both completed with `no problems found`,
exit `0`, and no sanitizer diagnostics. After strengthening the harness, the
audit binary was rebuilt and rerun with
`./bin/exhaustive_tests 1 0x5eed1234`; it again completed with `no problems
found` and exit `0`.

Two previously unchecked test contracts now fail at the operation boundary:
the exhaustive `ecmult_multi` matrix requires a return value of `1` before
consuming its Jacobian output, and the EllSwift create/decode round trip
requires `secp256k1_pubkey_load` to accept the decoded public key before the
group comparison. These are test-only oracle changes; they do not alter the
library ABI or production behavior. They prevent a future failure return or
malformed decoded object from being silently compared as if it were valid.

The oracle was independently mutation-tested in a disposable clean-master
worktree. Removing the second `secp256k1_fe_add(&r->x, &t)` term from the
production `secp256k1_gej_double` formula, then building the same sanitized
`exhaustive_tests` target, caused
`./bin/exhaustive_tests 1 0x5eed1234` to abort with exit `134` at
`src/ecmult_gen_compute_table_impl.h:45` (`double_u` must equal the generator).
This proves the model catches a one-line group-arithmetic regression before
the broader matrix; the mutation was not committed. No new clean-master
production defect was found, so this result is **Informational/oracle
validation**, not a severity-rated vulnerability. Existing findings remain
rated against unmodified master, and a public or non-cryptographic nonce
buffer is not a Critical erasure finding.

## 2026-07-17 Post-Rebase Complete Multi-Worker Corpus Replay

`origin/master` and `l0rinc/master` were fetched before this replay and both
remained at unmodified master `11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`.
`git rebase origin/master` reported that `codex/fuzz-oracles` was already up
to date. The current Clang ASan/UBSan build was also current (`ninja: no work
to do`), so no source or build artifact was silently substituted for the
rebased tree.

Fresh copies of all tracked corpora were replayed with two workers and two
jobs per target, `-runs=1`, `-timeout=5`, and sanitizer options
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. The copied seed counts were:

    api_roundtrip 47  context 11       hash 10        scalar 7
    field 20        group 21            ecmult_const 8 ecmult_multi 24
    ecdh 7          ellswift 15         xonly_tweak 14 recovery 12
    schnorrsig 15   musig 67

`api_roundtrip`, `context`, `hash`, `scalar`, `field`, `group`,
`ecmult_const`, `ecdh`, `ellswift`, `xonly_tweak`, `recovery`, `schnorrsig`,
and `musig` loaded every copied seed and had exit `0` in both jobs. The first
`ecmult_multi` pass also loaded all 24 seeds, but both workers hit the
command-level five-second libFuzzer timeout on the existing
`pippenger window 1261` seed while executing the independent affine
reference's modular inversion. The stacks contained no sanitizer diagnostic,
production assertion, memory fault, or invalid result; this is an expensive
oracle input, not a production finding or a claim that a timeout is safe to
ignore generally.

As the control for that runtime boundary, all 24 copied `ecmult_multi` seeds
were rerun with the same `-workers=2 -jobs=2 -runs=1` configuration and
`-timeout=60`. Both jobs exited `0`, no sanitizer or assertion diagnostic was
emitted, and the artifact directory remained empty. This separates the
known reference cost from a hang or availability defect in production code.
No fuzzer source, production source, or tracked corpus file changed during
the replay.

This pass found no new master-relative bug and changes no severity. Existing
findings remain rated against unmodified master: Medium for internal scratch
allocation overflow and malformed opaque/MuSig state barriers, Medium/latent
for the forced-int64 field magnitude boundary and secret HMAC-state lifetime,
Low for bounded latent scalar-shift behavior and documented tweak-input
aliasing, and Informational for oracle-only checks. A public or
non-cryptographic nonce buffer is not a Critical erasure finding. The prior
production fixes and deterministic tests remain the strongest proof for their
respective findings; this replay is negative evidence for the current oracle
set, not evidence that later fork patches can be used to validate clean
master.

## 2026-07-17 Static Context Keypair Barrier

The x-only target now has a gated 31-byte seed,
`static context keypair barrier`. It uses a valid keypair with
`secp256k1_context_static` and checks the documented split between a
public-only operation and a secret-dependent mutation: `keypair_xonly_pub`
must project the valid x-only key and parity successfully, while
`keypair_xonly_tweak_add` must reject the same keypair because the static
context has no generator precomputation. The rejection must invoke the
default illegal callback and leave the keypair in the documented invalid
(all-zero) state. The rejection half runs under the repository's
`SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS=ON` fuzz build, whose non-aborting
default callback exposes that otherwise fatal static-context path; the normal
callback build still runs the public-only projection half.

This is **Informational oracle hardening**, not a clean-master production
finding. The general context contract says that operations consuming a
secret key or keypair require a non-static context; the x-only projection is
the deliberate public-only exception exercised here. The oracle prevents a
future refactor from treating the static context as capable of secret
generator multiplication, while avoiding the stale assumption that every
keypair accessor needs signing state. It does not change any master-relative
severity: malformed opaque state, callback-failure barriers, and the
reachable forced-int64 arithmetic boundary remain **Medium** or
**Medium/latent** as previously recorded; bounded/documented alias cases
remain **Low/latent**; cleanup and model checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable production mutation changed only the
`ret = 0` assignment in the no-generator branch of
`src/modules/extrakeys/main_impl.h` to `ret = 1`, leaving the callback and all
other code unchanged. The external-callback focused replay used
`-handle_abrt=0 -runs=1 -timeout=60 -rss_limit_mb=0` and exited `134`; the
unmutated control with the same 31-byte seed exited `0`. Native 5x52,
forced-int64/10x26, and external-callback focused controls all passed under
Clang ASan/UBSan. The complete 15-file x-only corpus also passed with
`-workers=2 -jobs=2 -runs=1 -timeout=60 -rss_limit_mb=0` on all three builds;
each job exited `0` with no sanitizer report, output artifact, or assertion
failure. The mutation was restored before the audit tree was verified.

## 2026-07-17 Direct Ecmult Domain and Release-Style Recheck

The remote refresh was repeated with the correct remote names: both
`origin/master` and `l0rinc/master` still resolve to
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`. `git rebase origin/master`
reported that `codex/fuzz-oracles` was already up to date. No fork commit was
cherry-picked during this pass, because there was no ref movement and no new
patch context to reconcile.

A source-level audit then followed every production caller of
`secp256k1_ecmult_multi_var` and the direct Strauss/Pippenger batch helpers.
The public production path computes its batch size through
`secp256k1_ecmult_multi_batch_size_helper`, which caps each batch at
`ECMULT_MAX_POINTS_PER_BATCH` before the `2*n + 2` Pippenger entry arithmetic
or the Strauss allocation products are evaluated. Its callback offset is the
same bounded partition index. The only direct single-batch callers are the
internal tests and benchmark/fuzzer surfaces; passing `SIZE_MAX` directly to
those helpers is outside the production contract and would be an invalid
internal caller domain, not a master-reachable input. No new production
integer-overflow or callback-index finding was claimed from that domain.

The ASan/UBSan `noverify_tests` target was rebuilt with `ninja` and its full
default randomized suite completed with exit `0`. This supplies a release-style
check of the production paths without VERIFY assertions; it did not change the
negative multi-worker corpus result or any severity. The clean-master ledger is
reiterated unchanged: **Medium** for internal scratch allocation overflow,
malformed opaque public/MuSig state, callback-failure memory safety, and secret
HMAC-state lifetime; **Medium/latent** for the forced-int64 field magnitude
boundary; **Low/latent** for bounded scalar-shift behavior and documented tweak
input aliasing; and **Informational** for oracle-only cleanup and model checks.
A public or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 MuSig Session-Random/Cache Alias Oracle

The MuSig target now has a gated 41-byte seed,
`session-random-cache-overlap`. It copies a valid aggregate key cache, treats
the first 32 bytes of that copy as the explicitly `In/Out`
`session_secrand32` buffer, and passes the same cache as the optional aggregate
cache. A separate cache snapshot feeds the independent BIP327 nonce
transcript. The oracle compares the return value, both secret nonce scalars,
and the serialized public nonce, then checks the exact documented side effect:
the shared first 32 bytes are zero and the remaining cache bytes are unchanged.

This is **Informational oracle hardening**, not a new clean-master finding.
The public declaration marks `session_secrand32` as `In/Out`, but it does not
promise arbitrary overlap with the cache's `In` object; this seed is a
deliberate ordering stress case, not a supported caller pattern or a security
rating. Unmodified master predates the session-random invalidation fix and
therefore leaves this deliberate alias unchanged; the test is preserving the
current branch's read-before-invalidation behavior rather than claiming that
master corrupts a cache. A public nonce buffer has no standalone cryptographic
meaning and is not a Critical cleanup finding.

For causal proof, a disposable production mutation inserted
`secp256k1_memczero(session_secrand32, 32, 1)` immediately before the internal
nonce derivation, but only when the session-random pointer exactly equaled
`keyagg_cache->data`. The focused seed then reached the invalid-cache barrier
and aborted with the libFuzzer illegal-argument failure under
`-handle_abrt=0`; all 67 pre-existing MuSig inputs remained green in a
68-execution replay and exited 0. After restoring the source, the focused seed
passed under Clang ASan/UBSan on both native 5x52 and forced-int64/10x26
backends. The mutation was never committed, no production behavior changed,
and no master-relative severity changed.

After that focused proof, all 279 tracked corpus inputs were replayed from the
current tree with `-workers=2 -jobs=2 -runs=1 -timeout=60 -rss_limit_mb=0`
under native 5x52 and forced-int64/10x26 Clang ASan/UBSan builds. Every target
command returned zero, including the new 68-file MuSig corpus. The forced
int64 `ecmult_multi` replay emitted one `slow-unit-*` file for the known
`pippenger-window-1261` input; this is a harness-cost artifact, not a
production timeout or sanitizer result. Isolated replays of that exact input
returned zero in 7.2 seconds native and 11.8 seconds forced-int64, with empty
artifact directories. No assertion, sanitizer diagnostic, OOM, or crash was
observed. The campaign therefore adds negative evidence only: existing
findings remain rated against unmodified master, and a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Static Context ECDH and EllSwift Barriers

The ECDH and EllSwift targets now include focused static-context seeds:
`static context ecdh barrier` and `static context ellswift barrier`. These
exercise the part of the context contract that ordinary randomized contexts
cannot reach. ECDH is the documented exception to the general static-context
restriction. EllSwift decode, encode, and x-only XDH likewise use only public
operations or constant-time point multiplication; EllSwift create remains
excluded because its header explicitly requires a non-static context.

The ECDH oracle compares a context-independent callback result between the
dynamic and static contexts, checks that NULL and the explicit default hash
function agree under the static context, and verifies the static result
against a compressed-public-key SHA256 reference. The EllSwift oracle checks
static decode and encode round trips, compares custom-callback XDH between
dynamic and static contexts, compares BIP324 XDH outputs, and invokes the
standalone BIP324 transcript model with the static context.

This is **Informational oracle hardening**, not a clean-master production
finding. Unmodified master passed the new seeds; no severity is raised. The
existing ECDH module tests and fuzzer used dynamically allocated contexts,
and the EllSwift tests did not exercise these allowed operations with
`secp256k1_context_static`, so a regression limited to the static object
could pass both the normal unit suite and the prior fuzz corpus. This does
not alter the standing ratings: malformed opaque state, callback-failure
barriers, and the reachable forced-int64 arithmetic boundary remain
**Medium** or **Medium/latent**; bounded/documented alias cases remain
**Low/latent**; cleanup and model-only checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, two disposable production mutations were tested separately:
`if (ctx == secp256k1_context_static) return 0;` was inserted immediately
after the context check in `src/modules/ecdh/main_impl.h`, and the same
mutation was inserted in `secp256k1_ellswift_xdh` in
`src/modules/ellswift/main_impl.h`. The matching focused seed reached the
new assertion and exited `134` with `-handle_abrt=0`; the unmutated control
exited `0`. The focused seeds passed on native 5x52, forced-int64/10x26, and
external-callback Clang ASan/UBSan builds after restoration. The complete
8-file ECDH and 16-file EllSwift corpora were replayed with two workers and
two jobs on all three builds, with no sanitizer report, timeout, assertion,
OOM, or output artifact. Neither mutation was committed, and no production
behavior changed.

## 2026-07-17 Static Context Recovery Barrier

The recovery target now has a gated `static context recovery barrier` seed.
It feeds a fixed-wire `(r, s, recid)` vector through recoverable-signature
parse/serialize, conversion, public-key recovery, compressed public-key
serialization, and ECDSA verification using both the randomized context and
`secp256k1_context_static`. The generator encoding is an independent expected
value, so the comparison does not merely prove that two context paths share a
bad result.

This is **Informational oracle hardening**, not a clean-master production
finding. The recovery headers do not restrict these operations to a proper
context, and their implementation performs no generator-table operation for
the recovery equation. Existing recovery tests and the fuzzer exercised the
same contracts through a randomized context only, so a static-only rejection
or accidental dependency on generator precomputation could have passed. The
standing clean-master ratings are unchanged: malformed opaque state,
callback-failure barriers, and the reachable forced-int64 arithmetic boundary
remain **Medium** or **Medium/latent**; bounded/documented alias cases remain
**Low/latent**; cleanup and model-only checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable mutation added an illegal-argument check for
`secp256k1_context_static` immediately after the context check in
`secp256k1_ecdsa_recoverable_signature_parse_compact`. The focused seed then
reached the new assertion and exited `134` with `-handle_abrt=0`; the restored
control exited `0`. The focused seed and the complete 13-file recovery corpus
passed with two workers and two jobs on native 5x52, forced-int64/10x26, and
external-callback Clang ASan/UBSan builds. Recovery unit and no-VERIFY tests
also passed on all three configurations. The mutation was restored before
the tree was committed, and no production behavior or severity changed.

## 2026-07-17 Static Context Schnorr Verification Barrier

The existing `schnorrsig-generator-equation` seed now runs its independent
fixed-wire BIP340 equation through both the randomized context and
`secp256k1_context_static`. Each context separately parses and serializes the
x-only generator coordinate, then verifies the same independently constructed
signature. This extends an existing oracle instead of adding a duplicate
corpus input, while covering the Schnorr module's static-context boundary.

This is **Informational oracle hardening**, not a clean-master production
finding. `secp256k1_schnorrsig_verify`, x-only parsing, and x-only
serialization accept a general context and do not require generator
precomputation; the public verifier's implementation uses variable-time
point multiplication and the context hash backend only. Existing Schnorr
tests and fuzz inputs used randomized contexts, so a static-only rejection or
an accidental generator-table dependency could have escaped. The clean-master
severity ledger is unchanged: malformed opaque state, callback-failure
barriers, and the reachable forced-int64 arithmetic boundary remain
**Medium** or **Medium/latent**; bounded/documented alias cases remain
**Low/latent**; cleanup and model-only checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable mutation added
`ARG_CHECK(ctx != secp256k1_context_static);` immediately after the context
check in `src/modules/schnorrsig/main_impl.h`, inside
`secp256k1_schnorrsig_verify`. The existing
`src/fuzz/corpora/schnorrsig/generator-equation` input then exited `134` with
`-handle_abrt=0`; the restored control exited `0`. The focused input and the
complete 15-file Schnorr corpus passed with two workers and two jobs on native
5x52, forced-int64/10x26, and external-callback Clang ASan/UBSan builds.
Schnorr unit and no-VERIFY tests passed on all three configurations. The
mutation was restored before the tree was committed, and no production
behavior or severity changed.

## 2026-07-17 Static Context X-Only Public Tweak Barrier

The existing `xonly tweak affine reference` seed now parses the x-only input,
applies the public x-only tweak, serializes the resulting full public key, and
checks the tweak witness through `secp256k1_context_static` as well as the
randomized context. The expected point remains the independent byte-level
affine result, and the static path reparses its own x-only input so no opaque
key representation is treated as portable between contexts.

This is **Informational oracle hardening**, not a clean-master production
finding. `secp256k1_xonly_pubkey_tweak_add` and
`secp256k1_xonly_pubkey_tweak_add_check` have no non-static context restriction;
they operate on public points through the ordinary public tweak helper. The
prior affine oracle covered the arithmetic independently but only used a
randomized context, so a static-only rejection or accidental generator-table
dependency could have escaped. The clean-master severity ledger is unchanged:
malformed opaque state, callback-failure barriers, and the reachable
forced-int64 arithmetic boundary remain **Medium** or **Medium/latent**;
bounded/documented alias cases remain **Low/latent**; cleanup and model-only
checks remain **Informational**. A public or non-cryptographic nonce buffer is
not a Critical erasure finding.

For causal proof, a disposable mutation added
`ARG_CHECK(ctx != secp256k1_context_static);` immediately after the context
check in `secp256k1_xonly_pubkey_tweak_add`. The existing
`src/fuzz/corpora/xonly_tweak/affine-reference` input then exited
`134` with `-handle_abrt=0`; the restored control exited `0`. The focused input
and the complete 15-file x-only corpus passed with two workers and two jobs on
native 5x52, forced-int64/10x26, and external-callback Clang ASan/UBSan builds.
X-only extrakeys unit and no-VERIFY tests passed on all three configurations.
The mutation was restored before the tree was committed, and no production
behavior or severity changed.

## 2026-07-17 Static Context Public Tweak Boundary

The existing `api_roundtrip/independent-tweak-order-boundary` fixture now
re-serializes the generated public key, reparses it through
`secp256k1_context_static`, and applies both public add and public multiply
tweaks there. Successful results are compared by compressed wire bytes with
the independent order-minus-one scalar reference; invalid results must still
zero the static output. This keeps the cross-context check at the public
serialization boundary instead of treating an opaque `secp256k1_pubkey` as
portable state.

This is **Informational oracle hardening**, not a clean-master production
finding. The deterministic suite already checks basic static-context public
tweak acceptance, but it does not exercise this independent boundary equation
or the static failure-zeroization path. The clean-master severity ledger is
unchanged: malformed opaque state, callback-failure barriers, and reachable
forced-int64 arithmetic boundaries remain **Medium** or **Medium/latent**;
bounded/documented alias cases remain **Low/latent**; cleanup and model-only
checks remain **Informational**. A public or non-cryptographic nonce buffer is
not a Critical erasure finding.

For causal proof, a disposable
`ARG_CHECK(ctx != secp256k1_context_static);` was inserted immediately after
the context check in `secp256k1_ec_pubkey_tweak_add` and, separately, in
`secp256k1_ec_pubkey_tweak_mul` in `src/secp256k1.c`. The exact existing
`src/fuzz/corpora/api_roundtrip/independent-tweak-order-boundary` input then
exited `134` with `-handle_abrt=0` under each mutant and exited `0` after
restoration. The full API-roundtrip corpus and deterministic API tests passed
on native, forced-int64/10x26, and external-callback Clang ASan/UBSan builds.
The mutations were restored before commit, and no production behavior or
severity changed.

## 2026-07-17 API Roundtrip Bounded Exploration

After the static-context boundary oracle was restored, the existing
`api_roundtrip` corpus was used as the seed corpus for bounded exploratory
fuzzing. Each configuration used `-workers=2 -jobs=2 -max_total_time=45
-timeout=60 -rss_limit_mb=0`; all four jobs per configuration completed with
exit code `0` and produced no sanitizer report, oracle failure, or crash
artifact. The ordinary newly discovered corpus units were removed after the
run, leaving the original tracked corpus unchanged.
The native 5x52 jobs completed 455 and 462 runs, forced-int64/10x26 completed
281 and 285, and external-callback Clang ASan/UBSan completed 470 and 463.
This was a negative exploration result: no clean-master production finding or
severity change was established, and the existing Medium/Medium-latent,
Low/latent, Informational, and non-Critical nonce-cleanup ratings remain in
force.

## 2026-07-17 Static Context Lifecycle Barrier

The `context` target now has a gated 25-byte
`static context lifecycle` input. In the external-default-callback build it
passes the actual `secp256k1_context_static` singleton through randomization
with and without a seed, heap cloning, preallocated clone-size reporting,
both callback setters, and both destroy APIs. Every unsupported operation must
return its documented failure result, invoke the default illegal callback once,
and leave the singleton untouched. The helper is compiled as a no-op in the
ordinary callback build because the default callback aborts by design.

This is **Informational oracle hardening**, not a clean-master production
finding. The deterministic suite already checks much of the same policy using
a writable copy of the static context; the fuzzer previously did not bind the
actual singleton and count the public callback routing across all lifecycle
entry points. The clean-master severity ledger is unchanged: malformed opaque
state, callback-failure memory safety, and secret-state lifetime remain
**Medium** where previously proven; the reachable forced-int64 magnitude
boundary remains **Medium/latent**; bounded internal arithmetic and documented
tweak-input overlap remain **Low/latent**; and a public or
non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable production mutation changed only the
`ARG_CHECK(secp256k1_context_is_proper(ctx))` guard in
`secp256k1_context_randomize` to `ARG_CHECK(1)`. The exact
`src/fuzz/corpora/context/static-context-lifecycle` input exited 134 under
the external Clang 22.1.7 ASan/UBSan build with `-handle_abrt=0`; the restored
control exited 0. The restored 12-file context corpus passed with
`-workers=2 -jobs=2 -runs=1 -timeout=60 -rss_limit_mb=0` under native
5x52, external-callback native, and external-callback forced-int64/10x26
Clang ASan/UBSan builds. The static-context unit and no-VERIFY slices also
passed. The mutation was restored before this commit and no production
behavior changed.

## 2026-07-17 Static Context SHA Backend Barrier

The existing `context/static-context-lifecycle` seed now also calls
`secp256k1_context_set_sha256_compression(secp256k1_context_static, NULL)` and
requires exactly one additional default illegal callback. This closes the
remaining context-lifecycle hole around the mutable SHA backend: the setter
must reject the actual read-only singleton before `secp256k1_hash_ctx_init`
could write its state. The check is kept in the external-default-callback
configuration because the ordinary default callback aborts by design.

This is **Informational oracle hardening**, not a clean-master production
finding. The setter's documented input is a context object, while its
implementation requires a proper writable context; the existing deterministic
SHA callback tests exercised dynamically allocated contexts only. No
master-relative severity changes: malformed opaque state, callback-failure
memory safety, and secret-state lifetime remain **Medium** where proven; the
forced-int64 magnitude boundary remains **Medium/latent**; bounded/documented
alias cases remain **Low/latent**; cleanup and model checks remain
**Informational**. A public or non-cryptographic nonce buffer is not a
Critical erasure finding.

For causal proof, a disposable mutation changed only
`ARG_CHECK_VOID(secp256k1_context_is_proper(ctx))` in
`secp256k1_context_set_sha256_compression` to `ARG_CHECK_VOID(1)`. The exact
existing lifecycle seed then produced an ASan write-side `SEGV` in the static
singleton; the restored control exited zero. This is the intended causal
failure: the guard prevents a setter that writes `hash_ctx` from reaching the
read-only object. After restoration, the focused seed and complete 12-file
context corpus passed with two workers and two jobs on native 5x52,
external-callback native, and external-callback forced-int64/10x26 Clang
ASan/UBSan builds. The static-context unit and no-VERIFY slices passed as
well. The mutation was restored before commit and no production behavior
changed.

## 2026-07-17 Static Context Public Combine Boundary

The `api_roundtrip/static-context-public-combine` fixture now parses fixed
compressed SEC1 encodings for `G`, `2G`, and `-G` through
`secp256k1_context_static`. It checks `G + 2G = 3G` against the fixed
compressed `3G` wire vector in both operand orders, then checks that `G + -G`
returns failure and zeroes the opaque output. The expected result is therefore
independent of the fuzzer's generated secret keys and of a dynamic-context
combine call.

This is **Informational oracle hardening**, not a clean-master production
finding. The public header does not restrict `secp256k1_ec_pubkey_combine` to
proper writable contexts, and the deterministic suite covered the operation
only with its writable test context. No master-relative severity changes: the
existing malformed opaque state, callback-failure memory-safety, and secret
state lifetime findings remain **Medium** where proven; the forced-int64
magnitude boundary remains **Medium/latent**; bounded/documented alias cases
remain **Low/latent**; cleanup and model checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable
`ARG_CHECK(ctx != secp256k1_context_static);` was inserted immediately after
the context check in `secp256k1_ec_pubkey_combine`. The exact fixture exited
134 under the external-callback Clang ASan/UBSan build with `-handle_abrt=0`.
After restoring the production code, the complete 48-file API-roundtrip
corpus passed with `-workers=2 -jobs=2 -runs=1 -timeout=60
-rss_limit_mb=0` on native 5x52, external-callback native, and
external-callback forced-int64/10x26. The combine-focused deterministic test
`ec_illegal_argument_tests` passed with one iteration on all three builds.
The mutation was restored before commit and no production behavior or severity
rating changed.

## 2026-07-17 Schnorrsig Bounded Discovery

The existing 15-file `schnorrsig` corpus was fuzzed in isolated disposable
copies with `-workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Native Clang jobs completed 237 and 243
executions, external-callback Clang jobs completed 240 and 237, and
external-callback forced-int64/10x26 jobs completed 139 and 138. All six
jobs exited zero with no sanitizer report, oracle failure, crash artifact, or
timeout. Generated mutation units were discarded. This is negative
clean-master evidence: no production finding or severity change was
established, and the existing Medium/Medium-latent, Low/latent,
Informational, and non-Critical nonce-cleanup ratings remain in force.

## 2026-07-17 Recoverable ECDSA Signing Alias Boundary

The `recovery/recoverable-sign-input-output-overlap` fixture checks exact
aliasing of `secp256k1_ecdsa_sign_recoverable` output with the message hash and
with the secret key. Its independent fixed vector uses `z = 0`, `d = 1`, and
`k = 1`, so `r = s = x(G)` and `recid = 0`; the check serializes all three
output fields after each alias case. This is a separate optional-module
boundary from the normal ECDSA signer because it exercises the 65-byte
recoverable object and recovery-id save path.

This is **Informational alias-contract hardening**, not a clean-master
production finding. The recovery header describes the input and output roles
but does not state a non-overlap precondition; master produced the fixed
signature consistently in native Clang, external-callback Clang, and external-
callback forced-int64/10x26 Clang ASan/UBSan builds. Existing recovery tests
and the fuzzer used disjoint signing buffers, so they did not exercise this
read-before-save transition. The master-relative severity ledger is unchanged:
malformed opaque state, callback-failure memory safety, and secret-state
lifetime remain **Medium** where proven; the forced-int64 magnitude boundary
remains **Medium/latent**; bounded/documented aliases remain **Low/latent**;
cleanup and model checks remain **Informational**. A public or
non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable `memset(signature, 0, sizeof(*signature));`
was inserted immediately after the output argument check in
`secp256k1_ecdsa_sign_recoverable`. The exact fixture exited 134 in all three
builds with `-handle_abrt=0`, while the previous 13-file recovery corpus
passed 14 executions per job in both worker jobs for every configuration.
After restoration, the 14-file corpus including the new fixture passed 15
executions per job in both worker jobs for every configuration, with no
sanitizer report, oracle failure, or crash artifact. The mutation was restored
before commit; no production behavior or severity rating changed.

The bounded follow-up used isolated copies of the same 14-file corpus with
`-workers=2 -jobs=2 -max_total_time=30 -timeout=60 -rss_limit_mb=0
-handle_abrt=0`. Native jobs completed 426 and 429 executions, external-
callback jobs completed 434 and 435, and external-callback
forced-int64/10x26 jobs completed 262 and 260. All six jobs exited zero with
no sanitizer report, oracle failure, crash artifact, or master-relative
severity change; generated mutation units were discarded with the temporary
copies.

## 2026-07-17 Tagged SHA Output Overlap Boundary

The `context/tagged-sha256-output-overlap` fixture compares the public tagged
hash API against the standalone SHA transcript reference while making the
32-byte output exactly equal to the tag storage in one case and overlap the
message storage at an interior offset in another. Guard bytes around the
overlaps are checked as well. This exercises the natural read-before-finalize
ordering that the existing disjoint-buffer checks could not observe.

This is **Informational alias-contract hardening**, not a clean-master
production finding. The public documentation does not state a non-overlap
precondition for `hash32`, `tag`, or `msg`, and no failure or digest mismatch
was found on master. The severity ledger is unchanged: malformed opaque state,
callback-failure memory safety, and secret-state lifetime remain **Medium**
where proven; the forced-int64 magnitude boundary remains
**Medium/latent**; bounded/documented aliases remain **Low/latent**; cleanup
and model checks remain **Informational**. A public or non-cryptographic nonce
buffer is not a Critical erasure finding.

For causal proof, a disposable `memset(hash32, 0, 32);` was inserted
immediately after the output argument check in `secp256k1_tagged_sha256`. The
new exact seed exited 134 under the external-callback Clang ASan/UBSan build
with `-handle_abrt=0`, while the previous 12-file context corpus passed in
both worker jobs under the same mutant. After restoration, the complete
13-file corpus passed with `-workers=2 -jobs=2 -runs=1 -timeout=60
-rss_limit_mb=0` on native 5x52, external-callback native, and
external-callback forced-int64/10x26; `tagged_sha256_tests` also passed with
one iteration on all three builds. The mutation was restored before commit
and no production behavior or severity rating changed.

## 2026-07-17 Context Bounded Discovery

The expanded 13-file `context` corpus was then fuzzed in disposable copies
with `-workers=2 -jobs=2 -max_total_time=45 -timeout=60 -rss_limit_mb=0`.
The native 5x52 jobs completed 597 and 602 executions, the external-callback
native jobs completed 595 and 598, and the external-callback
forced-int64/10x26 jobs completed 358 and 360. Every job exited zero with no
sanitizer report, oracle failure, or crash artifact. Newly discovered mutation
corpus units were discarded after each run. This is a negative clean-master
result: no production finding or severity change was established, and the
existing Medium/Medium-latent, Low/latent, Informational, and non-Critical
nonce-cleanup ratings remain in force.

## 2026-07-17 API Roundtrip Post-Combine Discovery

After adding the fixed-wire static-context combine oracle, the 48-file
`api_roundtrip` corpus was fuzzed in disposable copies with
`-workers=2 -jobs=2 -max_total_time=45 -timeout=60 -rss_limit_mb=0`.
Native 5x52 jobs completed 468 and 469 executions, external-callback native
jobs completed 475 and 476, and external-callback forced-int64/10x26 jobs
completed 282 each. All six jobs exited zero with no sanitizer report, oracle
failure, or crash artifact. Newly discovered mutation corpus units were
discarded. This is a negative clean-master result: no production finding or
severity change was established, and the existing Medium/Medium-latent,
Low/latent, Informational, and non-Critical nonce-cleanup ratings remain in
force.

## 2026-07-17 ECDSA Signing Alias Boundary

The `api_roundtrip/ecdsa-sign-input-output-overlap` fixture checks exact
aliasing of the `secp256k1_ecdsa_sign` output with each 32-byte input. It uses
the independent `d = z = k = 1` equation vector, so the expected compact
signature is fixed rather than copied from a disjoint signing call. The
message/output and seckey/output cases both pass on master in native Clang,
external-callback Clang, and external-callback forced-int64/10x26 Clang
ASan/UBSan builds.

This is **Informational alias-contract hardening**, not a clean-master
production finding. The public header specifies the input and output roles
but does not state a non-overlap precondition; no master failure, sanitizer
report, or inconsistent signature was observed. The severity ledger is
unchanged: malformed opaque state, callback-failure memory safety, and
secret-state lifetime remain **Medium** where proven; the forced-int64
magnitude boundary remains **Medium/latent**; bounded/documented aliases
remain **Low/latent**; cleanup and model checks remain **Informational**. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

For causal proof, a disposable `memset(signature, 0, sizeof(*signature));`
was inserted immediately after the output argument check in
`secp256k1_ecdsa_sign`. The exact fixture exited 134 under all three builds
with `-handle_abrt=0`, while the previous 48-file API corpus passed 49
executions per job in both worker jobs for every configuration. After the
mutation was restored, the 49-file corpus including the new fixture passed
50 executions per job in both worker jobs for every configuration. The
mutation was restored before commit; no production behavior or severity
rating changed.

## 2026-07-17 MuSig Bounded Stateful Discovery

The existing 70-file MuSig corpus was fuzzed in isolated disposable copies
with `-workers=2 -jobs=2 -max_total_time=30 -timeout=60 -rss_limit_mb=0
-handle_abrt=0`. Native Clang jobs completed 71 runs each in 85 seconds,
external-callback Clang jobs completed 71 runs each in 85 seconds, and
external-callback forced-int64/10x26 jobs completed 71 runs each in 146 and
147 seconds. All six jobs exited zero with no sanitizer report, oracle
failure, crash artifact, or command-level timeout. The longer forced-int64
wall time was an expensive existing corpus/reference transition, not a
production timeout or availability finding. Generated mutation units were
discarded. No clean-master production finding or severity change was
established; the existing Medium/Medium-latent, Low/latent, Informational,
and non-Critical nonce-cleanup ratings remain in force.

## 2026-07-17 Ecmult-Multi and Full-Test Verification

The current 24-file `ecmult_multi` corpus was replayed from isolated copies
with `-workers=2 -jobs=2 -max_total_time=30 -timeout=60 -rss_limit_mb=0
-handle_abrt=0`. Native 5x52 jobs completed 82 and 85 executions,
external-callback native jobs completed 117 each, and external-callback
forced-int64/10x26 jobs completed 35 and 36. Every job exited zero without a
sanitizer report, oracle failure, crash, or command-level timeout. The
forced-int64 jobs recorded the existing `pippenger window 1261` input as an
11-second libFuzzer slow unit; the generated slow-unit files were discarded
with the temporary corpus. This is expensive independent-reference work,
not a production availability finding.

After the rebase check, the current Clang ASan/UBSan build also passed all 224
CTest cases with `ctest --output-on-failure -j2`, including verify and
no-verify slices and all enabled optional modules. No clean-master production
finding or master-relative severity change was established. Existing
malformed-state, callback-failure, secret-state-lifetime, and forced-int64
arithmetic findings retain their recorded ratings; public or
non-cryptographic nonce material is not a Critical erasure finding.

## 2026-07-17 l0rinc 10x26 Follow-up Reconciliation

The fetched fork branch `l0rinc/l0rinc/field-10x26-normalize-overflow`
points to `b938a5d` (`field_10x26: avoid normalize overflow`). It was reviewed
against the current audit branch and deliberately not cherry-picked: the
branch already contains the same production behavior in two independently
documented commits. `0d03dda` repairs the first-pass carry in
`normalize`, `normalize_var`, and `normalize_weak`; `0346c09` separately
repairs both `normalizes_to_zero` predicates, adds the exact false-zero corpus
unit, and keeps an independent canonical-byte oracle. The current source also
contains the later word-serialization change, so applying `b938a5d` directly
would conflict with unrelated edits and replace stronger split regression
coverage with an alternate formulation.

The arithmetic formulations are equivalent: the fork's
`x * 0x1000003D1ULL` combines the current implementation's
`x * 0x3D1UL` and `(x << 6)` carry into one 64-bit expression. All five
affected paths are covered in the current tree. The detached `b938a5d` tree
passed `tests -t=fe_normalize_max_magnitude` and its complete randomized test
binary under forced-int64/10x26. The current fixed tree passed the same
focused test in both default and forced-int64 Clang ASan/UBSan builds, and all
20 tracked field corpus inputs passed in the forced-int64 sanitizer binary.

The clean-master severity proof was repeated at current `origin/master`
`11dad6d`: a disposable test-only replay of the exact nonzero state
`n[0]=0xffff0f91`, `n[1]=0xfffff040`, `n[9]=0x0fc00000` aborted at the first
`normalizes_to_zero` assertion with exit 134 under forced-int64/10x26. The
state represents `63*p + 2^58 + 2^32`, whose canonical residue is
`2^58 + 2^32`. This remains a distinct **Medium/latent internal field
correctness** finding on master, with potentially High arithmetic impact if
the documented magnitude-32 state is reached; it is not a demonstrated
remote key or signature vulnerability. The fork follow-up therefore adds no
new finding and does not reduce the original severity or hide the zero-
predicate bug behind its normalize repair.

## 2026-07-17 MuSig MemorySanitizer Worker Campaign

The current audit tree was built with Clang 22.1.7 MemorySanitizer and origin
tracking using `-fsanitize=memory -fsanitize-memory-track-origins=2` and
`-fno-omit-frame-pointer`, with assembly disabled and the libFuzzer runtime
enabled. The complete 68-file MuSig corpus was copied to a disposable
directory and replayed with `-runs=1 -timeout=60 -rss_limit_mb=0`. Every seed
completed with exit zero under `MSAN_OPTIONS=halt_on_error=1:abort_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`.

The same corpus then ran with `-verbosity=0 -workers=2 -jobs=2
-max_total_time=30 -timeout=60 -rss_limit_mb=0 -handle_abrt=0`. Both jobs
loaded all 68 seeds and exited zero; no MSan/UBSan diagnostic, assertion,
timeout, OOM, or crash artifact was produced. Temporary corpus and artifact
directories were kept outside the repository. This is negative sanitizer
evidence, not proof that clean master is defect-free and not a new production
finding; the existing master-relative Medium, Medium/latent, Low/latent, and
Informational ratings remain unchanged. A public or non-cryptographic nonce
buffer is not a Critical erasure finding.

## 2026-07-17 Ecmult-Multi MemorySanitizer Worker Campaign

The current audit tree's `fuzz_ecmult_multi` was built with Clang 22.1.7
MemorySanitizer, origin tracking, assembly disabled, and the libFuzzer
runtime enabled. A fixed-input replay of all 24 tracked `ecmult_multi`
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 24 seeds and exited
zero. There were no MSan/UBSan diagnostics, assertions, crashes, command
timeouts, OOMs, or artifact files. The existing high-window Pippenger input
made this campaign substantially slower under origin tracking, consistent
with the earlier ASan/UBSan campaign's documented expensive reference work;
that wall time is not a production availability finding. Temporary corpus
and artifact directories were outside the repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Ecmult-Const MemorySanitizer Worker Campaign

The current audit tree's `fuzz_ecmult_const` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 8 tracked
`ecmult_const` seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 8 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This includes affine-reference, canonical-infinity,
generator-alias, nonnormalized-fraction, odd-multiple, and x-only fraction
cases. Temporary corpus and artifact directories were outside the
repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Group MemorySanitizer Worker Campaign

The current audit tree's `fuzz_group` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 21 tracked group
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 21 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This includes affine/Jacobian cancellation,
infinity, batch conversion, nonnormalized storage, and the off-curve
opaque-public-key barrier. Temporary corpus and artifact directories were
outside the repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Hash MemorySanitizer Worker Campaign

The current audit tree's `fuzz_hash` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 10 tracked hash
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 10 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This includes HMAC independent-reference and
finalization/reuse, RFC6979 stream boundaries, SHA midstate, and one-shot
output cases. Temporary corpus and artifact directories were outside the
repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Scalar MemorySanitizer Worker Campaign

The current audit tree's `fuzz_scalar` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 7 tracked scalar
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 7 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This includes the shift-width, inverse, lambda,
wNAF, conditional-add, and bit-boundary cases. Temporary corpus and
artifact directories were outside the repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Field MemorySanitizer Worker Campaign

The current audit tree's `fuzz_field` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 20 tracked field
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 21 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 20 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This includes the forced-int64 magnitude-32,
normalization, zero-predicate, and independent-byte-reference cases.
Temporary corpus and artifact directories were outside the repository and
were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 API Roundtrip MemorySanitizer Worker Campaign

Recoverable ECDSA is compiled into the aggregate `fuzz_api_roundtrip`
target; this configuration has no standalone `fuzz_recovery` binary. The
current audit tree's aggregate target was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 49 tracked API seeds
completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 50 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 49 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. This covers the recovery compact/retry/equation and
signing-overlap seeds together with the other public API boundaries.
Temporary corpus and artifact directories were outside the repository and
were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Context MemorySanitizer Worker Campaign

The current audit tree's `fuzz_context` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 13 tracked context
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 14 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 13 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. The replay covered impossible SHA lengths, callback
replacement and rejection, clone/reset lifecycle, allocator paths, and
secret-operation equivalence. Temporary corpus and artifact directories
were outside the repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 X-Only Tweak MemorySanitizer Worker Campaign

The current audit tree's `fuzz_xonly_tweak` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 15 tracked X-only
tweak seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 16 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 15 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. The replay covered parity/tweak invariants, partial
opaque keypairs, input/output overlap, invalid tweaks, and static-context
projection. Temporary corpus and artifact directories were outside the
repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 ECDH MemorySanitizer Worker Campaign

The current audit tree's `fuzz_ecdh` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 8 tracked ECDH seeds
completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 9 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 8 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. The replay covered custom callback failure, invalid
secret fallback, static-context ECDH, and built-in hash cleanup. Temporary
corpus and artifact directories were outside the repository and were
removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Schnorrsig MemorySanitizer Worker Campaign

The current audit tree's `fuzz_schnorrsig` was run from the Clang 22.1.7
MemorySanitizer build with origin tracking, assembly disabled, and the
libFuzzer runtime enabled. A fixed-input replay of all 15 tracked Schnorr
seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 16 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 15 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. Temporary corpus and artifact directories were
outside the repository and were removed. The legacy sign alias emitted its
existing compile-time deprecation warning; it did not affect execution.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 EllSwift MemorySanitizer Worker Campaign

The current audit tree's `fuzz_ellswift` was run from the existing Clang
22.1.7 MemorySanitizer build with origin tracking, assembly disabled, and
the libFuzzer runtime enabled. A fixed-input replay of all 16 tracked
EllSwift seeds completed with exit zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`; libFuzzer completed its 17 total runs
including initialization.

The same isolated corpus then ran with
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Both jobs loaded all 16 seeds and exited
zero. No MSan/UBSan diagnostic, assertion, crash, command timeout, OOM, or
artifact was produced. Temporary corpus and artifact directories were
outside the repository and were removed.

This is negative sanitizer evidence, not proof that clean master is
defect-free and not a new production finding. The existing master-relative
Medium, Medium/latent, Low/latent, and Informational ratings remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Forced-Int64 External-Callback MemorySanitizer Campaign

The current audit tree was also configured in `/tmp/secp256k1-msan-int64-ext2`
with Clang 22.1.7, assembly disabled, MemorySanitizer origin tracking,
recovery enabled, `SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64`, and
`SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS=ON`. The configuration used the
following relevant options:

```
CC=clang CXX=clang++ cmake -S . -B /tmp/secp256k1-msan-int64-ext2 \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_BUILD_TESTS=OFF \
  -DSECP256K1_BUILD_BENCHMARK=OFF \
  -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF \
  -DSECP256K1_BUILD_CTIME_TESTS=OFF \
  -DSECP256K1_BUILD_EXAMPLES=OFF \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_FUZZ_USE_LIBFUZZER=ON \
  -DSECP256K1_ASM=OFF \
  -DSECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY=int64 \
  -DSECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_FLAGS='-O1 -g -fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=memory'
```

The complete corpus matrix covered all 14 enabled fuzz targets:
`fuzz_api_roundtrip` (49 seeds), `fuzz_context` (13), `fuzz_hash` (10),
`fuzz_scalar` (7), `fuzz_field` (20), `fuzz_group` (21),
`fuzz_ecmult_const` (8), `fuzz_ecmult_multi` (24), `fuzz_ecdh` (8),
`fuzz_ellswift` (16), `fuzz_xonly_tweak` (15), `fuzz_recovery` (14),
`fuzz_schnorrsig` (15), and `fuzz_musig` (68). Fixed-input replays for
every target exited zero under
`MSAN_OPTIONS=halt_on_error=1:abort_on_error=1:exit_code=86` and
`UBSAN_OPTIONS=halt_on_error=1`. Each target then ran with the same private
corpus and
`-verbosity=0 -workers=2 -jobs=2 -max_total_time=30 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`; both workers/jobs for every target exited
zero and produced no artifact. A post-run log scan found no MSan/UBSan
diagnostic, assertion, crash, command timeout, or OOM. The int64 MuSig
replay was slower because of its arithmetic backend, but it completed
without a production-availability failure.

This cross-configuration campaign is negative sanitizer evidence, not proof
that clean master is defect-free and not a new finding. It exercises the
existing API round-trip, MuSig state, and field oracles under a different
wide-multiply implementation and external callback projection. The
master-relative Medium, Medium/latent, Low/latent, and Informational ratings
remain unchanged. In particular, a public or non-cryptographic nonce buffer
is not a Critical erasure finding.

## 2026-07-17 Context-Clone Allocation-Failure Negative Control

The context boundary was checked with a disposable production mutation at
`secp256k1_context_clone`: immediately after `checked_malloc` returned, the
mutation replaced the result with `NULL` and explicitly replayed the
`"Out of memory"` error callback. A small public-API probe installed returning
error and illegal callbacks, then called `secp256k1_context_clone`. The mutated
build produced exactly one error callback, exactly one subsequent
`"prealloc != NULL"` illegal callback, and a NULL clone with exit zero. The
unmutated control returned a non-NULL clone and made neither callback call.

This is not a clean-master production finding. `checked_malloc` already routes
allocation failure through the context error callback, while the public error
callback documentation says that after a returning internal-error callback
anything may happen, including a crash. The follow-up
`secp256k1_context_preallocated_clone(ctx, NULL)` therefore cannot be used to
claim a violated return-value contract or a memory-safety defect. The exact
mutation also bypasses the allocator itself; it proves callback sequencing,
not an allocator implementation failure. No production or fuzzer change is
justified by this path, and existing master-relative severities remain
unchanged. A public or non-cryptographic nonce buffer is not a Critical
erasure finding.

## 2026-07-17 Parser Input/Output Alias Negative Control

A clean-master public-API probe used a valid generator encoding as both the
serialized input and the opaque output storage. On
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, both calls returned zero:

    ec_pubkey_parse overlap ret=0 bytes_equal=0
    xonly_pubkey_parse overlap ret=0 bytes_equal=0

This is expected from the implementations: `secp256k1_ec_pubkey_parse` and
`secp256k1_xonly_pubkey_parse` clear their `Out` object before reading the
separate `In` byte sequence. The headers do not describe either object as
`In/Out`; the full-key parser explicitly says failed output is undefined, and
the x-only parser promises only an invalid output on failure. Therefore this
is not a master-relative bug, and no parser alias oracle or production change
is justified. Treating this as supported would turn an undocumented pure
`Out` plus `In` overlap into a false positive, unlike the documented
`In/Out` tweak and session-random cases already covered above. The same
boundary applies to the counter-nonce path: its `Out` nonce objects and `In`
keypair are separate roles, while its defined keypair-loading and failure
cleanup transitions are already exercised by the existing corpus.

The probe was linked against the clean-master ASan/UBSan shared library and
ran with no sanitizer diagnostic. This negative control does not alter any
existing severity rating. A public or non-cryptographic nonce buffer is not a
Critical erasure finding.

## 2026-07-17 Rebase Check and ASan/UBSan State Recheck

The audit branch `codex/fuzz-oracles` was checked against both configured
remotes before this run. `origin/master` and `l0rinc/master` were both
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, and that commit was already an
ancestor of audit HEAD `e13cbec`. No rebase or cherry-pick was needed, so the
existing l0rinc reconciliation and its context notes remain in their original
commits.

The current Clang ASan/UBSan fuzz build replayed isolated copies of the
repository corpora with two workers and two jobs per target:

- `fuzz_api_roundtrip`: 49 seeds, 2,497 bytes; jobs completed 312 and 315
  executions and reached 4,004 coverage edges.
- `fuzz_musig`: 68 seeds, 2,673 bytes; both jobs completed 69 executions and
  reached 4,460 coverage edges. MuSig's state-heavy setup made the jobs take
  83 and 84 seconds despite the 30-second per-job fuzzing budget.
- `fuzz_ecmult_multi`: 24 seeds, 857 bytes; jobs completed 84 and 85
  executions and reached 3,649 coverage edges.

All six workers exited zero. There was no ASan/UBSan diagnostic, assertion
failure, timeout, OOM, crash artifact, or corpus artifact left in the
repository. These are negative sanitizer and worker results, not proof that
clean master is defect-free. They found no new clean-master production bug and
do not change the existing master-relative ratings: the confirmed Medium
scratch-wraparound issue, malformed opaque-state and callback-contract
findings, the Medium/latent 10x26 normalization defect, and the lower-severity
arithmetic and cleanup findings remain separately tracked. A public or
non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Scalar Rounded-Shift Boundary Mutation Recheck

The current upstream refs remain aligned at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, and that commit is already an
ancestor of this audit branch, so no rebase or cherry-pick was needed for this
recheck. The fixed `fuzz_scalar` binary was first run against the exact
39-byte corpus input `scalar/mul-shift-over-512`; both the native 5x52 ASan/
UBSan build and the forced-int64 10x26 ASan/UBSan build exited zero.

To test whether the boundary oracle is causal, a disposable production
mutation changed `if (shift > 512)` to `if (shift >= 512)` in both
`src/scalar_4x64_impl.h` and `src/scalar_8x32_impl.h`. The same corpus input
then aborted both binaries at the fuzzer's independent base-2^16 product
comparison: the reference retains the rounded `shift == 512` result, while
the mutation returns zero before computing it. Neither run emitted an
ASan/UBSan diagnostic; the failure was the intended `FUZZ_CHECK` abort. The
mutation was restored, both targets were rebuilt, and the fixed input was
replayed successfully on both backends.

This is regression evidence for the existing scalar rounded-shift guard, not
a new clean-master finding. The clean-master `shift > 512` memory-safety edge
and the branch's fix remain **Low/latent master-relative** because the path is
internal and requires an invalid over-width shift; the `shift == 512` boundary
is a distinct valid contract and is now shown to be protected by an
independent oracle. No production change or severity adjustment is justified.

## 2026-07-17 Current Clean-Master Recovery/Schnorr/API Reiteration

The configured `origin/master` and `l0rinc/master` refs both remain at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, already present in
`codex/fuzz-oracles`; no rebase or additional cherry-pick was needed. For this
pass, a disposable source worktree was detached at that exact master commit
and received only the existing fuzzer/CMake projection, the baseline-only
`SECP256K1_SHA256_MAX_SIZE` compatibility definition, and temporary
file/line reporting for `FUZZ_CHECK`. No production file in that baseline was
changed.

The same focused inputs were replayed with Clang 22.1.7 ASan/UBSan binaries,
`-runs=1 -handle_abrt=0`, against clean master and the fixed audit branch:

- `context/sha256-impossible-lengths`: clean master timed out after an ASan
  heap-buffer-overflow while the impossible tagged-SHA length was consumed;
  the fixed branch exited zero. This reiterates the **Medium, low practical
  exploitability** impossible-SHA-length contract finding from `ab36b78`/
  `e5d3e19`, not a new remote-sized-input claim.
- `schnorrsig/sign32-custom` and the dedicated
  `schnorrsig/sha256-impossible-lengths` input: clean master reached UBSan's
  non-zero offset from a null pointer at `src/util.h:438` and timed out;
  the fixed branch exited zero. This is the Schnorr projection of the same
  **Medium, low practical exploitability** length-boundary finding.
- `api_roundtrip/privkey-der-export-failure`: clean master failed the output
  oracle at `src/fuzz/api_roundtrip.c:2654`, showing stale bytes in the
  documented 279-byte failed-export region; the fixed branch exited zero.
  This remains **Low** (`36a009f`), because the return value is zero and the
  bytes beyond the documented capacity are intentionally preserved.
- `recovery/recoverable-compact` and `recovery/recovery-point-equation`: clean
  master failed `src/fuzz/recovery.c:733`, where an explicitly exported
  RFC6979 callback did not use the caller's SHA compression backend; the fixed
  branch exited zero. This reiterates the **Low** dispatch/performance finding
  in `c8870e5`; it does not imply nonce reuse, forgery, or a changed signature.
- `recovery/opaque-recoverable-signature-state`: the normal clean-master run
  hits the preceding callback assertion, so the disposable harness briefly
  bypassed only all three callback-routing assertions to isolate the later
  state barrier. With those assertions bypassed, clean master aborted at
  `src/scalar_impl.h:43` while serializing an opaque signature whose `r` or
  `s` bytes were replaced by `0xff`; the fixed branch exited zero. The
  callback assertions were restored and the clean baseline rebuilt. This is
  the existing **Medium** malformed opaque recoverable-signature state finding
  fixed by `b76fd58`, not a second bug: the raw opaque object has no public
  parser, but corrupted/directly constructed state must not reach scalar
  invariants or trust a magic-free recovery-id byte.

The isolation is important for severity: the callback-routing defect can mask
the malformed-state defect in a combined fuzzer input, but fixing or
cherry-picking the former does not make the latter disappear on clean master.
All fixed-branch replays completed without sanitizer diagnostics, and no new
production patch is justified by this reiteration. The exact production fixes,
mutations, deterministic tests, and verifier commands remain in their commit
messages; this entry records the current master-tip proof and the ordering
dependency between the oracles. A public or non-cryptographic nonce buffer is
not a Critical erasure finding.

## 2026-07-17 Current Clean-Master MuSig State Ordering

The five focused MuSig state inputs
`opaque-nonce-state`, `noncanonical-nonce-storage`,
`keyagg-cache-semantic-state`, `partial-keypair-nonce-counter-invalid`, and
`overflow1-secnonce-scalar` all pass on the fixed Clang 22.1.7 ASan/UBSan
branch. On the clean-master projection at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`, the ordinary
`opaque-nonce-state` replay first aborts in
`src/field_5x52_impl.h:29`. This is the existing **Medium** noncanonical
opaque-public-key/nonce-storage boundary, not evidence that the later nonce
state is safe or unsafe by itself.

To make the ordering explicit, disposable harness-only isolation bypassed
each earlier helper one at a time and rebuilt after every change. The clean
baseline then reached these existing master failures:

- `fuzz_musig.c:1878` and `fuzz_musig.c:1909`: invalid and zero-sized
  `musig_pubkey_agg` calls leave a poisoned keyagg-cache output nonzero. This
  is the **Low to Medium** stale-state finding fixed by `5c0977c`; it requires
  ignoring a failed return and is not a nonce-erasure issue.
- `fuzz_musig.c:1969`: an invalid opaque cache/public point reaches public-key
  serialization instead of a clean rejection. This is the **Medium** cache
  point-boundary finding fixed by `f4ef1c0`.
- `fuzz_musig.c:2047`: an invalid cache parity byte reaches public tweaking.
  This is the **Medium** semantic-cache finding fixed by `645a3bd`.
- `fuzz_musig.c:3384` (and the dedicated partial-keypair seed at
  `fuzz_musig.c:3413`): a keypair with mismatched secret/public halves reaches
  nonce generation. This is the **Medium** inconsistent opaque-keypair
  finding fixed by `5f8416e`.
- After those barriers were isolated, `fuzz_musig.c:3540` showed a malformed
  opaque public-nonce point reaching serialization. This remains the
  **Medium** opaque MuSig nonce-state finding fixed by `d4a62f0`/
  `394cc5d`; the dedicated partial-keypair input still stopped at its earlier
  cleanup assertion, so no stronger scalar claim is made from that input.

The isolation bypassed only fuzzer helper calls and was fully restored; the
clean projection was rebuilt and rerun, reproducing the first
`field_5x52_impl.h:29` abort. This ordering matters for severity and proof:
fixing a noncanonical point or stale cache output does not make a later
secret-nonce or session transition valid on clean master. No new production
bug is claimed in this pass, and no nonce cleanup is rated Critical merely
because a public/non-cryptographic nonce object was invalidated. The existing
production commit messages retain the deterministic tests, exact mutations,
and verifier commands for each finding.

## 2026-07-17 Fresh ASan/UBSan Multi-Worker Exploration

The current fixed Clang 22.1.7 ASan/UBSan build ran fresh generated-input
campaigns for `fuzz_ecmult_multi` and `fuzz_musig`. Each target started from a
private copy of its tracked corpus (`24` and `68` seed files respectively)
and used `-workers=2 -jobs=2 -max_total_time=60 -timeout=60
-rss_limit_mb=0 -handle_abrt=0`. All four job managers returned exit zero.
The ecmult campaign reached the independent affine-double oracle at
`src/fuzz/ecmult_multi.c:675`; neither target emitted an ASan/UBSan report,
`FUZZ_CHECK` failure, crash artifact, command timeout, or OOM. The temporary
corpora and generated units were removed after the jobs completed.

This is fresh negative sanitizer and worker evidence, not proof that clean
master is defect-free and not a new production finding. It does not change
the existing master-relative Medium, Medium/latent, Low/latent, or
Informational ratings. The clean-master differential replays and exact
production mutations remain the stronger evidence for those ratings. A
public or non-cryptographic nonce buffer is not a Critical erasure finding.

## 2026-07-17 Forced-Int64 Arithmetic Recheck

The fixed Clang 22.1.7 ASan/UBSan build configured with forced 64-bit
arithmetic also completed isolated multi-worker campaigns for `fuzz_field`
and `fuzz_scalar`. Each target used a private copy of its tracked corpus
(`20` and `7` seed files respectively), `-workers=2 -jobs=2`,
`-max_total_time=35 -timeout=60 -rss_limit_mb=0 -handle_abrt=0`, and the
same fail-fast sanitizer environment. The field jobs reported 20 input files
and the scalar jobs reported 7 in every worker; both managers returned zero.
No ASan/UBSan report, `FUZZ_CHECK` failure, crash artifact, timeout, or OOM
was observed, and both temporary corpora were removed afterward.

This is negative evidence for the forced-int64 arithmetic configuration only.
It neither downgrades the existing Medium/latent 10x26 normalization finding
nor claims that clean master is defect-free. The earlier cross-target log
capture was excluded because concurrent libFuzzer job managers shared their
worker-log names; only the isolated runs above are part of this record.

## 2026-07-17 Full ASan/UBSan Test Verification

After the worker campaigns, `ctest --test-dir /tmp/secp256k1-next-asan
--output-on-failure -j2` completed all `224/224` registered tests. The final
CTest log contains `224` `Test Passed.` records and no `Test Failed` or `Not
Run` records; this includes both the ordinary and no-verify test executables.
This verifies the current branch build and its deterministic regression suite
at the same fixed Clang 22.1.7 ASan/UBSan configuration. It is build/test
evidence only: it establishes no new production finding and does not alter
the master-relative severity ledger.

## 2026-07-17 Current Clean-Master Keypair Replay Ordering

The tracked 51-byte input
`src/fuzz/corpora/schnorrsig/opaque-keypair-consistency` (the literal
`opaque keypair secret and public state consistency\n`) was replayed once
against a freshly rebuilt ASan/UBSan projection of clean master
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53` and once against the fixed branch.
The clean command was `fuzz_schnorrsig -runs=1 -timeout=60 <input>`; it
reported UBSan's `src/util.h:438` null-output-pointer failure and stopped in
`nonce_function_bip340_impl` while the harness intentionally exercised
`secp256k1_fuzz_check_bip340_nonce_failure_cleanup` at
`src/fuzz/schnorrsig.c:390`. The branch replay executed the same input to
completion with no sanitizer or `FUZZ_CHECK` failure.

This reiterates the existing **Medium** `e789b5e` finding: exported built-in
BIP340/RFC6979 nonce callbacks must reject invalid direct arguments rather
than dereference a NULL output buffer. It is a direct callback crash, not a
Critical nonce-erasure finding; the nonce object in this replay is public and
non-cryptographic. A disposable attempt to bypass only the earlier
`impossible_msglen` helper did not change the result, so that harness change
was restored and the clean projection was rebuilt. Consequently this replay
is not standalone proof for the later **Medium** inconsistent opaque-keypair
finding in `5f8416e`: its strongest proof remains that commit's minimal
production mutation and deterministic tests. No new production bug or
severity change is claimed.

## 2026-07-17 Focused Public-State Worker Recheck

The current branch was rechecked after confirming that `HEAD` is already based
on both `origin/master` and `l0rinc/master` at
`11dad6d06c0ea8fd6d9d423d32bddd18b70b8b53`; no rebase or additional fork
cherry-pick was needed. The Clang 22.1.7 ASan/UBSan build ran private copies
of the tracked `api_roundtrip` (`49` files), `context` (`13`), `ecmult_multi`
(`24`), and `musig` (`68`) corpora with
`-workers=2 -jobs=2 -max_total_time=45` for the first two targets and
`-max_total_time=60` for the latter two, plus `-timeout=60
-rss_limit_mb=0 -handle_abrt=0`. Each manager completed with exit status
zero; the worker logs showed no ASan/UBSan report, `FUZZ_CHECK` failure,
crash artifact, timeout, or OOM. The mutation phase expanded the target
corpora during the runs, but no generated input was retained.

This is fresh negative evidence for public API state transitions, context
lifecycle/reset paths, batch multiplication failure-state handling, and MuSig
nonce/cache/session transitions on the fixed branch. It is not a clean-master
replay and therefore does not establish that master is defect-free or change
the existing severity ledger. In particular, the existing **Medium** findings
remain rated against clean master, and the public, non-cryptographic MuSig
nonce object remains outside a Critical erasure classification. No new
production mutation, regression test, or bug claim is justified by this run.
