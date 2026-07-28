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
