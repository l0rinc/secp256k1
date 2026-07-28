# Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit

## Cycle 1: 2026-07-28

Status: confirmed Bitcoin Core wallet passphrase persistence defect; repaired in
a disposable Core worktree and recorded as Core commit `1c1300e8b7`. The
libsecp256k1 audit checkout was not changed except for this journal and the
uber-goal state journal.

### Controller selection and repository state

- Controller cycle: 58.
- Catalog goal: `88:bitcoin-wallet-recovery`.
- Draw timestamp: 2026-07-28.
- Draw seed: `2193848575`.
- Eligible pool size: 15; selected index: 10.
- Eligible pool: `52,53,72,73,74,77,81,82,84,87,88,89,95,97,98`.
- Attachment SHA-256: `1639d16123a404f70037ff15f15464f26fbd7ee0fe363f441064c6dd15f72102`.
- Audit checkout: `/tmp/secp256k1-oracles-next`, branch
  `codex/fuzz-oracles`, HEAD at cycle start
  `6eb752f2e5c0a7e8fd619f1cbe3c16af3cd4e2d2`, clean before this journal.
  Remotes were `origin=https://github.com/bitcoin-core/secp256k1` and
  `l0rinc=https://github.com/l0rinc/secp256k1`.
- User secp256k1 checkout remained clean and detached at
  `e153e2681f7bf1dd74894e2170213e3983030989`.
- The user's Bitcoin Core checkout was not modified: branch
  `codex/btc-fuzz-oracles`, HEAD
  `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`, modified
  `src/test/blockencodings_tests.cpp`, and untracked `fuzz-0.log` and
  `fuzz-1.log` remained present.
- No relevant worker, daemon, fuzz, sanitizer, test, benchmark, or build job
  was running at cycle start. The temporary wallet daemon is stopped during
  handoff cleanup.

### Candidate screening and hypothesis

The selected goal covers encryption, passphrase changes, key material, durable
wallet state, recovery, and faulted database operations. The distinct
hypothesis was that `CWallet::ChangeWalletPassphrase` reports success and
publishes a new in-memory master-key record before checking whether the
database write succeeded.

The clean current Core source at
`/mnt/my_storage/bitcoin/src/wallet/wallet.cpp:636-666` calls
`EncryptMasterKey` directly on the map entry, ignores the boolean returned by
`WalletBatch::WriteMasterKey`, logs success, and returns `true`. The database
contract at `src/wallet/walletdb.cpp:151-154` returns the underlying write
result. The SQLite implementation at `src/wallet/sqlite.cpp:490-521` returns
false after a non-`SQLITE_DONE` `sqlite3_step` and logs the failure. The RPC
caller at `src/wallet/rpc/encrypt.cpp:159-163` treats the boolean as the
complete passphrase-change result.

This creates a split-brain state: the process can unlock with the new
passphrase while the durable wallet still contains the old encrypted master
key. A crash or restart then loses the apparent change. The RPC's existing
boolean-to-error mapping also reports a database failure as "old passphrase
incorrect", which is a residual diagnostic issue but not needed to establish
the false-success defect.

History/blame showed the ignored write was present in the long-lived original
implementation. Recent changes `a8333fc9ff9adaa97a1f9024f5783cc071777150`
and `846545947cd3b993c40362b9d0afcd7b4f5f05bd` did not add a write-result
check. Search of the local history and existing wallet tests found no current
duplicate repair or direct database-failure passphrase-change test.

### Deterministic fault harness

The clean Core binary was `/mnt/my_storage/bitcoin/build/bin/bitcoind`, a
wallet-enabled dynamically linked Linux build. A scratch SQLite interposer
`/tmp/wallet-88/fail_master_key_sqlite.so` wrapped `sqlite3_step`. When
`WALLET88_FAIL_FILE` existed, it returned `SQLITE_IOERR` once for the wallet
master-key `INSERT or REPLACE` statement. It was compiled with:

```sh
gcc -shared -fPIC -O2 -Wall -Wextra -std=c11 \
    -o /tmp/wallet-88/fail_master_key_sqlite.so \
    /tmp/wallet-88/fail_master_key_sqlite.c -ldl -lsqlite3
```

Each daemon used a fresh regtest datadir, wallet `w88`, no listen/discovery or
DNS seed activity, and the following faulted launch shape:

```sh
env LD_PRELOAD=/tmp/wallet-88/fail_master_key_sqlite.so \
    WALLET88_FAIL_FILE=/mnt/my_storage/.wallet-88/fail \
    /mnt/my_storage/bitcoin/build/bin/bitcoind -regtest \
    -datadir=/mnt/my_storage/.wallet-88 -daemon=1 -listen=0 \
    -discover=0 -dnsseed=0 -natpmp=0 -fallbackfee=0 -dbcache=16 \
    -maxmempool=5 -printtoconsole=0 -rpcbind=127.0.0.1 \
    -rpcallowip=127.0.0.1
```

The wallet was created and encrypted through JSON-RPC, and the old passphrase
was checked before touching the failure flag. The RPC sequence used
`walletpassphrasechange(["old-pass-88","new-pass-88"])`, followed by
`walletpassphrase` with both passphrases, then a daemon stop/restart and the
same checks. All wallets, credentials, and datadirs were scratch-only.

### Before-repair evidence

On the clean Core binary, after creating and encrypting `w88`, the injected
change returned apparent success:

```json
{"result":null,"error":null,"id":"change"}
```

The same process then accepted `new-pass-88` and rejected `old-pass-88`.
After stopping, removing the fault flag, and restarting the same datadir, the
old passphrase was accepted and the new passphrase was rejected. The wallet
log contained both the success message and the SQLite failure:

```text
[w88] Wallet passphrase changed to an nDeriveIterations of 213391
[warning] Unable to execute write statement: disk I/O error
```

This is a direct clean-master reproduction of a false success and durable
passphrase rollback. It is not merely a theoretical partial-write concern.

### Repair and independent after-repair evidence

A disposable Core worktree at `/mnt/my_storage/.bitcoin-wallet-88-fix` was
created from Core HEAD `00c4bb06ae9bf903af6ff72dbd6b097f36830ce6`. The repair
build used:

```sh
cmake -S /mnt/my_storage/.bitcoin-wallet-88-fix \
      -B /mnt/my_storage/.bitcoin-wallet-88-fix-build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
      -DBUILD_BENCH=OFF -DENABLE_WALLET=ON -DWITH_ZMQ=OFF
cmake --build /mnt/my_storage/.bitcoin-wallet-88-fix-build \
      --target bitcoind -j8
```

The build completed through `[320/320] Linking CXX executable bin/bitcoind`.
The one-file patch committed as `1c1300e8b7`:

```text
wallet: preserve passphrase on database failure
```

It encrypts a copy of `CMasterKey`, checks `WriteMasterKey`, publishes the
copy to `mapMasterKeys` only after the durable write succeeds, and restores
the prior lock state on the new failure exits. This ordering prevents the
in-memory record from getting ahead of SQLite.

Running the same injected fault against the repaired binary returned failure
instead of success:

```json
{"result":null,"error":{"code":-14,"message":"Error: The wallet passphrase entered was incorrect."},"id":"change"}
```

The message is the pre-existing RPC mapping noted above. The important state
assertions both passed: `old-pass-88` unlocked in the same process and after a
restart, while `new-pass-88` was rejected in both cases. The SQLite warning
remained visible, and no new durable master-key record was published.

The repaired daemon also passed a no-fault control before the final restart;
its log recorded a successful passphrase change, and `new-good-88` unlocked
in the same process. After restart with the rotated RPC cookie, the exact
control output was:

```text
new_good_after_restart={"result":null,"error":null,"id":"new-restart"}
old_after_restart={"result":null,"error":{"code":-14,"message":"Error: The wallet passphrase entered was incorrect."},"id":"old-restart"}
```

This separates the repaired fault behavior from a normal successful change.

### Verdict

Confirmed as a current Bitcoin Core wallet integrity and persistence-contract
defect. A local wallet RPC caller can receive success for a passphrase change
that was not durably written, and automation can believe the new passphrase
is authoritative until a restart reveals the old one. The defect does not
expose private keys, accept invalid consensus data, or itself erase wallet
funds; it is a local wallet availability/integrity issue with Low/Medium
master-relative severity. It is not a remote primitive and does not meet the
controller's High/Critical gates.

Independent verification consisted of the source/database contract trace, the
clean-master same-process versus restart differential, and the repaired
binary's same-process versus restart differential under the identical fault
schedule. Existing tests missed it because successful encryption and wallet
passphrase paths do not inject a failed SQLite master-key write or assert
durable behavior across restart.

### Limitations and handoff

- The fault injection is Linux/x86_64 and dynamically interposes SQLite; BDB,
  other filesystems, Windows/macOS, and static-link configurations were not
  exercised.
- The harness targets the SQLite master-key write and does not prove every
  backup, descriptor migration, keypool, rescan, or external-signer failure
  path. Those remain distinct follow-up cells for goal 88.
- No integrated Core test was added because the user's Core checkout was
  already dirty and must remain untouched. The deterministic RPC plus SQLite
  fault schedule is recorded above; the disposable interposer, worktree,
  build, and wallet datadirs are removed after handoff.
- The repair deliberately leaves the existing RPC diagnostic mapping for a
  false return as a separate goal-4/goal-46 API error-contract candidate.

Goal 88 is exhausted only for this passphrase database-write hypothesis. It
must be reopened if new wallet persistence callers, fault-injection hooks, or
descriptor/keypool/backup/recovery evidence changes the assumptions. The next
eligible queue is `52,53,72,73,74,77,81,82,84,87,89,95,97,98`.
