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
