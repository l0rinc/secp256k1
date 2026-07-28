# Allocation, syscall, clock, randomness, and callback fault injection

## Cycle 1: 2026-07-28

Status: confirmed Bitcoin Core RPC startup defect; repair validated in a
disposable Core worktree and recorded as Core commit `34415a3962`. No source
change was made in the libsecp256k1 audit checkout.

### Controller selection and repository state

- Controller cycle: 57.
- Catalog goal: `93:system-fault-injection`.
- Draw seed: `2318999372`.
- Eligible pool size: 16; selected index: 12.
- Eligible pool: `52,53,72,73,74,77,81,82,84,87,88,89,93,95,97,98`.
- Attachment SHA-256: `1639d16123a404f70037ff15f15464f26fbd7ee0fe363f441064c6dd15f72102`.
- Audit checkout: `/tmp/secp256k1-oracles-next`, branch `codex/fuzz-oracles`,
  HEAD at cycle start `9f999768fa6eba990979a4c0d3245b143ab0e121`, with a clean
  worktree. Its remotes were `origin=https://github.com/bitcoin-core/secp256k1`
  and `l0rinc=https://github.com/l0rinc/secp256k1`.
- `/mnt/my_storage/secp256k1` was clean and detached at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- `/mnt/my_storage/bitcoin` was not modified: branch
  `codex/btc-fuzz-oracles`, HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`,
  modified `src/test/blockencodings_tests.cpp`, and untracked `fuzz-0.log` and
  `fuzz-1.log` remained present.
- No relevant worker, daemon, fuzz, sanitizer, test, benchmark, or build job
  was running at cycle start. Scratch daemons were stopped before handoff.

### Candidate screening and hypothesis

The raw libsecp allocation surface was screened first. Existing evidence in
`agent-journal/critical-history-sweep.md`, `src/fuzz/README.md`, and the
current fuzz targets already covers context cloning, scratch allocation and
rollback, ecmult callbacks, and callback-induced failure paths. No distinct
unindexed libsecp production allocation fault was found, so the cycle moved to
the actual Bitcoin Core caller surface.

The selected hypothesis was that `GenerateAuthCookie` in
`/mnt/my_storage/bitcoin/src/rpc/request.cpp:100-146` reports success after a
write or close failure because it only checks `is_open()` before writing and
then unconditionally renames the temporary file. This violates the public
contract in `src/rpc/request.h:32-43`: `Error` means auth data could not be
saved, while `Ok` means it was saved to disk and returned in `user`/`pass`.
The production caller in `src/httprpc.cpp:249-276` treats `Ok` as permission
to start cookie-authenticated HTTP RPC.

The analogous settings repair `ca00827fab5ed261104502a06a5dfd4c55145c28`
checks both write and close failures before renaming `settings.json`; history
search found no corresponding cookie fix.

### Deterministic fault harness

The scratch-only interposer `/tmp/fault-93/fail_cookie_io.c` was compiled as:

```sh
gcc -shared -fPIC -O2 -Wall -Wextra \
    -o /tmp/fault-93/fail_cookie_io.so \
    /tmp/fault-93/fail_cookie_io.c -ldl
```

It identifies file descriptors whose `/proc/self/fd` target contains
`/.cookie.tmp` and fails `write`/`writev` with `EIO` when
`COOKIE_FAIL_MODE=write`. It also has a `close` mode. The prebuilt clean
Core binary was `/mnt/my_storage/bitcoin/build/bin/bitcoind`, version
`v31.99.0-b08815bbb523`; the repaired validation binary was built from the
disposable worktree with:

```sh
cmake -S /tmp/bitcoin-fault-93-fix -B /tmp/bitcoin-fault-93-fix-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
  -DBUILD_BENCH=OFF -DENABLE_WALLET=OFF -DWITH_ZMQ=OFF
cmake --build /tmp/bitcoin-fault-93-fix-build --target bitcoind -j8
```

All daemon tests used a fresh regtest datadir, `-listen=0`, disabled discovery
and DNS seeding, `-dbcache=16`, `-maxmempool=5`, and a scratch filesystem with
adequate free space. The write-mode failure is observed at `close()` because
the standard stream buffers the short cookie record; this is still the
production write/flush failure that the unchecked close status exposes.

### Before-repair evidence

On the clean Core binary, a normal startup created a 75-byte `__cookie__:<64
hex>` file, started HTTP RPC, and authenticated a `getblockcount` request.
With the exact same daemon command and
`LD_PRELOAD=/tmp/fault-93/fail_cookie_io.so COOKIE_FAIL_MODE=write`:

- `regtest/.cookie` was created with `0` bytes.
- `debug.log` still said `Generated RPC authentication cookie ...`,
  `Using random cookie authentication.`, and `Starting HTTP server ...`.
- The daemon remained alive.
- A curl request using the on-disk cookie returned HTTP `401` with an empty
  body (`/tmp/fault-93/writefail/rpc.out`).

The control close-mode run retained a 75-byte cookie and returned the valid
JSON response with HTTP `200`; the libc close interposition did not trigger on
this libstdc++ path, so it is not claimed as an independent close failure.
The write/flush reproduction is sufficient for the confirmed defect.

The exact pre-repair log lines were:

```text
Generated RPC authentication cookie /tmp/fault-93/writefail/regtest/.cookie
Using random cookie authentication.
Starting HTTP server with 16 worker threads
```

### Repair and after-repair evidence

In disposable Core worktree `/tmp/bitcoin-fault-93-fix`, the smallest repair
checks `file.fail()` immediately after insertion, checks it again after
`file.close()`, removes `.cookie.tmp` on either failure, and returns
`AuthCookieResult::Error` before `RenameOver`. It was committed as:

```text
34415a3962 rpc: reject failed cookie writes
```

The repaired Release `bitcoind` passed a no-fault control: 75-byte cookie,
`2:64` field shape, and authenticated `getblockcount` HTTP `200` with
`{"result":0,"error":null,"id":"fault-93"}`.

The same production write-failure harness then produced:

```text
patched_write_pid=absent cookie=absent tmp=absent
log=Unable to close cookie authentication file;Unable to start HTTP server;
```

The daemon did not reach HTTP startup and did not rename an invalid cookie.
The explicit close-mode control was also run; on this platform it left a
75-byte cookie and the daemon stayed up, so no stronger close-mode claim is
made. The changed source compiled and linked under the same Release build.

### Verdict

Confirmed as a local RPC availability and API-contract defect in current
Bitcoin Core master-relative code. A local filesystem write/flush error can
leave an empty or partial on-disk cookie while startup returns `Ok` and the
daemon advertises random-cookie authentication. A local RPC client cannot
authenticate, although a separately configured `rpcuser`/`rpcpassword` path
is unaffected. This is not a consensus, wallet-key, funds, privacy, or
remotely reachable primitive; severity is Low/Medium local availability, not
High/Critical under the controller gate.

Independent evidence was the source/caller/contract trace, the clean-master
daemon fault reproduction with HTTP `401`, and the repaired disposable daemon
with no cookie and failed HTTP startup. Existing tests missed it because they
exercise successful cookie creation but do not inject a write/flush failure in
the RPC cookie stream. The analogous settings test/fix supplied prior-art
evidence without being the same code path.

### Limitations and handoff

- The fault harness is Linux/x86_64 and dynamically interposes libc I/O; no
  Windows/macOS or static-link evidence was collected.
- The close-only interposer did not intercept libstdc++'s close implementation
  in this build, so only the write/flush failure is confirmed.
- No integrated Core test was added because the user's Core checkout was
  already dirty and must remain untouched. The deterministic daemon harness
  is preserved by its exact source, build, command, and output above.
- The disposable Core worktree, build tree, daemon datadirs, and interposer are
  scratch artifacts and are removed after the journal commit. The audit branch
  contains only evidence journals, not the Core source fix.

Next queue: draw a fresh eligible catalog goal excluding active campaigns
`49`, `61`, and `78`, and exclude goal `93` until new platform or partial-I/O
evidence changes this hypothesis. The next eligible pool is
`52,53,72,73,74,77,81,82,84,87,88,89,95,97,98`.
