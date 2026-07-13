# secp256k1 Fuzz Oracles

Current upstream `master` did not contain tracked fuzz targets or seed corpora
before this directory was added. These targets are intentionally small public-API
oracles that exercise contract boundaries rather than only maximizing coverage.

Targets:

- `fuzz_api_roundtrip`: pubkey, two-, three-, and four-term public-key combine with intermediate-infinity transitions, ECDSA compact, arbitrary-signature verification equation, fixed- and variable-nonce equations, valid-nonce retry, empty/NULL/invalid sort, DER, independently parsed private-key DER, signing, verification, normalization
- `fuzz_context`: context randomize, clone, reset, invalid-flag rejection, deterministic signing consistency, and a standalone tagged-SHA reference
- `fuzz_hash`: shared standalone SHA-256 reference, raw-SHA256 HMAC reference, full-stream RFC6979 sequencing, chunking consistency, and finalized-state cleanup
- `fuzz_scalar`: scalar bit-extraction boundaries and rounded multiply-shift
  boundaries against independent byte/product references
- `fuzz_field`: internal field normalization, arithmetic, strict input parsing, encoding, add-int boundaries, maximum-magnitude consistency, and a byte-level maximum-residue reference
- `fuzz_group`: Jacobian/affine group-operation agreement, independent canonical-coordinate equality, fractional curve-membership, finite and mixed-infinity batch conversion, rescale aliasing, and state cleanup
- `fuzz_ecmult_const`: constant-time multiplication, affine generator conversion, and NULL-generator equivalence
- `fuzz_ecmult_multi`: internal scratch/no-scratch multi multiplication consistency, callback batching/failure barriers, scratch accounting, checked allocation multiplication, and defined scalar-state transitions
- `fuzz_ecdh`: ECDH symmetry with a standalone default-SHA reference and coordinate passthrough hashers
- `fuzz_ellswift`: EllSwift encode/decode, modulo-alias wire encodings, randomizer influence, inverse-branch round trips, an independent BIP324 decode vector and SHA transcript, XDH symmetry, built-in hash cleanup
- `fuzz_xonly_tweak`: x-only serialization, standalone byte-level curve-membership parsing, parity, tweak, keypair equivalence, partial keypair projections, invalid comparator ordering
- `fuzz_recovery`: recoverable ECDSA round trips, arbitrary parsed-signature recovery, independent recovery point equations, and valid-nonce retry when recovery is enabled
- `fuzz_schnorrsig`: Schnorr sign/verify, standalone BIP340 tagged-SHA reference, arbitrary-signature BIP340 verification equation, empty-message pointer equivalence, `sign32`/`sign_custom` equivalence, and an independent BIP340 point-equation model
- `fuzz_musig`: MuSig key aggregation, optional aggregate outputs, tweak equivalence, x-only-tweak signing, standalone tagged-SHA transcripts, nonce/signature round trips, counter-nonce optional-input equivalence, mixed-infinity effective-nonce modeling, arbitrary parseable partial-signature verification equations, and independent partial- and final-signature point equations

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

The upstream exhaustive ECDH module model was also replayed on 2026-07-13.
Default ASan/UBSan order-13 and order-7 binaries, plus the forced-int64/10x26
ASan/UBSan order-7 binary, each ran two iterations across every reduced-order
key combination and reported exit code 0 with `no problems found`. This is
reduced-model and cross-backend evidence only; it does not change any
master-relative severity rating.

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
  Direct RFC6979 callback attempts at `UINT_MAX` (`e862628`) are a separate
  availability bug: the unsigned retry loop can wrap and hang. These are
  public callback/API misuse paths, not wire-format attacks.
- **Medium:** HMAC/RFC6979 secret-state retention after finalization
  (`5cfe7f7`). This is a lifetime/cleanup finding rather than a demonstrated
  disclosure; severity must not be raised to critical without a memory-read
  primitive.
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
  The same target now includes a standalone SHA-256 compression, schedule, and
  padding model. It compares production one-shot and split writes against that
  model at lengths 0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 191, and 192,
  using a nonzero deterministic message pattern as well as the input-derived
  message. The seven tracked hash inputs total 317 bytes, including
  `sha256-independent-reference`; fixed replays pass on default, forced-int64,
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
- **Low:** impossible SHA256 lengths (`ab36b78`), scalar rounded-shift
  bounds (`422bab2`), EllSwift zero-`u` normalization (`119b407`), built-in
  ECDH failure-output cleanup (`bb15eb0`), NULL preallocated context
  storage (`a9253c2`), and partial `ecmult_multi` results after a later batch
  fails (`5bd9ae8`). The latter is an internal helper boundary: callers must
  honor the failure return, so it is state hygiene rather than a remotely
  reachable cryptographic defect.
- **Informational oracle hardening:** `fuzz_ecmult_multi` now forces a
  Pippenger invocation across two batches, independently checks the repeated
  point equation, and rejects callback failures immediately before and after
  the batch boundary. Clean master passes; using a relative callback offset
  for the second batch makes the focused `repeated-pippenger-batches` seed
  abort. This is internal callback-index and failure-state coverage, not a
  current-master production finding; the existing target already covered
  single-batch Pippenger and multi-batch Strauss behavior.
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
- **Informational oracle gaps:** EllSwift decode/XDH postconditions (`792f43f`),
  empty public-key aggregation (`c5c0afe`), ECDSA verification's invalid-
  opaque-key API boundary (`ef25d27`), and the public-key serializer's wrong
  flag-type boundary currently pass on master; their mutations prove that the
  harness would catch a regression, not that master is presently vulnerable.
  The existing `fuzz_ecmult_const` target also transitively covers
  `secp256k1_ge_table_set_globalz`: replacing the accumulated `zs` inverse with
  the per-entry `zr[i]` makes the `scalar-derived-xonly-fractions` seed abort,
  while the restored clean-master helper replays successfully. No duplicate
  helper-only assertion is needed; this is coverage evidence, not a new
  production finding.
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
  The Schnorr target also checks that custom nonce callbacks receive the
  normalized secret key and matching x-only public key. A mutation that passes
  the secret-key buffer in place of the x-only key still produces signatures
  accepted by ordinary verification, so this callback-domain contract needs
  its own oracle (this commit). It also exercises the documented empty-message
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

If a clean-master replay stops at an earlier known failure, isolate the later
contract with its dedicated seed or a minimal production mutation. Do not
claim the later behavior was tested merely because a follow-up fix lets the
full harness continue.

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
  trigger remains unproven.
- PR #11 (`d1dca5c`) repeats the checked `pubkey_load` return paths already
  covered by `5ad8052`, `f9f1a6e`, and `7767442`.
- PR #12 (`944932c`, force-updated from `e153e26` on 2026-07-13) is exactly the
  behavior-preserving 5x52 word-serialization optimization already recorded as
  `91e4f02`; the force update did not change its source tree. Its commit
  intentionally follows the master-based findings and states that it is not
  security evidence.

No new l0rinc commit was cherry-picked in this refresh: every relevant commit
is either already represented with stronger current-master proof, already in
master, or changes performance/comments without adding a contract. This keeps
the discovery order intact. Fork patches are never used to prove that a
current-master failure was absent; every important barrier still has its own
seed or minimal production mutation, and severity is always assigned against
the clean baseline before later fixes are applied.

The additional fork refs were audited on 2026-07-13. The
`musig-cleanup-failures` branch (`bb02b1e`) is an older cleanup stack whose
production edits are already represented by the current MuSig cleanup commits
and the stronger opaque-state barriers here; replaying it would duplicate or
remove those barriers. `detached10` through `detached19` are alternative
force-inline or field-CMOV optimization snapshots. `detached20` through
`detached22` are broader snapshots containing the same optimization and
behavior-changing stacks already classified above. None adds a distinct
clean-master finding or a reason to change an existing severity rating.
