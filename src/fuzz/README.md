# secp256k1 Fuzz Oracles

Current upstream `master` did not contain tracked fuzz targets or seed corpora
before this directory was added. These targets are intentionally small public-API
oracles that exercise contract boundaries rather than only maximizing coverage.

Targets:

- `fuzz_api_roundtrip`: pubkey, ECDSA compact, DER, private-key DER, signing, verification, normalization
- `fuzz_context`: context randomize, clone, reset, deterministic signing consistency
- `fuzz_hash`: HMAC/RFC6979 chunking consistency and finalized-state cleanup
- `fuzz_scalar`: scalar rounded multiply-shift boundaries against an independent product
- `fuzz_field`: internal field normalization, arithmetic, encoding, and maximum-magnitude consistency
- `fuzz_group`: Jacobian/affine group-operation agreement and state cleanup
- `fuzz_ecmult_const`: constant-time multiplication against scalar-derived points
- `fuzz_ecmult_multi`: internal scratch/no-scratch multi multiplication consistency
- `fuzz_ecdh`: ECDH symmetry with default and coordinate passthrough hashers
- `fuzz_ellswift`: EllSwift encode/decode, XDH symmetry, built-in hash cleanup
- `fuzz_xonly_tweak`: x-only serialization, parity, tweak, keypair equivalence
- `fuzz_recovery`: recoverable ECDSA round trips when recovery is enabled
- `fuzz_schnorrsig`: Schnorr sign/verify and `sign32`/`sign_custom` equivalence
- `fuzz_musig`: MuSig key aggregation, tweak equivalence, nonce/signature round trips

Standalone corpus replay:

```sh
cmake -B build-fuzz -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_BUILD_TESTS=ON
cmake --build build-fuzz -j"$(nproc)"
ctest --test-dir build-fuzz -L secp256k1_fuzz --output-on-failure
```

Autotools standalone build:

```sh
./autogen.sh
mkdir -p build-autotools-fuzz
cd build-autotools-fuzz
../configure --enable-fuzz --enable-module-recovery
make -j"$(nproc)" fuzz_api_roundtrip fuzz_context fuzz_hash fuzz_scalar fuzz_field fuzz_group fuzz_ecmult_const fuzz_ecmult_multi fuzz_ecdh \
  fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig
make check TESTS="fuzz_api_roundtrip fuzz_context fuzz_hash fuzz_scalar fuzz_field fuzz_group fuzz_ecmult_const fuzz_ecmult_multi fuzz_ecdh fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig"
```

ASan/UBSan replay:

```sh
CC=clang cmake -B build-fuzz-asan -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_APPEND_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DSECP256K1_APPEND_LDFLAGS="-fsanitize=address,undefined"
cmake --build build-fuzz-asan -j"$(nproc)"
ctest --test-dir build-fuzz-asan -L secp256k1_fuzz --output-on-failure
```

libFuzzer multi-worker run:

```sh
CC=clang cmake -B build-fuzz-libfuzzer -S . \
  -DSECP256K1_BUILD_FUZZ=ON \
  -DSECP256K1_FUZZ_USE_LIBFUZZER=ON \
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON \
  -DSECP256K1_APPEND_CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DSECP256K1_APPEND_LDFLAGS="-fsanitize=address,undefined"
cmake --build build-fuzz-libfuzzer -j"$(nproc)"
mkdir -p fuzz-work/api_roundtrip
cp src/fuzz/corpora/api_roundtrip/* fuzz-work/api_roundtrip/
build-fuzz-libfuzzer/bin/fuzz_api_roundtrip \
  -print_funcs=0 -workers=4 -jobs=4 -max_total_time=60 \
  fuzz-work/api_roundtrip
```

When a target fails, replay the generated input against this branch and clean
`master`, then classify the finding as a production bug, stale oracle, invalid
domain construction, sanitizer-only issue, or already-covered behavior.

## Current-Master Finding Ledger

The following severities describe the exact `origin/master` audit baseline
(`ebf5943` when recorded), before the corresponding branch fixes. A later
minor fix must not be allowed to hide a more serious master failure. Every
production finding has a focused corpus and a mutation or deterministic proof
documented in its commit message.

- **High:** `ecmult_multi/scratch-wrap-create` (`b827e0e`). A caller-controlled
  scratch allocation size can wrap before allocation and corrupt memory.
- **Medium:** `group/off-curve-opaque-pubkey` and related API paths
  (`5ad8052`); noncanonical opaque public-key storage and cache bypass
  (`2b7a931`); inconsistent opaque keypairs used for signing or nonce creation
  (`9e70605`); opaque recoverable-signature state (`6c8a008`); x-only parity
  state (`f812b84`); direct NULL dereferences in exported built-in nonce
  callbacks (`ab0cb33`); and HMAC/RFC6979 secret-state retention after
  finalization (`5cfe7f7`). These require malformed local opaque state or an
  invalid direct API call unless the commit says otherwise; they are not all
  wire-format remote attacks.
- **Low:** impossible SHA256 lengths (`ab36b78`), scalar rounded-shift
  bounds (`422bab2`), EllSwift zero-`u` normalization (`119b407`), built-in
  ECDH failure-output cleanup (`bb15eb0`), and NULL preallocated context
  storage (`a9253c2`). Public nonce cleanup (`52c573b`) is stale-state hygiene,
  not critical secret erasure when the nonce has no cryptographic meaning.
- **Informational oracle gaps:** empty public-key aggregation (`c5c0afe`),
  ECDSA verification's invalid-opaque-key API boundary (`ef25d27`), and the
  public-key serializer's wrong flag-type boundary currently pass on master;
  their mutations prove that the harness would catch a regression, not that
  master is presently vulnerable.

If a clean-master replay stops at an earlier known failure, isolate the later
contract with its dedicated seed or a minimal production mutation. Do not
claim the later behavior was tested merely because a follow-up fix lets the
full harness continue.

## l0rinc Fork Duplicate Audit

The l0rinc remote was refreshed against the same `origin/master` baseline on
2026-07-12. Relevant fork refs were compared before replaying them. The
following mappings preserve discovery order and prevent later fixes from
masking a master-branch failure:

- `boundary-condition-bugs` (`65d38b0`): the 10x26 magnitude-32 normalization
  defect is already fixed by `cf5631f`, with the boundary test in `7ef0714` and
  the fuzzer oracle in `139f6ab`.
- `l0rinc/fe-equal-magnitude-bound` (`994b350`): already an ancestor of master;
  the corrected magnitude bound and its boundary coverage are retained.
- `l0rinc/reject-invalid-loaded-pubkeys` (`d1dca5c`): duplicates the ECDH and
  public-key-combine return checks in `5ad8052`; the corresponding fuzzer
  barriers are `f9f1a6e` and `7767442`.
- `musig-cleanup-failures` (`bb02b1e`) and `detached2`/`detached3`
  (`13308e3`/`51e93c4`): duplicate final-signature aggregation cleanup in
  `50bb71f`.
- `musig-clear-invalid-seckey-pubnonce` (`7ed2abc`): duplicate invalid-secret
  nonce-output coverage in `52c573b` and `04a853c`; the public nonce is not
  cryptographic secret material, so this remains stale-state hygiene.
- `detached` (`6e60f8d`): duplicate NULL-tweak keypair invalidation in
  `f48cd99`.
- `detached15` (`8363a2d`): the full 66-byte aggregate-nonce comparison is
  already present in `10cf586` and in the MuSig fuzzer's serialization and
  commutativity checks.
- `detached20` (`248be19`): duplicate BER length-helper correction in
  `d4d1519`, with the malformed-length fuzzer oracle in `0cf6f5d`.
- `detached5` (`b9a169b`): an optimization follow-up to the hash-context
  refactor; it adds no distinct contract and is not cherry-picked.
- `l0rinc/scratch-free-warning` (`c0f32d4`), field serialization/CMOV refs,
  and comment-only refs are test-warning, optimization, or documentation
  changes without a new production oracle; they are intentionally not
  cherry-picked.

These are duplicate or already-covered findings, not new severity claims. The
severity of each underlying master behavior remains the one recorded above and
in the owning commit message. A fork patch is never used as proof that a
current-master failure was absent; each important barrier has its own seed or
minimal mutation proof.
