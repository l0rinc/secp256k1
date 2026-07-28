# Goal 81: specification, test-vector, and formal-model drift audit

## Cycle metadata

- Selected by seed 13671422447867256256, pool 77 81 82 95 97, index 1.
- Cycle timestamp: 2026-07-28T11:16:51Z through 2026-07-28T11:31:30Z.
- Audit branch: codex/fuzz-oracles, audit HEAD before this journal commit:
  0b0ea7849a1e7ac5b40076ac4cbb204e624adb4c.
- Bitcoin Core evidence base: protected branch codex/btc-fuzz-oracles,
  HEAD 00c4bb06ae9bf903af6ff72dbd6b097f36830ce6.
- The protected Core checkout retained its pre-existing dirty state only:
  M src/test/blockencodings_tests.cpp, ?? fuzz-0.log, and ?? fuzz-1.log.

## Hypothesis and trust boundary

Hypothesis: the Taproot signature-message implementation could have drifted
from BIP341 in the under-tested annex/spend-type and hash-type combinations,
especially at the CompactSize boundary and at SIGHASH_SINGLE rejection.
The trust boundary is an untrusted witness and signature supplied to Bitcoin
Core consensus script validation. A mismatch could change the message checked
by Schnorr verification; this cycle did not assume that a mismatch would be a
consensus defect without an invalid-block or divergent-validation reproducer.

This is a distinct cell from the prior precomputed-sighash equivalence work
(062b97b284), the existing BIP341 wallet-vector work, and the prior parser,
nonce, and bounded-model campaigns. Those cover no-annex/precomputation and
other cryptographic contracts; this cycle targeted annex serialization and
hash-type rejection.

## Contract evidence

The primary specification is BIP341, sections Common signature message and
Taproot key path spending signature validation:

https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki

The relevant rules are: spend_type = ext_flag * 2 + annex_present; an annex
is hashed as SHA256(CompactSize(size(annex)) || annex) including its 0x50
prefix; SIGHASH_SINGLE requires a corresponding output; and only hash types
00, 01, 02, 03, 81, 82, and 83 are defined.

Current Core implements these rules in:

- src/script/interpreter.cpp:1492-1580: epoch/hash type, transaction
  component hashes, spend_type, annex hash, SIGHASH_SINGLE output hash, and
  the final TapSighash hash.
- src/script/interpreter.cpp:1961-1969: witness parsing removes an annex
  only when the final witness element begins with 0x50, computes its hash via
  HashWriter << annex, and initializes the execution data.
- src/test/script_tests.cpp:1632-1700: the committed BIP341 wallet vectors,
  component hashes, seven input/hash-type cases, and the independent expected
  sigMsg hash check.

The local vector file is the BIP341 wallet vector data and has SHA256
403e19fb81dd1f31e745699216308f61fb403774b2aafa87b631b8f7c042d37f.
The current production interpreter and vector-test sources have SHA256
57a6f328c7e6202f499e48366a86eb26e09c8ebb78e92eef7e839207230be333 and
6ff875e60d9966ecff0da811f419c535a0c3785f3db40c30e32f68af61906b4c,
respectively.

## Independent model

An inline Python model parsed the unsigned transaction structurally, encoded
CompactSize and CTxOut fields, computed all five BIP341 component hashes,
constructed the full epoch-prefixed signature message, and computed the
TapSighash tagged SHA256. It compared against the vector's independent sigMsg
and sigHash fields, not against Core output.

Key output:

VECTOR_PASS input=0 hash_type=3 sigmsg_bytes=175
VECTOR_PASS input=1 hash_type=131 sigmsg_bytes=126
VECTOR_PASS input=3 hash_type=1 sigmsg_bytes=175
VECTOR_PASS input=4 hash_type=0 sigmsg_bytes=175
VECTOR_PASS input=6 hash_type=2 sigmsg_bytes=143
VECTOR_PASS input=7 hash_type=130 sigmsg_bytes=94
VECTOR_PASS input=8 hash_type=129 sigmsg_bytes=126
ANNEX_MODEL input=0 hash_type=0 annex_bytes=1 sigmsg_bytes=207 sighash=07dd75978b3d505a3e30d744497cad93070316cd07c0f6f9d9b9f783f5dd744f
ANNEX_MODEL input=0 hash_type=1 annex_bytes=2 sigmsg_bytes=207 sighash=63e21013133c796d7739acb317ac8f26b6511079a5683316502b5e73541d6c50
ANNEX_MODEL input=0 hash_type=3 annex_bytes=254 sigmsg_bytes=207 sighash=364fac2da935e8b09d4d09499dd504c84249410bfb1bcb3d1bd456ec4527d015
ANNEX_MODEL input=0 hash_type=131 annex_bytes=254 sigmsg_bytes=158 sighash=cb7c0c67baedafb0f3a8c82ff955286225648c55da149cd2eeac8a9385e7660f
MODEL_RESULT PASS components=5 vectors=7 annex_cases=4

The first model attempt incorrectly hashed full spent CTxOut records for
sha_scriptpubkeys; BIP341 hashes CompactSize-prefixed scripts there. That
mistake was corrected before the pass above, and the correction is itself
recorded to prevent treating the first failed oracle as repository evidence.

## Core implementation probe

A disposable C++ harness linked the existing Core build's consensus object
and loaded the same BIP341 wallet vector transaction and UTXOs. It initialized
PrecomputedTransactionData, set ScriptExecutionData to an annex-present
state, computed the annex hash using Core's HashWriter << annex, and called
SignatureHashSchnorr directly. It covered:

- annex lengths 1 and 2 with SIGHASH_DEFAULT and SIGHASH_ALL;
- a 254-byte annex at the CompactSize 0xfd boundary with SIGHASH_SINGLE;
- a 254-byte annex at the same boundary with SIGHASH_SINGLE|ANYONECANPAY;
- input 8 with SIGHASH_SINGLE and no corresponding output;
- undefined hash types 0x04 and 0x84.

The unmodified probe output was:

ANNEX_CASE input=0 hash_type=0 annex_len=1 actual=07dd75978b3d505a3e30d744497cad93070316cd07c0f6f9d9b9f783f5dd744f
ANNEX_CASE input=0 hash_type=1 annex_len=2 actual=63e21013133c796d7739acb317ac8f26b6511079a5683316502b5e73541d6c50
ANNEX_CASE input=0 hash_type=3 annex_len=254 actual=364fac2da935e8b09d4d09499dd504c84249410bfb1bcb3d1bd456ec4527d015
ANNEX_CASE input=0 hash_type=131 annex_len=254 actual=cb7c0c67baedafb0f3a8c82ff955286225648c55da149cd2eeac8a9385e7660f
INVALID_CASE_REJECTED input=8 hash_type=3
INVALID_CASE_REJECTED input=0 hash_type=4
INVALID_CASE_REJECTED input=0 hash_type=132

The current Core script_tests suite also passed as a containing-suite run:

build/bin/test_bitcoin --run_test=script_tests --log_level=test_suite
Running 21 test cases...
*** No errors detected

The suite includes bip341_keypath_test_vectors, which passed all seven
no-annex vector cases.

## Oracle sensitivity control

A disposable Core worktree changed only the annex contribution in
SignatureHashSchnorr from execdata.m_annex_hash to uint256{}. The same probe
was rebuilt against that mutated interpreter.cpp; it failed the first annex
vector with exit code 3 and actual hash
37b5c091a8e0ad73326692c5a1bb52b8439a5c7e7e4a2dc077098af77597788b.
This proves the independent annex oracle detects a realistic wrong-data
mutation. The mutation worktree and all probe artifacts were removed.

## Verdict

Dismissed for this cell. The current implementation matches the BIP341
contract, seven official no-annex vector cases, four independently modeled
annex cases, and three rejection cases. The mutation control fails as
expected. No production bug, consensus divergence, or fix commit is claimed.

There is a test-coverage opportunity, not a confirmed defect: the committed
Core BIP341 vector test has no annex-present vector. Adding a durable annex
fixture would be a separate test-quality change and needs maintainer-style
review; this cycle does not modify the protected Core checkout.

## Limitations and handoff

- The annex probe calls SignatureHashSchnorr directly with prepared execution
  data; it does not run the full witness interpreter, so it does not
  independently test witness-stack annex detection or script-path execution.
- The probe used the existing release Core build. No new ASan/UBSan Core build
  was needed after the deterministic source/model agreement; no sanitizer
  diagnostic was observed in this cell.
- The Core checkout remains protected and dirty only with the user's existing
  block-encoding test/log files. The audit worktree remains the only commit
  target.

## Next queue

Keep Goal81 pending and exclude this annex/hash-type cell from immediate
rediscovery. Candidate fresh cells are BIP342 ext_flag=1/tapscript
codeseparator serialization, vector-generation provenance, and any future
specification changes affecting sighash caching. The controller's next draw is
Goal82 (secp-field-scalar-matrix), seed
14643566124539001421, pool 77 82 95 97, index 1.
