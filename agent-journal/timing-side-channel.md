# Statistical timing-side-channel campaign

## Cycle 62: 2026-07-28

### Scope and draw

- Branch: `codex/fuzz-oracles`.
- Base: `origin/master` at `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`.
- Cycle start HEAD: `13257d59056785ba81fae805d90062a1a330068a`.
- The audit branch was ahead by 1226 commits and was otherwise clean before
  the source fix recorded below. The protected secp checkout remained detached
  at `e153e2681f7bf1dd74894e2170213e3983030989`; no protected worktree was
  modified.
- Draw: `2026-07-28T05:43:47Z`, seed `1794593845`, pool size 12,
  pool `52 53 72 74 77 81 82 84 87 89 95 97`, index 1, goal 53.
- Catalog entry: `statistical timing-side-channel campaign`. The goal-78
  translation-validation journal already covered older scalar/field timing
  probes, so this cycle selected a fresh public-key generation operation and
  a separate ctime attribution check.

### Hypothesis and trust boundary

The selected operation was `secp256k1_ec_pubkey_create`, whose secret input is
the 32-byte private key and whose public output is a serialized public key.
The hypothesis was that scalar-dependent branches, table indices, or variable
work in generator multiplication might produce a measurable timing difference
between low- and high-Hamming-weight secret scalars. A second hypothesis was
that the current audit branch's recent opaque-key and MuSig validation changes
had crossed a secret/public boundary without a ctime regression test.

The operation's caller is the public API in `src/secp256k1.c:737-762`.
`secp256k1_ecmult_gen` uses fixed-loop generator multiplication and conditional
moves for table selection. The source and disassembly review covered
`src/ecmult_gen_impl.h`, `secp256k1_ecmult_gen_gej`, and the API's invalid-key
fallback. The historical ecmult timing family already covered in
`agent-journal/critical-history-sweep.md` was excluded as a duplicate.

### Timing experiment

The scratch Release build was configured from the audit branch with x86_64
assembly, modules disabled, tests disabled, and `SECP256K1_VALGRIND=OFF` in
`/tmp/secp256k1-goal53-build`; it built the static library successfully. The
disposable harness was `/tmp/secp256k1-goal53-timing.c` and executable
`/tmp/secp256k1-goal53-timing`. It pinned to the first available CPU, used TSC
cycles, randomized class order, and measured 120,000 samples per class. The
classes were scalar 1 and scalar n-1.

Initial run:

```
class=low-hamming-key-1 n=120000 mean=62808.651 median=62636 p95=62790 min=62370 max=231338
class=high-hamming-key-n-1 n=120000 mean=62804.782 median=62636 p95=62792 min=62374 max=237326
welch_t=0.671323 mean_delta=3.869 sink=000000000000005b
```

Three interleaved reruns produced Welch t-statistics `-0.451355`,
`-0.879812`, and `-0.241282`. Their medians and p95 values were equal or
within two cycles; means were polluted by scheduler outliers of roughly
25-29 million cycles. This is supporting evidence only, not a proof of
constant-time behavior. The candidate was dismissed as a measurable timing
signal in this setup, with the data and harness retained only as scratch
evidence.

### ctime attribution and repair

The audit-branch ctime build used Release, all enabled modules, x86_64
assembly, `SECP256K1_VALGRIND=ON`, and only ctime tests. Before the repair,

```
valgrind --tool=memcheck --quiet --error-exitcode=99 --track-origins=yes \
  --num-callers=18 /tmp/secp256k1-goal53-ctime-build/bin/ctime_tests
```

returned 99 with reports through `keypair_load`/`ge_eq_var`, MuSig nonce
generation and partial signing, and silent payments. A disposable worktree at
the clean `origin/master` commit `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`
was built with the same ctime configuration and exited 0. The reports were
therefore attributed to audit-branch changes, principally the keypair
consistency check from `bbca5a725` and the static-keypair compatibility path
from `e126fb621`, not to clean upstream master.

The repair makes two explicit boundaries:

1. `src/modules/extrakeys/main_impl.h` declassifies the derived public point
   immediately before comparing it with the already declassified public half
   of the opaque keypair.
2. `src/modules/musig/session_impl.h` aggregates secret-derived overflow and
   zero checks with bitwise `|` rather than short-circuiting `||`; the combined
   validity flag is declassified only after the expression and before the
   public failure branch. This preserves the secret inputs while avoiding a
   secret-dependent branch before declassification.

After the repair, the rebuilt ctime command exited 0 with no Memcheck output.
The same Release/Valgrind-enabled build was then reconfigured with the normal
test target. `bin/tests` built and exited 0 after 16 iterations in 61.264s:

```
Tests running silently. Use '-log=1' to enable detailed logging
iterations = 16
jobs = 0. Sequential execution.
random seed = 2924b30fd7983a2288cba2cbdbe347ac
Total execution time: 61.264 seconds
```

### Verdict

Confirmed audit-branch ctime/constant-time validation regression; fixed in the
cycle commit. Master-relative severity is no clean-origin finding: the exact
pre-repair ctime errors were introduced by the audit branch, and no remote
timing primitive or key disclosure was demonstrated. If the branch-local
short-circuit checks were merged unchanged, they would be a Medium constant-
time hygiene risk because secret-derived validity was evaluated before the
declassification boundary. The explicit public-point declassification is a
test-contract repair, not a claim that the derived point is secret after the
public consistency comparison.

Existing functional tests missed this because they do not track secret-defined
memory or distinguish short-circuit evaluation from bitwise aggregation. The
clean-origin ctime control and the repaired ctime run provide independent
attribution and regression evidence. No Bitcoin Core caller is affected by
this libsecp256k1-only cycle; the actual callers are the public keypair and
MuSig module APIs exercised by `ctime_tests`.

### Limitations and handoff

- dudect-style statistical timing was not run because the class timing showed
  no stable signal and this cycle's stronger evidence was the ctime gate.
- Only the x86_64 assembly backend was executed. The existing goal-82 and
  goal-69 queues cover backend and architecture differentials.
- ctime passing is evidence, not a proof of constant-time behavior. Future
  cycles should revisit secret/public boundaries after new module changes and
  run the int64/fallback build when the queue reaches those cells.
- Scratch paths `/tmp/secp256k1-goal53-*` and the disposable origin worktree
  are disposable and are removed after commit verification.
- Next eligible queue: `52,53,72,74,77,81,82,84,87,89,95,97`; exclude this
  exact pubkey-create/branch-local ctime cell, goal-72 active-block flush
  ordering, and goal-84 duplicate-participant map-cardinality cell unless
  new code or evidence reopens them.

## Cycle 65: randomized Schnorr signing timing matrix

### Scope and draw

- **Date:** 2026-07-28.
- **Base:** `origin/master` `0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`.
- **Cycle-start HEAD:** `db37c9567ba296745541f6f3d892252ae0e8778b`.
- **Draw:** seed `3987825996`, pool size 11, pool
  `53 72 74 77 81 82 84 87 89 95 97`, index `0`, goal `53`.
- **Excluded cells:** the prior `ec_pubkey_create` timing experiment and the
  branch-local ctime regression/repair from cycle 62; ECDH timing and ecmult
  historical families are already covered by `critical-history-sweep.md`.

### Hypothesis and caller

The fresh operation was `secp256k1_schnorrsig_sign32`, called by Bitcoin
Core's `KeyPair::SignSchnorr` at `src/key.cpp:426-439` and wrapped by
`CKey::SignSchnorr` at `src/key.cpp:272-275`. These are wallet/local signing
paths; this experiment does not claim a consensus or peer timing primitive.
The hypothesis was a stable timing difference between secret scalar classes
at the public signing API, despite ctime memory-taint checks already passing.

### Measurement

A clean scratch CMake Release shared build was configured from cycle-start
HEAD with all modules enabled, tests/benchmarks/fuzz/exhaustive/ctime disabled,
Valgrind disabled, and x86_64 assembly enabled. The configuration reported
GCC 16.1.0, `-O2`, `USE_ASM_X86_64=1`, and completed the library build.

The final disposable matrix harness was
`/tmp/secp256k1-goal53-schnorr/schnorr_timing_matrix.c`, SHA-256
`f7e3be25bd25d46afe8a00c42de7a60c5e24ef99d944ae666faa226e21b966fc`. It
created 32 valid deterministic keypairs in each class, with the first byte
MSB clear or set, kept the 32-byte message and 32-byte auxiliary input fixed,
pinned to the first available CPU, randomized key selection and class order,
and measured 60,000 samples per class. Signature bytes were folded into a
digest after each call so the operation and output remained observable.

Clang 22.1.7 matrix runs:

```text
class=msb-clear-random n=60000 mean=132388.999 median=132010 p95=132266 min=131634 max=309372
class=msb-set-random n=60000 mean=132394.535 median=132012 p95=132268 min=131636 max=315128
cpu=0 keysets=32 welch_t=-0.402522 mean_delta=-5.536 digest=d66d86584fdffd74

class=msb-clear-random n=60000 mean=132441.403 median=132058 p95=132304 min=131058 max=315840
class=msb-set-random n=60000 mean=132442.830 median=132062 p95=132304 min=131138 max=349078
cpu=0 keysets=32 welch_t=-0.100478 mean_delta=-1.427 digest=d66d86584fdffd74

class=msb-clear-random n=60000 mean=133258.156 median=132090 p95=132396 min=130946 max=4240240
class=msb-set-random n=60000 mean=133169.947 median=132094 p95=132400 min=130938 max=320934
cpu=0 keysets=32 welch_t=1.009761 mean_delta=88.209 digest=d66d86584fdffd74
```

GCC 16.1.0 matrix runs:

```text
class=msb-clear-random n=60000 mean=133116.250 median=132064 p95=132384 min=130958 max=344786
class=msb-set-random n=60000 mean=133083.893 median=132088 p95=132378 min=130902 max=288794
cpu=0 keysets=32 welch_t=0.619772 mean_delta=32.357 digest=d66d86584fdffd74

class=msb-clear-random n=60000 mean=133117.741 median=132076 p95=132528 min=130874 max=363126
class=msb-set-random n=60000 mean=133180.757 median=132110 p95=132642 min=130912 max=355616
cpu=0 keysets=32 welch_t=-1.173609 mean_delta=-63.016 digest=d66d86584fdffd74
```

The exploratory fixed-key `1` versus `n-1` run was deliberately not used as
the verdict: its three Clang statistics were `1.036114`, `-2.160415`, and
`-2.477615`, showing why single-key classes are too confounded by exact-key
effects.

### Assembly and verdict

`objdump` on the same release library showed
`secp256k1_schnorrsig_sign32` calling `secp256k1_schnorrsig_sign_internal`,
which loads the scalar and calls `secp256k1_ecmult_gen_gej`. The latter has
fixed loop bounds visible as comparisons against `0x100`, `0x6`, and `0x20`;
table selection uses `sete` and SIMD masks rather than a secret-indexed branch.
The remaining conditional branches in the signing wrapper are argument/key
validation, nonce-result validity, and fixed loop control. The ctime evidence
from cycle 62 independently covers secret-defined memory behavior.

**Dismissed: no stable Schnorr timing signal or clean-master defect.** The
randomized classes stayed within roughly 0-64 cycles at median/p95, while
scheduler outliers reached millions of cycles; Welch statistics changed sign
across repeated compiler runs and stayed near zero. No source change is
justified. Existing functional tests missed this because they assert signing
correctness rather than timing distributions; ctime and assembly are the
stronger non-statistical controls available here.

Master-relative severity is none. This is x86_64 release evidence only; it is
not a proof for other compilers, architectures, microarchitectures, or all
secret classes. Reopen after a signing-path/compiler/backend change, or with a
noise-controlled dudect setup that produces a reproducible effect and a
matching secret-dependent mechanism. Scratch build and harness paths are
disposable. The next queue is
`53,72,74,77,81,82,84,87,89,95,97`, excluding this exact randomized Schnorr
signing cell and the prior cycle-62 cells.
