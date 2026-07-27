# Reusable Continuous Agent Goals

This is the local operational catalog for the 99 reusable goals supplied by
the maintainer. The original supplied document is the source specification;
this file preserves its numbered identities, slugs, titles, size metadata,
and distinct campaign scopes while keeping the shared run protocol in one
place. The rotating controller is
[`uber-goal.md`](./uber-goal.md), and its durable state is
[`uber-goal-state.md`](./uber-goal-state.md).

## Shared protocol

Every selected goal is a finite evidence-backed cycle, not a declaration that
the repository is complete. Begin on a fresh branch or the existing audit
branch after recording base, HEAD, remotes, dirty state, and running jobs.
Read this catalog, the state file, the goal-specific journal, existing
findings, relevant history, issues/PRs, and prior verification before choosing
a hypothesis. Re-rank the remaining queue using current evidence and choose a
distinct unexplored surface.

For each candidate, record the hypothesis, trust boundary, caller path,
contract/invariant, source and history evidence, exact command and key output,
and a verdict of confirmed, dismissed, or inconclusive. Search semantic
duplicates before reporting. External reports, branches, tools, and
alternative implementations are discovery seeds, not proof. Use the narrowest
reproducer and independent verification when practical.

A confirmed finding needs the strongest available proof: failing-before and
passing-after behavior, a minimized fuzz seed or fixture, a first-invalid
sanitizer/static trace, a mutation or coverage delta, a reproducible benchmark
or profile table, a build-matrix result, or a rigorous bounded proof. Fix one
root cause per buildable commit, with a deterministic regression test whenever
reproducible. Commit notes must state mechanism, reachability, master-relative
severity, Bitcoin Core caller impact, exact seed/source and commands, why
existing tests missed it, repair ordering/masking effects, limitations, and
handoff.

For Bitcoin Core-facing claims, rate the actual master branch and actual
callers. High/Critical requires proof of invalid-block or invalid-witness
acceptance, consensus divergence, key/funds/privacy loss, or a comparable
remote primitive. A witness-sigop issue is High/Critical only with a minimized
proof that an invalid block is accepted. A nonce or retry counter without
standalone cryptographic meaning is not Critical merely because it is not
cleared. Unsupported aliasing or malformed opaque objects are not findings
without a documented contract and reachable caller. Do not use a later fix to
hide a master finding; isolate clean-master, mutation, and repaired stages and
explain their order.

Keep scratch data, wallets, keys, databases, and fault injections isolated.
Do not leave fuzz, sanitizer, daemon, or profiling jobs running. If there is
no confirmed finding, commit at most a focused journal/evidence snapshot.
After each cycle update the goal journal and state, mark the chosen goal
exhausted only when its current hypothesis space is genuinely searched or
blocked, then select another pending goal. Never manufacture a commit merely
to show activity.

## Catalog

The `prompt-chars` values are the supplied character counts. Each entry is a
standalone `/goal` when expanded with the shared protocol above.

| # | Slug | Title | Prompt chars | Distinct campaign scope |
|---:|---|---|---:|---|
| 0 | `continuous-bug-mining` | Continuous evidence-first bug mining | 3597 | Rotate current diff, history, coverage, mutations, sanitizers, static reports, profiles, platform matrices, advisories, alternative implementations, and contributor branches; find few definitive bugs and leave exact handoffs. |
| 1 | `comment-code-contract` | Source comment versus implementation contract audit | 3495 | Compare nontrivial comments with code, callers, tests, docs, blame, and rationale, especially lifetime, locks, serialization, compatibility, secrets, and bounds. |
| 2 | `assertion-invariant-audit` | Assertion, Assume, and invariant reachability audit | 3492 | Challenge assertions, VERIFY/CHECK assumptions, release-only validation, and impossible-state claims across untrusted, fuzz, optional, and persisted paths. |
| 3 | `current-pr-leftovers` | Current branch and PR leftover sweep | 3482 | Find partial migrations, stale names/comments/tests, omitted optional/build/generated files, duplicate old logic, and unresolved review objections in the current stack. |
| 4 | `public-interface-contracts` | Public API, CLI, RPC, config, and help contract audit | 3426 | Trace public values from registration and parsing through defaults, runtime, persistence, restart, errors, help, docs, and observation. |
| 5 | `boundary-off-by-one` | Boundary-condition and off-by-one audit | 3464 | Prove zero/one/max, first/last, empty/full, signed sentinels, widths, offsets, limits, scalar/field bounds, and `<` versus `<=` behavior. |
| 6 | `serialization-untrusted-input` | Serialization, deserialization, and untrusted-input sweep | 3443 | Audit parsers and encoders for allocation/CPU bounds, truncation, canonical encodings, duplicate forms, partial mutation, and parse-then-assume paths. |
| 7 | `resource-exhaustion-variants` | Untrusted-interface resource-exhaustion variant analysis | 3473 | Mine DoS shapes such as unbounded work, retry storms, cache bypass, queue growth, compaction amplification, and accounting failures with explicit resource bounds. |
| 8 | `locking-threading` | Locking, threading, and scheduler audit | 3419 | Map locks, atomics, callbacks, queues, shutdown, lock order, races, cancellation, and destruction; use TSan and deterministic barriers rather than sleeps. |
| 9 | `hit-frequency-coverage` | Hit-frequency and suspicious-branch coverage audit | 3385 | Rank rare high-risk branches by behavior and trust boundary, explain why they are rare, and add execution-plus-property oracles only where justified. |
| 10 | `fuzz-target-gaps` | Fuzz-target gap and harness-realism audit | 3473 | Compare production entry points with targets, remove unrealistic constraints and silent catches, and add structured input and strong state oracles. |
| 11 | `sanitizer-valgrind` | Sanitizer and Valgrind true-positive sweep | 3381 | Run ASan/UBSan/TSan/MSan/LSan/Valgrind and relevant workloads; minimize first-invalid operations and distinguish project, tool, dependency, and suppression issues. |
| 12 | `static-analysis-true-positives` | Static-analysis true-positive campaign | 3476 | Use compiler warnings, clang-tidy/analyzer, CodeQL/Semgrep, IWYU, and semantic queries for lifetime, nullability, overflow, unchecked results, locks, and dead stores. |
| 13 | `secret-lifetime-zeroization` | Secret-data lifetime and zeroization audit | 3421 | Trace keys, nonces, seeds, tweaks, passphrases, and secret temporaries through copies, callbacks, errors, logs, destructors, and optimized clearing. |
| 14 | `secret-control-flow` | Secret-dependent control-flow and memory-access audit | 3412 | Track secret taint into branches, loops, table indexes, addresses, helper selection, and failure exits; use ctime, ctgrind, dudect, and assembly evidence. |
| 15 | `public-object-validation` | Public object parsing and validation variant analysis | 3405 | Compare key, x-only key, signature, scalar, script, record, and API parsing for malformed, noncanonical, infinity, duplicate, and output-on-failure behavior. |
| 16 | `api-misuse-resistance` | Public API misuse-resistance audit | 3450 | Attack ownership, lifetime, aliasing, context capabilities, callback obligations, invalidation, thread safety, secret/public status, and examples from an adversarial caller view. |
| 17 | `build-matrix-modules` | Build-matrix and module-configuration audit | 3393 | Enumerate compiler, module, assembly, debug/VERIFY, exhaustive, sanitizer, static/shared, and cross combinations; find uncompiled or untested high-risk interactions. |
| 18 | `exhaustive-algebraic` | Exhaustive and algebraic-invariant audit | 3411 | Test parse/serialize, arithmetic, inverse, normalization, tweak, sign/verify, cache, and state identities over exhaustive small domains and independent references. |
| 19 | `benchmark-integrity` | Benchmark correctness and measurement-integrity audit | 3386 | Verify benchmark setup, timed region, result use, cache state, inputs, units, and claimed operation; require repeated raw samples and mutation sensitivity. |
| 20 | `micro-optimization` | Simple micro-optimization discovery and proof | 3392 | Select one measured hot operation, propose a narrow helper/allocation/hash/branch/lookup improvement, and require interleaved release measurements plus correctness proof. |
| 21 | `rebuild-recovery-profile` | Long-running rebuild, recovery, and compaction profiling | 3389 | Profile fixed rebuild, reindex, recovery, snapshot, or compaction phases by CPU, RSS, I/O, locks, allocator, and database behavior before changing code. |
| 22 | `full-sync-ibd-profile` | Full sync, IBD, import, and end-to-end profiling | 3390 | Run controlled local IBD/import/reindex workloads and separate download, validation, script/crypto, chainstate, I/O, compaction, and logging costs. |
| 23 | `perf-flamegraph-investigation` | Perf and flamegraph investigation without forced commits | 3381 | Use perf/flamegraphs, counters, lock and scheduler views to rank bottlenecks; leave a detailed journal when no measured safe fix is proven. |
| 24 | `disk-io-amplification` | Disk I/O, persistence growth, and write-amplification audit | 3403 | Measure reads, writes, fsyncs, compaction, logs, temp files, and persistent growth; check durable authority and crash consistency before reducing work. |
| 25 | `performance-regression-bisect` | Recent performance-regression bisect | 3417 | Bisect stable workloads with interleaved measurements, matching profiles and data, then preserve correctness intent with the smallest causal repair. |
| 26 | `cross-subsystem-bug-shapes` | Bug fixed in one subsystem but present in another | 3364 | Mine historical fixes for structural bug shapes and search analogous parsers, caches, indexes, queues, APIs, and modules for independent reachable variants. |
| 27 | `error-path-state` | Error-path partial-state mutation audit | 3359 | Enumerate every failure edge and caller-visible mutation; inject early/late failures and verify unchanged, zeroed, invalidated, rolled-back, or documented partial state. |
| 28 | `weak-test-oracles` | Weak-test oracle and mutation-survival audit | 3376 | Find tests that assert only success/no-crash/logs, apply targeted mutants, and add the smallest postcondition that kills valuable non-equivalent survivors. |
| 29 | `dead-stale-code` | Dead code, stale feature, and TODO archaeology | 3433 | Audit uncalled code, dormant macros, obsolete compatibility paths, TODOs, disabled tests, and stale docs across all supported builds and history. |
| 30 | `security-logging` | Security-sensitive and misleading logging audit | 3390 | Trace secrets, paths, peer identifiers, wallet data, attacker strings, severity, rate, escaping, and inaccurate diagnostics into logs and errors. |
| 31 | `cross-layer-contracts` | Cross-layer docs, examples, tests, and implementation audit | 3378 | Build contract tables across source, public docs, examples, schemas, tests, help, release notes, and implementation; fix only proven contradictions. |
| 32 | `whole-history-leftovers` | Whole-history incomplete-fix and migration mining | 3360 | Walk history in ranges for security, parser, persistence, crypto, locking, resource, and migration fixes; search current HEAD for surviving old shapes. |
| 33 | `external-vulnerability-variants` | External vulnerability and advisory variant analysis | 3405 | Convert CVEs, advisories, OSS-Fuzz issues, and related-project bugs into local structural queries and reachable minimal vectors. |
| 34 | `uncovered-code-classification` | Uncovered-code classification and closure audit | 3376 | Classify uncovered code as platform, hard error, missing scenario, harness artifact, dead, or bug-blocked; close only behaviorally important gaps. |
| 35 | `mutation-testing` | Mutation-testing campaign | 3452 | Run condition, call, state-write, arithmetic, bound, return, and cleanup mutants; classify survivors and strengthen dangerous oracles. |
| 36 | `cross-tool-analysis-matrix` | Cross-tool sanitizer and static-analysis matrix | 3383 | Cross GCC/Clang, ASan/UBSan/TSan/MSan/LSan/Valgrind, hardening, and static-analysis cells; isolate configuration-specific defects and suppressions. |
| 37 | `build-dead-zones` | Build dead-zone and conditional-compilation audit | 3373 | Map every feature/platform/compiler guard and CI exclusion to true/false builds; detect polarity, declaration, source-list, and test omissions. |
| 38 | `failure-cleanup-crash-safety` | Failure cleanup and crash-safety audit | 3385 | Model resources, locks, files, durable commit points, rollback, retry, startup recovery, and abrupt termination under deterministic failure injection. |
| 39 | `generated-artifact-determinism` | Generated-artifact and test-vector determinism audit | 3336 | Identify generators, pinned tools, vectors, schemas, tables, and artifacts; regenerate under clean locale/timezone/toolchains and explain every diff. |
| 40 | `multi-agent-adjudication` | Independent multi-agent disagreement and adjudication audit | 3418 | Separate scout, verifier, fixer, and reviewer roles; record disagreements and prevent patches from biasing truth judgments. |
| 41 | `history-seed-archaeology` | History archaeology from a seed topic | 3383 | Pick one topic, reconstruct its evolution with log/blame/PRs, and search current code/tests/docs for old assumptions and unfinished implications. |
| 42 | `ci-review-bot-followup` | CI, coverage-bot, and review-bot follow-up audit | 3387 | Collect live CI, sanitizer, fuzz, static, coverage, flaky, and review-bot results; reproduce and classify stale, infrastructure, tool, test, or project defects. |
| 43 | `option-api-lifecycle` | Option and API lifecycle audit | 3407 | Follow options/APIs through registration, storage, scheduling, runtime, persistence, restart, migration, disablement, and removal, including periodic triggers. |
| 44 | `secret-copy-optimization` | Secret-copy and compiler-optimization audit | 3377 | Trace secret-bearing aggregate copies, ABI spills, captures, moves, swaps, and optimized clears across compilers, LTO, architectures, and checkmem. |
| 45 | `constant-time-declassification` | Constant-time boundary and declassification audit | 3379 | Map secret/public/declassified dataflow and challenge variable-time helpers, logs, errors, branches, tables, and explicit declassification boundaries. |
| 46 | `api-output-on-failure` | Public API output-on-failure audit | 3411 | Inventory every exported output mutation and failure convention; prefill sentinels and test malformed inputs, aliasing, callbacks, contexts, and invalid objects. |
| 47 | `build-ci-parity` | Build-system and CI parity audit | 3357 | Compare CMake, alternate build systems, presets, install/export manifests, examples, benches, fuzzers, exhaustive/ctime tests, and CI flags. |
| 48 | `property-oracle-expansion` | Property, exhaustive, and algebraic oracle expansion | 3390 | Strengthen tests with identities, inverse/replay, idempotence, canonicalization, monotonicity, ordering, recomputation, and no-mutation properties. |
| 49 | `critical-history-sweep` | Critical whole-history must-fix sweep | 3381 | Progress initial commit to HEAD for reachable UB, memory, DoS, consensus, funds/key/privacy, parser, race, secret, and critical-check defect shapes. |
| 50 | `fuzz-introspector-blockers` | Fuzz Introspector blocker and complexity audit | 3367 | Compare static call-tree complexity with dynamic coverage and repair high-risk blockers through dictionaries, structured input, mutators, or new targets. |
| 51 | `differential-metamorphic` | Invariant, differential, and metamorphic audit | 3385 | Compare fast/reference, old/new, batch/split, apply/revert, incremental/recompute, parser/serializer, and alternative implementations over defined domains. |
| 52 | `integer-arithmetic-audit` | Integer overflow, narrowing, signedness, and division audit | 3423 | Trace attacker/state integers through arithmetic, casts, shifts, divisions, allocations, indexes, time, resource accounting, and serialization on 32/64-bit widths. |
| 53 | `timing-side-channel` | Statistical timing-side-channel campaign | 3383 | Select secret-bearing primitives, use randomized dudect-style classes and assembly/dataflow analysis, and treat statistics as evidence requiring mechanism proof. |
| 54 | `raii-resource-leaks` | RAII, smart-pointer, and resource-leak audit | 3389 | Map ownership and destruction of objects, handles, locks, threads, callbacks, iterators, snapshots, and custom deleters under failure, cancellation, and restart. |
| 55 | `alternative-implementation-diff` | Alternative-implementation compatibility-difference audit | 3409 | Mine btcd, libbitcoin, rust-bitcoin, libwally, crypto libraries, databases, and forks for shared-contract vectors and local variants. |
| 56 | `stale-pr-resurrection` | Stale PR critical-fix resurrection audit | 3398 | Revisit closed/abandoned/draft PRs and issues for credible consensus, funds, DoS, wallet/key, crypto, data, recovery, and unmerged regression evidence. |
| 57 | `local-reasoning-contracts` | Local-reasoning domain and relationship audit | 3446 | Write legal domains, ownership, locks, postconditions, invalidation, persistence authority, and state relationships for one high-risk function/module at a time. |
| 58 | `helper-reuse` | Exact helper reuse and minimal helper-extension audit | 3424 | Prove exact equivalence before reusing or minimally extending helpers, including errors, side effects, ownership, diagnostics, ordering, and tests. |
| 59 | `supply-chain-security-gates` | C/C++ supply-chain and security-gate audit | 3432 | Audit vendored code, dependencies, hashes, URLs, toolchain/container pins, CI actions, generated files, release signing, provenance, and workflow permissions. |
| 60 | `reviewer-preference-skill` | Historical reviewer-preference mining and reusable review skill | 3401 | Mine merged/closed/contentious PRs for durable review rules, evidence expectations, commit-stack style, and counterexamples; validate on held-out PRs. |
| 61 | `stateful-contract-fuzzing` | Stateful contract-fuzzer expansion | 3421 | Upgrade one target to a production-like state machine with pre/postconditions, inverse/replay, idempotence, ordering, accounting, failure cleanup, and realistic sequences. |
| 62 | `rejected-finding-resurrection` | Rejected-finding resurrection and assumption attack | 3439 | Attack prior dismissals by widening inputs, caller paths, schedules, configurations, and historical states; confirm or overturn assumptions without artificial reachability. |
| 63 | `loupe-style-pipeline` | Loupe-style scout, verifier, fixer, and reporter pipeline | 3557 | Enforce scout/independent verifier/minimal fixer/final reviewer stages with leases, provenance, semantic/hash deduplication, preserved PoCs, and journal transitions. |
| 64 | `finding-dedup-recurrence` | Finding deduplication, recurrence, and semantic-fingerprint audit | 3441 | Index findings by code path, trust boundary, bug shape, source/sink, version, reproducer hash, and semantic summary; detect duplicates and recurrences. |
| 65 | `contributor-branch-radar` | Contributor-branch and work-in-progress radar | 3463 | Refresh public contributor branches/forks, bases, divergence, touched contracts, tests, review concerns, migrations, useful seeds, and conflicts without copying unpublished work. |
| 66 | `backport-correctness` | Cherry-pick, backport, and release-branch correctness audit | 3429 | Compare patch-id/range-diff/semantics across maintenance branches for missing prerequisites, conflict errors, generated drift, guards, and regression tests. |
| 67 | `release-version-differential` | Release-to-release behavioral and consensus differential | 3396 | Feed identical blocks, transactions, scripts, RPC/config cases, wallets, databases, indexes, and transcripts across releases; classify expected versus undocumented drift. |
| 68 | `architecture-abi-parity` | Architecture, endianness, word-size, and ABI parity audit | 3388 | Run and compare supported architectures, widths, alignment, endianness, atomics, filesystem/socket/time APIs, serialized outputs, and platform-only skips. |
| 69 | `backend-differential` | SIMD, assembly, and portable-reference backend differential | 3389 | Force portable, SIMD, assembly, hardware, and reference backends and compare outputs, errors, aliasing, state mutation, constant-time behavior, and dispatch. |
| 70 | `compiler-optimization-differential` | Compiler, optimization, LTO, PGO, and BOLT differential | 3423 | Compare GCC/Clang optimization/LTO/PGO/BOLT builds for UB, miscompiles, barriers, constant-time behavior, profile stability, correctness, and size/performance tradeoffs. |
| 71 | `deterministic-simulation` | Deterministic simulation and failure-schedule exploration | 3437 | Build seeded schedules for time, randomness, network/disk outcomes, task order, retries, and shutdown with final-state, progress, durability, and resource invariants. |
| 72 | `filesystem-crash-consistency` | Filesystem, power-loss, and crash-consistency injection | 3413 | Inject short/corrupt/dropped writes, ENOSPC/EIO, truncation, reorder assumptions, permissions, and kills around durable boundaries on scratch devices. |
| 73 | `network-state-machine` | Network fragmentation, reordering, and partial-I/O state-machine audit | 3386 | Exercise fragmented/coalesced reads, short writes, EOF, duplicates, reconnects, half-close, backpressure, stalls, and framing/handshake state transitions. |
| 74 | `memory-pressure-allocator` | Memory pressure, OOM, allocator, and fragmentation audit | 3413 | Profile heap/RSS/fragmentation/caches and inject allocation failure or realistic memory limits across sync, mempool, wallet, RPC, fuzz, and recovery workloads. |
| 75 | `build-throughput-cacheability` | Build throughput, dependency graph, and container-cache audit | 3439 | Measure clean/incremental/no-op/parallel builds, compiler/header/linker/custom-command costs, Docker/CI cache keys, and stale-result risks. |
| 76 | `reproducible-builds` | Reproducible binaries, Guix, and toolchain-provenance audit | 3393 | Rebuild release artifacts in clean pinned environments, compare hashes/diffs, trace timestamps/paths/locale/toolchain/generated/signing differences, and verify provenance. |
| 77 | `symbolic-model-checking` | Symbolic execution and bounded-model-checking campaign | 3389 | Use CBMC/KLEE on bounded arithmetic, parsers, codecs, cache transitions, queues, crypto helpers, and cleanup with explicit production-domain assumptions. |
| 78 | `translation-validation` | Compiler-transformation validation and miscompile isolation | 3441 | Compare LLVM IR/translation with Alive2 where possible, reduce suspicious arithmetic/alias/shift/constant-time behavior, and separate source UB from compiler bugs. |
| 79 | `fuzz-corpus-stewardship` | Fuzz-corpus stewardship, minimization, and transfer audit | 3388 | Inventory, minimize, deduplicate, transfer, and replay fuzz corpora with coverage/runtime/provenance, qa-assets/bitcoinfuzz inputs, and old crashers. |
| 80 | `fuzz-engine-differential` | Fuzz-engine and property-framework differential | 3399 | Compare libFuzzer, AFL++, Honggfuzz, and FuzzTest with equivalent semantics, seeds, budgets, coverage growth, crash classes, and cross-engine reproduction. |
| 81 | `spec-vector-drift` | Specification, test-vector, and formal-model drift audit | 3489 | Map BIPs, protocol docs, module docs, Sage, Wycheproof, vectors, comments, and implementation; regenerate and resolve traceable rule drift. |
| 82 | `secp-field-scalar-matrix` | secp256k1 field and scalar representation matrix | 3414 | Compare 5x52/10x26, 4x64/8x32, VERIFY/exhaustive, inversion, compiler, and architecture paths across magnitude, carry, aliasing, and serialization boundaries. |
| 83 | `secp-group-ecmult` | secp256k1 group, ecmult, and formula-parity audit | 3395 | Compare affine/Jacobian, infinity, formulas, wNAF/windows, generator/arbitrary multiplication, endomorphism, batch inversion, and exhaustive/reference relations. |
| 84 | `secp-nonce-session` | secp256k1 nonce, signing, Schnorr, and MuSig state-machine audit | 3398 | Model ECDSA/Schnorr/ECDH/extrakeys/MuSig signing state, nonce reuse, binding, duplicate/cancelled participants, malformed codecs, context, and failure cleanup. |
| 85 | `bitcoin-consensus-mutation` | Bitcoin consensus mutation-score and kill-test audit | 3409 | Mutate consensus/script/sighash/activation/limits/cache-key checks and require tests or fuzzers to kill reachable non-equivalent mutants. |
| 86 | `bitcoin-chainstate-symmetry` | Bitcoin chainstate, reorg, prune, and index crash-symmetry audit | 3402 | Model connect/disconnect, reorg, flush, restart, prune, snapshots, block/undo files, and indexes with interrupted writes and replay symmetry. |
| 87 | `bitcoin-mempool-accounting` | Bitcoin mempool, package, and eviction-accounting audit | 3408 | Fuzz package/RBF/conflict/expiry/trim/reorg sequences and compare graph, fee, ancestor/descendant, eviction, ordering, and resource accounting with recomputation. |
| 88 | `bitcoin-wallet-recovery` | Bitcoin wallet encryption, backup, descriptor, and keypool recovery audit | 3417 | Inject database faults/crashes through encryption, descriptors, keypool, migration, backup/restore, rescan, and external signer lifecycle and verify durable key state. |
| 89 | `bitcoin-p2p-accounting` | Bitcoin P2P transport, permission, and peer-accounting audit | 3436 | Model peer/transport handshake, permissions, quotas, download state, queues, bytes, timeouts, disconnect, fragmentation, reconnect, and shutdown races. |
| 90 | `historical-knowledge-recipes` | Whole-PR and commit knowledge-base recipe synthesis | 3407 | Extract reusable technical recipes from every commit/PR: invariant, bug shape, rejected design, benchmark, fixture, platform caveat, migration, and follow-up. |
| 91 | `compiler-binary-hardening` | Compiler and binary-hardening configuration audit | 3451 | Check warning policy, stack/heap/linker/CFI/PIE/RELRO/FORTIFY/hardening flags and final binaries; require a concrete blocked defect or gained diagnostic. |
| 92 | `abi-alignment-aliasing` | ABI layout, alignment, aliasing, and object-lifetime audit | 3367 | Audit packed/union/reinterpret/placement/memcpy/layout/lifetime/overalignment/strict-aliasing and C/C++ ABI boundaries across compilers and architectures. |
| 93 | `system-fault-injection` | Allocation, syscall, clock, randomness, and callback fault injection | 3395 | Fail Nth allocations, opens, I/O, fsync/rename, sockets, threads, entropy, clocks, schedulers, database calls, and callbacks; verify rollback and retry. |
| 94 | `bindings-ffi-parity` | Bindings, FFI, and language-wrapper parity audit | 3389 | Compare C APIs with Rust/Python/Java/Go/C#/JNI and other bindings for widths, ownership, nullability, callbacks, exceptions, threads, buffers, secrets, and features. |
| 95 | `database-semantics-differential` | Database-engine and persistence-semantics differential | 3458 | Compare LevelDB assumptions with RocksDB/Pebble/alternatives for comparator, snapshots, iterators, batches, WAL/MANIFEST, checksums, filters, compaction, and corruption. |
| 96 | `todo-deferred-work` | TODO, FIXME, stub, and deferred-work challenge audit | 3394 | Turn TODOs, disabled tests, expected failures, stubs, compatibility code, and deferred work into current falsifiable questions, then fix only concrete defects. |
| 97 | `cpp-defect-taxonomy` | C and C++ defect-taxonomy sweep | 3482 | Cycle through null, UB, lifetime, bounds, arithmetic, aliasing, race, deadlock, cleanup, recursion, format, and unchecked-result classes with a subsystem grid. |
| 98 | `float-sanitizer-fuzz-exclusions` | Floating-point edge values, sanitizer resurrection, and fuzz-exclusion audit | 3863 | Exercise float edge values, recover suppressed sanitizer cells, and challenge fuzzer catches/early returns/clamps with production-boundary proof and safe failure assertions. |

## Status rules

The state file is authoritative for `pending`, `active`, `exhausted`,
`blocked`, `confirmed`, and `dismissed` work. A goal is not exhausted because
one corpus passed or because no obvious issue was found. It is exhausted only
after its stated evidence sources and highest-risk hypotheses have been
checked, documented, and either proven or explicitly blocked. A new upstream
commit, branch, tool, caller, or relevant finding reopens affected goals.
