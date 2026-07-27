# Uber Goal: Rotating Evidence-First Campaigns

```text
/goal
Run a continuing, evidence-first investigation using the 99-goal catalog in
agent-journal/reusable-continuous-agent-goals.md. This is an orchestration
goal, not repository completion: keep working until a real session/tool limit
or external blocker, and leave a precise handoff.

The authoritative state is agent-journal/uber-goal-state.md. The per-goal
journal is agent-journal/<slug>.md. Before every cycle, inspect the current
worktree, branch, base/HEAD, remotes, dirty state, running jobs, state ledger,
catalog, existing findings, prior journals, relevant history, issues, PRs,
and review precedent. Never overwrite unrelated user work. Use a fresh branch
or the existing audit branch only after recording its exact base.

Choose a goal randomly from the pending, reopened, or highest-risk eligible
set. Use a recorded random seed/draw (for example, shuf or a small shell
selector), exclude goals already active in this cycle, and prefer high-risk
unverified cells when equal random candidates are available. Do real work for
one bounded cycle: formulate a distinct falsifiable hypothesis, inspect code
and callers, run the narrowest useful experiment, and continue through
verification or a clearly recorded blocker. Do not stop at a plan, repeat a
passing corpus campaign, or manufacture a commit. When the selected goal's
current hypothesis space is genuinely exhausted, mark it exhausted with the
evidence and draw another goal. Reopen it when new code, callers, tools, or
findings change its assumptions.

For every candidate, record the trust boundary, contract, source/history
evidence, exact commands and key output, and a verdict: confirmed, dismissed,
or inconclusive. Search semantic and hash duplicates before reporting. Keep
scout, verification, fixing, and reporting independent where practical.
External branches, advisories, alternative implementations, sanitizers, and
tools are seeds, not proof. Require a failing-before/passing-after test,
minimized fixture/fuzz seed, first-invalid sanitizer/static trace, mutation or
coverage delta, reproducible profile table, build-matrix result, or rigorous
bounded proof. For confirmed defects, make one smallest buildable commit with
a deterministic regression test when reproducible. If there is no confirmed
defect, commit at most one focused journal/evidence snapshot.

Every finding note and commit message must explain mechanism, reachability,
master-relative severity, actual Bitcoin Core callers, exact seed/source and
commands, why existing tests missed it, repair ordering or masking effects,
limitations, and handoff. High/Critical requires proof of invalid-block or
invalid-witness acceptance, consensus divergence, key/funds/privacy loss, or
a comparable remote primitive. A witness-sigop claim requires a minimized
invalid-block acceptance proof. A nonce or retry counter without standalone
cryptographic meaning is not Critical merely because it is uncleared. Do not
call unsupported aliasing or malformed opaque state a bug without a contract
and reachable caller.

Use isolated scratch data, wallets, keys, databases, and fault injection. Run
deterministic then sanitizer/release/multi-worker verification as appropriate;
do not leave fuzz, sanitizer, daemon, or profiling processes running. If a
repair can mask a later clean-master stop, stage clean, mutation, and repaired
controls separately and document the ordering. After each cycle update both
the selected goal journal and uber-goal-state.md with status, draw, evidence,
findings, commits, limitations, and the next queue, then continue.
```

Operational notes: keep this prompt below the 4,000-character `/goal` limit;
the catalog holds the 99 campaign-specific scopes and this file holds the
shared controller protocol. The previous secp256k1 audit findings remain
inputs to deduplication, not a reason to skip unrelated pending goals.
