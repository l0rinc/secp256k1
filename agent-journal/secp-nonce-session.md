# Goal 84: secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit

## Cycle 60: repeated MuSig participant keys

- Timestamp: `2026-07-28T05:00:04Z` draw recorded by the controller.
- Draw seed: `3695385067`.
- Eligible pool: `52 53 72 74 77 81 82 84 87 89 95 97`.
- Selected index: `7`; selected goal: `84` (`secp-nonce-session`).
- Audit branch/base: `codex/fuzz-oracles`, base `a46f7999ad5392d5f32dc27de1c6c71889996819`.
- Protected secp checkout: `/mnt/my_storage/secp256k1`, detached clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- Protected Core checkout: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`,
  with only its pre-existing `src/test/blockencodings_tests.cpp` modification
  and `fuzz-0.log`/`fuzz-1.log` untracked files.

### Hypothesis and trust boundary

Core accepts a `musig(KEY,KEY,...)` descriptor containing repeated participant
keys and computes the aggregate key, but its signing/PSBT state uses
`std::map<CPubKey, ...>` for participant nonces and partial signatures. A
repeated key therefore cannot occupy two participant slots. The hypothesis was
that this creates an unsafe partial state, nonce reuse, or an unexpected
successful signature; the trust boundary is a locally imported descriptor and
its wallet/PSBT signing state, not malformed opaque libsecp256k1 input.

The contract evidence is mixed. BIP327 v1.0.3 explicitly describes duplicate
individual public keys as optional application support and says applications
may reject them: <https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki>.
BIP390 v0.2.0 explicitly permits repeated participant public keys in a
`musig()` descriptor: <https://github.com/bitcoin/bips/blob/master/bip-0390.mediawiki>.
BIP373 defines MuSig2 PSBT participant keydata using participant public keys,
which does not itself provide a slot identifier:
<https://github.com/bitcoin/bips/blob/master/bip-0373.mediawiki>.
The current Core descriptor parser preserves duplicate participants in
`src/script/descriptor.cpp:613-813,1991-2043`; current signing code indexes
nonces and partial signatures by `CPubKey` in `src/musig.cpp:165-207,254-264`
and `src/script/sign.cpp:145-188,282-373`.

### Verification

The clean-master Core source was checked with `git status --short --branch`,
`git rev-parse HEAD`, `rg`, history/blame searches, and the descriptor,
signing, PSBT, and functional wallet callers. Existing tests cover distinct
participant keys and parallel rounds, but no repeated-key descriptor or
duplicate-slot PSBT state.

A disposable Core worktree at `/tmp/bitcoin-goal84-dup` was checked out at the
clean Core HEAD. It added only a temporary `bip328_tests` case for
`rawtr(musig(key1,key1,key2))`. The test expanded the descriptor to three
participants, confirmed the first two evaluated keys were equal, generated
valid nonces for the two unique private keys, and passed the resulting
two-entry `CPubKey`-keyed nonce map to `CreateMuSig2PartialSig`.

Commands and key output:

```text
cmake -S /tmp/bitcoin-goal84-dup -B /mnt/my_storage/bitcoin-goal84-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_BENCH=OFF \
  -DBUILD_GUI=OFF -DBUILD_FUZZ_BINARY=OFF -DWITH_ZMQ=OFF -DENABLE_WALLET=ON
ninja -C /mnt/my_storage/bitcoin-goal84-build test_bitcoin
[543/543] Linking CXX executable bin/test_bitcoin
/mnt/my_storage/bitcoin-goal84-build/bin/test_bitcoin \
  --run_test=bip328_tests/duplicate_participant_state --log_level=test_suite
Running 1 test case...
*** No errors detected
/mnt/my_storage/bitcoin-goal84-build/bin/test_bitcoin \
  --run_test=bip328_tests --log_level=test_suite
Running 4 test cases...
*** No errors detected
```

The focused test reproduced the expected behavior: descriptor expansion,
aggregate-key calculation, and unique-key nonce generation succeeded, while
partial signing returned null because the two-entry map cannot represent the
three participant slots. The failure occurred before libsecp256k1 consumed the
secret nonce. There was no crash, partial signature, nonce invalidation, or
nonce reuse. The full temporary `bip328_tests` suite also passed.

### Verdict

**Inconclusive as a compatibility contract question; dismissed as a current
security/correctness defect.** This is a safe, deterministic application
limitation, not an exploitable MuSig nonce or signing-state failure. BIP327's
application-level duplicate-support rule provides a basis for controlled
rejection, while BIP390's valid repeated-key descriptor syntax means silently
accepting the descriptor but being unable to sign remains worth documenting or
revisiting. The evidence does not justify a broad slot-indexed PSBT redesign,
nor does it justify rejecting duplicate descriptors at parse time because that
would change the currently accepted descriptor contract.

No production source change or Core fix commit was made. The temporary test,
worktree, and build are disposable and are removed after the journal commit.
The lead should be reopened only with an explicit Core/BIP373/BIP390 contract
decision, a real duplicate-key PSBT caller, or evidence that an external
implementation expects Core to sign repeated-key descriptors. Until then,
exclude this exact map-cardinality hypothesis from random re-selection while
keeping other MuSig session, nonce, Schnorr, and wrapper-state cells eligible.

### Handoff

Prior goal-84 evidence already covered libsecp nonce cleanup, failed-output
cleanup, malformed opaque state, keypair consistency, and session semantics.
The next distinct goal-84 cells are explicit public API lifecycle/error-output
contracts, Core PSBT duplicate-slot semantics after a specification decision,
and cross-wrapper parity. Preserve the source links, exact reproducer shape,
and the safe-failure verdict when reopening this goal.

## Cycle 67: nonce counter boundary and API-equivalence matrix

- Timestamp: `2026-07-28T09:02:00Z` draw recorded by the controller.
- Draw seed: `862797388`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`.
- Selected index: `4`; selected goal: `84` (`secp-nonce-session`).
- Audit branch/base: `codex/fuzz-oracles`, base `319d56edbc85f0c71b28ffd11efd689e8dc0874c`.
- Protected secp checkout: `/mnt/my_storage/secp256k1`, detached clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- Protected Core checkout: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`,
  with only its pre-existing `src/test/blockencodings_tests.cpp` modification
  and `fuzz-0.log`/`fuzz-1.log` untracked files.

### Hypothesis and trust boundary

The fresh hypothesis was that `secp256k1_musig_nonce_gen_counter` could mishandle
zero, 32-bit rollover, or the two 64-bit extremes, or could disagree with the
equivalent explicit nonce transcript. A failure could create repeated public
nonces for distinct counters or make the counter API derive different secret
nonce state from the documented byte transcript. The trust boundary is a
caller-provided unique counter and optional public message/cache/extra-input
transcript paired with a valid opaque keypair; the caller, not the library,
must prevent counter reuse.

The source writes the big-endian counter into the first eight bytes of a
zero-initialized 32-byte buffer at `src/modules/musig/session_impl.h:516-542`.
The existing fuzz helper mirrors that encoding at `src/fuzz/musig.c:2823-2832`.
Counter zero is an intentional special case: its 32-byte transcript is all
zero and `nonce_gen` rejects an all-zero `session_secrand32`, while the counter
API requires a keypair and can safely bind counter zero to that keypair.

### Verification

The independent probe `agent-journal/goal84-counter-boundary-probe.c`
enumerated counters `0, 1, 2, 2^32-2, 2^32-1, 2^32, 2^64-2, 2^64-1` across all
eight combinations of absent/present message, keyagg cache, and extra input.
For each of 64 cases it required counter generation success, deterministic
repeat output, distinct serialized output from the preceding boundary case,
and exact equivalence to `nonce_gen` with the same leading-eight-byte explicit
transcript for every nonzero counter. It separately required explicit
`nonce_gen` rejection for counter zero and checked that successful explicit
calls zeroed the session-random buffer.

The probe source SHA-256 is
`0c61dfe7c385c778d1b31f356eb66b22484ec1d47cadc1ef2e70b86cbfaee6c8`.
Commands and key output:

```text
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  /mnt/my_storage/goal84-counter-probe/audit-native-sanitized
PASS counter-boundary cases=64 combinations=8

ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  /mnt/my_storage/goal84-counter-probe/audit-int64-clang
PASS counter-boundary cases=64 combinations=8

ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  /mnt/my_storage/goal84-counter-probe/audit-int64-gcc
PASS counter-boundary cases=64 combinations=8

/mnt/my_storage/goal84-counter-probe/clean-origin-release
PASS counter-boundary cases=64 combinations=8

/mnt/my_storage/secp256k1-build-next/bin/tests -t=musig -log=1
16 iterations; all 12 MuSig tests passed; total execution time 0.976 sec
```

The initial probe also supplied useful negative controls: it failed at counter
zero when it incorrectly expected the explicit all-zero transcript to succeed,
and it failed at counter one when it incorrectly placed the counter in the
trailing eight bytes. Correcting those independent oracle mistakes produced
the four passing matrices above. No production mutation was needed because
the existing explicit-equivalence check and the edge matrix are both sensitive
to either encoding error.

### Verdict

**Dismissed.** No counter collision, endian/placement error, repeat
nondeterminism, sanitizer finding, or clean-master behavioral discrepancy was
found. Counter zero is a documented/API-compatible distinction rather than a
nonce-reuse defect. Master-relative severity is none and no production source
change is justified.

This evidence is x86_64-only and uses one fixed valid keypair plus fixed
optional values. It does not prove behavior on a big-endian host or cover
external language wrappers. Reopen with a wrapper mismatch, a non-x86
execution result, or a new caller that violates the unique-counter contract.
Keep explicit lifecycle/error-output and cross-wrapper cells eligible for later
goal-84 draws; exclude this exact counter-boundary cell.

## Cycle 78: randomized and cloned MuSig context matrix

- Timestamp: `2026-07-28T10:06:08Z`.
- Draw seed: `1056055882`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`.
- Selected index: `4`; selected goal: `84` (`secp-nonce-session`).
- Audit branch/base: `codex/fuzz-oracles`, cycle-start HEAD
  `1a638d64919979fe0a5ae28647524f2dc615048e`.
- Protected secp checkout: `/mnt/my_storage/secp256k1`, detached clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- Protected Core checkout: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, with only the pre-existing
  `src/test/blockencodings_tests.cpp` modification and `fuzz-0.log`/
  `fuzz-1.log` untracked files.

### Hypothesis and trust boundary

The context contract says generator blinding and context cloning/randomization
must not change public results. The existing context campaign compares ECDSA
and ordinary Schnorr signing across randomized, cloned, and preallocated
contexts, while the MuSig campaign checks complete nonce/session/signing
transcripts and custom SHA backends separately. The fresh hypothesis was that
MuSig could use a blinded generator table or cloned context state incorrectly,
producing different public nonces, partial signatures, or final signatures
for the same transcript. The trust boundary is a caller-created, full
signing context and valid keypairs, not malformed opaque state or concurrent
mutation of a context.

The relevant contract and implementation evidence is in
`include/secp256k1.h:28-52,286-292,313-326,427-445`,
`src/secp256k1.c:63-76,226-239`, `src/ecmult_gen_impl.h:300-340`, and the
MuSig state machine at `src/modules/musig/session_impl.h:350-470,580-790`.
The existing comparison boundaries are `src/fuzz/context.c:209-283,530-590`
and `src/fuzz/musig.c:3861-4000`; neither supplied this cross-campaign
randomized/clone MuSig matrix. The earlier MuSig cleanup, malformed-opaque,
counter-boundary, duplicate-key, and final-output investigations in this
journal and `src/fuzz/README.md` were searched and excluded from this cell.

### Verification

A disposable public-API C probe used fixed valid secret keys 1 and 2. Each
round used a 32-byte message filled with `0x30 + variant`, extra input filled
with `0x70 + variant`, and a nonzero session random filled with `0x10 +
variant`. Signer 0 used `musig_nonce_gen` with the message, cache, and extra
input; signer 1 used `musig_nonce_gen_counter` with counter `17 + variant`
and the same optional inputs. The probe then performed key aggregation,
nonce aggregation, nonce processing, both partial signs, both independent
partial-signature verifications, final aggregation, static-context Schnorr
verification, and serialization of the aggregate key, public nonces,
aggregate nonce, partial signatures, and final signature.

Four variants compared a context randomized with `seed_a` against a clone
randomized with a different `seed_b`. Four more variants compared the same
pair after both contexts were reset with `secp256k1_context_randomize(ctx,
NULL)`. The full serialized result was compared byte-for-byte for every
variant. The disposable probe source was removed after verification.

Commands and key output:

```text
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 /mnt/my_storage/goal84-musig-context-matrix-native-clang
PASS randomized-clone variant=0 digest=bc3280984d63da4c
PASS randomized-clone variant=1 digest=e43a9e3ace4d1307
PASS randomized-clone variant=2 digest=5a8003777f985cd1
PASS randomized-clone variant=3 digest=b2c4a46839dca473
PASS reset-clone variant=4 digest=54ccdd10d5c0d610
PASS reset-clone variant=5 digest=4964bd6f6c555dad
PASS reset-clone variant=6 digest=d00a49934dbd2182
PASS reset-clone variant=7 digest=f54cb68413539ab9
PASS musig-context-matrix cases=8
```

The same eight-case probe passed with the native and forced-int64 shared
libraries under Clang 22.1.7 and GCC 16.1.0 ASan/UBSan probe builds. The
optimized forced-int64 Clang RelWithDebInfo and GCC RelWithDebInfo libraries
also produced the identical eight digest sequence. Library artifact hashes
were `e950bcbaa242a7456689db7e15f10e631095bab98d8e14c95343138cbc6a2b40`
(native Debug), `4cb8d553730beacf6b5926cc341a430686bfb9caeb3e9a4a9af78b2a7a3cbd73`
(forced-int64 Debug), `da0de5ec30a5da409c1be0a3e33f790662b20dec666008af133d45832896e849`
(forced-int64 Clang RelWithDebInfo), and
`668bee4dd71f2c1a76ef2fcf2ccdabc529012f84cf6450e13b6c094c159ef865`
(forced-int64 GCC RelWithDebInfo).

The existing MuSig test suite independently passed all 12 test groups and 16
iterations in native Clang, forced-int64 Clang, and forced-int64 GCC builds.
The three runs reported `*** PASSED` for every group and total times of
0.978, 1.670, and 0.305 seconds respectively; no sanitizer or diagnostic
was emitted.

For mutation sensitivity, a disposable source overlay added one to the
partial-signature scalar immediately after its correct MuSig response was
computed. After `cmake --build /tmp/secp256k1-oracles-next-build-native
--target secp256k1 -j4`, the focused matrix exited 1 with
`FAIL partial_sig_verify`. The overlay was restored, the library rebuilt, and
the clean matrix passed again. This proves the independent partial/final
verification is sensitive to a real signing-state mutation; it is not a
claim that clean master contains that mutation.

### Verdict

**Dismissed.** No context-dependent MuSig output, clone divergence,
serialization mismatch, invalid final signature, or sanitizer finding was
observed. The result is informational negative evidence and no production
source change or repair commit is justified. There is no Bitcoin Core
consensus or invalid-block caller for this MuSig signing state; a real
regression would affect an authorized/application signing workflow rather
than block validation.

Limitations are x86_64-only execution, two signers, fixed keys, and the
public default SHA backend. The matrix exercises both random and counter
nonce APIs, optional transcript inputs, context reset, and a cloned context,
but not a non-x86 runtime or concurrent context use. Reopen this cell only
for a new context/backend/architecture change, a wrapper mismatch, or a
caller-specific failure. Keep remaining goal-84 lifecycle and cross-wrapper
cells eligible, but exclude this exact randomized/clone matrix.

### Handoff

No scratch probe, mutation, library rebuild, sanitizer, or test process
remains running. The next cycle should draw another eligible goal rather than
repeat the completed context matrix.

## Cycle 107: MuSig public-nonce aggregation permutation and cancellation

- **Controller selection:** Goal 84 (`secp-nonce-session`), selected by the
  uber-goal state as cycle 107; this is a fresh aggregation/state-machine cell,
  not the earlier repeated-participant, counter-boundary, or randomized-context
  work.
- **Timestamp:** 2026-07-28T15:09:25Z.
- **Audit checkout:** `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, clean source HEAD
  `fcc11a4c65c766168583c75bb873f3a2331f0606`.
- **Hypothesis and trust boundary:** `secp256k1_musig_nonce_agg` must produce
  the same aggregate public nonce and processed session for every permutation of
  valid, attacker-supplied public nonces. A Jacobian accumulation or exceptional
  point-handling error could make the result depend on input order, especially
  when an intermediate sum reaches infinity. The trust boundary is the public
  nonce array supplied by an untrusted aggregator or signer; the check does not
  assume that a public nonce is paired with a local secret nonce.

### Source and oracle

The implementation initializes both accumulators at infinity and adds every
loaded nonce point in `src/modules/musig/session_impl.h:546-563`; it then batch
converts both sums in `musig_nonce_agg` at `:565-585`. Session creation consumes
the aggregate at `:642-681`, including the BIP327 finite fallback when the
effective nonce is infinity. Existing fuzz coverage has targeted direct and
intermediate cancellation, but the goal journal had no permutation matrix.

A disposable C probe generated six valid counter-derived public nonces for
fixed keypairs, replaced nonce 1 with the exact two-point inverse of nonce 0
using public parse/negate/serialize APIs, and enumerated all `6! = 720` input
orders. For each order it compared the serialized 66-byte aggregate and all
133 session bytes against the baseline order. The inverse pair forces one
intermediate cancellation in some orders and a later cancellation in others.
The temporary probe was removed after execution; no source or production test
file was changed.

### Verification

The sanitized native probe was compiled with Clang 22.1.7 and
`-fsanitize=address,undefined`, linked to
`build-integrated-asan/lib/libsecp256k1.so.6.0.2`, and run with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. It printed exactly:

    PASS permutations=720 inverse_pair=1

The same probe under the forced-`int64` Clang RelWithDebInfo library
`/mnt/my_storage/secp256k1-build/oracles-next-int64/lib/libsecp256k1.so.6.0.2`
printed the same line. SHA-256 hashes were
`0e614e8603283b7918f0d7cb10e232ad24d679b017e1e54aec0d3cfa1fc3c877` for the
sanitized native library and
`da0de5ec30a5da409c1be0a3e33f790662b20dec666008af133d45832896e849` for the
forced-`int64` library. The focused commands

    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./bin/tests -t=musig -log=1
    ./bin/tests -t=musig -log=1

passed all 12 MuSig test groups in the sanitized native and forced-`int64`
builds; the native run took 6.949 seconds and the forced-`int64` run 0.316
seconds, with no sanitizer diagnostics.

### Mutation control and verdict

In a disposable source mutation only, the aggregation loop condition at
`session_impl.h:553` was changed from `i < n_pubnonces` to
`i + 1 < n_pubnonces`, omitting the final input. After rebuilding the
sanitized library, the probe failed at order `0,1,2,3,5,4` with
`FAIL aggregate order=0,1,2,3,5,4` and exit status 1. The mutation was restored,
the library rebuilt, and the clean probe passed again. This demonstrates that
the metamorphic oracle detects a real aggregation-input defect rather than
blindly accepting every output.

**Dismissed.** No order-dependent aggregate, session divergence, infinity
handling failure, sanitizer finding, or focused-test failure was observed. No
production source change or finding commit is justified.

The oracle is intentionally a permutation property: its baseline aggregate is
the clean implementation's first order, so it does not independently prove
the absolute group sum for arbitrary points. The exact inverse construction,
the intermediate-infinity orders, the production test vectors, and the
omitted-input mutation provide independent sensitivity for this cell. Evidence
is x86_64-only, uses six fixed keypairs, and covers native and forced-`int64`
representations but not other architectures, concurrent callers, or malformed
opaque nonce objects (already covered by separate fuzz cells).

### Handoff

The temporary probe and mutation are gone, the audit checkout is clean, and no
process remains running. Exclude this exact six-input permutation/cancellation
cell. Keep Goal 84 eligible for a distinct public signing or cross-wrapper
lifecycle hypothesis; otherwise draw another controller goal.

## Cycle 113: Core MuSig2 partial-sign failure nonce lifecycle

### Selection and scope

The controller selected Goal `84`, `secp-nonce-session`, with seed
`1104874022`, index `2`, from the eligible pool `77 82 84 87 95`, at
`2026-07-28T17:18:33Z`. Prior Goal84 cells for nonce retry, session cleanup,
counter boundaries, randomized/cloned contexts, and six-input public-nonce
permutation/cancellation were excluded.

This cycle tested a fresh cross-wrapper lifecycle hypothesis in Bitcoin Core:
`CreateMuSig2PartialSig` might return an error after libsecp has consumed a
secret nonce while leaving the Core `MuSig2SecNonce` wrapper marked valid. A
caller could then retain a stale nonce object, misread its state, or attempt a
reuse through a different path. The trust boundary is malformed or
mis-associated public nonce data supplied to the Core MuSig2 wrapper; the
secret key and secret nonce remain local.

### Source tracing

The protected Bitcoin Core checkout was inspected at base
`00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. In `src/musig.cpp:234-243`, the
wrapper calls `secp256k1_musig_partial_sign`, immediately calls
`secnonce.Invalidate()` on success at line 238, and only then performs its
defensive `secp256k1_musig_partial_sig_verify` at line 241. Thus a verification
failure cannot leave the wrapper valid. The `git blame` result traces this
ordering to the original MuSig2 helper commit `8ba5f68b1df`, rather than to a
recent incidental change.

The libsecp implementation in `src/modules/musig/session_impl.h` loads the
opaque nonce and explicitly zeroes the full nonce object before checking the
remaining partial-sign arguments and key/cache/session state. Therefore every
valid call that reaches the partial-sign operation is one-use even when that
operation returns failure. The Core wrapper's explicit invalidation also frees
its secure allocation and makes `IsValid()` false.

### Reproducer and verification

A disposable Bitcoin Core worktree was created at
`/tmp/bitcoin-goal84-113`, with build directory
`/tmp/bitcoin-goal84-113-build`; the protected checkout was not modified. A
temporary `bip328_tests` case created two valid keypairs and MuSig2 nonces,
swapped the two serialized public nonces while preserving a valid aggregate,
then called `CreateMuSig2PartialSig` for key 1. The expected behavior was a
missing partial signature plus an invalidated `MuSig2SecNonce`.

The disposable Release build was configured with Ninja, wallet enabled, tests
enabled, GUI/bench/fuzz binaries disabled, and ZMQ disabled. After correcting
the temporary harness to use this checkout's `uint256::FromHex` API, the
following commands passed:

    /tmp/bitcoin-goal84-113-build/bin/test_bitcoin --run_test=bip328_tests/partial_sig_failure_invalidates_secret_nonce --log_level=all
    /tmp/bitcoin-goal84-113-build/bin/test_bitcoin --run_test=bip328_tests --log_level=message

The focused run reported both `!partial_sig.has_value()` and
`!secnonce1.IsValid()` as passed. The full BIP328 suite reported `*** No
errors detected` and `Running 4 test cases...`.

As an independent oracle-sensitivity control, a disposable source mutation
moved `secnonce.Invalidate()` below the verification call. The same focused
test then exited 201 with:

    error: ... check !secnonce1.IsValid() has failed
    *** 1 failure is detected in the test module

The mutation and temporary test were restored and removed. `git diff --check`
and `git status --short` are clean in the disposable Core worktree. The
protected Core checkout still has exactly its pre-existing paths:
`src/test/blockencodings_tests.cpp`, `fuzz-0.log`, and `fuzz-1.log`.

### Verdict and handoff

**Dismissed.** The suspected stale wrapper state is prevented by the existing
ordering, libsecp's unconditional nonce zeroization, and the focused failure
fixture. No production change, regression test, or finding commit is
justified. The mutation demonstrates that the test would detect a real
ordering regression instead of merely accepting the failure result.

Exclude this exact Core post-sign-verification nonce-lifecycle cell from future
Goal84 work. Keep separate, contract-driven investigation of moved-from
`MuSig2SecNonce` behavior or reachable pre-sign failure paths only if their
public ownership contract can be established; do not treat invalid opaque
state or an undocumented moved-from call as a confirmed defect.
