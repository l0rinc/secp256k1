# Network fragmentation, reordering, and partial-I/O state-machine audit

## Cycle 1: 2026-07-28

Status: dismissed the current socket zero/short-write hypothesis. The
deterministic socket-level oracle, the existing V2 transport test, and all
current `net_tests` passed. No production source change or Core fix commit was
justified.

### Controller selection and repository state

- Controller cycle: 59.
- Catalog goal: `73:network-state-machine`.
- Draw seed: `1209637965`.
- Eligible pool size: 13; selected index: 3.
- Eligible pool: `52,53,72,73,74,77,81,82,84,87,89,95,97`.
- Attachment SHA-256: `1639d16123a404f70037ff15f15464f26fbd7ee0fe363f441064c6dd15f72102`.
- Audit checkout: `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, HEAD at cycle start
  `3a1ea12c150dacffc64d55aca097872db9a15264`, with a clean worktree. Its
  remotes were `origin=https://github.com/bitcoin-core/secp256k1` and
  `l0rinc=https://github.com/l0rinc/secp256k1`.
- `/mnt/my_storage/secp256k1` remained detached and clean at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- `/mnt/my_storage/bitcoin` remained at branch `codex/btc-fuzz-oracles`,
  HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, with only the pre-existing
  modified `src/test/blockencodings_tests.cpp` and untracked `fuzz-0.log` and
  `fuzz-1.log`.
- No relevant daemon, fuzz, sanitizer, test, benchmark, or build job was
  running at the start of the cycle.

### Existing evidence and hypothesis

The prior Core history sweep already indexed `d93e4f7e26` and its malformed
net-message queue oracles, so that finding was excluded as a duplicate. The
current `net_tests/v2transport_test` at `src/test/net_tests.cpp:1414` already
feeds deterministic transport interactions through positive fragmented reads
and partial transport writes, but it does not exercise the actual
`CConnman::SocketSendData` path with a zero-byte nonblocking socket write.

The distinct hypothesis was that a zero return from `Sock::Send`, followed by a
short positive return, could lose or duplicate bytes, incorrectly clear a V1
or V2 transport buffer, corrupt `nSendBytes`, or cause the receive side to be
skipped or disconnected. The source contract is concentrated in
`src/net.cpp:1607-1684`: positive writes call `MarkBytesSent` for exactly the
returned count and stop on a short write; zero or accepted nonblocking errors
leave the transport state in place. `SocketHandlerConnected` at
`src/net.cpp:2147-2247` suppresses receive only when a positive send made
progress while data remains, preserving progress when the send made no
progress. The V1/V2 transport state machine and `GetBytesToSend`/`MarkBytesSent`
contracts were traced through these callers and the existing tests.

### Deterministic socket-level oracle

A disposable Core worktree was created at `/tmp/bitcoin-net-73-test` from the
required Core HEAD, with build output moved to
`/mnt/my_storage/bitcoin-net-73-build` after the initial `/tmp` tmpfs filled
during linking. The disposable-only test added `ScriptedSendSock`, whose
`Send` script returns zero, then one byte, then full chunks, while `Recv`
returns `-1/EAGAIN`. The test sends a V1 `PING` containing
`0x123456789abcdef0` through `CConnman::PushMessage`, calls
`SocketHandlerPublic()` twice, and independently constructs the expected V1
wire bytes with a separate `V1Transport`.

The disposable test was compiled with:

```sh
cmake -S /tmp/bitcoin-net-73-test -B /mnt/my_storage/bitcoin-net-73-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTS=ON \
  -DBUILD_BENCH=OFF -DENABLE_WALLET=ON -DWITH_ZMQ=OFF
cmake --build /mnt/my_storage/bitcoin-net-73-build --target test_bitcoin -j8
```

The first identical build under `/tmp/bitcoin-net-73-build` reached the final
link but failed with `final link failed: No space left on device`; this was a
scratch filesystem condition, not a source or test failure. The disk-backed
build completed all `543/543` Ninja steps. The focused command was:

```sh
env TMPDIR=/mnt/my_storage/bitcoin-net-73-tmp \
  /mnt/my_storage/bitcoin-net-73-build/bin/test_bitcoin \
  --run_test=net_tests/socket_send_retries_zero_and_short_writes \
  --log_level=message
```

It exited `0` with `*** No errors detected`. The oracle verified that the
send queue was moved into the transport after the optimistic zero-byte write,
the exact expected wire byte vector was emitted after the short write, the
transport buffer was empty, and `node->nSendBytes` equaled the wire length.
The four socket calls are normal V1 framing (`24`-byte header and `8`-byte
payload): the initial zero/short retry sequence does not duplicate bytes.

The independent existing controls also passed:

```sh
env TMPDIR=/mnt/my_storage/bitcoin-net-73-tmp-v2 \
  /mnt/my_storage/bitcoin-net-73-build/bin/test_bitcoin \
  --run_test=net_tests/v2transport_test --log_level=message
# *** No errors detected; Running 1 test case...

env TMPDIR=/mnt/my_storage/bitcoin-net-73-tmp-all \
  /mnt/my_storage/bitcoin-net-73-build/bin/test_bitcoin \
  --run_test=net_tests --log_level=message
# *** No errors detected; Running 19 test cases...
```

The first version of the disposable oracle expected three sends. Instrumenting
the mock showed attempts of lengths `24,24,23,8`: the fourth attempt is the
normal V1 payload after the 24-byte header, not a retry or duplicate. The
oracle was corrected to expect four calls and then passed. This was a test
oracle mistake, not a production finding.

### Verdict and limitations

Dismissed for the current hypothesis. Clean-source reasoning, an independent
wire-byte construction, the actual CConnman socket path, the existing V2
transport state machine, and the full network unit suite all agree that zero
and short writes preserve state and accounting on this build. No production
repair, regression commit, or audit-checkout source change was made.

Coverage is Linux/x86_64 Release and the custom socket-level control is V1;
V2 was verified through its existing deterministic transport interaction test,
not through a new full CConnman V2 handshake/socket fixture. This cycle did
not cover network reordering, EOF at every byte, platform socket semantics,
or a real remote peer. Those remain reopenable only with a distinct caller,
transport, platform, or failure-schedule hypothesis. The temporary Core
worktree, build, and test directories are removed after the journal commit.

Next queue: `52,53,72,74,77,81,82,84,87,89,95,97`.
