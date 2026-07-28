# Goal 89: Bitcoin P2P transport, permission, and peer-accounting audit

## Cycle 80

Status: confirmed for the selected outbound transaction-inventory queue
resource-accounting hypothesis. No protected-source fix was committed in
this cycle; the confirmed finding and exact reproduction are the handoff.

Controller draw:

- Draw seed: 4015881993.
- Eligible pool: 77 81 82 87 89 95 97.
- Selected index: 4.
- Selected goal: 89, bitcoin-p2p-accounting.
- Cycle timestamp: 2026-07-28T10:39:38Z.

Repository state:

- Audit worktree: /tmp/secp256k1-oracles-next, branch codex/fuzz-oracles,
  HEAD at cycle start dadbb70ca7c5c18698d2c56178c51b3d2d411a8f.
- Protected Bitcoin Core checkout: /mnt/my_storage/bitcoin, branch
  codex/btc-fuzz-oracles, HEAD 00c4bb06ae9bf903af6ff72dbd6b097f36830ce6.
  It retained exactly its pre-existing M src/test/blockencodings_tests.cpp,
  ?? fuzz-0.log, and ?? fuzz-1.log entries after the cycle.
- Protected secp256k1 checkout remained detached and clean at
  e153e2681f7bf1dd74894e2170213e3983030989.
- The live probe used the existing Core binary
  /mnt/my_storage/bitcoin/build/bin/bitcoind; no daemon or test process
  remained after verification.

Hypothesis and trust boundary:

The per-peer outbound transaction-announcement set can retain more entries
than the documented per-send limit because enqueueing has no hard bound. A
relaying peer whose announcement timer has not fired, or whose socket is slow,
can therefore accumulate distinct wtxids while accepted transactions keep
calling the broadcast path. The trust boundary is the local node's mempool,
peer-manager, and P2P send scheduler. An external peer can reach the ordinary
accepted-transaction path by submitting valid transactions; local RPC and the
ForceRelay gateway path also reach it. The finding is distinct from inbound
transaction-request accounting in TxDownloadMan.

Source and history evidence:

- src/net_processing.cpp:306-315 defines
  TxRelay::m_tx_inventory_to_send as an unbounded std::set<Wtxid>. The nearby
  MAX_PEER_TX_ANNOUNCEMENTS check is not applied to this set; that constant is
  used by src/node/txdownloadman_impl.cpp for inbound requests.
- src/net_processing.cpp:2374-2394 inserts a new wtxid for every connected
  relaying peer after the handshake. The only admission checks are peer
  existence, handshake completion, and the known-inventory filter; there is no
  queue-size or byte-size check.
- src/net_processing.cpp:3289-3304 calls this path after a transaction is
  accepted into the mempool. src/node/transaction.cpp:129-134 reaches the
  same path for local MEMPOOL_AND_BROADCAST_TO_ALL submission.
- src/net_processing.cpp:4614-4628 can invoke it again for an already
  mempool-resident transaction received from a ForceRelay peer. Identical
  entries are deduplicated, but distinct accepted or stale wtxids remain
  queued until the send loop handles them.
- src/net_processing.cpp:6213-6224 only schedules periodic relay. The
  SendMessages path at :6264-6290 snapshots the complete set and removes at
  most broadcast_max, where :6281-6282 caps the per-call drain at
  INVENTORY_BROADCAST_MAX == 1000. The cap limits draining, not admission.
  Entries removed from the mempool are still retained until the drain reaches
  them, then skipped at :6292-6295.
- src/net_processing.cpp:1942-1947 and src/rpc/net.cpp:248-250 expose the
  live queue length as getpeerinfo()["inv_to_send"], providing an independent
  observation point without instrumenting private state.
- Existing test/functional/p2p_tx_download.py checks a one-entry queue and
  eventual draining, but has no assertion that a frozen relay timer or slow
  peer cannot accumulate more than the per-send limit.
- Future origin/master commit 026f70e05f96f5795c888ed69ba9128169ec5b7f
  explicitly describes per-peer rate limiting as a storage/compute risk and
  removes the per-peer drain cap. That commit is corroborating history, not
  the oracle for this current-branch reproduction; the current branch was
  tested independently.

Independent reproduction:

Scratch harness: /mnt/my_storage/bitcoin-goal89-queue-probe.py.
SHA-256: eb4f1103d391f04cfc154b24d62f3c4f43e7a80c0ebaab9bbabcb47d80407d9c8.
It starts one regtest node with -persistmempool=0 -maxmempool=100, generates
1,200 independent confirmed UTXOs through MiniWallet, freezes mock time,
connects one ordinary relaying P2PInterface, and submits 1,100 accepted
transactions. It samples getpeerinfo()[0]["inv_to_send"] every 200
transactions.

Command:

python3 /mnt/my_storage/bitcoin-goal89-queue-probe.py

Configuration and artifacts:

- Core config: /mnt/my_storage/bitcoin/build/test/config.ini.
- Test directory and preserved framework log:
  /mnt/my_storage/bitcoin-goal89-queue-probe.
- Fixed random seed: 8902.

Key output from the unmodified current-branch binary:

    submitted=200 inv_to_send=200
    submitted=400 inv_to_send=400
    submitted=600 inv_to_send=600
    submitted=800 inv_to_send=800
    submitted=1000 inv_to_send=1000
    QUEUE_PROBE_RESULT submitted=1100 inv_to_send=1100
    Tests successful

The existing transaction-download control suite also passed on the same
binary:

python3 test/functional/p2p_tx_download.py --configfile=/mnt/my_storage/bitcoin/build/test/config.ini --tmpdir=/mnt/my_storage/bitcoin-goal89-functional --nocleanup --randomseed=8901

It covered relay permissions, large inventory batches, inbound request
limits, relay timing, rejection handling, and the normal one-entry
inv_to_send drain, and ended with Tests successful.

Verdict and impact:

Confirmed. The reproduction exceeds the 1,000-item per-send cap while the
peer remains connected and the node continues accepting transactions. The
queue stores a wtxid plus ordered-set node overhead per peer; multiple peers
multiply the retained metadata. In production, a slow or non-reading peer,
high accepted-transaction rate, mempool churn that leaves stale queued IDs,
or a ForceRelay gateway can extend retention beyond this deterministic
timer-free demonstration. This cycle proves missing admission accounting and
retention, not a complete remote denial-of-service severity bound; peer
socket backpressure, transaction policy, and the global mempool limit still
need to be included in a final impact assessment.

No source fix was committed because the current upstream line contains a
later per-peer-rate-limit redesign and the smallest compatible fix requires a
choice between queue admission/drop semantics and the follow-up global relay
rate limiter. A speculative local cap could silently drop announcements or
change ForceRelay behavior. The exact finding, source trace, control suite,
and reproducer are preserved for a dedicated fix/backport cycle.

Dismissed or excluded candidates:

- MAX_PEER_TX_ANNOUNCEMENTS=5000 is not a cap for the outbound set; it only
  limits ordinary inbound transaction requests, so it is not a separate
  finding.
- One-entry relay timing and permission behavior passed the existing control
  suite; no permission regression was claimed from that test.
- No source, test, build, or protected checkout files were changed by the
  probe. Generated test/cache/ output was removed; the three pre-existing
  Core worktree entries were preserved.

Next queue:

Revisit Goal 89 only for a distinct transport, permission-transition,
partial-I/O, disconnect/shutdown, or peer-accounting hypothesis. Exclude this
outbound inventory-queue admission cell from immediate rediscovery. The next
controller draw is recorded in uber-goal-state.md.

## Cycle 101: split-read v2 responder accounting

Status: confirmed test-framework defect. No protected Bitcoin Core source,
test, or build file was changed; the exact correction and regression test are
left as a report-ready handoff.

Controller draw:

- Draw seed: 12233973803523494861.
- Eligible pool: 77 81 82 84 87 89 95 97.
- Selected index: 5.
- Selected goal: 89, bitcoin-p2p-accounting.
- Cycle timestamp: 2026-07-28T12:49:31Z.

Repository state and provenance:

- Audit worktree: /tmp/secp256k1-oracles-next, branch codex/fuzz-oracles,
  clean at HEAD 890d42e14a6f48cddf8e04fc6f445feccf318ce2 before this journal
  snapshot.
- Protected Bitcoin Core checkout: /mnt/my_storage/bitcoin, branch
  codex/btc-fuzz-oracles, HEAD 00c4bb06ae9bf903af6ff72dbd6b097f36830ce6.
  Its pre-existing `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`,
  and `?? fuzz-1.log` entries were preserved exactly.
- Relevant current-file SHA256 values:
  `test/functional/test_framework/v2_p2p.py`
  0ba6bfc9495623bdcf759873465341ac381e419837c6951872fff064cb2f64ad;
  `test/functional/test_framework/p2p.py`
  42ac037071df9d1bca7defd21edeb96333380e1d8fd338dbb3b4df3d8a10ea28;
  `test/functional/p2p_v2_misbehaving.py`
  9a2f6d65ce067d9eb6bcf10c0ed7fd44d5815c0cf2448f99d63ded21eff327ce;
  `test/functional/p2p_v2_transport.py`
  48da9db6999297799a6f7437d2f7271f9d2a2413928e09ebfb1cccb1d0f61643.

Hypothesis and trust boundary:

The test-only v2 responder can lose bytes when the initiator's 16-byte V1
prefix probe is split across socket reads. `EncryptedP2PState.received_prefix`
is persistent, but `respond_v2_handshake()` returns the total prefix length
instead of the number of bytes consumed from the current `BytesIO` chunk. Its
caller in `P2PConnection._on_data_v2_handshake()` then executes
`self.recvbuf = self.recvbuf[length:]`, treating that cumulative length as a
current-buffer offset. A mismatching v2 key byte or the remaining v2 key and
garbage can therefore be discarded. This is a defect in the functional-test
peer implementation, not in the production C++ transport; it can make tests
fail to exercise the real continuation path and can hide bugs in code under
test.

Source and history evidence:

- `test/functional/test_framework/v2_p2p.py:130-150` appends one byte to the
  persistent `received_prefix` and returns `len(self.received_prefix)` on
  empty input, mismatch, and full match. The function has no local consumed
  counter.
- `test/functional/test_framework/p2p.py:275-302` passes a new `BytesIO` over
  the current `recvbuf` and slices that current buffer by the returned length.
  The caller therefore requires a per-call delta, not a cumulative prefix
  length.
- The original helper was introduced by commit `b89fa59e71` (`[test]
  Construct class to handle v2 P2P protocol functions`). `git log -S` found
  no later correction to the cumulative return.
- The production C++ transport keeps all received bytes in its transport
  buffer and has separate V1-prefix, key, and packet states, so this exact
  cumulative/delta mismatch does not transfer to `src/net.cpp`.

Independent reproduction:

Command:

    PYTHONPATH=test/functional python3 - <<'PY'
    from io import BytesIO
    from test_framework.messages import MAGIC_BYTES
    from test_framework.v2_p2p import EncryptedP2PState
    prefix = MAGIC_BYTES['regtest'] + b'version\x00\x00\x00\x00\x00'
    def probe(second_bytes):
        state = EncryptedP2PState(initiating=False, net='regtest')
        first = BytesIO(prefix[:4])
        state.respond_v2_handshake(first)
        second = BytesIO(second_bytes)
        reported, _ = state.respond_v2_handshake(second)
        return reported, second.tell(), second_bytes[second.tell():reported]
    assert probe(b'X' + b'preserved-after-mismatch') == (5, 1, b'pres')
    assert probe(prefix[4:] + b'preserved-after-full-prefix') == (16, 12, b'pres')
    print('HELPER_PROBE_ASSERTIONS=pass')
    PY

Key output from the actual helper:

    HELPER_PROBE mismatch reported/actual/lost= 5 1 b'pres'
    HELPER_PROBE full_match reported/actual/lost= 16 12 b'pres'
    HELPER_PROBE_ASSERTIONS=pass

The first case models a four-byte network-magic chunk followed by a chunk
whose first byte mismatches the V1 prefix. The caller removes five bytes from
the second chunk even though the helper read one, losing four subsequent
bytes. The second case models a four-byte prefix chunk followed by the final
12 prefix bytes and payload; it reports 16 while consuming 12 and loses the
same four-byte prefix-relative slice. A minimal correction is to initialize a
local `consumed = 0`, increment it for each `response.read(1)`, and return
`consumed` on every path. The persistent `received_prefix` remains the input
to `complete_handshake()`.

Existing-test verification and gap:

- `python3 test/functional/p2p_v2_misbehaving.py --configfile=build/test/config.ini --tmpdir=/mnt/my_storage/goal89-p2p-v2-misbehaving --cachedir=/mnt/my_storage/bitcoin/build/test/cache --randomseed=8901 --portseed=18901`
  passed. Its early-key test deliberately sends four matching bytes, then the
  remainder plus garbage, but only checks that the custom peer receives a
  response and later disconnects; it does not assert that the responder
  retained every byte after the split.
- `python3 test/functional/p2p_v2_transport.py --configfile=build/test/config.ini --tmpdir=/mnt/my_storage/goal89-p2p-v2-transport --cachedir=/mnt/my_storage/bitcoin/build/test/cache --randomseed=8902 --portseed=18902`
  passed. Its raw V1-prefix test writes 15 then 1 bytes, and its normal peer
  paths use a first-byte mismatch or complete writes; neither asserts current
  chunk consumption after a persistent prefix.
- The duplicate-cipher suspicion was dismissed before this finding: the
  current `v2_enc_packet()` has one length-cipher call, and `FSChaCha20.crypt`
  is stateful but not duplicated at the current HEAD.

Verdict and impact:

Confirmed as a test-framework correctness defect. The current helper can
silently discard v2 key, garbage, or payload bytes at a split-read boundary,
causing a false timeout or preventing a test peer from completing a handshake.
This is not evidence of a Bitcoin Core production transport vulnerability and
does not justify a Core source fix. It does justify correcting the test
helper and adding a regression case that feeds a split prefix plus trailing
bytes, then asserts the trailing bytes reach `complete_handshake()` or the
V1 fallback decision intact. The existing passing tests are insufficient
because their assertions observe the expected high-level outcome, not buffer
conservation.

No commit was made in the protected Core checkout. No daemon, test process, or
scratch artifact remained after the functional runs. The prior outbound
inventory-queue admission cell remains excluded from immediate rediscovery.

Next queue:

Goal89's split-read responder cell is complete and excluded. Continue with the
fresh controller draw in `uber-goal-state.md`; keep Goal89 eligible only for a
different permission, partial-I/O, disconnect/shutdown, or peer-accounting
cell.
