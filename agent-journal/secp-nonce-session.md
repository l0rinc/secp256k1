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
