# Goal 87: Bitcoin mempool, package, and eviction-accounting audit

## Cycle 81

Status: confirmed for the `unbroadcast` memory-accounting cell; no protected
Bitcoin Core source change was made. Goal 87 remains pending for distinct
package, RBF, graph, fee, eviction, reorg, and expiry cells.

Controller draw:

- Draw seed: `10788815325715498911`.
- Eligible pool: `77 81 82 87 95 97`.
- Selected index: `3`.
- Selected goal: `87`, `bitcoin-mempool-accounting`.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`.
- Audit branch: `codex/fuzz-oracles`.
- Protected Core: `/mnt/my_storage/bitcoin`, branch
  `codex/btc-fuzz-oracles`, base `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`.
- Protected Core was unchanged apart from its pre-existing `M
  src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and `?? fuzz-1.log`.
- Protected secp256k1 remained clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- No node, fuzz, test, or build process remained running at handoff.

Prior-finding search:

- `agent-journal/memory-pressure-allocator.md` already covers the 5,658-file
  `tx_pool` corpus replay and its allocator/RSS interpretation. That work is
  not repeated as a graph-accounting finding.
- The non-ancestor contributor branch `codex/fuzz-oracles-current` contains
  L0rinc commit `f97dbc3e7098e558b1f9ebc71a8879b567f2a6b3`, which strengthens
  `tx_pool` transition oracles and reports no clean-master production bug.
  It is useful prior evidence but was not treated as proof for this exact
  clean HEAD.

Hypothesis and trust boundary:

`CTxMemPool::DynamicMemoryUsage()` may omit the locally retained
`m_unbroadcast_txids` set. A local wallet or RPC caller can submit transactions
which remain in that set until a peer requests them or the periodic retry
logic removes them. If the set is omitted, `getmempoolinfo.usage`, mempool
trimming, and the chainstate cache-space calculation underreport live memory.
The trust boundary is local transaction submission and peer relay; this is
not a remote consensus or unauthenticated network claim.

Source trace on the unmodified base:

- `src/txmempool.h:282-284` declares `std::set<Txid> m_unbroadcast_txids`.
- `src/txmempool.h:539-557` inserts, removes, and exposes that set.
- `src/node/transaction.cpp:97-103` adds accepted local broadcast
  transactions to it.
- `src/net_processing.cpp:1734-1746` retries it and
  `src/net_processing.cpp:2676-2681` removes entries after GETDATA; ordinary
  mempool removal also calls `RemoveUnbroadcastTx` at
  `src/txmempool.cpp:284-287`.
- `src/txmempool.cpp:778-781` counts `mapTx`, `mapNextTx`, `mapDeltas`,
  `txns_randomized`, the txgraph, and cached entries, but not
  `m_unbroadcast_txids`.
- `src/rpc/mempool.cpp:1063-1074` publishes this value as `usage`, and
  `src/validation.cpp:2799-2803` uses it when balancing the mempool and coin
  cache. `src/txmempool.cpp:868` uses it as the trim stop condition.

Independent verification:

1. Static accounting model. A disposable C++ model using the repository's
   `memusage::DynamicUsage(const std::set<X, Y>&)` implementation populated
   1,100 distinct `uint256` values. The command was:

   `g++ -std=c++20 -O2 -Isrc /mnt/my_storage/bitcoin-goal87-set-usage.cpp -o /mnt/my_storage/bitcoin-goal87-set-usage && /mnt/my_storage/bitcoin-goal87-set-usage`

   It printed:

   `SET_USAGE_RESULT count=1100 before=0 after=88000 delta=88000`

   The scratch source and binary were removed after capture. This establishes
   that the omitted container has nonzero project-estimated dynamic usage.

2. Runtime state transition. The disposable probe
   `/mnt/my_storage/bitcoin-goal87-unbroadcast-probe.py` (SHA256
   `21615d71539447c339d7c02fbabff49df0fcb359d23d4f6c5809b54b07462055`) ran
   the unmodified Core binary with 1,200 generated blocks, 1,100 accepted
   local transactions, frozen relay timing, and one ordinary relaying peer.
   Command:

   `python3 /mnt/my_storage/bitcoin-goal87-unbroadcast-probe.py`

   Result:

   `UNBROADCAST_USAGE_RESULT before_usage=1234736 after_usage=1234736 before_unbroadcast=1100 after_unbroadcast=1024`

   Relay removed 76 unbroadcast entries while the reported mempool usage was
   exactly unchanged. This matches the source trace: set membership changes
   do not affect `DynamicMemoryUsage()`.

3. Existing behavioral controls. The focused Core suite

   `./build/bin/test_bitcoin --run_test=mempool_tests,txpackage_tests,txgraph_tests,rbf_tests --log_level=test_suite`

   passed 24 test cases with `*** No errors detected`. The existing `tx_pool`
   corpus and its sanitizer interpretation remain recorded in the Goal 74
   journal; they found no graph invariant or production RSS defect, but they
   do not account for this omitted set.

Finding:

The hypothesis is confirmed as an incomplete dynamic-memory accounting path.
The omitted set is bounded by locally submitted transactions still in the
mempool, so this is a local resource-accounting and operational-limit issue,
not a remote DoS or consensus vulnerability. On the model above, 1,100
retained entries add an estimated 88,000 bytes outside the reported usage.
The omission can make `usage` and the `maxmempool` trim/cache-sharing decisions
low by that amount, with the gap growing as retained local submissions grow.

Proposed minimal fix:

Add `memusage::DynamicUsage(m_unbroadcast_txids)` to the return expression in
`src/txmempool.cpp:781`, and add a regression assertion that adding an
unbroadcast marker increases `DynamicMemoryUsage()` by the set estimate. No
source fix was committed because the disposable validation build exhausted
the host filesystem before linking `test_bitcoin`.

Validation limitation:

- A disposable fix worktree at `/tmp/bitcoin-goal87-unbroadcast-fix` was
  configured successfully with a new mempool regression test.
- `cmake --build /tmp/bitcoin-goal87-unbroadcast-fix/build --target
  test_bitcoin -j2` reached 11% and failed while archiving LevelDB/test
  libraries with `No space left on device`. The worktree and partial build
  were removed. This is an environment blocker for the proposed patch, not a
  failed behavioral result.
- Evidence is Linux x86_64 and uses the project's allocator estimate. A full
  build, maxmempool boundary test, and review of intended accounting scope are
  still required before upstreaming a source patch.

Verdict and next queue:

Record the unbroadcast-memory cell as **confirmed**, exclude it from immediate
rediscovery, and continue Goal 87 with package `test_accept` state preservation,
RBF replacement and fee-delta accounting, ancestor/descendant recomputation
after reorg and trim, eviction ordering, and expiry cleanup. Do not repeat the
Goal 74 RSS corpus cell or the prior `tx_pool` oracle-hardening campaign.

Exact handoff: restore free space, build the proposed one-line fix and test in
a disposable Core worktree, then run the focused mempool/package/RBF suites.
If that passes, commit the source fix separately from this journal; otherwise
retain this finding as a report-ready accounting defect and continue with the
next distinct Goal 87 cell.

## Cycle 82

Status: dismissed for the bounded package, replacement, graph, and eviction
state-preservation cell. The earlier `m_unbroadcast_txids` dynamic-memory
finding remains confirmed; no protected Bitcoin Core source change was made.
Goal 87 remains pending for independent eviction accounting, reorg symmetry,
expiry cleanup, package fee deltas, and newly exposed cluster-limit paths.

Controller cycle: 102.

Repository state:

- Audit worktree: `/tmp/secp256k1-oracles-next`, branch `codex/fuzz-oracles`,
  clean after commit `905b68e2`.
- Protected Core: `/mnt/my_storage/bitcoin`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, unchanged apart from its
  pre-existing `M src/test/blockencodings_tests.cpp`, `?? fuzz-0.log`, and
  `?? fuzz-1.log`.
- Temporary test data used `/mnt/my_storage` because `/tmp` was full. No test,
  fuzz, or daemon process remained at handoff.

Hypothesis and trust boundary:

Package acceptance, package RBF staging, or TxGraph trimming may leave the
main mempool graph, fee diagram, removal set, or outpoint bookkeeping changed
after a rejected or partially applicable operation. The trust boundary is a
local package/RBF caller and the policy-only cluster-limit machinery. A
failure would be a local correctness/accounting defect, not a consensus claim.

Source and oracle trace:

- `CTxMemPool::ChangeSet` starts TxGraph staging, stages removals/additions,
  and its destructor aborts un-applied staging in `src/txmempool.h:595-635`.
- `src/txgraph.cpp:2626-2679` marks staged locators missing and restores the
  main graph on abort; `CommitStaging()` applies removals before moving staged
  clusters to main.
- The package evaluator asserts its acceptance result and then runs the
  relevant mempool invariants at `src/test/fuzz/package_eval.cpp:534-555`.
- The independent TxGraph model fuzzer compares counts, existence, ancestry,
  descendants, clusters, diagrams, block-builder ordering, worst-chunk
  removal, trim closure, and final `SanityCheck()` in
  `src/test/fuzz/txgraph.cpp:575-1388`.
- Package RBF unit tests check replacement chunk monotonicity and fee
  diagrams; the focused suite includes `txpackage_tests` and `rbf_tests`.

Independent verification:

1. Focused unit suites, with temporary storage redirected:

   `TMPDIR=/mnt/my_storage/bitcoin-goal87-tmp-mempool ./build/bin/test_bitcoin --run_test=mempool_tests --log_level=message`

   passed 4 cases with `*** No errors detected`. The package/RBF replay:

   `TMPDIR=/mnt/my_storage/bitcoin-goal87-tmp-tests ./build/bin/test_bitcoin --run_test=txpackage_tests,rbf_tests --log_level=test_suite`

   passed 14 cases with `*** No errors detected`.

2. The non-sanitized TxGraph model fuzzer ran

   `FUZZ=txgraph ./build_fuzz_nosan/bin/fuzz -runs=10000 -max_len=4096 -print_final_stats=1`

   to completion: 10,000 runs, coverage `1738`, feature count `3435`, corpus
   `227/4172b`, average `109` executions/second, peak RSS `1831 MB`, and no
   assertion or fuzzer failure.

3. Existing package corpora were replayed in the non-sanitized build:

   - `tx_package_eval`: 2,116 executions over 2,115 files, coverage `7158`,
     features `40612`, peak RSS `1832 MB`, no failure.
   - `ephemeral_package_eval`: 1,672 executions over 1,671 files, coverage
     `6465`, features `40744`, peak RSS `1832 MB`, no failure.
   - `package_rbf`: 1,000 executions, coverage `2635`, features `14147`, peak
     RSS `1833 MB`, no failure.

4. The broad `tx_package_eval` corpus was replayed in the ASan/UBSan build:

   `FUZZ=tx_package_eval ./build_fuzz/bin/fuzz /mnt/my_storage/qa-assets/fuzz_corpora/tx_package_eval -runs=1 -max_len=4096 -print_final_stats=1`

   completed 2,117 executions, coverage `14334`, features `84505`, corpus
   `872/1293Kb`, peak RSS `1833 MB`, with no ASan, UBSan, assertion, or
   invariant failure.

The corpus inputs were the existing `qa-assets` sets: 2,115
`tx_package_eval` files (77,567,620 bytes), 1,671 `ephemeral_package_eval`
files (9,748,246 bytes), and 891 `package_rbf` files (78,899,303 bytes).
The tested source snapshot was Core `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`;
the relevant source hashes were `txmempool.cpp`
`68552be0be58fe9343f4eca54585de48a806691d7b321e5088ad865338d80651`,
`txgraph.cpp`
`6019e0c9ece5c336874f25c6d9805b742c1e7c8103c2478bebc66e8a9a853ae9`, and
the package/RBF/graph fuzzers
`58d3bb90571153b0efda8003662755efb55c36bf4620de36a607d6d37d0039be`,
`e8ce3d97f80ca1d10de79ef666ffedfd5c3e622b3a13a5e168ebe6035a3c2e85`, and
`3b9a6b85800a39b0a62c23fe910f87e7b2a0bb46963fc54b9e81b2494fde20df`.

Verdict:

Dismissed for this exact package/RBF staging and graph-accounting cell. The
independent model and production package paths agreed across the exercised
state space; focused unit suites and the sanitizer replay found no changed
state, stale graph reference, invalid removal closure, fee-diagram mismatch,
or resource-accounting assertion. This does not prove all package behavior;
expiry, reorg reinsertion, unbroadcast accounting, and larger or newly
generated cluster-limit cases remain separate cells. Exclude this exact
hypothesis from immediate rediscovery.

Next queue:

- Verify whether `Expire()` and `removeForReorg()` report and remove exactly
  the same graph closure under shared ancestors, already-missing parents, and
  repeated timestamps.
- Recheck the confirmed unbroadcast-memory finding only after storage is
  available for a disposable source fix and boundary regression test.
- Continue with eviction fee/accounting and package fee-delta cells if the
  next expiry/reorg cell is clean.
