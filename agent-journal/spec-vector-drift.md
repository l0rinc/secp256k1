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

## Cycle 99: BIP342 tapscript code-separator serialization

### Cycle metadata

- Selected by seed 234100655729448865, pool 77 81 82 84 87 89 95 97,
  index 1.
- Cycle timestamp: 2026-07-28T12:18:00Z through 2026-07-28T12:35:56Z.
- Audit branch: codex/fuzz-oracles, audit HEAD before this journal commit:
  8f05a64eef47edb0c6385aa0395c986def0a0fff.
- Bitcoin Core evidence base: protected branch codex/btc-fuzz-oracles,
  HEAD 00c4bb06ae9bf903af6ff72dbd6b097f36830ce6.
- The protected Core checkout retained its pre-existing dirty state only:
  M src/test/blockencodings_tests.cpp, ?? fuzz-0.log, and ?? fuzz-1.log.

### Hypothesis and trust boundary

Hypothesis: BIP342 tapscript sighashes could drift at the code-separator
boundary, for example by serializing a byte offset rather than an opcode
position, by failing to count parsed opcodes in an unexecuted branch, by
encoding the 0xffffffff sentinel incorrectly, or by losing the field in a
sighash-cache path. The trust boundary is an untrusted Taproot witness and
script executed by Bitcoin Core consensus validation. A mismatch would alter
the Schnorr message accepted by a node and could cause consensus divergence.

This is a distinct cell from the previous annex/hash-type cycle. The exact
annex/hash-type cases remain excluded from immediate rediscovery.

### Contract and implementation evidence

The primary specification is BIP342, Common Signature Message Extension:

https://github.com/bitcoin/bips/blob/master/bip-0342.mediawiki

BIP342 requires ext_flag=1 for tapscript, key_version=0, and a four-byte
little-endian `codesep_pos`. The position is the last executed
OP_CODESEPARATOR, uses 0xffffffff when none executed, starts at zero, counts
each parsed opcode once including pushes and opcodes in unexecuted branches,
and does not count push data bytes as opcodes.

The current Core path is:

- src/script/interpreter.cpp:443-445 initializes the sentinel and execution
  data before parsing.
- src/script/interpreter.cpp:449-456 increments opcode_pos once per parsed
  instruction.
- src/script/interpreter.cpp:1058-1066 updates m_codeseparator_pos only when
  OP_CODESEPARATOR is executed.
- src/script/interpreter.cpp:1492-1580 appends spend_type, the BIP342 leaf
  hash, key version, and m_codeseparator_pos to the TapSighash preimage.
- test/functional/test_framework/script.py:817-858 independently implements
  the BIP341 common message and BIP342 leaf/code-separator extension.
- test/functional/feature_taproot.py:765-784 exercises code separators before
  signatures, after signatures, and on both conditional branches;
  1273-1303 exercises 700 signature operations with 100 separators through
  the tapscript sighash cache.

### Full functional evidence

The deterministic functional command was:

    timeout 300 python3 test/functional/feature_taproot.py --configfile=/mnt/my_storage/bitcoin/build/test/config.ini --randomseed=810342 --portseed=810342 --tmpdir=/mnt/my_storage/goal81-taproot-run-810342 --cachedir=/mnt/my_storage/goal81-taproot-cache-810342 --loglevel=INFO

It completed successfully. Core validated 2,800 generated spending cases,
then two additional four-case post-activation sets, including the
code-separator and 700-operation sighash-cache scenarios. The run ended with
`Tests successful` and no functional failure.

The source hashes for the checked evidence were:

    src/script/interpreter.cpp 57a6f328c7e6202f499e48366a86eb26e09c8ebb78e92eef7e839207230be333
    test/functional/feature_taproot.py 07837cd34e9f26316ceec6f268870e9f7b5bf76dd3cd3e8dd85da7d973a7adeb
    test/functional/test_framework/script.py 82e250fadd393b972ae9d5d508f458fc872ea0dd947ca162693a006c3e488bbf
    build/bin/test_bitcoin 05c3b35c89beb331b45974ba26299355a468572b35ac83a7b14e00f9dff389f5

### Direct Core/Python differential matrix

A disposable C++ harness linked the protected Core consensus library and
constructed a fixed two-input, two-output transaction plus a tapscript with
two OP_CODESEPARATOR instructions and a 75-byte push. It captured the actual
execution positions and called SignatureHashSchnorr for both inputs, all
valid Taproot hash types 00, 01, 02, 03, 81, 82, and 83, and positions
0xffffffff, 0, 1, 3, 4, and 0xfffffffe. The independent Python
`TaprootSignatureHash` model parsed the emitted transaction and UTXOs and
compared every resulting hash.

The Core execution-position output was:

    CODESEP direct 0
    CODESEP after_push 2
    CODESEP unexecuted_branch 4

The differential result was:

    TAPSCRIPT_ORACLE cases=84 mismatches=0 codesep_paths={'direct': 0, 'after_push': 2, 'unexecuted_branch': 4}

The harness source SHA256 was
170b42d1f5710536a2a5f6c9c709f25a579eff91260bad5671b30f7914d40473; the
release harness binary SHA256 was
c1577cd417915ce4aa0f0c92915c5a8656132a613b864c3d064dfa130308bb33; and the
captured output SHA256 was
511f7d048e65ec91f2d0bf5b4ac328629a6d1d270ffc01bbd901171d44ce8e27.

### Sanitizer and mutation controls

The same 84-case harness linked the existing address/undefined-sanitized
fuzz-build archives. With
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`
it produced:

    ASAN_UBSAN_ORACLE cases=84 mismatches=0 diagnostics=0

The sanitized binary SHA256 was
af1958f68a5af97a356fb9a443e62a01d2b7aebbe277f38d88c6759ef7d5e7c3; its
output matched the release output byte-for-byte.

A disposable Core worktree changed only the final serialized
`m_codeseparator_pos` to the sentinel. The independent model then reported:

    MUTATION_ORACLE cases=84 mismatches=70 first=(0, 0, 0, 'e603d7edcf28f97f8fcd6efe30e74ddbe2d965ad5462944038e0e25d6ceab45f', '4a26294450496a5b850bb1f3f2e19f01950f6e42f539339bccff61d6925c3912')

The mutated binary SHA256 was
8e21f6d332bca0766eea9dad7ecc7b365182e5c9b766cc6122f0d3a3d8b1e8fa and
the mutated output SHA256 was
f47c9f007c7be847199888f9fd03cbaef6730a33d82dd7d5a0a97c9ffc717ab7.
This proves the oracle detects a realistic wrong-extension mutation while
the unmodified implementation passes.

### Verdict

Dismissed for this cell. The authoritative BIP342 contract, the full Core
functional suite, the independent 84-case Python differential, the direct
interpreter position capture, and the ASan/UBSan replay all agree. No
production bug, consensus divergence, or fix commit is claimed.

### Limitations and handoff

- The direct matrix calls SignatureHashSchnorr with prepared execution data;
  the full functional run supplies the independent witness/script execution
  and signature-validation coverage.
- The mutation changes only the final extension field, so it proves hash
  oracle sensitivity but does not exercise a mutated parser or cache
  implementation.
- The matrix is Linux x86_64 and uses the current embedded LevelDB/Core build;
  it does not provide Windows, 32-bit, or alternate implementation evidence.
- The protected Core checkout was not modified. All disposable worktrees,
  binaries, and output files are outside the audit branch.

### Next queue

Keep Goal81 pending and exclude this BIP342 code-separator cell from immediate
rediscovery. Candidate fresh cells are vector-generation provenance and
future specification changes affecting sighash caching. The controller should
draw again from the current eligible pool after committing this handoff.
