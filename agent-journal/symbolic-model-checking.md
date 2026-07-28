# Goal 77: Symbolic execution and bounded-model-checking campaign

## Cycle 69

Status: dismissed for the selected CompactSize parser hypothesis.

Controller draw:

- Draw seed: `3820308283`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`.
- Selected index: `1`.
- Selected goal: `77`, `symbolic-model-checking`.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`, branch `codex/fuzz-oracles`.
- Audit base at cycle start: `514520e63421745aed9b7e8d8e39f54a90dafeda`.
- Protected Bitcoin Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`.
- Protected Core remained unchanged with only its pre-existing
  `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and `?? fuzz-1.log`.
- The supplied catalog attachment was read in full: 3,577 bytes, SHA-256
  `1639d16123a404f70037ff15f15464f26fbd7ee0fe363f441064c6dd15f72102`.
- `cbmc`, `klee`, and `esbmc` are not installed. No symbolic-tool result is
  claimed; this cycle uses an explicit finite-state bounded proof with an
  independent oracle and mutation control.

Hypothesis and trust boundary:

The `ReadCompactSize` discriminator or canonical/range boundary might accept
the wrong value, consume the wrong number of bytes, or mishandle truncation at
an untrusted serialized-input boundary. The contract is: prefixes below 253
encode themselves in one byte; prefix 253 carries a little-endian uint16 whose
value must be at least 253; prefix 254 carries a little-endian uint32 whose
value must be at least 65536; prefix 255 carries a little-endian uint64 whose
value must be at least 2^32; and the default range check rejects values above
`MAX_SIZE == 0x02000000`. Every truncated payload must throw an end-of-data
failure without returning a value.

Actual Core callers include vector/string/map deserializers in
`src/serialize.h:637,683,806,841,880,971`, compact-size use in
`src/blockfilter.cpp:54,109`, UTXO snapshot parsing in
`src/validation.cpp:6070,6079`, and multiple fuzz targets. The input is
network, block, RPC/config, or persisted serialization depending on caller;
the parser itself has no trusted caller-side byte guarantee.

Source and history evidence:

- `src/serialize.h:291-363` contains the paired size/encoder/decoder contract.
  `GetSizeOfCompactSize` and `WriteCompactSize` use the same three thresholds
  as `ReadCompactSize`; the decoder adds canonical and default range checks.
- `src/test/serialize_tests.cpp:159-176` covers power-of-two round trips and
  `:208-243` covers representative noncanonical encodings, but neither is an
  exhaustive payload matrix, a range-check false/true matrix, nor every
  truncation prefix.
- Blame shows the threshold logic is historical and the noncanonical rejection
  was introduced by `8dc206a1e2` on 2013-08-07. No current issue, PR, or prior
  audit journal contains this exact bounded parser-oracle cell.

Independent bounded proof:

The scratch harness `/tmp/goal77-compactsize-harness.cpp` implements the
expected parser independently using byte spans and explicit thresholds. Its
SHA-256 is
`4d6771e3459c0a2c831867bd3b2d9861f4a730e39c6c646fa7d60e2f8514faff`.

It checks both `range_check` values and verifies returned values, exception
class, complete consumption, canonical encodings, and
`GetSizeOfCompactSize`/`WriteCompactSize` agreement. The finite domain contains:

- every one-byte prefix from 0 through 252;
- all 65,536 payloads under the 253 discriminator;
- all 65,536 low payloads under both 254 and 255 discriminators;
- every threshold, `MAX_SIZE`, just-over-limit, 32/64-bit edge, and maximum
  value; and
- every truncation length for selected canonical and noncanonical encodings.

Total result: `PASS compactsize bounded-cases=393955 digest=bb07dc5cf1d0389f`.

Verification matrix:

- Clang 22.1.7 O2 against the protected source: exit 0, digest above;
  binary SHA-256 `72028fbd2ab6358bc68dc16ede480f7d188ff6a7531ccd383de890eddd2a8539`.
- Clang O0: same pass and digest.
- GCC 16.1.0 O2: same pass and digest; binary SHA-256
  `e41bef28be460939d6fe72161ea4e2ada9f20bb276c16b96ec1b36755b34e9fa`.
- Clang O2 ASan/UBSan with leak detection and halt-on-error: same pass and
  digest; binary SHA-256
  `1125af7b3de632b16d001a150a1e7013d15cc6b22883c9c6c85e9c5d51048dfb`.
- Repository control: `./build/bin/test_bitcoin --run_test=serialize_tests
  --log_level=test_suite` ran all 15 selected cases and ended with
  `*** No errors detected`.

Mutation and clean-control proof:

A detached scratch Core worktree was created from the protected HEAD. The
temporary production mutation changed `else if (chSize == 253)` to
`else if (chSize == 254)` in `src/serialize.h`. The mutated harness binary
SHA-256 was
`2b7575c5e51ea8c14b341c5fc1ed1885967c4d9aac6457bd4833840874bb90a9`.
It aborted on the first 253-prefix case with:
`mismatch prefix=253 expected=1:0 actual=3:0`.

The mutation was restored in the scratch worktree, rebuilt, and rerun. The
restored control again printed the exact clean digest and exited 0. The
scratch worktree was removed. No mutation or scratch source remains in either
protected checkout.

Verdict:

The hypothesis is dismissed on current Core. The decoder's threshold,
canonicality, range, truncation, and consumption behavior matched the
independent bounded oracle across 393,955 cases and multiple compiler/runtime
configurations. Existing tests were narrower than this proof but did cover the
public contract; no source change or regression-test commit is justified.

This is not an unbounded proof and does not cover every 32/64-bit payload,
alternate C++ stream implementation, non-Linux platform, or every caller's
post-parse allocation policy. It also does not claim CBMC/KLEE coverage. Reopen
with a distinct formal kernel or a caller-level malformed-input/resource
hypothesis, not this same CompactSize discriminator cell.

Next queue: retain distinct cells in `74,77,81,82,84,87,89,95,97`, excluding
this CompactSize bounded-parser cell and the prior goal-74 CCoinsMap retained
capacity cell.

## Cycle 71: Core SHA-256 streaming and padding boundaries

Status: dismissed for the selected bounded streaming-state hypothesis.

### Selection and repository state

- Selected goal: `77`, `symbolic-model-checking`.
- Draw seed: `2791938646`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`; length `9`; zero-based index
  `1`; selected goal `77`.
- The prior goal-77 CompactSize discriminator cell is excluded. This cycle
  uses a new Core cryptographic streaming cell.
- Audit branch/base: `codex/fuzz-oracles`, HEAD
  `25e07204e0c8a8fde75f0a18e604b0994bf41d94`, parent
  `0cb41f2d6108de5999494789b37e508697555f75`, remote `origin/master` at
  `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`.
- Protected Core branch: `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`; its pre-existing dirty state
  remained `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`. Protected secp remained detached and clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- No real fuzz, sanitizer, daemon, benchmark, or profiling job was running
  before or after the cycle. All scratch binaries and source were removed.

### Hypothesis and trust boundary

The hypothesis was that `CSHA256::Write` or `CSHA256::Finalize` mishandles an
exact 64-byte buffer transition or a padding boundary, causing a streamed
hash to differ from the one-shot SHA-256 contract. The relevant control is
`src/crypto/sha256.cpp:699-731`: `bufsize + len >= 64` flushes a partial block,
`end - data` processes full blocks, and `Finalize` chooses 56 or 120 bytes of
padding from `bytes % 64`. The probe also exercised `CSHA256::Write(nullptr, 0)`
and a default empty `std::span`, because `src/hash.h:45-47` forwards empty
spans to this function.

This is production-reachable cryptographic state, not an arbitrary internal
object: `CHash256` and `HashWriter` in `src/hash.h:31-47,108-137` feed hashes
used by transaction and witness commitments (`src/validation.cpp:4086,4197`),
script hashing (`src/script/interpreter.cpp:1048-1052,1942`), descriptor and
address identifiers, tagged hashes, HMAC, and the public hash helpers. The
contract is byte-exact SHA-256, correct incremental composition, and safe
in-place double-hash output where `HashWriter::GetHash` deliberately reuses
the result buffer.

### Source, history, and existing tests

The current implementation is still the long-lived code introduced by
`36fa4a78aca` (`Split up crypto/sha2`, 2014); blame shows the relevant
streaming and padding logic has no later semantic rewrite. Existing
`crypto_tests` has NIST vectors and randomized chunk splits, and `sha256d64`
cross-checks optimized batch hashing, but it does not exhaustively enumerate
every two-part split at every short-message length, explicit null/empty input,
or the in-place `CHash256` alias as one bounded control table. Searches of the
prior audit journals found no identical SHA-256 streaming-state campaign.

### Independent bounded proof

A disposable C++20 harness used OpenSSL 3.5.3's one-shot `SHA256` as an
independent oracle. For each deterministic byte pattern of every length
`0..1024`, it checked the Core one-shot result and every two-part split,
including an explicit zero-length `Write(nullptr, 0)` before the split. It
also checked byte-at-a-time streaming for every length `0..256`, empty/null
input, and an in-place 32-byte `CHash256` double hash. The bounded run covered
`1025` lengths, `525825` two-part split cases, `257` bytewise lengths, and one
alias case.

The sanitized standard implementation was built and ran with:

```text
clang++ -std=c++20 -O2 -g -DDISABLE_OPTIMIZED_SHA256 \
  -fsanitize=undefined,address,pointer-overflow \
  -fno-sanitize-recover=undefined,pointer-overflow \
  -I/mnt/my_storage/bitcoin/src goal77_sha256_empty.cpp \
  /mnt/my_storage/bitcoin/src/crypto/sha256.cpp -lcrypto \
  -o /tmp/goal77_sha256_stream_san
/tmp/goal77_sha256_stream_san
PASS implementation=standard lengths=1025 split_cases=525825 \
  bytewise_lengths=257 alias_case=1
```

The same sanitized standard harness under GCC 16.1.0 printed the same `PASS`.
The Core archive build exercised the auto-selected optimized implementation
and printed:

```text
PASS implementation=sse4(1way);sse41(4way);avx2(8way) lengths=1025 split_cases=525825 bytewise_lengths=257 alias_case=1
```

The sanitized and optimized binary hashes were respectively
`77439bee1e13af42e2557e1ee422b02c994b8d1ac0be0b289d8a98721e032be0` and
`63384cebe726005c5e322fb35199a6bca4054e42d2c5a3789ab634920242af35`.
The source hash at the protected Core HEAD was
`31130079f338477ab4e0ddb923a7846bc66b6bdcc814ac6225f462f6aef85755`.

The repository command
`/mnt/my_storage/bitcoin/build/bin/test_bitcoin --run_test=crypto_tests
--log_level=test_suite` ran all `17` selected test cases and ended with
`*** No errors detected`. The release test binary hash was
`05c3b35c89beb331b45974ba26299355a468572b35ac83a7b14e00f9dff389f5`.

### Mutation control

A copy of `sha256.cpp` was changed only in the scratch build from
`bufsize + len >= 64` to `bufsize + len > 64`. The bounded harness rejected
the mutant immediately at the empty message boundary:

```text
actual=6a09e667 expected=e3b0c442
FAIL one-shot length=0
```

It exited `1`; the clean source passed again. This demonstrates that the
oracle detects the exact transition/padding defect the campaign targets. The
mutation was never placed in the protected Core checkout.

### Verdict

The hypothesis is **dismissed** for the tested production-domain state. Exact
boundary, empty-input, split, alias, standard-backend, optimized-backend,
GCC/Clang-sanitized, OpenSSL-differential, mutation, and repository-test
evidence found no SHA-256 streaming or padding defect. No Core source repair,
regression-test commit, or severity finding is justified. Master-relative
severity is none.

### Limitations and handoff

This is a bounded proof, not an unbounded CBMC/KLEE result; those tools were
not installed. It covers one deterministic pattern per length through 1024,
not every message byte string, `size_t`/`uint64_t` maximum-length accounting,
32-bit execution, alternate non-x86 optimized backends, or arbitrary external
callers. The sanitizer run did not diagnose the zero-length null pointer
operation, but that runtime result is not a language-lawyer proof for every
possible pointer contract. Reopen with a newly reachable caller contract,
large-total-length accounting, a new backend, or a compiler/architecture
divergence; do not repeat this same short streaming cell.

Next queue: retain `74,77,81,82,84,87,89,95,97`, excluding this goal-77
SHA-256 streaming/padding cell and the prior CompactSize cell. No production
commit was made; this cycle is a focused journal-only evidence snapshot.
