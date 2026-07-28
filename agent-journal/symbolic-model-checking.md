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

## Cycle 74

Status: dismissed for the selected VarInt encoder/decoder hypothesis.

Controller draw:

- Draw seed: `7234767300004701769`.
- Eligible pool: `74 77 81 82 84 87 89 95 97`.
- Selected index: `1`.
- Selected goal: `77`, `symbolic-model-checking`.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, HEAD `f5f595726cbc53b8c21b45f7abda1665e8567c8c`.
- Protected Bitcoin Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. Its only dirty entries were
  the pre-existing `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- Protected secp256k1 remained clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- `cbmc`, `klee`, and `esbmc` remain unavailable; this is an explicit finite
  bounded proof, not a symbolic-tool or unbounded-proof claim.

Hypothesis and trust boundary:

`ReadVarInt` or `WriteVarInt` might mishandle a base-128 continuation,
overflow boundary, size calculation, truncation, or signed nonnegative mode.
The trust boundary is serialized bytes from disk, network, undo, chainstate,
index, and compressed-object records. Production caller shapes include
`src/undo.h:27-48`, `src/coins.h:79-89`, `src/txdb.cpp:48-54`,
`src/node/blockstorage.h:70-78`, `src/chain.h:344-351`,
`src/compressor.h:72-108`, and `src/flatfile.h:19-20`.

Source, history, and existing-test evidence:

- `src/serialize.h:365-387` defines the one-to-one MSB base-128 format and
  subtract-one rule. `:401-465` implements mode checks, size computation,
  writing, overflow checks, and decoding.
- Existing `src/test/serialize_tests.cpp:106-157` checks 100,000 small signed
  values, sparse large unsigned values, and representative bit patterns, but
  it does not compare an independent arbitrary-precision model against raw
  malformed/truncated byte sequences across all integer widths.
- History seeds include `45f09618f2` (the prior ReadVarInt integer-overflow
  fix), `d4fce93fa2` (inline reads), `2e907f0393` (size shortcut),
  `2d4518f1ab` (VarInt specializations), and `34a3f7b374` (SizeComputer
  specialization merge). No current journal contained this exact cross-mode
  bounded oracle.

Independent bounded proof:

A disposable C++20 harness at `/tmp/goal77-varint-harness.cpp` used a
`boost::multiprecision::cpp_int` model independent of Core's integer type and
stream implementation. Its source SHA-256 was
`6a1d9f163404c5b0505d064ae3bdab912d220a3008430554c34e7c5647ec4991`.
For each of `uint8/16/32/64_t` and nonnegative `int8/16/32/64_t`, it checked
the independent encoding, Core encoding, `GetSizeOfVarInt`, wrapper size,
decoded value, and complete consumption. It also compared success/failure,
decoded value, and consumed prefix for every one-byte and two-byte sequence,
fixed continuation/termination patterns through length 12, and 50,000
deterministic random raw inputs per type. Each mode covered 116,152 raw cases.

Value counts were `256, 20013, 20030, 20031` for unsigned 8/16/32/64-bit
types and `128, 20008, 20026, 20031` for signed nonnegative 8/16/32/64-bit
types. The Clang O2 ASan/UBSan/pointer-overflow run printed:

`PASS varint-bounded digest=a6e66cc80af7db40`

It exited 0 in 9.27 seconds with 299,132 KiB maximum RSS. The same harness
under GCC 16.1.0 O2 with the same sanitizers printed the same digest and
exited 0 in 7.64 seconds with 304,008 KiB maximum RSS. Clang O0 without
sanitizers printed the same digest and exited 0 in 2.19 seconds with 4,372
KiB maximum RSS. The Clang O2, GCC O2, and Clang O0 harness hashes were,
respectively, `5fa19025c42c6fab2867a931a6f55efa3ae754c71cfc0d62c4462735141e601a`,
`630fd052c4be5d31a26afbbd683ddc7861c08d12e714722e3ac9273897d1dcbb`, and
`2d015f5f33ed54508366936074d1ca4f3a9368a20096ab3102324727b5e5509d`.

The first scratch run reported `s8 value=-128`; investigation showed the
harness had converted out-of-domain candidate values above the signed
maximum instead of filtering them. The harness was corrected, and the clean
matrix above was rerun from the corrected source. No production failure was
hidden by that correction.

Mutation and repository controls:

- A disposable Core worktree from protected HEAD changed `n++` at
  `src/serialize.h:460` to `n += 2`. The sanitized mutant failed immediately
  at `u8 value=128` with `ReadVarInt(): size too large: iostream error`.
  The mutant binary SHA-256 was
  `2985aed236781820a3e657a9e84ac0b9d3a65e7fb2a16a299e1eec60fdb8c06a`.
- The mutation was restored, `git diff --check` and a zero source diff passed,
  and the disposable worktree was removed. The clean Core header SHA-256 was
  `a6f778a2e3d0638d88651798f498488d746caf939d08cc148ca23b7d39ba987b`.
- `./build/bin/test_bitcoin --run_test=serialize_tests --log_level=test_suite`
  ran all 15 serialization test cases and ended with `*** No errors detected`.

GCC emitted one `-Wstringop-overread` warning while instantiating the generic
`DataStream` byte-write path from `ser_writedata8`; the sanitized execution
was clean and the warning was not in VarInt control flow. It is recorded as
an unrelated static-analysis lead for goals 12/97, not as a VarInt finding.

Verdict:

The hypothesis is dismissed on current Core. The independent model matched
the encoder, decoder, size calculation, signed nonnegative mode, overflow and
truncation outcomes, and consumed-prefix behavior across the bounded matrix.
The mutation control failed immediately and the repository serialization
suite passed. No production source change, regression test, or severity
finding is justified.

Limitations and handoff:

This is a bounded proof, not CBMC/KLEE coverage. It does not exhaust every
three-through-ten-byte sequence, every full-width 32/64-bit value, negative
`NONNEGATIVE_SIGNED` inputs (explicitly outside its documented domain), or
non-Linux/32-bit execution. Reopen with a distinct caller-level allocation or
state-transition contract, compiler/architecture evidence, or a new formal
kernel; do not repeat this VarInt cell.

Next queue: retain goal `77` for other bounded kernels and the distinct cells
in `74,81,82,84,87,89,95,97`, excluding this VarInt cell and the earlier
CompactSize and SHA-256 cells.

## Cycle 114: script instruction parser bounds and failure state

### Selection and scope

The controller selected Goal `77`, `symbolic-model-checking`, with seed
`10179737623739671668`, index `0`, from the eligible pool `77 82 87 95`, at
`2026-07-28T17:42:46Z`. Goal77's previous CompactSize, SHA-256 streaming, and
VarInt cells were excluded.

This cycle selected the consensus-facing `GetScriptOp`/`GetOp` parser. The
hypothesis was that a direct push or `OP_PUSHDATA1/2/4` instruction could
misdecode a little-endian size, advance the cursor past a truncated prefix or
payload, or leave a caller-visible opcode/data output dirty on failure. The
trust boundary is serialized script bytes from transactions, P2SH redeem
scripts, witness-related paths, indexes, and fuzz inputs. The tested kernel is
the bounded parser at `src/script/script.cpp:313-363`, exposed through
`src/script/script.h:497-505` and used by `HasValidOps`, sig-op counting, and
P2SH script extraction.

### Source and model

The production function resets `opcodeRet` to `OP_INVALIDOPCODE` and clears the
optional data output before checking for an instruction. It consumes the
opcode, reads a one-, two-, or four-byte little-endian length only after the
whole prefix is available, advances past a valid prefix before testing payload
availability, and advances over the payload only on success. Non-push opcodes
consume exactly one byte.

A temporary `bip328`-independent test in disposable Bitcoin Core worktree
`/tmp/bitcoin-goal84-113` implemented a separate byte-level model with explicit
cursor, prefix, size, success, opcode, and data state. It checked every empty,
one-byte, and two-byte input (`65,793` raw inputs), every direct-push size from
0 through 75 with exact and one-byte-short payloads, `PUSHDATA1` sizes
0/1/75/76/255, `PUSHDATA2` sizes 0/1/255/256/65535, and `PUSHDATA4` sizes
0/1/256/0xffffffff with exact or truncated payloads where materializable.
The total was `65,968` cases. Each invocation initialized the output vector to
`aa` and the opcode to a non-invalid value, so output cleanup was part of the
oracle. It also checked the exact cursor offset on prefix and payload failure.

The first run found a harness-model error for `4c01`: the model returned the
one-byte cursor when the length prefix was present but the payload was short,
while the production parser correctly leaves the cursor after the prefix. The
model was corrected to record that prefix position; the corrected matrix was
rerun from scratch. No production failure was hidden by this correction.

### Verification

The test used the disposable Release build configured with GCC 16.1.0, tests
and wallet enabled, and GUI/bench/fuzz binaries and ZMQ disabled. The focused
command was:

    /tmp/bitcoin-goal84-113-build/bin/test_bitcoin --run_test=script_tests/script_getop_bounded_model --log_level=message

It passed with:

    PASS getop-bounded cases=65968 digest=30df9d2086ec46a1
    *** No errors detected

An independent Valgrind Memcheck run of the same focused binary and matrix
exited 0 with no diagnostics. The complete permanent `script_tests` suite
passed all `21` cases with `*** No errors detected` both before the mutation
control and after the production source was restored and rebuilt.

As an oracle-sensitivity control, a disposable source mutation changed the
production condition from `opcode <= OP_PUSHDATA4` to `opcode < OP_PUSHDATA4`.
The focused matrix rejected the mutant at the one-byte `4e` input with:

    error: ... GetOp status mismatch for 4e
    *** 1 failure is detected in the test module

The mutation and temporary test were removed with `apply_patch`. The restored
`src/script/script.cpp` SHA-256 is
`210b1720acce295de54c6a08c76b97fa4cfe835f64e0ff7e0730c57ebebb8c55`, matching
the disposable worktree's `HEAD` blob. `git diff --check` and status are clean
in that worktree. The protected Bitcoin Core checkout was not modified and
still has exactly its three pre-existing dirty paths.

### Verdict and limitations

**Dismissed.** The independent bounded model, output/cursor checks, Memcheck
run, permanent script suite, and boundary mutation found no parser defect. No
production source change, regression-test commit, or severity finding is
justified.

`cbmc`, `klee`, and `esbmc` are unavailable in this environment. This is an
explicit finite proof over the stated input set, not an unbounded symbolic
proof. It does not cover arbitrary long scripts, every full-width payload
allocation, concurrent callers, non-x86 execution, or downstream parser
contracts beyond the exercised `GetOp` state. Reopen only for a distinct
caller-level failure contract, platform/compiler divergence, or a newly
available symbolic tool; do not repeat this exact short-input parser cell.

### Handoff

Exclude this exact `GetScriptOp` bounded parser matrix from future Goal77 work.
Keep Goal77 eligible for other high-risk pure or state-machine kernels and
continue with an explicit finite model or a real symbolic-tool result.
