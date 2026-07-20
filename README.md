libsecp256k1
============

![Dependencies: None](https://img.shields.io/badge/dependencies-none-success)
[![irc.libera.chat #secp256k1](https://img.shields.io/badge/irc.libera.chat-%23secp256k1-success)](https://web.libera.chat/#secp256k1)

High-performance high-assurance C library for digital signatures and other cryptographic primitives on the secp256k1 elliptic curve.

This library is intended to be the highest quality publicly available library for cryptography on the secp256k1 curve. However, the primary focus of its development has been for usage in the Bitcoin system and usage unlike Bitcoin's may be less well tested, verified, or suffer from a less well thought out interface. Correct usage requires some care and consideration that the library is fit for your application's purpose.

Features:
* secp256k1 ECDSA signing/verification and key generation.
* Additive and multiplicative tweaking of secret/public keys.
* Serialization/parsing of secret keys, public keys, signatures.
* Constant time, constant memory access signing and public key generation.
* Derandomized ECDSA (via RFC6979 or with a caller provided function.)
* Very efficient implementation.
* Suitable for embedded systems.
* No runtime dependencies.
* Optional module for public key recovery.
* Optional module for ECDH key exchange.
* Optional module for Schnorr signatures according to [BIP-340](https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
* Optional module for ElligatorSwift key exchange according to [BIP-324](https://github.com/bitcoin/bips/blob/master/bip-0324.mediawiki).
* Optional module for MuSig2 Schnorr multi-signatures according to [BIP-327](https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki).
* Optional module for Silent Payments sending and receiving according to [BIP-352](https://github.com/bitcoin/bips/blob/master/bip-0352.mediawiki).

Implementation details
----------------------

* General
  * No runtime heap allocation.
  * Extensive testing infrastructure.
  * Structured to facilitate review and analysis.
  * Intended to be portable to any system with a C89 compiler and uint64_t support.
  * No use of floating types.
  * Expose only higher level interfaces to minimize the API surface and improve application security. ("Be difficult to use insecurely.")
* Field operations
  * Optimized implementation of arithmetic modulo the curve's field size (2^256 - 0x1000003D1).
    * Using 5 52-bit limbs
    * Using 10 26-bit limbs (including hand-optimized assembly for 32-bit ARM, by Wladimir J. van der Laan).
      * This is an experimental feature that has not received enough scrutiny to satisfy the standard of quality of this library but is made available for testing and review by the community.
* Scalar operations
  * Optimized implementation without data-dependent branches of arithmetic modulo the curve's order.
    * Using 4 64-bit limbs (relying on __int128 support in the compiler).
    * Using 8 32-bit limbs.
* Modular inverses (both field elements and scalars) based on [safegcd](https://gcd.cr.yp.to/index.html) with some modifications, and a variable-time variant (by Peter Dettman).
* Group operations
  * Point addition formula specifically simplified for the curve equation (y^2 = x^3 + 7).
  * Use addition between points in Jacobian and affine coordinates where possible.
  * Use a unified addition/doubling formula where necessary to avoid data-dependent branches.
  * Point/x comparison without a field inversion by comparison in the Jacobian coordinate space.
* Point multiplication for verification (a*P + b*G).
  * Use wNAF notation for point multiplicands.
  * Use a much larger window for multiples of G, using precomputed multiples.
  * Use Shamir's trick to do the multiplication with the public key and the generator simultaneously.
  * Use secp256k1's efficiently-computable endomorphism to split the P multiplicand into 2 half-sized ones.
* Point multiplication for signing
  * Use a precomputed table of multiples of powers of 16 multiplied with the generator, so general multiplication becomes a series of additions.
  * Intended to be completely free of timing sidechannels for secret-key operations (on reasonable hardware/toolchains)
    * Access the table with branch-free conditional moves so memory access is uniform.
    * No data-dependent branches
  * Optional runtime blinding which attempts to frustrate differential power analysis.
  * The precomputed tables add and eventually subtract points for which no known scalar (secret key) is known, preventing even an attacker with control over the secret key used to control the data internally.

Obtaining and verifying
-----------------------

The git tag for each release (e.g. `v0.6.0`) is GPG-signed by one of the maintainers.
For a fully verified build of this project, it is recommended to obtain this repository
via git, obtain the GPG keys of the signing maintainer(s), and then verify the release
tag's signature using git.

This can be done with the following steps:

1. Obtain the GPG keys listed in [SECURITY.md](./SECURITY.md).
2. If possible, cross-reference these key IDs with another source controlled by its owner (e.g.
   social media, personal website). This is to mitigate the unlikely case that incorrect 
   content is being presented by this repository.
3. Clone the repository: 
    ```
    git clone https://github.com/bitcoin-core/secp256k1
    ```
4. Check out the latest release tag, e.g. 
    ```
    git checkout v0.7.1
    ```
5. Use git to verify the GPG signature: 
   ```
   % git tag -v v0.7.1 | grep -C 3 'Good signature'

   gpg: Signature made Mon 26 Jan 2026 07:42:46 PM UTC
   gpg:                using RSA key 2840EAABF4BC9F0FFD716AFAFBAFCC46DE2D3FE2
   gpg: Good signature from "Pieter Wuille <pieter@wuille.net>" [unknown]
   gpg:                 aka "Pieter Wuille <pieter.wuille@gmail.com>" [full]
   gpg:                 aka "[jpeg image of size 5996]" [undefined]
   gpg: WARNING: This key is not certified with a trusted signature!
   gpg:          There is no indication that the signature belongs to the owner.
   Primary key fingerprint: 133E AC17 9436 F14A 5CF1  B794 860F EB80 4E66 9320
        Subkey fingerprint: 2840 EAAB F4BC 9F0F FD71  6AFA FBAF CC46 DE2D 3FE2
   ```

Building with Autotools
-----------------------

    $ ./autogen.sh       # Generate a ./configure script
    $ ./configure        # Generate a build system
    $ make               # Run the actual build process
    $ make check         # Run the test suite
    $ sudo make install  # Install the library into the system (optional)

To compile optional modules (such as Schnorr signatures), you need to run `./configure` with additional flags (such as `--enable-module-schnorrsig`). Run `./configure --help` to see the full list of available flags.

Building with CMake
-------------------

To maintain a pristine source tree, CMake encourages to perform an out-of-source build by using a separate dedicated build tree.

### Building on POSIX systems

    $ cmake -B build              # Generate a build system in subdirectory "build"
    $ cmake --build build         # Run the actual build process
    $ ctest --test-dir build      # Run the test suite
    $ sudo cmake --install build  # Install the library into the system (optional)

To compile optional modules (such as Schnorr signatures), you need to run `cmake` with additional flags (such as `-DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON`). Run `cmake -B build -LH` or `ccmake -B build` to see the full list of available flags.

### Cross compiling

To alleviate issues with cross compiling, preconfigured toolchain files are available in the `cmake` directory.
For example, to cross compile for Windows:

    $ cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-w64-mingw32.toolchain.cmake

To cross compile for Android with [NDK](https://developer.android.com/ndk/guides/cmake) (using NDK's toolchain file, and assuming the `ANDROID_NDK_ROOT` environment variable has been set):

    $ cmake -B build -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=28

### Building on Windows

The following example assumes Visual Studio 2022. Using clang-cl is recommended.

In "Developer Command Prompt for VS 2022":

    >cmake -B build -T ClangCL
    >cmake --build build --config RelWithDebInfo

Usage examples
-----------
Usage examples can be found in the [examples](examples) directory. To compile them you need to configure with `--enable-examples`.
  * [ECDSA example](examples/ecdsa.c)
  * [Schnorr signatures example](examples/schnorr.c)
  * [Deriving a shared secret (ECDH) example](examples/ecdh.c)
  * [ElligatorSwift key exchange example](examples/ellswift.c)
  * [MuSig2 Schnorr multi-signatures example](examples/musig.c)
  * [Silent Payments send and receive example](examples/silentpayments.c)

To compile the examples, make sure the corresponding modules are enabled.

Benchmark
------------
If configured with `--enable-benchmark` (which is the default), binaries for benchmarking the libsecp256k1 functions will be present in the root directory after the build.

To print the benchmark result to the command line:

    $ ./bench_name

To create a CSV file for the benchmark result :

    $ ./bench_name | sed '2d;s/ \{1,\}//g' > bench_name.csv

Reporting a vulnerability
------------

See [SECURITY.md](SECURITY.md)

Contributing to libsecp256k1
------------

See [CONTRIBUTING.md](CONTRIBUTING.md)

Fuzz-oracle audit: package test-accept state preservation
----------------------------------------------------------

This audit record covers Bitcoin Core's `ephemeral_package_eval` and `tx_package_eval` fuzz targets. The Core base was `origin/master` at `18c05d93016b28a9afd4c716dfe00b6e0accb30b0`; the audit branch also contains unrelated commit `1df24a61f7` in `validation.cpp`, outside the tested hunk. `origin/master` and `remotes/l0rinc/master` both resolve to that same commit, and `git log origin/master..remotes/l0rinc/master -- src/test/fuzz/package_eval.cpp src/validation.cpp src/txmempool.cpp` was empty, so no relevant l0rinc commit was cherry-picked.

The new oracle snapshots the complete externally relevant mempool state around every `test_accept` boundary: txid/wtxid and entry metadata, fee-prioritisation deltas, unbroadcast membership, total size and fee, sequence, transaction-update counter, and load-attempt state. Single-transaction package test accepts are checked before/after `ProcessNewPackage`; multi-transaction submissions are checked before/after the final `AcceptToMemoryPool(test_accept=true)` evaluation. Existing result-map, fee, replacement, TRUC, and ephemeral invariants remain in force.

Severity and Core reachability: clean master reproduced no production failure. This is Low/informational oracle hardening, not a confirmed production bug, and not consensus validation, invalid-block handling, peer-triggerable block acceptance, memory safety, or Critical severity. Core's local `testmempoolaccept` and package-evaluation RPC paths use `test_accept=true` (`src/rpc/mempool.cpp`); peer package submission in `src/net_processing.cpp` uses `test_accept=false`. If the modeled regression existed, it could mutate local policy-evaluation state, but the clean production contract is intact. No deterministic regression test was added because no clean-master bug was confirmed; the exact production mutation below is the regression proof for the oracle.

Frozen corpora:

* `tx_package_eval`: `/mnt/my_storage/qa-assets/fuzz_corpora/tx_package_eval`, copied to `/tmp/bitcoin-tx-package-eval-20260720/corpus`; 2,115 files and 77,567,620 bytes; manifest SHA-256 `b557a6668b4691fe18437b208d7fd35085d67edfb532a8a09af349ff394c6d22`.
* `ephemeral_package_eval`: `/mnt/my_storage/qa-assets/fuzz_corpora/ephemeral_package_eval`, copied to `/tmp/bitcoin-ephemeral-package-eval-20260720/corpus`; 1,671 files and 9,748,246 bytes; manifest SHA-256 `cfb431ce61ac0eee00c2122c5b700d03e4aaed48181c0ff252c2e85440886eba`.

Final source and replay evidence:

* `src/test/fuzz/package_eval.cpp` SHA-256: `c8c70263b3e3bfb83cd671e7a59794fc231c356188c9de62c2c376ffc6005860`.
* Clean `src/validation.cpp` SHA-256: `6f00f58f3cc9623fb02cfa8c776654e617b6a88e87c2b075bb855116b9ebfefc`.
* Final `/tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz` SHA-256: `303a14e89648882607018aad2205acb16f052356782d4986d7d59d3b686dce53`.
* `tx_package_eval`: 2,117 runs, coverage 14,748, peak RSS 499 MiB, exit 0; log SHA-256 `51ed15132f085b472780bf345fbc8320fcb3a388cb2654d4a3185af4eaffaa1c`.
* `ephemeral_package_eval`: 1,672 runs, coverage 13,426, peak RSS 321 MiB, exit 0; log SHA-256 `8613ff4816462d4220f21c66647797f21fb6bce76a6885ff780f75d71f2c7c2f`.
* Four jobs/four workers on `tx_package_eval` all exited 0 after 2,117 runs. Peak RSS was 505/510/499/502 MiB. Parent log SHA-256 `c7291a69d97155ee5cb14cf0c6128722c93face8b265bcc01542b023d11ea074`; worker log SHA-256 values were `74089fc604e0bc275aa4e48ba3fc8a0f8cbb22855a925885e9de1da28c1801a5`, `9250e71cd3b2ea26d3d86850e1c9a6cdaaa78cc8bb63deeb70a40837f15b6882`, `8af986887b570bb4603bd34a24715512034abcab6f6c9b9b63e64daf44892e81`, and `025f1be6a5e17198a0f90ff48dfb5da81fee06e58b4c2afc93d99d26733a7a49`.

Mutation proof:

* At `src/validation.cpp:1395`, mutate `if (args.m_test_accept)` to `if (false && args.m_test_accept)`. Mutated source SHA-256: `ec23cf1107cb51d099157b127e1301a8c0a92b959d1ec8339aa945a92b986cd4`; mutated final-harness binary SHA-256: `eedfde99b80f32e41b39ef60e9328cd1d1374407e6463b4467393d4364c57a54`.
* The frozen `tx_package_eval` corpus hit `AssertMempoolStateUnchanged` at `package_eval.cpp:109` from target line 597. The final one-input mutation replay exited 77; log SHA-256 `4c104001cddb75a805028ad922ed5082f861fd212b63eac1fb097adde2600fbc`.
* Artifact: `/tmp/bitcoin-tx-package-eval-20260720/mutation-artifacts/crash-19c473ce76ba37846d34800d8c2da8ad040726f7`; SHA-256 `090837c92fb69a1dbce82b74727b4165b36708afa4e2f415f1213ead981e3542`; Base64 `AAD6AKDqOVPqfwQAANcAACEA+zYAAAJN5QD4DwAAAG9VZ9v4ADEA+zYAAABN5QAAYwAAAABVO2ctAAAAAFcAagCDCwABHL8AClcB5AT4+izHAHo=`.
* Restoring the exact guard made that artifact pass in 8 ms with exit 0; restored log SHA-256 `bc1dc765e2fa832c7efd17fac6b3308a6a156326c49863b4909e52894caf6eb1`.

Verifier commands:

    cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2
    FUZZ=tx_package_eval /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz -runs=2115 /tmp/bitcoin-tx-package-eval-20260720/corpus
    FUZZ=ephemeral_package_eval /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz -runs=1671 /tmp/bitcoin-ephemeral-package-eval-20260720/corpus
    FUZZ=tx_package_eval /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz -jobs=4 -workers=4 -max_total_time=60 /tmp/bitcoin-tx-package-eval-20260720/corpus

Fuzz-oracle audit: external block-file import index contracts
---------------------------------------------------------------

Source commit `d5a7b999c4` strengthens Bitcoin Core's
`FUZZ=load_external_block_file`. The audit base is `origin/master`
`18c05d93016b28a9afd4c716dfe00b6e0accb30b0`; this branch also contains
unrelated audit commit `1df24a61f7` in `validation.cpp`, outside the tested
`LoadExternalBlockFile` hunk. The target calls
`ChainstateManager::LoadExternalBlockFile` (`src/validation.cpp:4983-5150`),
which Core reaches from `src/node/blockstorage.cpp:1276-1329` for `-reindex`
and `-loadblock=`. Its input is local external block-file/reindex data, not a
direct peer message. Clean master produced no production failure, so this is
informational/Low oracle hardening, not a confirmed bug or Critical invalid-
block vulnerability.

The harness snapshots each existing `CBlockIndex` identity, parent pointer,
height, chain work, version, block hash, merkle root, time, bits, and nonce
before import. It then requires old entries to remain at the same addresses
with unchanged identity/ancestry, every map key to match `GetBlockHash()`,
every parent pointer to resolve to the same indexed object, contiguous
parent/child heights, nondecreasing chain work, and an indexed active tip.
Legitimate storage/status updates and new entries remain allowed. The old
47-line target discarded the operation result and checked none of these
contracts.

l0rinc review and masking record: `origin/master` and
`remotes/l0rinc/master` both resolve to `18c05d93016b28a9afd4c716dfe00b6e0accb30b0`;
`git log origin/master..remotes/l0rinc/master -- src/test/fuzz/load_external_block_file.cpp src/validation.cpp src/node/blockstorage.cpp`
was empty. No relevant l0rinc commit was cherry-picked. No later fix or
cherry-pick was used to mask this clean-master result. Any later change must
state whether it preserves, changes, or masks this behavior and amend the
relevant commit and ledger with the exact evidence.

Corpus and replay evidence:

* Frozen corpus: `/mnt/my_storage/qa-assets/fuzz_corpora/load_external_block_file`, copied to `/tmp/bitcoin-load-external-block-file-20260720/frozen`; 577 files, 68,776,078 bytes; manifest SHA-256 `1eeba74f7240ff32ceeaba43a575786089dc7093e0ffde24960ce0febfaad324`.
* Original harness baseline: 761 runs, coverage 2356, 15,395 features, peak RSS 654 MiB, exit 0; log SHA-256 `110dd194609b281aeb434e610e405312686e24a3dc95fc554e16a017ab0dab41`.
* Final harness SHA-256 `c952cb659ce2a1a0ee4cc0a2d1ee6277336c43f7148c4e14af352f890bfab4af`; clean `src/validation.cpp` SHA-256 `6f00f58f3cc9623fb02cfa8c776654e617b6a88e87c2b075bb855116b9ebfefc`; final fuzzer SHA-256 `f967413c8ea4bf774a783b8e7231e777b149eeb6fdec55b5e1a14a8c9c4a3bde`.
* Clean frozen replay used `-merge=0 -runs=1 -timeout=60 -rss_limit_mb=4096`: 761 runs, coverage 2413, 15,745 features, 648 MiB RSS, exit 0, no artifacts; log SHA-256 `62635443e5127c61c67ce5b5497ee95977fcc0c62f24226a6ec953614bc75a7e`.
* Exact witness replay: 2 executed units, 178 MiB RSS, exit 0; log SHA-256 `b5d5b3764dfe69419401b1430bf704ee61445895b5942597235e79e258c1402d`.
* Four ASan/UBSan workers used `-merge=0 -jobs=4 -workers=4 -runs=1 -timeout=60 -rss_limit_mb=4096`; all exited 0 after 762 units, coverage 2413, and produced no artifacts. Peak RSS was 648/649/649/651 MiB. Parent log SHA-256 `c9b60c5b7d0d0321fab99b3d54b2fd37f33c35915e492681537bc2db47eac04f`; worker logs: `7a08ae40d63cd3a01cd6fb8f0e4788303802e6863b73a65e5bb488b2b7840476`, `880b265ddfb776a5afd2fb663a970466162d85b01a7ceb546a4bfb706882f8d9`, `ef5f6e1630ee6f7bb104a41123c163c37c68606e770cc64a70a151e4e2822251`, `5939adc24092a51e5a9a71207223935246c4ca3d0835b44e5a8edf484b3d7285`.

Mutation sensitivity proof, not a clean-master production finding:

* Immediately after a successful `AcceptBlock` in `LoadExternalBlockFile`, add `m_blockman.LookupBlockIndex(hash)->nHeight++`. Mutated production `src/validation.cpp` SHA-256: `f5408af31fc728c364c61ade6fcf9114aa4f5ef82c4a3e9a64f7108c2f2054c9`; mutated fuzzer binary SHA-256: `82680aff8f379198c9d67ac58eb7e2dcba0eb9d5cfc202584dcdce64488e2665`.
* Witness: `/tmp/bitcoin-load-external-block-file-20260720/python-block1-late-fuzz-seed.dat`, 164 bytes, SHA-256 `3252e3f6dfae7b61e4ba5bac39d444b385b8376c6f3d153035f8ea615deb6c39`:

      +r+12pAAAAAEAAAABiJuRhEaC1nKrxJgQ+tbvyjDTzpeMyofx7K3PPGIkQ+M0CldtgFgCjFho6scTPli/NNrRPmoXCOwvAYIeqJeEdvlSU3//38gAAAAAAECAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/////wJRAP7///8BAPIFKgEAAAABUQAAAAAAAAAAAAAAAP////8=

* The mutated run produced an assertion/libFuzzer deadly-signal abort after one input at `src/test/fuzz/load_external_block_file.cpp:89`, `index.nHeight == index.pprev->nHeight + 1`; the stack reached `AssertBlockIndexImportContracts`, the target at line 127, `test_one_input`, and `LLVMFuzzerTestOneInput`. Mutation log SHA-256: `b4413d9c945b13f3109e60b42bf6daeca495beff0470514dca1bf65b2b8f19e3`. No artifact was emitted because the oracle uses an assertion abort.
* Restoring production made the same witness pass. This is proof that the oracle detects the modeled regression, not proof of a clean-master defect. No deterministic production regression test was added because no clean-master bug was confirmed.

Severity ledger reiterated: private-broadcast failed-send retention is Medium
and feature-conditional; empty HEADERS initial-sync handoff is Medium
availability/IBD; peer transaction activity refresh is Low; process-message
block-storage failure handling is Low/local-write dependent; oversized
transport types are Low with current Core callers; ecmult scratch wrapping,
forced 10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention are
Medium latent or hygiene findings with limited reachability; banman invalid
subnet/unban integrity is Low/nice-to-have. The txdownloadman/txrequest,
connman, eviction, handshake, compact-block, headers-sync, coins-view, UTXO
snapshot, mempool-persistence, and package test-accept campaigns found no
additional clean-master production bug in their audited paths. Severity is
master- and Core-caller-relative: invalid fuzzer state or an invalid block
alone is not Critical, and a nonce with no cryptographic meaning is not
critical merely because it is not cleared.

Verifiers were `clang-format --dry-run --Werror`, `git diff --check`,
`cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2`,
the clean replay, the exact witness replay, the documented production
mutation replay, and the four-worker command above. No fuzz process or
artifact remains.

Fuzz-oracle audit: addrman state and transition contracts
-----------------------------------------------------------

Source commit `8d59855176` strengthens `data_stream_addr_man`, `addrman`, and
`addrman_serdeser`. The base is Bitcoin Core `origin/master` and
`remotes/l0rinc/master`, both `18c05d93016b28a9afd4c716dfe00b6e0accb30b0`;
the source branch was at `d5a7b999c4388aa54655f65af2e95d64308001fb` before
this commit. The l0rinc query
`git log origin/master..remotes/l0rinc/master -- src/test/fuzz/addrman.cpp src/addrman.cpp src/addrdb.cpp src/net_processing.cpp`
was empty, so there was no relevant commit to cherry-pick and no later fix was
used to mask this result.

Core caller and severity boundary:

* `src/net_processing.cpp:5820-5878` receives peer `ADDR`/`ADDRV2` records,
  filters them, and calls `AddrMan::Add`; `Good` and `SetServices` are reached
  from connection handling at lines 3885 and 3702. `src/addrdb.cpp:196-225`
  loads and serializes the local `peers.dat` state. These are address-gossip,
  peer-selection, and local-persistence paths, not consensus or block
  validation paths.
* No clean-master production bug was confirmed. The result is Low/informational
  oracle hardening. Invalid fuzzer state or an invalid block alone is not
  High/Critical, and a nonce with no cryptographic meaning is not Critical
  merely because it is not cleared. Any future failure must be rated from the
  actual Core caller, input origin, and master behavior.

Oracle contracts:

* `GetEntries` output must agree with new/tried flags, bucket and position
  bounds, reference multiplicity, unique address identity, `Size`, and the
  existing production `CheckAddrman` invariant where the state is at most
  4,096 entries.
* `GetAddr` results must be valid and unique, exactly match the expected set for
  small states, and be bounded, filtered, and drawn from the expected state for
  large states. The oracle deliberately uses `GetNetClass` for `GetAddr` and
  `GetNetwork` for `Size`/`Select`, matching their separate production
  contracts.
* `Select` must return an address from the requested table/network whenever
  one exists, or an invalid address otherwise. `Good`, `Attempt`, `Connected`,
  and `SetServices` have operation-local before/after assertions. Successful
  deserialization and serialize/deserialize round trips receive the same
  state checks.
* The full table oracle is scheduled at every 64 operations for ordinary
  states and every 1,024 operations for large states. Large `GetAddr` checks
  use a 256-result cap and skip scans for network classes absent from a large
  table; operation-local assertions remain on every transition. This bounds
  harness amplification without weakening the exact small-state contracts.

Frozen corpora and final verification:

* `addrman`: source `/mnt/my_storage/qa-assets/fuzz_corpora/addrman`, frozen
  at `/tmp/bitcoin-addrman-20260720/frozen`; 1,793 files and 104,247,504
  bytes. `manifest.entries` SHA-256 is
  `9ccbbc4d27321b245c0c9afa005cacfc2b6108b3e2c2920890bfcf2bacb76d4e` and
  `manifest.sha256` SHA-256 is
  `079f75b805076cbf69c7c35481e4a0479237d37e11e6c6f087bd516077731c72`.
  The original execution-only clean baseline completed 1,794 runs, coverage
  3,825, features 21,443, peak RSS 673 MiB, exit 0; log SHA-256
  `eb2e835edef55d62a63b6a22ef033c7a9864a1c2f27f87e278022db7634dfd41`.
* The final oracle replay used 1,547 existing inputs below 64 KiB,
  4,666,251 bytes; the selected-file content manifest SHA-256 is
  `c10e8dede3d3746dbb21b8712061490913f3e0fb0879e20a2089d451db988162`.
  It completed 1,548 runs, coverage 4,656, features 27,545, peak RSS 463
  MiB, exit 0, no artifacts; log SHA-256
  `094c99623bd7bab192a61843440d443c035dc72a18b720a4a896e7486f75e0cb`.
  Four sanitizer workers over the same subset all exited 0 after 1,548 runs
  with the same coverage/features and 463 MiB peak RSS. Parent log SHA-256
  `e644ff03f8628ab92ac0cf56340689656f89e139a8ed4742556034dd137314c4`;
  worker logs: `9ed0294a41716eafe6266c22d1884ca08b5516197f4c8ec74ef29765cfee5e6e`,
  `c68a1f09d692b80358b80700243a51b3929e1edbcd2d228367206d70c5d5aecc`,
  `5750637309d5d76a8a2e340956df78a48137926f0cc4eec62b85e569b79219e7`,
  and `973c6e7335af6b28597434f53c2192b1746714b6678281a534dbbf40e3879509`.
* `addrman_serdeser`: source `/mnt/my_storage/qa-assets/fuzz_corpora/addrman_serdeser`,
  frozen at `/tmp/bitcoin-addrman-20260720/serdeser/frozen`; 1,206 files and
  23,454,548 bytes. `manifest.entries` SHA-256 is
  `5696db9698d145dd4ee3549077e25860782f629e39fdaffee95136dcd238552e` and
  `manifest.sha256` SHA-256 is
  `7cf2c3323067272c3488e3f677935562ca4562e4cad6dd10b7d7fc6df756a3e5`.
  The final full replay completed 1,207 runs, coverage 4,334, features
  24,142, peak RSS 646 MiB, exit 0; log SHA-256
  `f7a4d690100e3ddd5387336491a720df780c40d9bbb6e0015b1d07c000cdda63`.
  Four workers all completed 1,207 runs, coverage 4,334/features 24,142,
  exit 0, with peak RSS 645/643/645/647 MiB. Parent log SHA-256
  `13369e9e0d5defc5f9a354013f5068e4931c20982b4fb89839700987913a6746`;
  worker logs: `bf4e0e5e24bea95e86c73a341a655d5cf9fb3b363c0516aa8b97faba824da9a3`,
  `7554ec518c624cba1d6cfeb31219bf32b58153da814a89f7f106b6ae63ec6c12`,
  `78ed33109cb529e8bddc834b2acdb06b678d59e14b477643f3ecb7333c5fbecd`,
  and `beb13690fc3d4b5882f7b20a3271cc709fb6af77150bf438dba1d73b8727cacd`.
* `data_stream_addr_man` over the bounded existing subset completed 1,548
  runs, coverage 764, features 1,086, peak RSS 402 MiB, exit 0, with no
  artifacts; log SHA-256 `30165d99ba00e139df2abd6c44cbba2c99dd78fff2ba96c8b7a37dd4046705e3`.
* Final clean hashes are `src/addrman.cpp`
  `b96f2c0f11ba797c3f7e65a2b52f599df81d1723538f409f1286d4bda4d6f1bf`,
  `src/test/fuzz/addrman.cpp`
  `de7adae4681b5b772a23e1e8f74e0f8e7186089147f9a1d1c48ac3039683439a`,
  `src/test/fuzz/util/net.h`
  `3056982666282b25409d9837c606bf60499b8f0ac5f932e1b6d79a90ffaf48e1`,
  and the clean ASan/UBSan fuzzer binary
  `efb1010e208b8397b4e5ba8af8e52614ad5fd487a0d7b601e23f5b42d7079ae5`.
  The complete 1,793-input final oracle replay was not used as a pass claim:
  large inputs entered multi-minute production `AddrMan::Add` callbacks that
  libFuzzer could not interrupt with `-timeout=60`. Those attempts were
  terminated and classified as corpus execution cost, not findings; the
  bounded subset and full serialization corpus are the completed final proof.

Mutation sensitivity proof, not a clean-master production finding:

* Temporarily changed `src/addrman.cpp:SetServices` from
  `info.nServices = nServices` to
  `info.nServices = ServiceFlags(info.nServices | nServices)`.
* Mutated production source SHA-256
  `98b1343039bce71a564a50f3a6cdc7f7605f9a0d22dace99f1a05d3839b133fe`;
  mutated fuzzer binary SHA-256
  `a8404400d4a34fbd8387a9f03fcc44b955919d8323c6386d0f59066794c80d9b`.
* Existing corpus input `manageable64/6e49c6eda5177106613fced9a731a8f2b4594275`
  is 738 bytes, SHA-256
  `cfc6a6594fec02ca7c28a056feb657260bab60615a4fbc3c486876a160fb221b`.
  The mutated run aborted after 1,075 executions at
  `src/test/fuzz/addrman.cpp:386`, asserting
  `after->nServices == n_services`; mutation log SHA-256
  `ec2490f7d78040462adf8cbc03cd0de75947639fedfa3d533eba40ca691651bb`.
  The restored final clean binary accepts the exact artifact in 22 ms; clean
  replay log SHA-256
  `af07f9f8738aaa54b2cd900c06b71575d412641e2e58e37b0c5d1e9014b09070`.
  This proves oracle sensitivity to a modeled replacement regression, not a
  master defect, so no production fix or deterministic regression test is
  claimed.

Rejected oracle and correction record:

The first `GetAddr` filter assertion incorrectly used `GetNetwork` instead of
the production method's `GetNetClass`. It failed on exact artifact
`oracle-artifacts/crash-935ea9dc5c38a59f222ab627166b12a6e44b6e77`, SHA-256
`076682611cdda4d348cdbdd76395a48f1c185ee29bced7a3fd1f851e111161f0`, with
Base64 `EyAgXN+JiACzAALAgAAAAAAAIDxdIAAcAAAAAAAkAAAAAAATICAC2RNdWR7/00BR`.
The stale log SHA-256 is
`6f26d1368ab4bd6b4aa602f3a0671da3fdc61d6f078467bea696ae4734c439f6`;
after changing only the oracle to `GetNetClass`, the exact artifact passed,
with corrected replay log SHA-256
`910863c7b1f02c508aa87abdc756d212a27959226a96b2b96c66ee05f41513a6`.
This was a stale/overbroad harness oracle, not a production finding.

Existing finding ledger is reiterated against unmodified master and actual
Bitcoin Core reachability: private-broadcast failed-send retention is Medium
and feature-conditional; empty HEADERS initial-sync handoff is Medium
availability/IBD; peer transaction activity refresh and process-message
storage failure are Low; oversized transport types are Low in current Core
callers; ecmult scratch wrapping, forced 10x26 magnitude-32 normalization,
and SHA/HMAC/RFC6979 retention are Medium latent or hygiene findings with
limited demonstrated reachability; banman invalid-subnet integrity is
Low/nice-to-have. The txdownloadman/txrequest, connman, eviction, handshake,
compact-block, headers-sync, coins-view, UTXO snapshot, mempool-persistence,
and package test-accept campaigns found no additional clean-master production
bug. Severity is master- and Core-caller-relative: invalid fuzzer state or an
invalid block alone is not Critical, and a nonce with no cryptographic meaning
is not Critical merely because it is not cleared.

Verifiers: `clang-format --dry-run --Werror` on the changed harness files,
`git diff --check`, `cmake --build /tmp/bitcoin-secp256k1-audit-current-build
--target fuzz -j2`, the bounded addrman replay, the full addrman_serdeser
replay, the data-stream replay, the exact mutation replay, and both four-worker
commands above. Production was restored to its clean hash and no fuzz process
or generated artifact remains in either source worktree.

## Coins-cache transition oracle audit (2026-07-20)

Scope and source state:

* Base is exact master `18c05d93016b28a9afd4c716dfe00b6e0accb30b` on the
  audit branch. Target is `FUZZ=coinscache_sim`; production behavior is
  unchanged. The harness now calls the existing `CCoinsViewCache::SanityCheck`
  after every simulated command, so dirty/fresh flags, the circular dirty
  list, dirty-count accounting, and cached dynamic-memory accounting are
  checked before a later reset, flush, or overwrite can repair an intermediate
  defect. No duplicate best-block model was added because the coins-view and
  stacked-view targets already cover best-block propagation.
* Original harness SHA-256 was
  `05a151e9bb8edb8a0b7aea6ab662de578684e0bfddb1efa578506c5dd89330ca`;
  final harness SHA-256 is
  `25d608c872df9bf66f16fe96a41de69b588119c5661e71c66c82bb06875dad49`.
  Unchanged production `src/coins.cpp` SHA-256 is
  `2c7ca3aab136509449d4810b5174f21ad8bbf6853d175a69d601fe7697c25096`.
  The final ASan/UBSan/fuzzer binary SHA-256 is
  `d18113d2809f067f61138f93da76329cb83ed3f54930d8c962519b7cbe069c08`.

Corpus and clean verification:

* Frozen corpus is `/tmp/bitcoin-coinscache-20260720/frozen`, copied from
  `/mnt/my_storage/qa-assets/fuzz_corpora/coinscache_sim`: 305 files and
  5,153,077 bytes. Sorted file-entry manifest SHA-256 is
  `a8d126f8bfd18f1344561cc79347ee394a6094dff0aaeecf620b635a91bcc44c`.
* The original execution-only baseline completed 309 executions, coverage
  2021, features 13192, peak RSS 214 MiB, exit 0, with no artifacts. The
  final clean replay completed 307 executions, coverage 2027, features 13223,
  peak RSS 213 MiB, exit 0, with no artifacts; final log SHA-256 is
  `c6b99abff0492f76526edac19c16e97da8c204402741b4ee80de34ccca9cebfd`.
* Four independent sanitizer workers loaded the same 305-file corpus and
  completed 307, 307, 308, and 310 executions. Each exited 0 with coverage
  2027, peak RSS 214 MiB, and no sanitizer, assertion, timeout, OOM, or
  artifact diagnostic. Parent log SHA-256 is
  `91bd43feac067a11deb41601490ff2311d9d599c68f187dde712109800894bc9`.

Differential oracle proof, not a master production finding:

* A disposable production mutation removed the single `++m_dirty_count` in
  `CCoinsViewCache::AddCoin` immediately after `SetDirty`. This models a
  broken publication of a cache state that `BatchWrite` and `Flush` rely on.
  Mutated production source SHA-256 was
  `bad0f00f95371389bc6872725eb281cf77c430c15aa725300d73ded3f11d28f4` and
  the enhanced mutated binary SHA-256 was
  `7e3e83450368f6cc2a39b94143b221503f1f36a7b4db8216526ce4403c56315d`.
* The exact proof input is four bytes `0d 00 00 05`, SHA-256
  `cd318afe91129a721a28f401677a18e89f37e314718d9b03eb22a73b29f24244`,
  Base64 `DQAABQ==`. Its reverse-consumed commands add a coin, then reset the
  cache. Enhanced replay with `-handle_abrt=0` exits 134 at
  `src/coins.cpp:365`, assertion
  `count_dirty == count_linked && count_dirty == m_dirty_count`; log SHA-256
  is `18fbdd06b54b6f22eb4bf3748eb57b1220b657e59b2d4b9a94638298a419eb75`.
* The same mutated production code with the teardown-only harness, with the
  per-transition checker invocation removed, accepts the exact input in exit
  0 and 1 ms; log SHA-256 is
  `5c775bfafa1a021c8f4ff1bb9396c54dd4739aa972f4d6dc0b553f2ab1292f39`.
  This is the needed counterfactual: reset erases the damaged state before
  the old final checker runs. The production mutation was restored, the clean
  binary accepts the same input, and no deterministic regression test is
  claimed because master did not fail.

Bitcoin Core reachability and severity:

* `CCoinsViewCache` is the UTXO cache used by chainstate block connection and
  rollback. `AddCoin`, `BatchWrite`, and `Flush` participate in `UpdateCoins`,
  `Chainstate::ConnectTip`, and periodic state flushes. `Uncache` is also
  reached from mempool trimming and failed transaction/package cleanup. The
  proof input directly exercises the cache API, not a serialized block or a
  peer message, so it does not prove that an invalid block triggers anything
  on master.
* Clean master has no production failure, so this campaign reports no new
  bug, severity, or production fix. If the modeled missing dirty-count update
  existed in production, a Core-reachable cache accounting failure could be a
  high-severity node-availability or UTXO-state-integrity problem; that is
  mutation impact context, not a vulnerability claim against master. Invalid
  fuzzer state alone is not Critical, and a nonce with no cryptographic
  meaning is not Critical merely because it is uncleared.

Cherry-pick context and reiterated findings:

* No l0rinc fork commit applies specifically to this oracle, so no cherry-pick
  was made. This commit is oracle-only and preserves master behavior. Any later
  production fix, minor fix, oracle change, or cherry-pick must retain the
  exact corpus/input, mutation, assertion, stack/status, Core caller and input
  origin, test gap, severity, and verifier commands, and must say whether it
  preserves, changes, or masks this clean-master behavior.
* Existing findings remain master- and Core-caller-relative: private-broadcast
  failed-send retention is Medium and feature-conditional; empty HEADERS IBD
  handoff is Medium availability; peer activity refresh, process-message local
  storage failure, and oversized transport types are Low in current callers;
  ecmult scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979
  retention are Medium latent or hygiene findings with limited reachability;
  banman invalid-subnet integrity is Low/nice-to-have. Earlier
  txdownloadman, txrequest, connman, eviction, handshake, compact-block,
  headers-sync, coins-view, UTXO snapshot, mempool-persistence, package
  test-accept, and addrman campaigns found no additional clean-master
  production bug.

Verifiers: `git diff --check`; `ninja -C
/tmp/bitcoin-secp256k1-audit-current-build bin/fuzz`; the final 305-file
replay; the exact proof replay; and the four-worker command
`FUZZ=coinscache_sim /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
/mnt/my_storage/qa-assets/fuzz_corpora/coinscache_sim -runs=1 -jobs=4
-workers=4`. No fuzz process remains.

## TxGraph transition oracle audit (2026-07-20)

Source commit: `12cb1074b4` (`fuzz: check txgraph invariants between transitions`).
The change adds a bounded `TxGraph::SanityCheck()` after every 16 completed
operations in `src/test/fuzz/txgraph.cpp`. This checks staged locators,
clusters, indexes, and memory accounting before a later graph operation can
repair an intermediate defect. The final source commit is based on master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`, with source parent
`1ab6d0642f`.

Exact source and build identities:

* The original txgraph fuzzer SHA-256 is
  `fdb21547290053462afe0881f4c501828cea214888f5ddd19d8fe6e3a4fdc13f`;
  the final fuzzer SHA-256 is
  `d0f8c9d56fda42eb5287e3e203d24c94c9f1378f8a61dab5aa27e718395f41a2`.
* Clean production `src/txgraph.cpp` SHA-256 is
  `9d93e3b9c6ab32e71884493d6e5cac221f7e67354359dae47b3298a4e3371499`.
  The final sanitizer fuzz binary SHA-256 is
  `5f17468e54cca1d32b0c6d1d960f6f7315f0f9d188573a9e256c7375e93d259e`.
* The corpus source is `/mnt/my_storage/qa-assets/fuzz_corpora/txgraph`.
  The frozen copy is `/tmp/bitcoin-txgraph-20260720/frozen`, containing
  3234 files and 1466623 bytes. The sorted per-file manifest SHA-256 is
  `7e6085982c68cbfe3b740a2545fae1789ebd2e011e4fb1b1464cd16a28a60e5d`.

Baseline and replay evidence:

* The original execution-only baseline completed 4237 executions with
  coverage 10184, features 71213, peak RSS 539 MiB, exit 0, and no artifacts.
  Its log SHA-256 is
  `2c585cffa42f7067cfcf438770e48dfb9bee6ce80ba8463c7479b9312582fb39`.
* The enhanced clean replay completed 4237 executions with coverage 10187,
  features 71466, peak RSS 564 MiB, exit 0, and no artifacts. Its log
  SHA-256 is
  `0d73e1802ecba9136806dc454f2cfeb2c1ff5fa9df291bb36aa29ebcfc1588fc`.
* The four-worker command was:
  `FUZZ=txgraph /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
  -jobs=4 -workers=4 -max_total_time=60 -print_final_stats=1
  -artifact_prefix=/tmp/bitcoin-txgraph-20260720/worker-artifacts/
  /tmp/bitcoin-txgraph-20260720/frozen`.
  Jobs 0, 1, 2, and 3 each processed 4237 executions, reached coverage
  10187, exited 0, and reported peak RSS 564, 564, 564, and 563 MiB.
  Worker log SHAs are respectively
  `12428c8404caf9f653011cbd49ccb632185ea1cc91aded73a64739437c631908`,
  `dbca6a697091848347c2c89a1aa7b3c26fdacf8f4193db04162f7a6bd921b5c1`,
  `46e84f809a8459f4960e2e7e74186200e5911d85187cee8e7d86548f4aeef99f`,
  and `ff6f8e93976ab5f6d2ece574055a860c4a822afa26d49e87e1240e007aecefb5`.
  The parent log SHA-256 is
  `6398fd01bd2428cd2fe1ae4844c37719cddaf08568c10dad7bcdfca45538cbe4`.
  No worker produced an artifact or sanitizer diagnostic.

Differential mutation proof, not a master production finding:

* The temporary production mutation removed the line
  `clusterset.m_cluster_usage += cluster_ptr->TotalMemoryUsage()` from
  `TxGraphImpl::AddTransaction` near line 2252. Mutated production source
  SHA-256 is
  `15abf031f2926dda7b9a1cfbce876efb8ec812aa79564aa0615d704b7748438a`;
  the enhanced mutated binary SHA-256 is
  `01999abf4d2ea9f8563ee6d7c68f1512719b6f60cc7263717a9f593372d0ce3e`.
* The exact proof input is 34 bytes at
  `/tmp/bitcoin-txgraph-20260720/staging-reset-proof-final2`, SHA-256
  `b0971a90eaf3f179a6d99a641864a7d637912de41dd2af6aaf0675d6551e2ea2`.
  Its bytes are `38`, fourteen `10` bytes, `00 00 00 34`, and fifteen
  `00` bytes. Base64 is
  `OBAQEBAQEBAQEBAQEBAQAAAANAAAAAAAAAAAAAAAAAAAAA==`.
  Because `FuzzedDataProvider` consumes configuration and command data from
  the end, this is `StartStaging`, `AddTransaction` with size 1, fourteen
  data-free `GetTransactionCount` operations, then `CommitStaging`.
* With the enhanced oracle and the mutation, the proof exits 134 at the
  `TxGraphImpl::SanityCheck()` assertion
  `clusterset.m_cluster_usage == recomputed_cluster_usage` near line 3047.
  At transition 16, stored staging usage is 0 and recomputed usage is 72.
  The proof log SHA-256 is
  `0550695eead3de78fbbd96c8f3aaa3a8d3892e9068d9cd6fd504a22fe00a1b2b`.
* Clean master accepts the same proof with exit 0. The clean proof log
  SHA-256 is
  `b8444e3bae6ed6286f1c9738dda87e3eb20bfede229828f1a82c5764ac01edcc`.
* The legacy fuzzer with the cadence removed, the same production mutation,
  and the same proof exits 0. Its fuzzer source SHA-256 is the original
  `fdb21547290053462afe0881f4c501828cea214888f5ddd19d8fe6e3a4fdc13f`,
  its mutated binary SHA-256 is
  `ba3e8e684f519c3e98da1c4eb9e83e7e20bc2e0209668c464f41cf39b5426257`,
  and its proof log SHA-256 is
  `5c113074a35db1fade28cfb98ddd8c3ccee0a3af5a9302089d99b4ab39bedc7e`.
  `CommitStaging` repairs/removes the corrupted intermediate staging state
  before the old final check. The mutation was restored before the final
  clean build.

Bitcoin Core reachability and severity:

* Bitcoin Core uses `TxGraph` through `CTxMemPool::m_txgraph` in
  `src/txmempool.cpp` for transaction admission, dependency and fee updates,
  removals, `CommitStaging`, `Trim`, and `DoWork`. This proof exercises an
  internal mempool graph transition. It is not an invalid-block consensus
  path and does not establish that a peer-supplied invalid block triggers a
  master failure.
* Severity on master is none: clean master has no production failure, so no
  production bug, fix, or deterministic regression test is claimed. If the
  modeled missing accounting update existed in production, its impact would
  be a Medium availability/resource-accounting risk, not High or Critical,
  because the affected caller is mempool accounting rather than block
  validity or consensus. Invalid fuzzer state alone is not Critical.
* The prior ledger remains reiterated and master-relative: external block
  import, addrman, and coins-cache campaigns found no clean-master production
  bug; private-broadcast failed-send retention remains Medium and
  feature-conditional; empty HEADERS IBD handoff remains Medium availability;
  peer activity refresh, local process-message storage failure, oversized
  transport types, and banman invalid-subnet integrity remain Low or
  nice-to-have in current Core callers; latent crypto/hygiene findings remain
  reachability-limited. A nonce without cryptographic meaning is not a
  Critical clearing finding.

Cherry-pick context and test gap:

* No l0rinc fork commit applies specifically to this target, so no cherry-pick
  was made. A later production fix, minor fix, oracle change, or cherry-pick
  must retain the exact corpus/input, mutation, assertion, stack/status, Core
  caller and input origin, test gap, severity, and verifier commands, and must
  state whether it preserves, changes, or masks this clean-master behavior.
* There is no deterministic production regression test for this target: the
  proof is a deliberately mutated production build, while clean master
  succeeds. The committed change is an oracle-only harness improvement.

Verifiers: `git diff --check`; `cmake --build
/tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j4`; the clean proof
replay; the mutated enhanced proof; the legacy counterfactual; and the
four-worker command above. No fuzz process remains after the replay.

## P2P handshake completion oracle audit (2026-07-20)

Source commit: `6f79689339` (`fuzz: assert completed handshake version
contract`), based on master `18c05d93016b28a9afd4c716dfe00b6e0accb30b` and
source parent `12cb1074b46b13df0b0e6782bdf93575e3eafb14`. The harness now
checks immediately after every `ProcessMessagesOnce` transition that
`fSuccessfullyConnected` implies a completed VERSION/VERACK exchange and
negotiated protocol/common versions at least `MIN_PEER_PROTO_VERSION`.
Production behavior is unchanged.

Exact identities:

* Clean `src/net_processing.cpp` SHA-256:
  `afc14cf644760b60670fa82fb088b03ffa792d421a52c5f6c73b2e67672cf419`.
  Original and final harness SHA-256 values are respectively
  `329893f381ac1c7f7909852d53252bdb7811f65fce7fa9ec5708119e16bfff92` and
  `97f848223f9f2b543f7eeb1bc90f31a5e89ad26a5a470615aa69dece4d862971`.
  Original clean binary SHA-256 is
  `492ea119ae7bd6b1c81b4b92b1a6f59b4f92e4eea21a1b06387dd94c0e3793b9`;
  final enhanced binary SHA-256 is
  `83e6771828fd28ff3f1955d21749bfc689e40ad259e68b08838cd497d67a2301`.
* Frozen corpus is `/tmp/bitcoin-p2p-handshake-20260720/frozen`, copied from
  `/mnt/my_storage/qa-assets/fuzz_corpora/p2p_handshake`: 1062 files,
  855719 bytes, sorted per-file manifest SHA-256
  `a014406156c96cf3120f8699da9a4ed1c686f24e54c1e6c1ad01ab23ef1d3d0e`.

Clean replay:

* `-merge=0 -runs=1062 -timeout=60 -rss_limit_mb=0 -use_value_profile=1`
  with the original harness exited 0 after 1113 executions, coverage 4118,
  features 22809, peak RSS 466 MiB; log SHA-256
  `43c466a2c4dbe39acf8ea9d3dc753778e1ef7839bf1413fef42f1fa9afa29ef2`.
* The same command with the enhanced harness exited 0 after 1115 executions,
  coverage 4118, features 22508, peak RSS 466 MiB, with no artifacts; log
  SHA-256 `984e061e882b95fa32104212a3c6b351adf0cbc0d5bbe37cf3c6e7c27cc5952d`.
* `-jobs=4 -workers=4 -max_total_time=15` exited 0 in all four jobs with no
  artifacts or diagnostics: 16183 executions/502 MiB, 15984/503 MiB,
  15941/506 MiB, and 3721/500 MiB. Combined log SHA-256 is
  `7963fc099c1a9f5b5aa7f14e7f6b26bcb3436dd1b803a4920f834cf8dc689b67`.
  This mode used a disposable writable corpus copy; libFuzzer can add files,
  so the frozen copy was restored and its manifest rechecked afterward.

Differential proof, not a clean-master production finding:

* The exact temporary production mutation changed
  `if (pfrom.nVersion == 0)` to
  `if (pfrom.nVersion == 0 && msg_type != NetMsgType::VERACK)`, modeling a
  missing gate that lets VERACK complete a peer before VERSION. Mutated
  production SHA-256 is
  `46ea9131cf5cb590d3e5f04f08490860ffddd375677dfbd89626fdbefa89a220`; the
  enhanced mutated binary SHA-256 is
  `7ba4c92bd14186279df595aedfe8c9e2e4216ffea508eeef2c8cfdfb7eaec31c`.
* Enhanced mutation replay exited 134 after 144 executions at
  `p2p_handshake.cpp:118`, assertion `version_after >=
  MIN_PEER_PROTO_VERSION`. Log SHA-256 is
  `aebcacc0faf7d70358d651f671ffcf9f1b3310e99986d50639615336900ad0ee`.
  The proof artifact is 94 bytes, SHA-256
  `5aed5e1a8b965e8be4d4a2f7b48a7a9d7ed09f33c487f562823217e96cd39570`,
  Base64
  `qTFOU1wylFRcKKhcAGVcK5FckZGRLJGRkZGRkZGRkZGRkZGR8/f5NaMA/xkfnh8fiB8fr4gfH6+wrq+vUK+A+f//AAD8ADIAAAkC/j97IA0NQQAAAJFnRwFnr68wMA==`.
* The unchanged harness with the same mutation and exact artifact exited 0
  after one execution. Its mutated binary SHA-256 is
  `d712d07a109d14596bdb58bff9eec650848b0163c794797253376eef89122dfc` and
  log SHA-256 is
  `22b382334208b973b94e7e8ab0dcf82d3858f055c12b10ef71f58423d1a5b5fa`.
  Clean production plus the enhanced harness accepted the same artifact with
  exit 0; proof log SHA-256 is
  `b5ae81fd7628f2d03bbc51582758d01901206f54d4c98664e33f2bd09c1c9b53`.

Bitcoin Core reachability and severity:

* The production caller is `PeerManagerImpl::ProcessMessage`, reached from
  Bitcoin Core's network message processing for remote peer messages. The
  fuzzer uses `ConnmanTestMsg::ReceiveMsgFrom` and `ProcessMessagesOnce`, so
  the input origin is a remote message-ordering transition, not an invalid
  block. Ordinary tests and the old oracle did not exercise this modeled
  regression because their version assertion only ran when `nVersion` was
  nonzero.
* No production bug reproduces on clean master, so master severity is N/A and
  no production fix or deterministic regression test is claimed. If the
  mutated state gate existed in production, the impact would be a
  peer-handshake protocol-integrity/availability risk, tentatively Medium,
  not High/Critical. Invalid block bytes alone are not Critical; a nonce with
  no cryptographic meaning is not Critical merely because it is uncleared.

Cherry-pick and finding policy:

* `origin/master` and `remotes/l0rinc/master` were both
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; no additional relevant l0rinc
  commit applied. A later fix, minor fix, oracle change, or cherry-pick that
  changes the VERSION/VERACK gate must repeat the exact corpus, artifact,
  mutation, assertion, status/stack, Core caller/input origin, test gap,
  severity, and verifier commands, and state whether it preserves, changes,
  or masks this behavior.
* The reiterated ledger remains master- and Core-caller-relative: private
  broadcast failed-send retention is Medium and feature-conditional; empty
  HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and
  banman invalid-subnet integrity are Low or nice-to-have in current callers;
  ecmult scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979
  retention remain reachability-limited Medium latent or hygiene findings.
  The addrman, coins-cache, txgraph, txdownloadman, txrequest, connman,
  eviction, compact-block, headers-sync, UTXO snapshot, mempool-persistence,
  package test-accept, and prior handshake campaigns found no additional
  clean-master production bug.

Verifiers: `git diff --check`; `cmake --build
/tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j$(nproc)`; the
original/enhanced replays; exact mutated, legacy, and clean artifact replays;
and the four-worker command above. `clang-format --dry-run --Werror` still
reports pre-existing violations in the unchanged clock-consumption block at
`p2p_handshake.cpp:83-85`; no unrelated formatting was changed. No fuzz
process remained after the audit.

## BufferedFile position-contract oracle audit (2026-07-20)

Source commit: `e0c7faffd2` (`fuzz: assert BufferedFile position contracts`),
based on master `18c05d93016b28a9afd4c716dfe00b6e0accb30b` and source parent
`6f7968933991eb6c1551a695cf4a6bb3789a5d6b`. Production behavior is unchanged.
The harness now checks two concrete `BufferedFile` contracts after fuzzed
operations: `SetLimit(n)` succeeds exactly when `n >= GetPos()` and does not
move `GetPos()`, while a successful in-range `SetPos(n)` sets `GetPos()` to
exactly `n`. Failed `SetPos` remains tracked so the existing `FindByte` guard
continues to prevent the known infinite-loop state.

Exact identities and corpus:

* Clean `src/streams.h` SHA-256:
  `216fa033e467ecb86bdcf2db7ad14b0d81a0c85a9a3ba3d8eb2dde8138146fca`.
  Original and enhanced `src/test/fuzz/buffered_file.cpp` hashes are
  respectively
  `acd84995c88c560d7769be836b9da8a1e570445a32f3259cd367886c4979b52e` and
  `83d5253eef63cb1b1869d23a457b29db5e854fb3a4d1b2d52ca46512db89df4e`.
  Final enhanced fuzz binary SHA-256:
  `87922fc001598da25a4b52b5d835678d7b499f77a9791b0e756aa8a094415e85`.
* Frozen corpus: `/tmp/bitcoin-buffered-file-20260720/frozen`, copied from
  `/mnt/my_storage/qa-assets/fuzz_corpora/buffered_file`: 268 files and
  3877306 bytes. The sorted path-independent per-file manifest SHA-256 is
  `cd7bc11723fa433c97b190a73852a78e226ccd2c8250a0da7f8f4eb684596342`.

Baseline, clean replay, and workers:

* The original command was
  `env FUZZ=buffered_file /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
  -merge=0 -runs=268 -timeout=60 -rss_limit_mb=0 -use_value_profile=1
  -print_final_stats=1 -artifact_prefix=/tmp/bitcoin-buffered-file-20260720/original-artifacts/
  /tmp/bitcoin-buffered-file-20260720/frozen`. It exited 0 after 505
  executions with coverage 642, features 7225, peak RSS 412 MiB, and log
  SHA-256 `53d3b1924a3a75650867827712b6b4c293149c6648520ce2395be40f8e3c8a37`.
* The enhanced clean replay with the same command exited 0 after 505
  executions with coverage 643, features 7167, peak RSS 426 MiB, and no
  artifacts. Final log SHA-256:
  `0d238a55264f14a04caa7f6f0a3d79265322241d90f97c520f356f77026da931`.
* The required worker command was
  `env FUZZ=buffered_file /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
  -jobs=4 -workers=4 -max_total_time=15 -print_final_stats=1
  -artifact_prefix=/tmp/bitcoin-buffered-file-20260720/worker-artifacts/
  /tmp/bitcoin-buffered-file-20260720/workers-input`. Jobs 3, 0, 2, and 1
  exited 0 with 1153, 1192, 1230, and 1202 executions; each reached coverage
  643 and features 2663, with peak RSS 446, 448, 454, and 430 MiB. No worker
  produced an artifact or diagnostic. Parent log SHA-256:
  `4e6e4158b191d660840423af6a5fd1d97cece988fa87e12f71f45d08d61d007a`.
  Workers used a disposable writable corpus copy because libFuzzer can add
  files; the frozen corpus was rechecked afterward.

Differential proof 1, `SetLimit` boundary:

* The temporary production mutation changed `if (nPos < m_read_pos)` to
  `if (nPos <= m_read_pos)` in `src/streams.h`. Mutated header SHA-256 is
  `911de47dccfb26fb30f75c327f463d9f6957a8c30bf0365d30e77211051eb47e` and
  the enhanced mutated binary SHA-256 is
  `2f535dd910bcf0894ef7bbb526d6ab397f3cea8873a336a0c0a67938c1f62172`.
* The enhanced mutation replay exited 134 after 30 executions at
  `buffered_file.cpp:51` on `limit_set == (requested_limit >= current_pos)`.
  Log SHA-256:
  `2ab9b58a0420dae92887b241f2da32efc547b292783a4e1606b72e73e6597765`.
  Exact artifact:
  `/tmp/bitcoin-buffered-file-20260720/mutation-limit-enhanced-artifacts/crash-2606a323cb5d03711b993be047d9199ff66eb193`,
  19 bytes, SHA-256
  `3d2e15783f79794fc45be34691184d83b68f2d2aa6c6ae56aa6cfc2b6af98a51`,
  Base64 `KP+7u7u7u7tbu7sAAPcHPR5cEw==`.
* The old harness with the same mutation and exact artifact exited 0 after
  two executions; mutated binary SHA-256
  `af155ae3e6df8962d688f900059de6aa2fecec45580e2350170a1d7b049b74d6`, log
  SHA-256 `75a48e39afb3499bfd0f3c688e8da08aa4588a15142c4d99088aad9b4aceef63`.
  Clean production plus the enhanced harness accepted the artifact with exit
  0; clean proof log SHA-256
  `03fab03383d2548bd78ed873e98c6c29ed50e8caff72b2778072323d418b9b27`.

Differential proof 2, `SetPos` in-range exactness:

* The temporary production mutation changed `m_read_pos = nPos` to
  `m_read_pos = nPos + (nPos < nSrcPos)`. It leaves the position within the
  source range but moves one byte past every strictly in-range request.
  Mutated header SHA-256 is
  `378f12df017dc02f1daa7fb9ef86f044c0edda8ca408cc93843149fbcef7f292` and
  the enhanced mutated binary SHA-256 is
  `9bd338ec5a9867e6c0b346d7854f934b6a115074fcd81e90cad8f438d62711e6`.
* Enhanced mutation replay exited 134 after 209 executions at
  `buffered_file.cpp:59` on `GetPos() == requested_pos`. Log SHA-256:
  `1ef79852dbd172756d6d7809efb6ae3ef0477bd499a4c01a107958bcb44bda94`.
  Exact artifact:
  `/tmp/bitcoin-buffered-file-20260720/mutation-pos-inrange-enhanced-artifacts/crash-bb1511c98b4d70ece0cde7b253c340fa69e49f67`,
  8238 bytes, SHA-256
  `8f95c16ebcefaebd894da8c452c01b8c14a85f93f5eefa3cbde881594e46171d`.
* The old harness with the same mutation and exact artifact exited 0 after
  two executions; mutated binary SHA-256
  `ac0d2a71d666246aee4dcc081cc5bd195f62fff7072f94a2ab0dcd9de2d36cf0`, log
  SHA-256 `c4b046614a997aaef135d6d1df77c1d69dde5afc971ecd04108db9ee643e6bfe`.
  Clean production plus the enhanced harness accepted it with exit 0; the
  one-input clean replay log SHA-256 is
  `b50ef5a7927b87258035e68ebcf489aa99d642109f9e5f2da331f6f6a38636b5`.

Rejected mutation, retained to prevent overclaiming:

* The naive mutation `m_read_pos = nPos + 1` generated enhanced artifact
  `/tmp/bitcoin-buffered-file-20260720/mutation-pos-enhanced-artifacts/crash-efe9ad1bfd87b796aeabb85b8881275aaef361aa`, 103 bytes, SHA-256
  `b3d81282266fa82871b639c77be2b057ecdb40162c2973da520eef9bb1b19401`,
  enhanced log SHA-256
  `6401eef002d6d6c5a98db8328a59f1b3b82dc3eef928347b985ef515b36a7f67`.
  The old harness did not pass the artifact: it hit the pre-existing
  production assertion `streams.h:538` (`m_read_pos <= nSrcPos`), exit 134;
  old mutated binary SHA-256
  `491c2a5c7ed8529371c927a0ba36700c91f4daa47d7e5122f57573ea23bc9283`, log
  SHA-256 `e9e8503722e83ab41c327c985891696efe450b1bfbf6bbdf43e1fc0313b2d6c5`.
  This is rejected evidence, not a claimed finding for the committed oracle.

Bitcoin Core caller, input origin, and severity:

* `ChainstateManager::LoadExternalBlockFile` in `src/validation.cpp:4983`
  constructs `BufferedFile` for `-reindex` and `-loadblock` external blk.dat
  parsing and uses `SetPos`, `SetLimit`, `FindByte`, and `SkipTo` around
  `validation.cpp:5003-5057` before deserializing/importing candidate blocks.
  The fuzz input is a synthetic file/operation stream. This is not a
  peer-supplied invalid-block consensus path.
* Clean master has no production failure. Master severity is N/A: this is
  oracle-only hardening, with no production fix or deterministic regression
  test claimed. If either modeled defect existed in production, the plausible
  impact would be Medium external block-import/reindex parsing or availability
  failure, not High/Critical; no network invalid-block trigger or consensus
  invalidity impact was demonstrated. Invalid fuzzer state or invalid block
  bytes alone is not Critical. A nonce without cryptographic meaning is not a
  Critical clearing finding.
* The old harness discarded `SetLimit`'s return value and only recorded
  `SetPos` failure, so it missed both clean contract violations modeled above.

Cherry-pick and masking policy:

* `origin/master` and `remotes/l0rinc/master` both resolve to
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; no additional relevant l0rinc
  commit applies to `BufferedFile`, so none was cherry-picked. Any later
  production fix, minor fix, oracle change, or cherry-pick must repeat the
  exact corpus/artifact, mutation, assertion, status/stack, Core caller/input
  origin, test gap, and severity, and state whether it preserves, changes, or
  masks this clean-master behavior.
* Existing findings remain reiterated and master/Core-caller relative:
  private-broadcast failed-send retention is Medium and feature-conditional;
  empty HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and banman
  invalid-subnet integrity are Low or nice-to-have in current callers; ecmult
  scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979 retention
  remain reachability-limited latent/hygiene findings. No clean-master
  production bug was established in the previously audited addrman,
  coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
  compact-block, headers-sync, UTXO snapshot, mempool-persistence,
  package-evaluation, or handshake paths.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j4`; `git diff --check`; the original/enhanced frozen-corpus replays;
exact mutated, legacy, and clean artifact replays; and the four-worker command
above. `clang-format --dry-run --Werror` still reports the pre-existing
include-order violation at `buffered_file.cpp:5`; no unrelated formatting was
changed. No fuzz process remained after verification.

## P2P transport receive/session oracle audit (2026-07-20)

Source commit: `b25efc953d3d57daf9fd525148740945f6be730e` (`fuzz: assert
transport receive/session state contracts`), based on master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b` and source parent
`e0c7faffd297d69bbfdd76ce21125d4bfdd371f3`. Production behavior is unchanged.
The harness now checks that retrieving a completed message clears readiness,
and that a completed V2 exchange reports V2 plus a non-null session ID while
V1 and V1 fallback report V1 with no session ID. These are narrow public
Transport contracts, not blanket accepted-message assumptions.

Exact identities and corpora:

* Clean `src/net.cpp` SHA-256:
  `cb55514eeaf4ce16336675d9bdca55c24b9c3fc7dc0685c0cb9d6fd12e3363a9`.
  Original and enhanced `src/test/fuzz/p2p_transport_serialization.cpp`
  hashes are respectively
  `2cd26f3fa04ba0c5049e9e431de6bdaa8826ca3a3b62c44be34c5262fa447dde` and
  `2af472aaff5bf43ed97ce56541a16eb9edde94739aa5fbe4527dd5fb06455631`.
  Final enhanced sanitizer fuzz binary SHA-256:
  `768550809b7ad90ba7bcb6c683bcd3c85bbed77dffc14527ded1ed79c71d1972`.
* V1 corpus: `/tmp/bitcoin-p2p-transport-serialization-20260720/frozen`,
  copied from `/mnt/my_storage/qa-assets/fuzz_corpora/p2p_transport_serialization`:
  292 files, 24999219 bytes, sorted path-independent manifest
  `fb1dca3d34a2558a8e41e2cf1c2ad133c5d1665dd8943a52bbf750c240e79d47`.
* V2 corpus: `/tmp/bitcoin-p2p-transport-serialization-20260720/v2-frozen`,
  copied from `/mnt/my_storage/qa-assets/fuzz_corpora/p2p_transport_bidirectional_v2`:
  1507 files, 2255646 bytes, sorted path-independent manifest
  `38f6814040749148d27609a5919955324e753d64ad5adcdf9bfea02b77a8beb8`.
  The frozen manifest remained unchanged after all runs.

Clean replays and workers:

* The V1 original replay (`-merge=0 -runs=292 -timeout=60
  -rss_limit_mb=0 -use_value_profile=1`) exited 0 after 293 executions,
  coverage 476, features 6510, peak RSS 304 MiB, with no artifacts. Log
  SHA-256: `8da09079c4fc695bdbe83870958924e3518000906b676da186f5e26a814056f5`.
  The enhanced replay exited 0 after 293 executions, coverage 476, features
  6561, peak RSS 311 MiB, with no artifacts. Log SHA-256:
  `3ce6e8a5ede3f9371692bc87068ac4dfc168109cbadf94eb01740a9fa1c6f4cf`.
* The V2 original replay (`-merge=0 -runs=1507` with the same sanitizer and
  value-profile options) exited 0 after 1508 executions, coverage 6604,
  features 34013, peak RSS 792 MiB, no artifacts. Old clean binary SHA-256
  `bb104bed40a1689464db1d1d2e1d5e5141195f0c2eade86a978653b1039c670f`; log
  SHA-256 `64373d6540019ca463d77fc57c93a058a0b39eb465285eee37df1c8e7c47d4cc`.
  The enhanced replay exited 0 after 1508 executions, coverage 6608,
  features 34193, peak RSS 797 MiB, no artifacts. Log SHA-256:
  `8bbab9794b946ca3be6f66837fdec495bde684a9a69308ff9775b4e74b4bbf46`.
* The four-worker command was
  `env FUZZ=p2p_transport_bidirectional_v2 /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
  -jobs=4 -workers=4 -max_total_time=15 -print_final_stats=1
  -artifact_prefix=/tmp/bitcoin-p2p-transport-serialization-20260720/v2-worker-artifacts/
  /tmp/bitcoin-p2p-transport-serialization-20260720/v2-workers-input`.
  All jobs exited 0 without artifacts or sanitizer diagnostics. Jobs 0, 1,
  2, and 3 processed 1632, 1628, 1664, and 1668 executions; all reached
  coverage 6608 and features 20485; peak RSS was 739, 734, 745, and 736 MiB.
  Worker log SHA-256:
  `27051828ec03e4c78d8f2ad12fef43496487bddee6acc6690ea195ad21b3e444`.
  The writable worker copy grew to 1509 files/2259969 bytes with manifest
  `ef652d358a65803d57e2c897e6ae050f907edd3fd632c1760ec0ab4d5d8d2e4a`;
  it was disposable and did not replace the frozen corpus.

Differential proof 1, V2 session reporting:

* The temporary production mutation added `m_recv_state != RecvState::APP`
  to the `V2Transport::GetInfo` state gate, modeling a completed APP session
  being reported as DETECTING with no session ID. Mutated `src/net.cpp`
  SHA-256:
  `2a679c5d0a11ea09e1364d389ff523b3755bbe1664b3663a69594afffff23c2b`.
  Enhanced mutated binary SHA-256:
  `e103f2d72c0cbfb87b33c69c5293c6f2054898eb79283bc05e215e11e0ccf406`.
* The enhanced V2 mutation replay exited 77 after 209 executions at
  `p2p_transport_serialization.cpp:340`, assertion
  `info0.transport_type == TransportProtocolType::V1`. Log SHA-256:
  `5f481ed7e80d06959d3a1340aeba50f12d48f99b516c7239909722d5161f5d93`.
  Artifact:
  `/tmp/bitcoin-p2p-transport-serialization-20260720/mutation-info-v2-artifacts/crash-2d2517af50ebf0b33b0c859b377fa29765f8a247`,
  76 bytes, SHA-256
  `900ad986195e4cc8e852bbdccad4f3201977feb0a0e0f676cfcaab31fdd7af1c`,
  Base64
  `cYAAAAAAAN3d3d3d3d3d3d3d3Snd3d3dFd3d3d3d3d3d3d3dAAAAPt3d3d3d3d3d3d3d3d3d3d3d3Wkw3d173d3dE/8AAADygAAAAA==`.
* The original harness with the same mutation and exact artifact exited 0.
  Its mutated binary SHA-256 was
  `e9ca597124cdd2074fa6c2d6179a25e0806de85056a44054d840a9a17c044644` and
  its log SHA-256 was
  `ce843deb36d41cab0489d732556a193b5c38d03448f4e5deda0a1df6767ec76b`.
  Clean production plus the enhanced harness accepted the same artifact with
  exit 0; clean proof log SHA-256
  `6d10a6bc1366550352c07fc2dd55d1247b0df15cf5af649b7dd72726867bcf6f`.
  A V1-only replay did not exercise this V2 mutation and is not treated as
  evidence; the proof domain is the V2 corpus.

Differential proof 2, V2 receive-state reset:

* The temporary production mutation removed
  `SetReceiveState(RecvState::APP)` after `ClearShrink(m_recv_decode_buffer)`
  in `V2Transport::GetReceivedMessage`. Mutated `src/net.cpp` SHA-256:
  `ddef2e4a249724d9accf1645e40039d0d2f59cbd10faf26bcf87e96a18e02d3b`.
  Enhanced mutated binary SHA-256:
  `c9fe836cefe03acd8e8979ffbc26d010a81b16c54f36e968a0d313d53d3848b7`.
* The enhanced V2 mutation replay exited 77 after 534 executions at
  `p2p_transport_serialization.cpp:285`, assertion
  `!transports[!side]->ReceivedMessageComplete()`. Log SHA-256:
  `b575c14b3309d6a3fc4d9051478da1ba26516a95884a854c0f190c85edb9ad8f`.
  Artifact:
  `/tmp/bitcoin-p2p-transport-serialization-20260720/mutation-reset-v2-artifacts/crash-87ef6d0f4af3715f5642fc2fd081ef33105bd0d9`,
  157 bytes, SHA-256
  `c1f5e991be59d4d75ee6b8e0cbf0955e158c612486ed3d551122d0918477e98d`,
  Base64
  `n+eV5/OggYIhAELCwsLCvcL0GBoAAAAAAAAAAQAAAAAAAAzk///CQsLCwsK9wvQYGBgYGP/IyMjIGcIBCCEAKAADEgADEgDj4+PjAAAADOT/+r+12nZlcnNpb24AAAAAAP/CQsLCwsK9wvQYGBgYGP/IyMjLGQEAAAAAAAAA/+Nz46vjJ8LEwsIhAAAtMQAAAAAAACYBBwAAwsLCwg==`.
* The original harness with the same mutation and exact artifact exited 0.
  Its mutated binary SHA-256 was
  `de656a1f0becd820ad9f05f3805767e7e1a6b7e6d667193fa5603101663de0f6` and
  its log SHA-256 was
  `98e870ed6c018c3ed09b2ba57d2644cccc30078453fe290306a61fe0ff675787`.
  Clean production plus the enhanced harness accepted the artifact with exit
  0; clean proof log SHA-256
  `076d3f9a2097475a4b39fc8dde95a26d760bf7cf79236d906ce1da2401100533`.
  V1 uses a separate `Reset()` implementation, so the V2 target is the
  exercising domain for this mutation.

Bitcoin Core caller, input origin, and severity:

* Remote peer wire bytes reach `CNode::ReceiveMsgBytes` in `src/net.cpp:668-690`,
  which calls `Transport::ReceivedBytes` and then `GetReceivedMessage`. This is
  a real network input path, not synthetic block validation.
* `Transport::GetInfo` is consumed by `CNode::CopyStats` at
  `src/net.cpp:622-654`, `CConnman::GetNodeStats`, and `getpeerinfo` at
  `src/rpc/net.cpp:217` and `308-309`. Before `fSuccessfullyConnected`,
  `src/net.cpp:2081-2087` also uses DETECTING to label a handshake timeout.
  Clean master has no failure: master severity is N/A. The modeled session
  reporting defect would be Low/informational, not a consensus, invalid-block,
  or Critical issue.
* The reset mutation affects a real remote state transition. If present in
  production, a subsequent packet could reach
  `ProcessReceivedPacketBytes` with `APP_READY` instead of `APP` and violate
  the `Assume` at `src/net.cpp:1218`. The differential proves the oracle catches
  the contract, but the legacy harness did not reproduce a daemon crash and no
  daemon-level crash is claimed. Counterfactual impact is tentatively Medium
  remote-connection availability, not High/Critical on this evidence.
* No production bug reproduces on clean master, so this is oracle-only
  hardening with no production fix or deterministic regression test. Invalid
  fuzzer state or invalid block bytes alone is not Critical. A nonce without
  cryptographic meaning is not a Critical clearing finding.

Cherry-pick and masking policy:

* `origin/master` and `remotes/l0rinc/master` both resolve to
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; no additional relevant l0rinc
  commit applies to this transport target, so none was cherry-picked. Any
  later fix, minor fix, oracle change, or cherry-pick must repeat the exact
  target/corpus/artifact, mutation, assertion, stack/status, Core caller/input
  origin, test gap, severity, and verifier commands, and state whether it
  preserves, changes, or masks this clean-master behavior.
* Existing findings remain reiterated and master/Core-caller relative:
  private-broadcast failed-send retention is Medium and feature-conditional;
  empty HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and banman
  invalid-subnet integrity are Low or nice-to-have in current callers; ecmult
  scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979 retention
  remain reachability-limited latent/hygiene findings. No clean-master
  production bug was established in the previously audited addrman,
  coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
  compact-block, headers-sync, UTXO snapshot, mempool-persistence,
  package-evaluation, handshake, or BufferedFile paths.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j4`; `git diff --check`; original/enhanced V1 and V2 frozen-corpus
replays; exact mutated, legacy, and clean artifact replays; and the four-worker
V2 command above. `clang-format --dry-run --Werror` still reports pre-existing
violations at `p2p_transport_serialization.cpp:69, 108, 177, 300-309, 361,
388-389`; no unrelated formatting was changed. No fuzz process remained after
verification.

## `block_index_tree` oracle

Source commit `22d2be54d5` (`fuzz: assert block index insertion contracts`)
adds an operation-local postcondition to `src/test/fuzz/block_index_tree.cpp`.
Before `BlockManager::AddToBlockIndex`, the harness snapshots
`ChainstateManager::m_best_header`. It then requires the inserted entry to
become the best header exactly when its chain work is greater, and requires the
old pointer to remain unchanged otherwise. The existing `BLOCK_VALID_TREE` and
parent-link assertions remain in place. This makes a bad best-header transition
fail at the insertion site instead of waiting for the later O(tree)
`CheckBlockIndex()` pass.

Audit identity and clean behavior:

* Core base master was `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; the source
  parent was `b25efc953d3d57daf9fd525148740945f6be730e`. Production behavior is
  unchanged.
* Clean `src/node/blockstorage.cpp` SHA-256:
  `2e5c4c9dc606e1cf42d81dc4fd6bba6a3edfb97f7f857422af96c828f09246b`.
  Original harness SHA-256:
  `035b02980d05cac97fe43cd76be4a70f99e5075df0554fac9c28008b394b638a`.
  Enhanced harness SHA-256:
  `1015d4afaddfac50173cae2fdb4a51fdd8c55cb4ad1d1b5a53a9b46366514382`.
  Final enhanced sanitizer fuzz binary SHA-256:
  `52bf6b3f18b1e9409b5b084218adcaa1db31110fc0c8351a1ebe82203fbd1eeb`.
* The frozen corpus is
  `/tmp/bitcoin-block-index-tree-20260720/frozen`, copied from
  `/mnt/my_storage/qa-assets/fuzz_corpora/block_index_tree`. It contains 4,396
  files and 19,764,724 bytes. Its sorted path-independent manifest is
  `d8a53adaa94c902d29021ccc88e3c2042b9e91ea98ec798e91def650ec32da09`.
  The frozen directory was not modified by fuzzing.
* The original `-merge=0 -runs=4396 -timeout=60 -rss_limit_mb=0
  -print_final_stats=1` replay exited 0 after 4,541 executions, with cov
  2,181, ft 20,544, and peak RSS 629 MiB. It produced no artifacts. Log
  SHA-256: `6fbb5fa7d09c00f7930ae1a6e9e288a10469602bb5b0242a098c17758b689c44`.
* The enhanced replay with the same command exited 0 after 4,542 executions,
  with cov 2,188, ft 14,354, and peak RSS 631 MiB. It produced no artifacts.
  Log SHA-256:
  `005263a7b00b680b2d25b341cd1c5a76a5d94a577660060115569f3583e3a90d`.

Multi-worker evidence:

`env FUZZ=block_index_tree /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
-jobs=4 -workers=4 -max_total_time=15 -timeout=60 -rss_limit_mb=0
-print_final_stats=1
-artifact_prefix=/tmp/bitcoin-block-index-tree-20260720/enhanced-worker-artifacts/
/tmp/bitcoin-block-index-tree-20260720/enhanced-workers`

All four jobs exited 0 with no artifacts or sanitizer diagnostics. Jobs 0, 1,
2, and 3 processed 4,539, 4,541, 4,537, and 4,539 executions; each reached
cov 2,188 and ft 14,363, 14,355, 14,355, and 14,364; peak RSS was 632, 626,
631, and 631 MiB. Worker log SHA-256:
`6a97ad2912342c372c72e086dfd89777976ad174a753568844ef8d39e199e4de`. The
disposable worker corpus remained at 4,396 files and 19,764,724 bytes, and no
fuzz process remained afterward.

Differential proof and its limitation:

* A temporary production mutation changed
  `best_header->nChainWork < pindexNew->nChainWork` to `>`, modeling a
  comparator-direction regression. Mutated `src/node/blockstorage.cpp`
  SHA-256: `72c32558c15df785be91677d61f0864254ff4f2f649f015ee986f9f7442d2d1e`.
  Enhanced mutated binary SHA-256:
  `65290e0164ac8c46ccde12898600b719a5051043e58e0be844467dc1884600b8`.
* The enhanced mutation replay exited 77 after 21 executions at
  `block_index_tree.cpp:86`, assertion `chainman.m_best_header == index`.
  Log SHA-256:
  `5684635edb4f27d211ece0d2d8446782b8a3c60528043e4fccbd8a30f50b9b55`.
  The exact artifact is
  `/tmp/bitcoin-block-index-tree-20260720/mutation-best-header-artifacts/crash-fb565ba0bf05cdac6b755cd350604b9be1073078`,
  5 bytes, SHA-256
  `49bdc48359856c9245421c6241722b232a06fa48fa433c3c6c7c61216621b4db`,
  Base64 `////AQU=`.
* The original harness with the same mutated production and exact artifact
  also failed, but later at the existing production assertion
  `validation.cpp:5315` in `ChainstateManager::CheckBlockIndex()`. It exited
  134 with no libFuzzer artifact. The old harness SHA-256 was
  `035b02980d05cac97fe43cd76be4a70f99e5075df0554fac9c28008b394b638a`, the
  old mutated binary SHA-256 was
  `79963b5b9b0ccbd5b4cba3c05db45abb0e8e1fccce538cea8d754a53801a6877`, and
  the control log SHA-256 was
  `b44a1e37c6b4ee06f73eff4afdcde813a8174cb4bb8ef7e07e40388ef5a0294a`.

This is therefore not a newly exposed production bug. The assertion is a
stronger and earlier oracle, but the modeled comparator defect is redundant
with the legacy structural checker. Clean production plus the enhanced
harness passed the full frozen corpus, so no production fix or deterministic
regression test is claimed.

Bitcoin Core reachability and severity:

* Remote header processing enters at
  `PeerManagerImpl::ProcessHeadersMessage` (`src/net_processing.cpp:3007`),
  calls `ProcessNewBlockHeaders` (`:3166`), then
  `AcceptBlockHeader` (`src/validation.cpp:4202`) and
  `AddToBlockIndex` (`src/node/blockstorage.cpp:224`). The same transition is
  also used by local reindex/loadblock processing and genesis initialization.
* The fuzzer directly constructs mocked-valid headers and calls
  `AddToBlockIndex`; it does not establish that arbitrary invalid block bytes
  can reach this production transition. Clean master has no production
  failure, so master-relative severity is N/A and this is Low/informational
  oracle hardening. The modeled comparator regression would affect best-header
  selection, but Core's existing `CheckBlockIndex()` already catches it. No
  High/Critical or invalid-block vulnerability is claimed.
* Existing findings remain reiterated and Core-caller relative: private
  broadcast failed-send retention is Medium and feature-conditional; empty
  HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and banman
  invalid-subnet integrity are Low or nice-to-have in current callers; ecmult
  scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979 retention
  remain reachability-limited latent/hygiene findings. Invalid fuzzer state or
  invalid block bytes alone is not Critical. A nonce without cryptographic
  meaning is not a Critical clearing finding.

Cherry-pick and masking policy:

`origin/master` and `remotes/l0rinc/master` both resolve to
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; no relevant l0rinc commit applies
to this target, so none was cherry-picked. Any later production fix, minor
fix, oracle change, or cherry-pick must repeat the exact target,
corpus/artifact, mutation, assertion, status/stack, Core caller/input origin,
test gap, severity, and verifier commands, and state whether it preserves,
changes, or masks this clean-master behavior.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j4`; `git diff --check`; original and enhanced frozen-corpus replays;
exact enhanced mutation and legacy control replays; and the four-worker
command above.

## `tx_pool` accepted-result oracle

Source commit `848eb7b5e9` (`fuzz: assert accepted transactions remain
indexed`) strengthens the stateful `tx_pool` target. After
`AcceptToMemoryPool` returns `VALID`, the harness requires both the txid and
wtxid to be present in the same `CTxMemPool`. It deliberately does not assert
membership for rejected results, replacements, or other result types. This is
a postcondition on the production API result, not an assumption that an
accepted-looking input is valid.

Master and clean identities:

* Core base master is
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; the source parent is
  `22d2be54d5cff17ea9fda9fc6c3cc09547f45c05`. Production behavior is
  unchanged.
* `origin/master` `src/validation.cpp` SHA-256 is
  `1b49bd6539860b5e06c93e5ec73a2735ca4521fb8c28fcd4b50268c44a0018ab`.
  The clean audit branch hash is
  `6f00f58f3cc9623fb02cfa8c776654e617b6a88e87c2b075bb855116b9ebfefc`; its
  only difference from master is the previously recorded `fNewBlock` storage
  ordering fix outside `AcceptToMemoryPool`. Clean `src/txmempool.cpp` is
  `4d011a74b788983200916cc4b467813e8d0e62334118c67a2ffb390a87b3bbff`,
  byte-identical to master.
* Original harness SHA-256:
  `807e5b8bd2e4dde1f23155e13cfb2f47e78e45a962da10aa4187f01a3355f993`.
  Enhanced harness SHA-256:
  `6393553562a8ed9c41fd4f87bf156540a4924d16e04db3610ae289570c03fbc0`.
  Final enhanced sanitizer binary SHA-256:
  `9fec9e5287b7f70b270bb5a0327b41e8371d80c6f718e42df0e49def271e9965`.

Frozen corpus and clean replay:

* The corpus is `/tmp/bitcoin-tx-pool-20260720/frozen`, copied from
  `/mnt/my_storage/qa-assets/fuzz_corpora/tx_pool`. It has 5,658 files and
  41,820,925 bytes. The sorted filename/content manifest SHA-256 is
  `3fc9394c051dabb5fa79fa942d84c48913a1ca8639c915e59bcae918a26b6689`.
  The frozen directory was not modified.
* The original `-merge=0 -runs=5658 -timeout=60 -rss_limit_mb=0
  -print_final_stats=1` replay exited 0 after 5,660 executions, cov 19,672,
  ft 117,570, peak RSS 541 MiB, with no artifacts. Log SHA-256:
  `5690c2e30c7b08f4fffa6481d9627b1f7408705b86678079f08967e42fa38e0f`.
* The enhanced replay with the same command exited 0 after 5,660 executions,
  cov 19,677, ft 117,579, peak RSS 546 MiB, with no artifacts. Log SHA-256:
  `6d6634ebb18becdccc61db9b7aa278011e948e5c2e6fefb9002445ac3860632d`.

Multi-worker evidence:

`env FUZZ=tx_pool /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz
-jobs=4 -workers=4 -max_total_time=15 -timeout=60 -rss_limit_mb=0
-print_final_stats=1
-artifact_prefix=/tmp/bitcoin-tx-pool-20260720/enhanced-worker-artifacts/
/tmp/bitcoin-tx-pool-20260720/workers`

All four jobs exited 0 with no artifacts or sanitizer diagnostics. Jobs 0, 1,
2, and 3 each processed 5,660 executions, all reached cov 19,677, ft was
117,580, 117,578, 117,569, and 117,574, and peak RSS was 545, 546, 546, and
545 MiB. Worker log SHA-256:
`52da96043f4aba3af4a1e717fed87b638856e8ab5d5262e85e91ca5ccc491934`. The
disposable worker corpus remained at 5,658 files and 41,820,925 bytes, and no
fuzz process remained afterward.

Differential proof:

* A temporary production mutation was inserted after
  `AcceptSingleTransactionAndCleanup` returned: for a `VALID` result, lock
  the pool and call `removeRecursive(*tx, MemPoolRemovalReason::EXPIRY)` before
  returning the still-`VALID` result. This models an API result/state
  divergence after acceptance. Mutated `src/validation.cpp` SHA-256:
  `7d861abff06ed59852f96f38a4361371bbb4c374ac906f95ec7a9518a4202b9f`.
  Enhanced mutated binary SHA-256:
  `0cf882087398839e726903b17b1908b2a03014e67d2e0feda5a046e88154a47b`.
* The enhanced mutation replay exited 77 after 24 executions at
  `tx_pool.cpp:481`, assertion `tx_pool.exists(tx->GetHash())`. Log SHA-256:
  `d1c776e230f284dff96f98018c3827eca66dc833549d195be5d71d049c8ce64f`.
  The exact artifact is
  `/tmp/bitcoin-tx-pool-20260720/mutation-artifacts/crash-c956a7bba2210989971d1d5709674218a56eb6fb`,
  152 bytes, SHA-256
  `7e9d24d50b7cb88d06581c72251271d473e514da0729f90f5a64de85a102f93b`,
  Base64
  `QQAAAACgplVVjiQAVVVVVVVVVbFVVVVVVVVVVVVVVVVTVf///09VVVVSVVVVVVVVVf////////////////////////////////////////9xAAAEZGF0YWPhcn5yaWVyc2ktLS0tLdotLS0tLS0tLS1dLS0tLS0tLS0tLS0tLS0tLS0Avt1DQwFDQ0NDQ/4FAE/1TERDsbU=`.
* The original harness with the same mutated production and exact artifact
  exited 0. Its SHA-256 was
  `807e5b8bd2e4dde1f23155e13cfb2f47e78e45a962da10aa4187f01a3355f993`;
  mutated legacy binary SHA-256 was
  `b25d2714a680d7bb236df972ac3184424e88ab585cde85899bdde5f722b5e1a7`; and
  control log SHA-256 was
  `c63ab3ca356c5cb136988d2654e61f64af31e4bb780dd11f4e4d5ec0d3231ee7`.
* Clean production plus the enhanced harness accepted the exact artifact with
  exit 0; clean exact log SHA-256:
  `267f8aea8d7f517139c304d9aab81fa5a313b22fc89cd329954733b6b0e1955c`.
  Clean production plus the original harness also accepted it with exit 0;
  clean legacy binary SHA-256 was
  `611b2f9d49cca909ee0e21d76f26fa99d5767151f3cff5c12b7ffae7eb2c374e`, and
  clean legacy log SHA-256 was
  `ed8532d3c63f4a1f495d0b1c359bb165eb51f4cd4dd3b999c2e21291e9c397da`.

This is an oracle-only change: clean master reproduces no failure, so no
production fix or deterministic regression test is claimed.

Bitcoin Core caller and severity:

* A remote TX message reaches `PeerManagerImpl::ProcessMessage`
  (`src/net_processing.cpp:4597`), which calls
  `ChainstateManager::ProcessTransaction` (`src/validation.cpp:4480`) and
  `AcceptToMemoryPool` (`src/validation.cpp:1782`). Orphan release at
  `src/net_processing.cpp:3315` and package processing at `:3260` also route
  accepted results through `ProcessValidTx`.
* `ProcessTransaction` checks the mempool after the API returns, but a stale
  `VALID` result can still reach `ProcessValidTx`, which updates peer activity
  and relay bookkeeping. The fuzzer calls the lower-level API directly so the
  result/state contract is checked at its boundary; the mutation does not
  claim that a remote peer can directly delete a transaction after acceptance.
* Clean master has no production failure. Master-relative severity is N/A and
  this is Low/informational oracle hardening. If the modeled divergence
  existed, impact would be limited to mempool acceptance/reporting, peer
  activity, and relay bookkeeping, not consensus, block validation, memory
  safety, or a Critical invalid-block path. Invalid block bytes alone are not
  Critical. A nonce without cryptographic meaning is not a Critical clearing
  finding.
* Existing findings remain reiterated and Core-caller relative: private
  broadcast failed-send retention is Medium and feature-conditional; empty
  HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and banman
  invalid-subnet integrity are Low or nice-to-have in current callers; ecmult
  scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979 retention
  remain reachability-limited latent/hygiene findings. No clean-master
  production bug was established in the previously audited addrman,
  coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
  compact-block, headers-sync, UTXO snapshot, mempool-persistence,
  package-evaluation, handshake, BufferedFile, or block-index paths.

Cherry-pick and masking policy:

`origin/master` and `remotes/l0rinc/master` both resolve to
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; no relevant l0rinc commit applies
to this target, so none was cherry-picked. Any later fix, minor fix, oracle
change, or cherry-pick must repeat the exact target/corpus/artifact, mutation,
assertion, status/stack, Core caller/input origin, test gap, severity, and
verifier commands, and state whether it preserves, changes, or masks this
clean-master behavior.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j4`; `git diff --check`; original/enhanced frozen-corpus replays; the
four-worker command; and exact enhanced-mutated, legacy-mutated,
enhanced-clean, and legacy-clean artifact replays.
