# secp256k1 Fuzz Oracles

Current upstream `master` did not contain tracked fuzz targets or seed corpora
before this directory was added. These targets are intentionally small public-API
oracles that exercise contract boundaries rather than only maximizing coverage.

Targets:

- `fuzz_api_roundtrip`: pubkey, ECDSA compact, DER, signing, verification, normalization
- `fuzz_context`: context randomize, clone, reset, deterministic signing consistency
- `fuzz_field`: internal field normalization and maximum-magnitude consistency
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
make -j"$(nproc)" fuzz_api_roundtrip fuzz_context fuzz_field fuzz_ecmult_multi fuzz_ecdh \
  fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig
make check TESTS="fuzz_api_roundtrip fuzz_context fuzz_field fuzz_ecmult_multi fuzz_ecdh fuzz_ellswift fuzz_xonly_tweak fuzz_recovery fuzz_schnorrsig fuzz_musig"
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
