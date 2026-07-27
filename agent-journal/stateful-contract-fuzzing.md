# Stateful Contract-Fuzzer Expansion

## Selection

- Catalog goal: `61`
- Draw seed: `4179223777703642971`
- Draw: `61`
- Selected on: `2026-07-27`
- Branch at selection: `codex/fuzz-oracles`
- HEAD at selection: `ebc658d0`
- Base at selection: `origin/master=0f6baf319fcae0d7f11a44fc9b4d4899b3f8082a`
- Status: exhausted for this bounded cycle

## Cycle hypothesis

Choose one stateful optional-module target whose existing oracle checks
individual operations but may not check a legal operation after an earlier
failure, invalidation, aliasing transition, or successful retry. Search the
current source, corpus names, production implementation, and existing
README ledger before changing code. Do not repeat the already documented
MuSig nonce/session, group alias, ecmult callback, API output, ECDH, or Silent
Payments state ladders.

## Evidence log

### 2026-07-27 bounded state-transition replay

The selected transition question was whether the existing Schnorrsig,
recoverable-ECDSA, and x-only/keypair state ladders missed a meaningful public
operation after an earlier failure, invalidation, aliasing transition, or
successful retry. The prior README ledger already covered MuSig nonce/session,
group alias, ecmult callback, API output, ECDH, and Silent Payments ladders, so
this cycle did not duplicate those surfaces.

The current fuzzer sources were:

    src/fuzz/schnorrsig.c      baa3a9b4ac5d98476fe8a4a3afed95737cb75107f8b9523913b95e897e7c34f2
    src/fuzz/recovery.c        d75f5bcdc726f6bae87138b0425d8aa26275d9a098a5da39f3d0143652267362
    src/fuzz/xonly_tweak.c     0ea640e205c208e60a728fc154e84163261622034dee9dec66fb78e2eef55f05

The production implementation hashes were:

    src/modules/schnorrsig/main_impl.h  67db2245d5fad2cfbfc910d9fda9187a6d26358c6988dde7e828b8c2cccf2897
    src/modules/recovery/main_impl.h    4ae9cb6b7f0c4e322d5ed1c278992f8de8ff8e84074e78c567b64bad83bd0b70
    src/modules/extrakeys/main_impl.h   6233eddd4539c3adbaa1bc4313ae67910a4df43a0e1f3141d83752eb02a73e5a

Clang ASan/UBSan Debug builds were rebuilt from this worktree for native and
forced-int64 arithmetic. A private-copy, one-input-at-a-time replay used
`-runs=1 -seed=61 -timeout=20 -rss_limit_mb=0` over 18 Schnorrsig, 17
recovery, and 20 x-only corpus files. Every input returned zero on both
backends. The status-log hashes were identical by backend:

    schnorrsig  576d1c288489227ece6705fe66557c0b46d8a11e674578e1d472f97bea2b33c2
    recovery    32775037b4ec6ff58845096916ed25fc655d54818ce9f374a2a351aeb9570744
    xonly       564f5ec405cee0f0abcb1abc083b5135b933459f77b29885763c2bf53208285

The native binary hashes were:

    fuzz_schnorrsig  4c66fb89fb50168522aee43d58859a05236ddbb9e38442af127311cb7bfa86bc
    fuzz_recovery    0c94ed32d6b888893bd121431397d903a5fc807b892d8897ac8717f8a43a2521
    fuzz_xonly_tweak e502216a26220122688a86a01c8a78b0b03e396e6092dfc17aed418c8db3d760

The forced-int64 binary hashes were:

    fuzz_schnorrsig  573e39bfd986e60669be182f1f2508cc76e77514f37336646098ba8829bfb8df
    fuzz_recovery    57218ddf5d04f0249c225cb767a57d7e8fd01f0eded6d702455889cc04adc6e7
    fuzz_xonly_tweak e6eed0a39d0a4cc18d415bf034b45657e3e52cc4627f3d5860160fe80132e02d

The corpus manifest hashes were `772a4de5d3672e4e2395e6940ca019f13c729c6820fbb46499d6615f784ff671`
(Schnorrsig), `297b3aaa5120c86142fb822f29d5ba2ae5bdc748f07c1afa7fc604c3ef1450a2`
(recovery), and `c9b31913ee7674853c05dbad504a6c4af9ebb52baf073c199d7f1eda98298dfc`
(x-only). Native and forced-int64 fuzzer binary hashes differed as expected;
the six hashes were recorded in the cycle command transcript under
`/tmp/secp256k1-stateful-cycle-61`.

A short mutation-guided run used private copies and `-seed=61
-max_total_time=4 -timeout=30 -rss_limit_mb=0`. The target build command was
`cmake --build /tmp/secp256k1-oracles-next-build-{native,int64} --target
fuzz_schnorrsig fuzz_recovery fuzz_xonly_tweak -j2`. Native executed 38, 70, and
21 units for Schnorrsig, recovery, and x-only; forced-int64 executed 22, 41,
and 21. All six runs returned zero, reported no timeout/OOM/crash, produced
zero artifacts, and left no fuzzer process. Run-log hashes were retained in
the same scratch directory.

This is a negative result, not a production finding: no new state transition
or missing oracle was demonstrated, no source or corpus change is justified,
and no independent fix commit is made. The evidence is limited to the three
targets and current tracked corpus; it does not prove arbitrary future
sequences or replace the existing MuSig/ECDH/Silent Payments evidence. The
goal-specific state space is exhausted for this cycle, so the uber-goal
controller rotates to another campaign.

## Handoff

If source, callers, tools, or findings change, reopen this goal with a new
transition and exact missing evidence. The controller has drawn another
pending goal using the uber-goal rules.
