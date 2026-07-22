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

## `policy_estimator_io` read/write state oracle

Source commit `4cb03a5556` (`fuzz: assert policy estimator read/write state
contracts`) strengthens `policy_estimator_io`. The original target discarded
both `CBlockPolicyEstimator::Read` and `Write` results and checked no state
after either operation. The new `CaptureEstimatorState` helper serializes the
estimator through its public const `Write` API into a temporary file. After a
failed `Read`, the exact serialized state must equal the pre-read state. After
a successful `Read`, `Write` must not change the serialized state. The target
does not assume rejected bytes are valid, or that a failed read must accept a
particular encoding. This mirrors the production contract documented at
`src/policy/fees/block_policy_estimator.cpp:1012-1013`: parsing uses temporary
variables so an exception cannot corrupt existing structures.

Master and clean identities:

* Core base master is
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; source parent is
  `848eb7b5e9fc5357007e951133d213ceae2f244b`. The production implementation
  is unchanged and matches master.
* `origin/master`
  `src/policy/fees/block_policy_estimator.cpp` SHA-256 is
  `d5642f08207163c0d442bcbf077ab4321189f8472a5cfa851f70d0ceb92b69c3`; the
  clean audit branch has the same hash. The unchanged header SHA-256 is
  `7435bb720c5c861ad8fdd440c9e3df8e107f8bd983a7563ad7f61abde6c22f45`.
* Original harness SHA-256:
  `c7eb2e1f6ce776d872604927a5375e09432b479948b95f0c95054416b3256eac`.
  Enhanced harness SHA-256:
  `336876e2e88b661c568edeb5c774d35d1a960f0048e49b9d8422c13cd0c12fee`.
  Final clean sanitizer fuzz binary SHA-256:
  `4013d0cd770d51aec62e0133e5c388d07499a63cac79840214461184a76fee0a`.

Frozen corpus and clean runs:

* Target: `policy_estimator_io`.
* Frozen directory: `/tmp/bitcoin-policy-estimator-io-20260720/frozen`.
* Source: `/mnt/my_storage/qa-assets/fuzz_corpora/policy_estimator_io`.
* 238 files and 5,777,773 bytes. Sorted filename/content manifest SHA-256:
  `daf58fbb3745f279b9b9569facaaf0186ab6adb5bbe34dfa9797163e54ba5072`.
  The frozen directory was not modified by fuzzing.
* Original replay used `-merge=0 -runs=238 -timeout=60 -rss_limit_mb=0
  -print_final_stats=1`. It exited 0 after 257 executions, cov 757, ft 2270,
  peak RSS 475 MiB, with no artifacts. Log SHA-256:
  `edd689c9da8ebf38a669e44304f308ca871390c2ea3f4f8ba4c5c9713ed207f7`.
* Enhanced replay with the same command exited 0 after 254 executions,
  cov 784, ft 2385, peak RSS 498 MiB, with no artifacts. Log SHA-256:
  `0475a5872873226a296154c33e51fcc37d3ffeb7d85101096057c4899da1ae23`.

Multi-worker evidence:

```text
env FUZZ=policy_estimator_io /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
  -jobs=4 -workers=4 -max_total_time=15 -timeout=60 -rss_limit_mb=0 \
  -print_final_stats=1 \
  -artifact_prefix=/tmp/bitcoin-policy-estimator-io-20260720/enhanced-worker-artifacts/ \
  /tmp/bitcoin-policy-estimator-io-20260720/workers
```

All four jobs exited 0 with no sanitizer diagnostics or artifacts. Jobs 0, 1,
2, and 3 processed 3,187, 3,087, 4,215, and 4,127 executions; each reached
cov 784; ft was 2,442, 2,441, 2,438, and 2,442; peak RSS was 499, 517, 499,
and 514 MiB. Parent worker log SHA-256:
`2b211ed06f4c56ef5a2dedb70434b93946365d69165508bb1c9330c58b3d1818`.
The disposable worker corpus grew from 238 files/5,777,773 bytes to 323
files/8,124,936 bytes; the frozen corpus remained unchanged and no fuzz
process remained afterward.

Differential proof that the oracle matters:

* Temporary production mutation: immediately before the `Read` catch block's
  `return false`, insert `buckets.clear()`. This models the exact class of
  bug the production comment forbids: an exception path changes persistent
  estimator state even though `Read` reports failure. Mutated production
  source SHA-256:
  `315ebce491bda45a55f9d1130605f81ac0dfe6760053d507c6f997d245df142a`.
  Enhanced mutated binary SHA-256:
  `c310aca5df4e0b0a98c669a1d59235b3fb80c5dae6b507191ee8ab2c58091432`.
* The frozen replay exited 77 after one execution at
  `src/test/fuzz/policy_estimator_io.cpp:51`, asserting
  `CaptureEstimatorState(block_policy_estimator) == before_read`. Log
  SHA-256:
  `813e06d428e561f03c17c58b5ce664149b53b9bf5cc2e54cfdd27f2a1854d511`.
* The exact minimized artifact is the empty file at
  `/tmp/bitcoin-policy-estimator-io-20260720/mutation-artifacts/crash-da39a3ee5e6b4b0d3255bfef95601890afd80709`,
  size 0, SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  Its Base64 representation is the empty string. The zero-byte input is
  sufficient because a newly constructed estimator has non-empty default
  buckets and the mutation clears them on the first malformed `Read`.
* The original harness, with the same mutated production and the complete
  frozen corpus, exited 0 after 254 executions, cov 765, ft 2277, peak RSS
  346 MiB, and produced no artifact. Original mutated binary SHA-256:
  `965fc7936292d0c9902c06c655fcbc765bc95562e67663043faf22cfb1812f6b`.
  Legacy replay log SHA-256:
  `eed97b83536ed3d98574fc5a70a2ec0534bff659dff73c5cea9855aeba89c4a1`.
* Clean production plus the enhanced harness accepted the exact empty
  artifact with exit 0; control log SHA-256:
  `267f8aea8d7f517139c304d9aab81fa5a313b22fc89cd329954733b6b0e1955c`.
  This proves the assertion is not an overbroad failure on master.
* No production mutation remains. No deterministic regression test or
  production fix is claimed because clean master reproduces no failure; this
  commit closes an oracle gap only.

Bitcoin Core caller, input origin, and severity:

* The estimator is created during node initialization at `init.cpp:1680-1685`,
  loads `fee_estimates.dat` from local disk in the constructor at
  `block_policy_estimator.cpp:561-575`, and periodically persists it through
  `FlushFeeEstimates` at `init.cpp:1682-1684`. RPC `estimatesmartfee` consumes
  it at `rpc/fees.cpp:65-91`; the chain interface forwards `estimateSmartFee`
  at `node/interfaces.cpp:739-747`.
* The fuzzed bytes model a local persisted fee-estimate file, not a remote
  block, transaction, witness, or peer-controlled consensus message. A
  malformed local file can already be rejected non-fatally by master. This
  audit found no clean-master corruption, crash, consensus effect, or memory
  safety issue.
* Master-relative severity: N/A for production; Low/informational for oracle
  hardening. Even the modeled divergence would be a local fee-estimation
  persistence/quality issue. It is not Critical merely because malformed bytes
  are accepted by a fuzzer. A nonce without cryptographic meaning would not
  make a clearing issue Critical. Reassess only if a clean-master reproduction
  demonstrates stronger Core impact.

Existing findings and cherry-pick context:

* Existing findings remain rated by actual Core reachability: private
  broadcast failed-send retention is Medium and feature-conditional; empty
  HEADERS IBD handoff is Medium availability; peer activity refresh,
  process-message local storage failure, oversized transport types, and banman
  invalid-subnet integrity are Low or nice-to-have in current callers; ecmult
  scratch wrapping, forced 10x26 normalization, and SHA/HMAC/RFC6979 retention
  remain reachability-limited latent/hygiene findings. No clean-master
  production bug was established in the previously audited addrman,
  coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
  compact-block, headers-sync, UTXO snapshot, mempool-persistence,
  package-evaluation, handshake, BufferedFile, block-index, or tx_pool paths.
* `origin/master` and `remotes/l0rinc/master` both resolve to
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`. No relevant l0rinc commit applies
  to this target, so none was cherry-picked. Any later production fix, minor
  fix, oracle change, or cherry-pick must repeat the exact target,
  corpus/artifact, mutation, assertion, status/stack, Core caller/input origin,
  test gap, severity, and verifier commands, and state whether it preserves,
  changes, or masks this clean-master behavior.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j2`; `git diff --check`; original and enhanced frozen-corpus replays;
the four-worker command; enhanced mutated replay; legacy mutated replay; and
the exact enhanced clean-artifact replay.

## `autofile` ownership oracle

Source commit `c3cb2b3d60` (`fuzz: assert AutoFile ownership postconditions`)
strengthens `src/test/fuzz/autofile.cpp`. After `AutoFile::fclose()`, the
target asserts `IsNull()`. At the terminal `release()` path it snapshots
whether the wrapper was open, requires a non-null returned `FILE*` when it was
open, requires `IsNull()` afterward, and closes a returned handle itself. The
same null postcondition is checked for terminal `fclose()`. These are narrow
ownership contracts matching `src/streams.h:429-443`; the target does not
assume arbitrary fuzzed I/O succeeds or that malformed local files are valid.

Master and clean identities:

* Core base master is
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; source parent is
  `4cb03a5556a6bea772e81d5a4c99720536aa54f3`. Production behavior is
  unchanged, and this commit contains only a fuzzer change.
* Clean `src/streams.h` SHA-256 is
  `216fa033e467ecb86bdcf2db7ad14b0d81a0c85a9a3ba3d8eb2dde8138146fca`;
  clean `src/streams.cpp` SHA-256 is
  `22712c377e971375572841027cd903ab83861ce58dc748b57a611f7394e0397a`.
* Original harness SHA-256 is
  `bdd00db6a5d2e7cb47c275e8cfb2485d03beea46bf7379f617d76ef7d79778af`;
  enhanced harness SHA-256 is
  `36b47577fdbbf23d7c1a615f427a2a8c5398b02adfa7c7aed690f23331777c8c`;
  final clean sanitizer fuzz binary SHA-256 is
  `601abe1b28cacea8a12d434f8b15f9e629ef6b6ca092b94db40f4b433fe26ab4`.

Frozen corpus and clean runs:

* Target: `autofile`. Source:
  `/mnt/my_storage/qa-assets/fuzz_corpora/autofile`. Frozen directory:
  `/tmp/bitcoin-autofile-20260720/frozen`.
* The corpus contains 339 files and 10,856,927 bytes. Its sorted
  filename/content manifest SHA-256 is
  `5d544536d7bb1d9cca828ec989c27001445df8c108a29908c8b3b99baeb1dff7`.
  The frozen corpus was not modified.
* The clean enhanced replay used:

  ```text
  env FUZZ=autofile /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
    -merge=0 -runs=339 -timeout=60 -rss_limit_mb=0 -print_final_stats=1 \
    -artifact_prefix=/tmp/bitcoin-autofile-20260720/final-artifacts/ \
    /tmp/bitcoin-autofile-20260720/frozen
  ```

  It exited 0 after 340 executions, cov 588, ft 3405, peak RSS 442 MiB, with
  no artifacts. Log SHA-256:
  `65af14a473f061aeccb0cf9f9c9592c0735a45025436759854298a84f9ed2424`.
* The four-worker command was:

  ```text
  env FUZZ=autofile /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
    -jobs=4 -workers=4 -max_total_time=15 -timeout=60 -rss_limit_mb=0 \
    -print_final_stats=1 \
    -artifact_prefix=/tmp/bitcoin-autofile-20260720/final-worker-artifacts/ \
    /tmp/bitcoin-autofile-20260720/workers
  ```

  Jobs 0, 1, 2, and 3 exited 0 after 7,292, 8,227, 8,631, and 10,255
  executions. Each reached cov 588; ft was 3,409, 3,411, 3,409, and 3,413;
  peak RSS was 450, 449, 455, and 448 MiB. Worker log SHA-256:
  `8da88f73013b5371830239b960dd25919b93971981a352fb693b20e2c1d87a07`.
  The disposable worker corpus grew to 419 files/13,300,135 bytes; the frozen
  corpus stayed unchanged and no fuzz process remained.
* The exact clean-control and mutation input is
  `/tmp/bitcoin-autofile-20260720/frozen/039e526302bfbf6efb436b24eb2041bdbd1aa6b2`,
  size 33,586 bytes, SHA-256
  `59205c1014d464cdd514a00f555002bdb75ab5682151d41ce06750c4c4dd7a64`.
  Clean exact replay exited 0; log SHA-256:
  `7db037fe6ca1f95dfe5680e696a6417efba1b067a8be018e0b182d6f2a2582ab`.

Differential proof that the oracle matters:

* The final refined temporary production mutation changed `release()` from
  returning the saved `m_file` to returning `nullptr`, while still closing a
  non-null `m_file` and setting `m_file` to `nullptr`:

  ```cpp
  std::FILE* ret{nullptr};
  if (m_file != nullptr) std::fclose(m_file);
  m_file = nullptr;
  return ret;
  ```

  This isolates the ownership-return-value contract. Mutated `src/streams.h`
  SHA-256:
  `56180b84a7173c85fe0b64eb727d2bdd8d1d9f9db7b98620aac1f574e25ebd8e`.
  Enhanced mutated binary SHA-256:
  `063d99fb72de0c08649a31486d2a82b36c9ec087f32928f9390eef446274f84e`.
* The full frozen mutation replay reached the new assertion at
  `src/test/fuzz/autofile.cpp:65`, `!was_open || f != nullptr`, after one
  input. Log SHA-256:
  `a89d2dbb04631975a9c4c009f4b5d330011384ad7173b2736f54ea5ea3f854d4`.
* The exact input above reproduced the same assertion; exact mutation log
  SHA-256:
  `6b3e9bc5bf3542ec3cdd993ba888f37dcd3bfba905c013bca1abc883fa412cd6`.
  LibFuzzer's signal handler remained alive after the assertion in this build,
  so the known process was killed during controlled cleanup. No artifact was
  emitted; the frozen path, size, content hash, command, assertion, and
  mutated binary are the reproducible corpus condition.
* The original harness with the same refined mutation exited 0 after 340
  executions, cov 587, ft 3404, peak RSS 441 MiB, with no artifact or
  diagnostic. Original harness SHA-256:
  `bdd00db6a5d2e7cb47c275e8cfb2485d03beea46bf7379f617d76ef7d79778af`.
  Legacy mutated binary SHA-256:
  `f0626e8321e7b1ba2157bca1b39164d89e230e8cd70559a0879d83a19213f6ea`.
  Legacy log SHA-256:
  `dc042cca4cdd6d0038adb0f16031d4a628e72a5f32beb7600752b6793b2ca304`.
  This is the differential proof that the new oracle catches a modeled
  ownership-return regression the old target missed.
* Earlier mutations were rejected as proof: removing `m_file = nullptr`
  triggered an ASAN double-free in the legacy harness, and variants returning
  `nullptr` without the isolated close/null behavior triggered legacy lifetime
  failures. They do not demonstrate a new oracle gap.
* Clean production plus the enhanced harness accepted the exact input with exit
  0. No production mutation remains; no production fix or deterministic
  regression test is claimed because clean master reproduces no failure.

Bitcoin Core caller, input origin, and severity:

* `AutoFile` is a local `FILE*` ownership wrapper used for addrman persistence
  (`src/addrdb.cpp:62-82,127-128`), block/undo storage
  (`src/node/blockstorage.cpp:834-842,997-1021,1106-1173,1293-1316`), mempool
  persistence (`src/node/mempool_persist.cpp:47,153-227`), UTXO snapshots
  (`src/node/utxo_snapshot.cpp:31-67`), fee estimates, indexes, RPC exports,
  and external block loading. No known production `src` caller invokes
  `release()` beyond tests/fuzzing; production ownership normally uses
  `fclose`, RAII, or `OpenBlockFile` wrappers.
* The fuzzed provider models local I/O and ownership transitions. It does not
  show that remote peers or invalid block/witness bytes can directly reach
  `release()`, and clean master has no failure.
* Master-relative severity is N/A for production and Low/informational for
  oracle hardening. The modeled bad return could cause a caller leak/close
  omission, while a different mutation could cause a double-close, but neither
  is a clean-master Core vulnerability here. A malformed local file or invalid
  block alone is not Critical. A nonce without cryptographic meaning is not a
  Critical clearing finding. Re-rate only from a clean-master reproduction with
  a concrete Core caller and input origin.

Existing findings and cherry-pick/masking context:

* Existing findings remain Core-caller relative: private broadcast failed-send
  retention is Medium and feature-conditional; empty HEADERS IBD handoff is
  Medium availability; peer activity refresh, process-message local storage
  failure, oversized transport types, and banman invalid-subnet integrity are
  Low or nice-to-have in current callers; ecmult scratch wrapping, forced
  10x26 normalization, and SHA/HMAC/RFC6979 retention remain reachability-
  limited latent/hygiene findings. No clean-master production bug was
  established in the previously audited addrman, coins-cache, txgraph,
  txdownloadman, txrequest, connman, eviction, compact-block, headers-sync,
  UTXO snapshot, mempool-persistence, package-evaluation, handshake,
  BufferedFile, block-index, tx_pool, or policy-estimator paths.
* `origin/master` and `remotes/l0rinc/master` both resolve to
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`. No relevant l0rinc commit
  applies to `autofile`, so none was cherry-picked. If a later fix, minor fix,
  oracle change, or cherry-pick alters this target, its amended commit message
  must repeat the exact target/corpus/artifact, mutation, assertion and
  status/stack, Core caller/input origin, test gap, severity, verifier commands,
  and whether it preserves, changes, or masks clean-master behavior.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j2`; `git diff --check`; clean frozen-corpus replay; four-worker command;
exact clean replay; enhanced mutated replay; legacy mutated replay; and exact
mutation replay.

## `checkqueue` error-result oracle

Source commit `588e3877bd` (`fuzz: assert checkqueue error-result propagation`)
strengthens `src/test/fuzz/checkqueue.cpp`. `DumbCheck::operator()` returns
`std::nullopt` for success and `1` for failure. The target now computes the
expected optional error from the checks actually submitted to each queue and,
after direct `CCheckQueue::Complete()` or explicit
`CCheckQueueControl::Complete()`, requires both result presence and value to
match. Unsubmitted checks are excluded. When explicit completion is not chosen,
the existing control destructor still exercises RAII completion, but its return
value is intentionally unobservable.

Master and clean identities:

* Core base master is
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; source parent is
  `c3cb2b3d60b02f2eeb05e1ef695b1d6e19eeb59d`. Production behavior is unchanged.
* Clean `src/checkqueue.h` SHA-256 is
  `8451bd879402afebd8d67cc11ac3029d6d50ebe6d88c3877a0476fa162f1dea`.
  Original harness SHA-256 is
  `214b28b770133afba7afc0d31cc06afb508f5a81a4a45bc845d6251298a83cff`.
  Enhanced harness SHA-256 is
  `fb79d79f07287283d1c1d62f036d62685edfec332f6abb0b39891326993ff9cb`.
  Final clean sanitizer fuzz binary SHA-256 is
  `144ad53caebc307faedda9a68125424d46ac9df127216fe4fe7e53a5500704a1`.

Frozen corpus and clean runs:

* Target: `checkqueue`. Source:
  `/mnt/my_storage/qa-assets/fuzz_corpora/checkqueue`. Frozen directory:
  `/tmp/bitcoin-checkqueue-20260720/frozen`.
* The corpus contains 98 files and 15,011 bytes. Sorted filename/content
  manifest SHA-256:
  `5988dbcb14d52fb5bf6e4b31d29b97f0f0b1b3449b29c0739e6cc8a5dbaee4d9`.
  The frozen corpus was not modified.
* Final clean replay:

  ```text
  env FUZZ=checkqueue /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
    -merge=0 -runs=98 -timeout=60 -rss_limit_mb=0 -print_final_stats=1 \
    -artifact_prefix=/tmp/bitcoin-checkqueue-20260720/final-clean-artifacts/ \
    /tmp/bitcoin-checkqueue-20260720/frozen
  ```

  It exited 0 after 197 executions, cov 353, ft 876, peak RSS 100 MiB, with
  no artifacts. Log SHA-256:
  `c63b199936cfffee12a02ad2a365956aaa8bb12cb272032165cfc028e9aeca91`.
* The exact clean control for the mutation input below exited 0; log SHA-256:
  `18e01ee9ef78a7a037bc5ba7773b0be489fe3797a555c8f82e5db2cad9a51293`.

Multi-worker evidence:

* The disposable worker directory was seeded from the frozen corpus before the
  final run: 98 files/15,011 bytes.

  ```text
  env FUZZ=checkqueue /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
    -jobs=4 -workers=4 -max_total_time=15 -timeout=60 -rss_limit_mb=0 \
    -print_final_stats=1 \
    -artifact_prefix=/tmp/bitcoin-checkqueue-20260720/worker-artifacts/ \
    /tmp/bitcoin-checkqueue-20260720/workers
  ```

  All four jobs exited 0 with no sanitizer diagnostics or artifacts. Jobs 0, 1,
  2, and 3 processed 1,645, 1,645, 1,636, and 1,642 executions; all reached
  cov 353 and ft 876; peak RSS was 102 MiB for each. Worker log SHA-256:
  `2039e501f7b7b00fcab7f023db1391709ec3d58b835619b25cabff4a0ac5df90`.
  The disposable corpus ended at 103 files/15,223 bytes; the frozen corpus
  stayed unchanged and no fuzz process remained.
* An earlier setup attempt used an empty worker directory and omitted the
  artifact directory, so all jobs failed before useful fuzzing. It is not part
  of the evidence; the seeded command above is the valid worker run.

Differential proof that the oracle matters:

* Temporary production mutation in `src/checkqueue.h` changed

  ```cpp
  local_result.has_value() && !m_result.has_value()
  ```

  to

  ```cpp
  local_result.has_value() && m_result.has_value()
  ```

  This prevents the first failed check from moving into the shared result, so
  `Complete()` can incorrectly return `std::nullopt`. Mutated header SHA-256:
  `2fbcc107fed3a60b160275e164a7d8b01c899e0e7b239b2d4d94cbdecac82f7d`.
  Enhanced mutated binary SHA-256:
  `1bd86244075b76a5d25bddc3c32329f72b2470be5becf10e43e052111cca2364`.
* The full frozen replay reached `checkqueue.cpp:60` after 42 executions.
  LibFuzzer's signal handler stayed alive after the assertion, so the
  controlled outer timeout returned 124 and no artifact was emitted. Mutation
  log SHA-256:
  `432fe56616704c24aae903acd0a48c8bac77dc047f2c4812caffd4f348618d04`.
* Temporary tracing, removed before the final build, identified the generated
  unit used for exact replay:
  `/tmp/bitcoin-checkqueue-20260720/trigger-input`, 7 bytes, hex
  `ffdb2801000200`, SHA-256
  `e2897dec912296eb9d5dff96edeb1f186fa959032a3d8eda174e9d9e1b41ac2f`.
  The non-instrumented mutated binary reached the same assertion,
  `result.has_value() == expected_result_1.has_value()`. The controlled
  timeout again returned 124 because the signal handler stayed alive; no
  artifact was emitted. Exact mutation log SHA-256:
  `b3ad394a7c6b35ddd40575086580db2d3a43e3eb82f3d481e0b3f3bf77c16ad7`.
* The original harness with the same mutated production exited 0 after 197
  executions, cov 336, ft 817, peak RSS 99 MiB, with no artifact or diagnostic.
  Original harness SHA-256:
  `214b28b770133afba7afc0d31cc06afb508f5a81a4a45bc845d6251298a83cff`.
  Legacy mutated binary SHA-256:
  `70fb35e735b0cf6e1ddc2b515d0b80a7b63878de83b88aa1f52f1165a8d7fd67`.
  Legacy log SHA-256:
  `d8db67dc305872db686f6d2ab57e3dc407f16b583c31ea22664c76a5d027dab3`.
  This is the differential proof that the new oracle catches a modeled result
  propagation regression the old target missed.
* Clean production plus the enhanced harness accepted the exact 7-byte input
  with exit 0. No production mutation remains. No deterministic production
  test or fix is claimed because clean master reproduces no failure.

Bitcoin Core caller, input origin, and severity:

* `ChainstateManager` owns the production `CCheckQueue<CScriptCheck>` at
  `src/validation.h:983` and initializes it at `src/validation.cpp:6162`.
  `ConnectBlock` creates `CCheckQueueControl` at `src/validation.cpp:2527`
  and converts a returned script-check error into `BLOCK_CONSENSUS` failure at
  `src/validation.cpp:2627-2629`. Remote block processing and local
  reindex/load paths use this block-validation boundary.
* The fuzzer uses synthetic `DumbCheck` objects and harness bytes. It does not
  prove that the 7-byte input is a remote block or that clean master accepts an
  invalid block. The mutation models corruption of the queue result at the
  production API boundary.
* Master-relative severity for current master is N/A: clean master has no
  production failure. The oracle change is Low/informational hardening, but
  the modeled regression would be High/Critical if a real production defect
  caused failed script checks for a remotely submitted invalid block to be
  lost, because `ConnectBlock` relies on this result to reject the block. This
  hypothetical consequence is not a current vulnerability. Invalid fuzzer
  state or malformed bytes alone is not Critical. A nonce without cryptographic
  meaning is not a Critical clearing finding.

Existing findings and cherry-pick/masking context:

* Existing findings remain Core-caller relative: private broadcast failed-send
  retention is Medium and feature-conditional; empty HEADERS IBD handoff is
  Medium availability; peer activity refresh, process-message local storage
  failure, oversized transport types, and banman invalid-subnet integrity are
  Low or nice-to-have in current callers; ecmult scratch wrapping, forced
  10x26 normalization, and SHA/HMAC/RFC6979 retention remain reachability-
  limited latent/hygiene findings. No clean-master production bug was
  established in the previously audited addrman, coins-cache, txgraph,
  txdownloadman, txrequest, connman, eviction, compact-block, headers-sync,
  UTXO snapshot, mempool-persistence, package-evaluation, handshake,
  BufferedFile, block-index, tx_pool, policy-estimator, or autofile paths.
* `origin/master` and `remotes/l0rinc/master` both resolve to
  `18c05d93016b28a9afd4c716dfe00b6e0accb30b`. No relevant l0rinc commit applies
  to `checkqueue`, so none was cherry-picked. If a later production fix, minor
  fix, oracle change, or cherry-pick alters this target, its amended commit
  message must repeat the exact target/corpus/artifact, mutation, assertion,
  status/stack, Core caller/input origin, test gap, severity, verifier commands,
  and whether it preserves, changes, or masks clean-master behavior.

Verifiers: `cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target
fuzz -j2`; `git diff --check`; clean frozen-corpus replay; seeded four-worker
command; full enhanced mutation replay; exact enhanced mutation replay; full
legacy mutation replay; and exact clean replay.

## PartiallyDownloadedBlock one-shot oracle audit (2026-07-20)

Source commit: 08f26f05c5 (fuzz: assert PartiallyDownloadedBlock is one-shot).
It is based on Bitcoin Core master
18c05d93016b28a9afd4c716dfe00b6e0accb30b, with source parent
588e3877bd0285f3a10a2d490958f9c9b417e08c. The l0rinc fork was at the same
master tip. No separate l0rinc/secp256k1 commit applies to this Bitcoin Core
block-encoding state contract, so none was cherry-picked. The earlier
compact-block status/cache oracle commits 01086e700fda and aef3a7b4d7f remain
the preceding context.

### Contract

PartiallyDownloadedBlock::FillBlock clears header and txn_available at
src/blockencodings.cpp:208-210 before returning READ_STATUS_OK or
READ_STATUS_FAILED. Its first guard at line 191 must reject every later call.
After the existing status matrix, the fuzzer now calls FillBlock a second time
with an empty missing-transaction vector and asserts READ_STATUS_INVALID
whenever the first call returned OK or FAILED. The assertion is conditional
on the status proving that cleanup was reached; it does not assume every
InitData failure is a valid FillBlock input.

### Core caller and severity

Remote CMPCTBLOCK announcements enter PeerManagerImpl at
src/net_processing.cpp:4744-4815; remote BLOCKTXN replies enter
ProcessCompactBlockTxns at 3564-3618. The status controls in-flight cleanup,
peer punishment, full-block GETDATA fallback, and eventual ProcessBlock. The
fuzzer uses serialized compact-block bytes and synthetic mempool/transaction
state, so synthetic reachability is not proof of network reachability.

No production bug was found on clean master. This is Low/informational oracle
hardening, not a production vulnerability claim. The synthetic mutation
replaced header.SetNull(); with a no-op while leaving txn_available.clear()
unchanged. If present in production, a repeated BLOCKTXN could reach FillBlock
with a retained header instead of the header-null guard. Current Core normally
removes successful requests and explicitly handles a null header on duplicate
replies; a caller-level reproduction of a retained failed partial block is
required before assigning production severity. The mutation is not High or
Critical. An invalid compact block alone is not Critical, and a nonce with no
cryptographic meaning is not a clearing-critical finding.

### Exact identities and corpus

Clean src/blockencodings.cpp SHA-256:
a87feb8df352ae9c868af6bacb5193261795153249e0a15ca33a05d686291838.
Parent harness SHA-256:
9c25e837ce502e3e04c7ccb521d3f4a112b626ba5a61fa0424afe1ada52fdc24.
Enhanced harness SHA-256:
d6e4f1d9033e6a35caf391c1f5d907bb43fe1b1c7778a64de5ed7275a3e9350b.
Final sanitizer fuzz binary SHA-256:
084799bc084b80612e8bda46a003eb51630d3482f1d0d221ca01437e2edd5f2f.

The source corpus is
/mnt/my_storage/qa-assets/fuzz_corpora/partially_downloaded_block.
The immutable evidence copy is
/tmp/bitcoin-partially-downloaded-block-20260720/frozen-clean, with 909
files and 148804324 bytes. Its path-independent sorted per-file manifest was
calculated with:

    (cd frozen-clean && find . -type f -printf '%P\0' | sort -z | \
      xargs -0 -r sha256sum | sha256sum)

Manifest SHA-256:
b2e29d7098214b92988a008c467bbce7c24dda1c2739c595b639ac7f69940945.
An earlier exploratory unbounded invocation added generated units to a
temporary copy; that directory was discarded. The evidence copy above was
freshly copied from the source corpus and was never used for worker output.

### Clean replay and workers

The final clean replay used:

    FUZZ=partially_downloaded_block /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
      -merge=0 -runs=909 -timeout=60 -rss_limit_mb=0 -use_value_profile=1 \
      -print_final_stats=1 \
      -artifact_prefix=/tmp/bitcoin-partially-downloaded-block-20260720/final-artifacts/ \
      /tmp/bitcoin-partially-downloaded-block-20260720/frozen-clean

It exited 0 after 910 executions, coverage 5304, features 53453, new units
0, peak RSS 796 MiB, and no sanitizer, assertion, timeout, OOM, crash, or
artifact output. Final replay log SHA-256:
7f30209ec6b86bb484c621223c7129c38b36dbbb140f28909f50a7fd202784a4.

The exact clean replay of
0119eb7451e664436c34a42c54ecfc6eeff00c2f exited 0 after one execution. The
input is 190055 bytes with SHA-256
ce219bae33d1dcca1e0e565ea97693870c3efc4cf4c53d36431db1c4abf93fea.
The exact clean log SHA-256 is
6181a83a3352d232fea8b9da1b2e8bd1744cec93af7c4b8714b7c0effdc8c17f.

Four seeded workers used:

    FUZZ=partially_downloaded_block /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
      -jobs=4 -workers=4 -max_total_time=60 -timeout=60 -rss_limit_mb=0 \
      -use_value_profile=1 -print_final_stats=1 \
      -artifact_prefix=/tmp/bitcoin-partially-downloaded-block-20260720/worker-artifacts/ \
      /tmp/bitcoin-partially-downloaded-block-20260720/workers

All four exited 0 without sanitizer diagnostics or artifacts. Jobs 0, 1, 2,
and 3 executed 1945, 1924, 1936, and 2125 units; added 44, 54, 59, and 74
units; and peaked at 871, 880, 911, and 862 MiB. All reached coverage 5304.
Combined worker log SHA-256:
5566e578367d807733619cb7d5d4d12006f423926b5aecc7595a52579bbb66fb.
Individual logs are retained under
/tmp/bitcoin-partially-downloaded-block-20260720/worker-logs/:

    job0 7e777fa1652851c7ceb7051c8833dee1fc682c9e78f430bcfd4d81e9133db883
    job1 db8ce0ffa15c819b27f49e819d26c39e098fe303e31d64e09b6faa0bc5ccb843
    job2 c5c2e7c1820f227d4003ead5c911c2707c15109ac7cee6c90458d8d4450bddc3
    job3 03426dc3be533bf5faf52587ce8592f6e5090b5b5050de475a33bc8a5c74ce7e

### Differential proof

The mutation removed only header.SetNull(); and left txn_available.clear()
unchanged. Mutated production src/blockencodings.cpp SHA-256:
bf73330cb0d75fbf6b7a858980923a64b51063270154ec6108e576af5e1af4c8.
Enhanced mutated fuzz binary SHA-256:
ed34f9ae1caf3cb53146c69dbff225dffb22d8375a52f1f0d6b53c9461c4cae2.

The full mutated replay exited 134 at
src/test/fuzz/partially_downloaded_block.cpp:146 on the new assertion, with
no artifact. Mutation log SHA-256:
815ef55031925c1d2254ce45705966e937a3aed903c149971373e752701721d4.
The exact seed above independently exited 134 at the same assertion; its log
SHA-256 is
a3b97501694233915b7f913bce94738a2ae7f0e9c265c5131c559ae1209f01a9.

The parent harness
9c25e837ce502e3e04c7ccb521d3f4a112b626ba5a61fa0424afe1ada52fdc24 ran the
same mutated production binary and the same seed once without a diagnostic
and exited 0. Legacy mutated binary SHA-256:
eb7a97f884cff5d28f3eb61ee4ec6cadad085eb02cd049e70734c07dd9b32046.
Legacy control log SHA-256:
91680759cacd3130563e575f34c1673c32839370183107252ef1a5462d4e300b.
This proves a previously silent oracle gap, not a clean-master production
defect. No production mutation remains.

### Existing findings and cherry-pick context

Existing findings remain Core-caller relative: private-broadcast failed-send
retention is Medium and feature-conditional; empty HEADERS IBD handoff is
Medium availability; ecmult scratch wrapping is Medium with low demonstrated
Core reachability; forced 10x26 magnitude-32 normalization and
SHA/HMAC/RFC6979 retention remain reachability-limited Medium
correctness/hygiene findings; last_tx_time, process-message local block
storage failure, oversized transport types, and banman invalid-subnet/unban
integrity are Low or nice-to-have under current callers. No clean-master
production bug was established in the previously audited txdownloadman,
txrequest, connman, eviction, headers-sync, UTXO snapshot,
mempool-persistence, package-evaluation, handshake, BufferedFile, block-index,
tx_pool, policy-estimator, autofile, checkqueue, or compact-block paths.
Invalid fuzzer state is not a production finding, and an uncleared
non-cryptographic nonce is not Critical.

No additional l0rinc commit was cherry-picked for this target because the
fork review found no applicable independent change. If a later cherry-pick,
minor fix, oracle change, or follow-up alters this behavior, amend the
relevant commit message and this ledger with whether clean-master behavior
was preserved, changed, or masked; exact corpus input or mutation, source
and binary hashes, assertion/status/stack, Core caller and input origin,
master-relative severity, test gap, and verifier commands must remain
recorded. A potential fix is not proof that master was vulnerable unless
clean master or the exact minimal production mutation reproduces the failure.

Verifiers:
cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j8;
clang-format --dry-run --Werror
src/test/fuzz/partially_downloaded_block.cpp; git diff --check; the clean
frozen-corpus replay; the seeded four-worker command; the full and exact
enhanced mutation replays; the parent-harness exact mutation replay; and the
exact final clean replay. The fuzz-only build did not expose
blockencodings_tests or net_tests targets, and no deterministic production
regression test was added because clean master has no confirmed production
defect.

## CBlock::SetNull reset oracle audit (2026-07-20)

Source commit: 9f63b1f80f (fuzz: assert CBlock::SetNull clears state). It is
based on Bitcoin Core master 18c05d93016b28a9afd4c716dfe00b6e0accb30b, with
source parent 08f26f05c517667b91dc3a34913651b46cebbdab. The l0rinc fork was
reviewed at the same master tip. No separate l0rinc/secp256k1 commit applies
to this CBlock reset contract, so none was cherry-picked. The existing block
oracle already checked serialization, validation-result coherence, hashes,
weights, and IsNull(); this audit adds the missing payload/cache
postconditions.

### Contract and Core boundary

CBlock::SetNull at src/primitives/block.h:100-107 must clear the transaction
payload and all memory-only validation caches, not merely make the header
null. After copying a fuzzed block and calling SetNull, the harness requires
vtx.empty(), !fChecked, !m_checked_witness_commitment, and
!m_checked_merkle_root.

BlockManager::ReadBlock calls SetNull at src/node/blockstorage.cpp:1052 before
reading local block-file bytes. The node interface calls ReadBlock and, on
failure, calls block.m_data->SetNull() at src/node/interfaces.cpp:458-462.
The fuzz input is serialized block data and models a local disk/file-read and
block-object reuse boundary, not a direct peer message.

No production bug was found on clean master. This is Low/informational oracle
hardening, not a production vulnerability claim. The proof-only mutation
retains vtx across SetNull while leaving the header and validation flags reset.
If present in production, a failed local block read could leave stale
transactions in a reused CBlock returned by the block-storage/interface path.
That is a local I/O/state-integrity concern; no consensus, remote
invalid-block, memory-safety, or Critical master finding is claimed. An
invalid block alone is not Critical, and a nonce with no cryptographic meaning
is not a clearing-critical finding.

### Exact identities and corpus

Clean src/primitives/block.h SHA-256:
07aa61de74a4aa8b9c9f67d87a6a9a1cf2729bb452e3642d95facec23fe0fd27.
Parent harness SHA-256:
6ce9214ff6b391c992d7486f5459c760a81cfbd01df5bb8c2142a253b6ece71d.
Enhanced harness SHA-256:
82fdb5f175dce32f3090ab1ad4c213519aa1f51124569ae088ee22ed13594dea.
Final ASan/UBSan/libFuzzer binary SHA-256:
c9df5f4a76b22f98b395722210ec2fedc55079aa4e0484fe69342e8288daa1d.

The source corpus is /mnt/my_storage/qa-assets/fuzz_corpora/block. The
immutable evidence copy is
/tmp/bitcoin-block-20260720/frozen-clean, with 965 files and 145288774 bytes.
Its path-independent sorted per-file manifest was calculated with:

    (cd frozen-clean && find . -type f -printf '%P\0' | sort -z | \
      xargs -0 -r sha256sum | sha256sum)

Manifest SHA-256:
cbb96936b2cfcdcfb133ffc58ac1952108bbfe38cc9148a7e777da586fb1cb16.
The frozen copy was kept separate from disposable worker output.

### Clean replay and workers

The final clean replay used:

    FUZZ=block /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
      -merge=0 -runs=965 -timeout=60 -rss_limit_mb=0 -use_value_profile=1 \
      -print_final_stats=1 \
      -artifact_prefix=/tmp/bitcoin-block-20260720/final-artifacts/ \
      /tmp/bitcoin-block-20260720/frozen-clean

It exited 0 after 966 executions, coverage 1623, features 20859, new units
0, peak RSS 848 MiB, and no sanitizer, assertion, timeout, OOM, crash, or
artifact output. Final replay log SHA-256:
94d92d58a5d86123b829844e4e29b207b62b6f24099cd4a3ce8b252032d780a3.

The exact clean replay of corpus input
002bcf9672a3177140f1cd70a6781bde6044c556 exited 0 after one execution. The
input is 784106 bytes with SHA-256
c2e39105f1ffccc71178fe57c3840bfdca7c74b72f8175f0c7aca26490176aa0.
The exact clean log SHA-256 is
549e374911cbe6385732e8655507fb850e5f91d1b8c024a28aa0cd8a4d0c1cf4.

Four seeded workers used:

    FUZZ=block /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz \
      -jobs=4 -workers=4 -max_total_time=60 -timeout=60 -rss_limit_mb=0 \
      -use_value_profile=1 -print_final_stats=1 \
      -artifact_prefix=/tmp/bitcoin-block-20260720/worker-artifacts/ \
      /tmp/bitcoin-block-20260720/workers

All four exited 0 without sanitizer diagnostics or artifacts. Jobs 0, 1, 2,
and 3 each executed 966 units, added no units, reached coverage 1623, and
peaked at 841, 838, 838, and 845 MiB. Combined worker log SHA-256:
377cf56b992d8108cae4d0e4a3bbea29c619fe865bfe7d411efc026a305f8f6b.
Individual logs are retained under
/tmp/bitcoin-block-20260720/worker-logs/:

    job0 388f5cbd193df387a0a5323e5029c936989143094ce915730951217fdf626cd3
    job1 9768c5e10119df08eacaadc7860d032c30873aff812d12cd07ed59d08556d04c
    job2 0d3ff6689dcc1329b11b85f8b0dee5c6ad5b23b8db98df6e4ac49f8f08b34da3
    job3 d42da8e908e5426da14ac8fc3248cafc24e8033bede636fead163f5e8d537cc8

### Differential proof

The production mutation replaced vtx.clear() at primitives/block.h:103 with
a comment/no-op. Mutated production header SHA-256:
7c49a8deda4116aaeb9fd8a12d7d4ee5fbb0f2e84518ff8a941396b5305f6c44.
Enhanced mutated fuzz binary SHA-256:
7fa014708e2a780fd5fdc2497e4d8b07dab4fa013a3f3c5e93557633bce60113.

The full mutated replay exited 134 at block.cpp:125 on block_copy.vtx.empty(),
with no artifact. Mutation log SHA-256:
b1f8722b9e78eb19d07363d9d6f7de0c11e549de911888ffbda4a3dda2538155.
The exact sorted seed above independently exited 134 at the same assertion;
its log SHA-256 is
cc5d433d170fda66b1370a672009c61a637ee9004970a67fd61a463e3be552ac.

The parent harness
6ce9214ff6b391c992d7486f5459c760a81cfbd01df5bb8c2142a253b6ece71d ran the
same mutated production binary and the same seed once without a diagnostic
and exited 0. Legacy mutated binary SHA-256:
cf0006fca0553be8a5274e31d7561ff3d3290b326119212bd1afc8b1ab0c2035.
Legacy control log SHA-256:
ff3ad242ddbee30b408c877838448930381abc2135b3d8561cadafb7232e7c3e.
This proves the new reset oracle catches a stale-payload regression the old
IsNull-only oracle missed; it does not prove a clean-master production defect.
No production mutation remains.

### Existing findings and cherry-pick context

Existing findings remain Core-caller relative: private-broadcast failed-send
retention is Medium and feature-conditional; empty HEADERS IBD handoff is
Medium availability; ecmult scratch wrapping is Medium with low demonstrated
Core reachability; forced 10x26 magnitude-32 normalization and
SHA/HMAC/RFC6979 retention are reachability-limited Medium
correctness/hygiene findings; last_tx_time, process-message local block
storage failure, oversized transport types, and banman invalid-subnet/unban
integrity are Low or nice-to-have under current callers. No clean-master
production bug was established in the previously audited addrman,
coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
headers-sync, UTXO snapshot, mempool-persistence, package-evaluation,
handshake, BufferedFile, block-index, tx_pool, policy-estimator, autofile,
checkqueue, compact-block, or PartiallyDownloadedBlock paths. Invalid fuzzer
state is not a production finding, and an uncleared non-cryptographic nonce
is not Critical.

No additional l0rinc commit was cherry-picked for this target because the
fork review found no applicable independent change. If a later cherry-pick,
minor fix, oracle change, or follow-up alters this behavior, amend the
relevant commit message and audit ledger with whether clean-master behavior
was preserved, changed, or masked; exact corpus input or mutation, source
and binary hashes, assertion/status/stack, Core caller and input origin,
master-relative severity, test gap, and verifier commands must remain
recorded. A potential fix is not proof that master was vulnerable unless
clean master or the exact minimal production mutation reproduces the failure.

Verifiers:
cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j8;
clang-format --dry-run --Werror src/test/fuzz/block.cpp; git diff --check; the
clean frozen-corpus replay; the seeded four-worker command; the full and exact
enhanced mutation replays; the parent-harness exact mutation replay; and the
exact final clean replay. The fuzz-only build did not expose block-specific
unit-test targets, and no deterministic production regression test was added
because clean master has no confirmed production defect.

## CBlockHeader::SetNull field-reset oracle audit (2026-07-20)

Source commit: 8ec4b31dcb84659b5965bb7ec7b0dc896af3367c
(fuzz: assert CBlockHeader::SetNull clears every field). It is based on
source parent 9f63b1f80fb710e7035b94ba5d23ed5aa2ea628f and Bitcoin Core master
18c05d93016b28a9afd4c716dfe00b6e0accb30b. The l0rinc fork was reviewed at
the same master tip. No separate l0rinc/secp256k1 commit applies to this
header reset contract, so none was cherry-picked.

The old `block_header` oracle checked only `IsNull()`, which observes `nBits`,
and compared two equally reset CBlock hashes. It could therefore accept a
`SetNull()` implementation that left another header field stale. The new
postconditions require `nVersion == 0`, null previous and merkle hashes,
`nTime == 0`, `nBits == 0`, and `nNonce == 0` after resetting a copied header.
The production implementation in `src/primitives/block.h` already assigns
these values; this is fuzzer-side oracle hardening, not a production behavior
change.

### Core caller boundary and severity

The shared reset is reached through `CBlock::SetNull()` at
`src/node/blockstorage.cpp:1052` and `src/node/interfaces.cpp:460` for local
block-file reads and reused block objects, through
`PartiallyDownloadedBlock::FillBlock()` at `src/blockencodings.cpp:209` after
remote CMPCTBLOCK/BLOCKTXN processing, and through
`HeadersSyncState::Finalize()` at `src/headerssync.cpp:55` for a peer's initial
header synchronization state. Fuzz input is serialized header data; the
actual Core boundaries include both local disk I/O and remote header/compact-
block state.

No clean-master production failure was reproduced. Severity is
Low/informational oracle hardening, not a production vulnerability claim. A
stale nonce could affect a null header's later hash/serialization or local
object state under the proof mutation, but current callers provide no
consensus, remote invalid-block, memory-safety, or denial-of-service impact.
An invalid block/header alone is not Critical, and a nonce with no
cryptographic meaning is not a clearing-critical finding.

### Exact identities and corpus

Clean `src/primitives/block.h` SHA-256:
`07aa61de74a4aa8b9c9f67d87a6a9a1cf2729bb452e3642d95facec23fe0fd27`.
Parent harness SHA-256:
`1fd8f88beea05396694aaf4ab5bf03afabb22c0f760da12bd4961be1db9a419a`.
Enhanced harness SHA-256:
`73d50144de66195f331c85cd97b0f2d3392676daa91a7768f40a1f8717f9324b`.
Final clean ASan/UBSan/libFuzzer binary SHA-256:
`9ba7494286e8a08b8d610ffd1029e08373ee1ad40983ed5622d4daa4fd0210bf`.

Source corpus:
`/mnt/my_storage/qa-assets/fuzz_corpora/block_header`.
Immutable evidence copy:
`/tmp/bitcoin-block-header-20260720/frozen-clean`, containing 121 files and
285055 bytes. The path-independent sorted per-file manifest command was:

    find . -type f -printf '%P\0' | sort -z | xargs -0 -r sha256sum | sha256sum

Manifest SHA-256:
`f1604f9015d4373416c06ef57ab3978045280524508f4a3d2851503dcf2f68b8`.

### Clean replay and workers

The final clean replay exited 0 after 122 executions, coverage 416, features
2723, no new units, peak RSS 109 MiB, and no sanitizer, assertion, timeout,
OOM, crash, or artifact. Its log SHA-256 is
`4fbe932367f898b9029a99b234f1a5b7e3a7cab37a4933d853f8a08eb253cd0`.

The exact corpus input
`011392c3197834c907ef4cfe7ac14d4e5378b971` is 91 bytes with SHA-256
`5602d6bbd84604b2a40a31f43af02cf962c76c3040f02b47d7482f546f922ed3`.
The final clean exact replay exited 0; its log SHA-256 is
`854a93afc682dc70d66c313deae0e316bb72ef19a37199c60ed017159486f6c4`.

The seeded worker command used `-jobs=4 -workers=4 -max_total_time=60`.
All workers exited 0 without diagnostics or artifacts. Jobs 0/1/2/3
executed `26371/23883/24357/27375` units, added
`266/279/282/285` units, and each peaked at 116 MiB. Individual worker log
SHA-256 values are:

    job0 45af5616ec334b432d1f2e18ee6323bfff868769326aa2ee007098a81aab5327
    job1 61ce5905f6ff838a77fc42d9f71adc515a1d072e98a6981d65522b3c04030654
    job2 4f4ed46325dd46efa14946e8338f6b8c4ff3f82735807266f48cb4f2ed51be16
    job3 5f8590dc27750e6914c5e940e2a33a3f792f9f7af6fb5f651c51438c388df6c9

Worker launch log SHA-256:
`bb681077aa138155db0d944e0377799a4c4165f6c0e02f59115f44dcc9bd7158`.

### Differential proof

The proof mutation replaced `nNonce = 0` at `primitives/block.h:51` with
`nNonce = nNonce`, leaving every other production line unchanged. Mutated
production header SHA-256:
`7ba09aa9c1d23bff63489ffb633ac30a9a61ced9f89d6859aea72ab7d3dcee47`.
Enhanced mutated binary SHA-256:
`33c7de061ffa8b3c82ded2d547a911411cc4671705203a7e1c19a0468cf02c13`.

The full mutation replay exited 77 after 21 executions at
`src/test/fuzz/block_header.cpp:40` on `mut_block_header.nNonce == 0`.
The generated libFuzzer artifact is
`/tmp/bitcoin-block-header-20260720/mutated-artifacts/crash-ebecb783e9ff8c43d118b28e82937af2b9bb31da`,
80 bytes, SHA-256
`05ee4af6cd77992ec4c90eacb08380d22ade32db21e28713a69c34b6c2e6e89c`.
The full mutation log SHA-256 is
`82c2fe7c4c6aa9b678165ee1e282d0fe875f2910cd2dcad1e9080a3bc9bdcaee`.
The exact trigger replay independently exited 77 at the same assertion; its
stack/log SHA-256 is
`1856ad1b46faf94a99a0b2aaafaf60fcae68f640f0e936547e3689debdd83903`.
The trigger is a generated mutation artifact, not a claim that the byte
sequence was already in the frozen corpus.

The parent harness
`1fd8f88beea05396694aaf4ab5bf03afabb22c0f760da12bd4961be1db9a419a` ran the
same mutated production binary and the same 80-byte trigger once without a
diagnostic and exited 0. Its legacy mutated binary SHA-256 is
`c1905f4a53ccd7d4257bbea855f019b967f728b0830cc0ccbd141c97c256bd04` and its
control log SHA-256 is
`65906f76088bce8efdafdda98a9ce5303c0676cd3da677d766d8c6eccd0aff6d`.
This proves the new field-level oracle closes an old `IsNull()`/hash-
comparison gap; it does not prove a clean-master production defect. The
production mutation was removed, and the restored clean binary is again
`9ba7494286e8a08b8d610ffd1029e08373ee1ad40983ed5622d4daa4fd0210bf`.

### Existing findings and cherry-pick context

Existing findings remain Core-caller relative: private-broadcast failed-send
retention is Medium and feature-conditional; empty HEADERS IBD handoff is
Medium availability; ecmult scratch wrapping is Medium with low demonstrated
Core reachability; forced 10x26 magnitude-32 normalization and
SHA/HMAC/RFC6979 retention are reachability-limited Medium
correctness/hygiene findings; last_tx_time, process-message local block
storage failure, oversized transport types, and banman invalid-subnet/unban
integrity are Low or nice-to-have under current callers. No clean-master
production bug was established in the previously audited addrman, coins-cache,
txgraph, txdownloadman, txrequest, connman, eviction, headers-sync, UTXO
snapshot, mempool-persistence, package-evaluation, handshake, BufferedFile,
block-index, tx_pool, policy-estimator, autofile, checkqueue, compact-block,
PartiallyDownloadedBlock, or CBlock reset paths. Invalid fuzzer state is not a
production finding, and an uncleared non-cryptographic nonce is not Critical.

No additional l0rinc commit was cherry-picked for this target because the fork
review found no applicable independent change. If a later cherry-pick, minor
fix, oracle change, or follow-up alters this behavior, amend the relevant
commit message and ledger with whether clean-master behavior was preserved,
changed, or masked; exact corpus input or mutation, source and binary hashes,
assertion/status/stack, Core caller and input origin, master-relative
severity, test gap, and verifier commands must remain recorded. A potential
fix is not proof that master was vulnerable unless clean master or the exact
minimal production mutation reproduces the failure.

Verifiers:
`cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j8`;
`clang-format --dry-run --Werror src/test/fuzz/block_header.cpp`;
`git diff --check`; the final clean frozen-corpus replay; the exact clean
trigger replay; the seeded four-worker command; the full and exact enhanced
mutation replays; the parent-harness exact mutation control; and the final
clean identity check. The fuzz-only build did not expose a block_header-
specific unit-test target, and no deterministic production regression test
was added because clean master has no confirmed production defect.

## Compressed-amount deserialization oracle audit (2026-07-20)

Source commits:
`3c4ca02163819de727776031fc6cdb30fdad316f` (`compressor: reject out-of-range
decompressed amounts`) and
`f5845897328d844480a0aea85f0cbbafff3a0f0b` (`fuzz: assert deserialized
compressed amounts are in range`). The first is an amended cherry-pick of
l0rinc commit `40e996720e6c7aba9f5d3b90762abcb5337420ea`; the second is an
amended cherry-pick of `f06ae0fdc2e195639b4b614f4e564ecdb45285c8`. The source
parent is `8ec4b31dcb84659b5965bb7ec7b0dc896af3367c`; the audit base is Bitcoin
Core master `18c05d93016b28a9afd4c716dfe00b6e0accb30b`, which was also the
`l0rinc/master` tip.

### Finding, caller boundary, and severity

On clean master, `AmountCompression::Unser()` accepted every compressed
`uint64_t` and assigned `DecompressAmount()` to `CAmount`. A value above
`MAX_MONEY`, including one that becomes negative after unsigned-to-signed
conversion, could therefore cross the serialized Coin boundary. The
production fix rejects it with `std::ios_base::failure`, asserts the range
contract on valid serialization, and adds Coin-side pre/postconditions and
deterministic boundary tests.

Bitcoin Core reaches this code from local chainstate reads at
`src/txdb.cpp:88` (`CCoinsViewDB::GetCoin`), local undo files at
`src/node/blockstorage.cpp:715` and `src/validation.cpp:2197/4699`, and
locally supplied assumeutxo snapshots at `src/validation.cpp:5780/5839`.
Ordinary peer block and transaction bytes do not use `TxOutCompression`, so an
invalid network block cannot directly trigger this parser. The finding is
Low severity on master: it is a real local serialization/integrity defect, but
not a remote consensus, invalid-block, memory-safety, or High/Critical issue.
The snapshot path already had an explicit `MoneyRange()` rejection, so that
path was defense-in-depth; the chainstate and undo contract remained
unguarded. A non-cryptographic nonce-clearing issue is not Critical.

The l0rinc production commit also changed the existing snapshot error return
to `Assert()`. That conflicted with the earlier audit branch. Resolution kept
the explicit `MoneyRange()` error path, so the fixed compressor normally throws
before the check while the check remains defense-in-depth. The functional test
expects the deserialization failure. This is a deliberate branch-local
conflict resolution, not evidence that the production fix is unnecessary. If
a later fix or cherry-pick changes this ordering, amend both source commit
messages and this ledger with whether clean-master behavior was preserved,
changed, or masked.

### Exact inputs and clean-master control

Each generated input encodes `CompressAmount(MAX_MONEY + 1)` =
`18,900,000,000,000,001`, followed by an empty script:

    txoutcompressor_deserialize  a0c8adb1d183ff0106
    coins_deserialize            00a0c8adb1d183ff0106
    txundo_deserialize           0100a0c8adb1d183ff0106
    blockundo_deserialize        010100a0c8adb1d183ff0106

Input SHA-256 values, in the same order, are:

    d4942b07bf29346a4b0d5067f23001fe9811a4d5e93c1e88f94c1547029deee0
    f55052eabb656aaf082d05e499fcf196889a317b3c503cd5309018d2a093e62c
    50a9d770cbf17d029145340d2f076534ebb44b98431d775f8ab5e13b0e5141dd
    d0b9dc19a9253eac83343b321ec1bf44a18b9d4f9816b945b003331997bbd421

Unmodified master `src/compressor.h` is
`224e13896b124af1fde21b2db6bf18eda0ee32d76f4f7da0db6d8cf7e3309b24`;
master `src/test/fuzz/deserialize.cpp` is
`c3690351b7d84dc619a7b153f6a2e5365f725997b670c311023a2c2587b63681`; and
the clean-master fuzz binary is
`dd1eacc6471155ff6fd9f12d256b08512268dc17dd4f2fbf6768f4ff013ab3ef`.
Each clean-master control ran one execution and exited 0 without a
diagnostic. Log SHA-256 values are:

    txout  ca9f87fa9f43d12b241d8d7e33de1f2eaaae28a2163d894e2a8be9969ded05ea
    coins  5d050b5d7f5633d1e684f3f9a77e661fbe60546a1daf37113bc098dd169c32a5
    txundo 63853f72885c4b39284e25f0c98c4aa11a6257c370fbc18df218173690c4995c
    block  537f1eb8d296c0c31f8fbaf2099accc22f59f22117410d61f85a042890e8d98e

This is the clean-master acceptance proof, distinct from the fixed-tree
invalid-input result.

### Fixed-tree replay and corpora

The final fixed source hashes are:

    src/compressor.h                 c304799fc99982f504ea226198a03f4ca3a89f560404725dbaa5a86477b90548
    src/coins.h                      d95f5472d1cecf7e961684ff78b9895af3e28278c88e6aa00e784cee3fb6d1
    src/validation.cpp               6f00f58f3cc9623fb02cfa8c776654e617b6a88e87c2b075bb855116b9ebfefc
    feature_assumeutxo.py            962550c5cc6dc8eb97372e607151bc4964d7ca6bc275445b44d3ecc57dac3bf3
    src/test/fuzz/deserialize.cpp     d1d0a094ae939f2ca3a9284de65e423810d86278cb864cd24ce21d2c6fa713ce

The final ASan/UBSan/libFuzzer binary is
`feedc9089ae828e36f688d6f3e42ebe964483fca7f54dcb6408c18d55a205b74`.
All four exact fixed replays ran once, exited 0, and produced no artifact.
Their log SHA-256 values are:

    txout  13cf3f35a3fa25d3cc917416c92db3ab826f9da31aae27ac1f91da16dc0796a8
    coins  c16c8421550c111dcebe4a2622ca1b345e5b1ccebe83d908492d1e26b5986567
    txundo c8eff86ff35a63705189d82e004c67a4c1fd6e0b7bc75e88cca7382d8e0cc570
    block  c50f6e9df4ea65b7103c7eedcc259aef59cbd9fd0eb733143169363c80b422a9

Existing corpora were frozen before mutation: txoutcompressor (99 files,
33938 bytes, manifest `e8ef9de26d33fc2d981cb0aef4a4dfb6a787fa2ef6b1d00353e95a8926305b82`),
coins (108/35314, `98769bf334b2343ef72e41100f236acb663ac880452ba4bae59e31fa83895dd5`),
txundo (216/17383548, `a309e79939f43d5c81e3e4821f4497633c944b68c2da00f261695aec10eb1502`),
and blockundo (250/28090566,
`cf473fe6ded44f2ce7227c4ee0087732eb343f0c95174812c634af239eae027d`). All
four fixed replays exited 0 with no new units or artifacts; their logs were
`4f40b96cb6590a1f1767c7ce8f83c57930308ff66857af4f4bb61551089e89f8`,
`6a4464359e16f8808d8070f037849b073e6cfc2e19141eb0b3174da9e66b476b`,
`7d0dc63d29e670825eed7c7690cf40d260a9be8650bc3899379463d57b9f0e89`, and
`bbe0494f865817d02e3b309077825eeb6aa20d7bdbf6d4f0ea26bb100c3e8b88`.

A seeded txoutcompressor run used `-jobs=4 -workers=4 -max_total_time=60`.
All workers exited 0 without artifacts: jobs 0/1/2/3 executed 110/93/175/115
units, added 31/7/54/31, and peaked at 104/104/105/105 MiB. Worker log hashes
are `8326207f94cc0fd6e4aac760eff6c8855931a7a3054b1d170fab4477dde000f`,
`ea0bf3220a59e8a719d8ddcdc0a22c6d3c080b2e266523ecfe58f9bfb3b4a26e`,
`586b32a01c6e235d11f857d8a2e80bf33c39fe772b5be9a7d59218a2f6b9eb61`, and
`4f4ee69238fd818ec5b69665156bc04d6b04d782f6ff0898daa3fab6a94badba`;
launch log `3f0295fc853c167ddaa00ec00c2f6f52f52a1c2ce8cee9787b3bfa2b621e5e10`.

### Differential oracle proof

A narrow production mutation removed only `AmountCompression::Unser()`'s range
guard. It produced compressor SHA
`031da1dcbe683dc7f4cacb4a4d10e76accf9a4ba1f51ae4cdd43f0db9b4dbb03` and
binary SHA `2b8f6dd82c3e4ea073e98697302abcd9cb6eef154fbfe28b89a078e1f8795c8a`.
The exact inputs then aborted at production range assertions or
`Coin::Unserialize()`, with logs
`7ec4dc9fb8cb0533d05fb321d0faeb14e9fce222c84c07da93d804cb1382dd23`,
`4bda675966d4d7a389d240fe6a47d689fd13a66dea7f5a335ed5434f423fe29d`,
`8b0e32968d62183397d04c40f8d7bb9b4b8761f2c3e31a19149e029541ac5f84`, and
`ad3391d405cb67367fbfd283d31c6edfaf2d69072553a4a774470e33ceb6767f`.
This proves the production contract fails immediately, but does not by itself
attribute the result to the fuzzer commit.

For the independent oracle proof, a temporary mutation removed the production
`Unser()` rejection, `AmountCompression::Ser()` range assertion, and both Coin
`Assume(MoneyRange())` checks while retaining the new fuzzer assertions.
Mutated compressor SHA:
`4e271c01b7ef0ae3f08d187a9edbba5e105f58855acb0b415290d7a4bba2efa0`;
mutated `src/coins.h` SHA:
`00c282e0e85a29d3c16a6ae017bea2fec1e2bf9387f11a2f5252e04f492cf866`;
mutated binary SHA
`2759ae272cc0e82c83700527acd83258d44dc39c53a351b8255334e6cc602872`.
Each exact input executed once and exited 77 at the new harness assertion,
with no artifact: txout line 308 log
`1e09fa5ea4dec3ac8247b2dbf6f62ab2b8987eaff72ea19065c330105d65a1f3`, coins
line 234 log `4871401c8b112c1f530e3dc06e949f1160c128b08c0f67c4befdb1055c3cd37d`,
txundo line 220 log
`b715fdddebbc280bf4ae14afe92f19a956478c71c8a1071ff9336dbd95c2e2f6`, and
blockundo line 229 log
`059f2d436318c05c1d90af7f65dac8c6ca4641ba6ebadb60e09fde0236134ec8`.
This synthetic mutation is the strongest proof that the postconditions close
the old silent-acceptance gap; it is not a claim that the fixed tree accepts
invalid amounts.

### Existing findings, test gap, and verifiers

The existing ledger remains Core-relative: private-broadcast failed-send
retention is Medium and feature-conditional; empty HEADERS IBD handoff is
Medium availability; ecmult scratch wrapping is Medium with low demonstrated
Core reachability; forced 10x26 magnitude-32 normalization and
SHA/HMAC/RFC6979 retention are reachability-limited Medium
correctness/hygiene findings; last_tx_time, process-message local storage
failure, oversized transport types, and banman invalid-subnet/unban integrity
are Low or nice-to-have under current callers. No clean-master production bug
was established in the previously audited addrman, coins-cache, txgraph,
txdownloadman, txrequest, connman, eviction, headers-sync, UTXO snapshot,
mempool persistence, package evaluation, handshake, BufferedFile, block-index,
tx_pool, policy estimator, autofile, checkqueue, compact-block,
PartiallyDownloadedBlock, CBlock, or CBlockHeader reset paths. Invalid fuzzer
state is not a production finding. No other l0rinc commit applies here.

`src/test/compress_tests.cpp` adds deterministic production coverage, but the
fuzz-only build did not expose `test_bitcoin`/`compress_tests`, so those tests
were not executed in this audit. Verification included the fuzz build, clean
master controls, fixed exact and corpus replays, the seeded four-worker run,
both differential mutations, sanitizer instrumentation,
`clang-format --dry-run --Werror`, and `git diff --check`. No fuzz or mutation
process remains running.

## UTXO total-supply state oracle audit (2026-07-20)

Source commit: `7935fe4324` (`fuzz: strengthen utxo total supply state
oracle`). The audit base was Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; this was also the
`l0rinc/master` tip. The source parent was
`f5845897328d844480a0aea85f0cbbafff3a0f0b`. No l0rinc pull-request commit
applied to this target, and no prior source audit covered
`src/test/fuzz/utxo_total_supply.cpp`.

### Oracle changes and Core boundary

The original harness requested `CoinStatsHashType::NONE`, making
`hashSerialized` a zero-value field, then compared only that field after a
rejected `MineBlock()` transition. The new oracle requests
`HASH_SERIALIZED`, asserts the implementation invariant
`nTransactionOutputs == coins_count`, and compares the pre-transition and
post-transition `CCoinsStats` values for height, block hash, transaction and
output counts, bogo size, serialized hash, total amount, and coin count. It
deliberately excludes `nDiskSize`: `ForceFlushStateToDisk()` may compact the
database and legitimately change that estimate.

The production API is used by Core's local `gettxoutsetinfo` RPC through
`src/rpc/blockchain.cpp:1017`, and by AssumeUTXO snapshot loading and
background validation through `src/validation.cpp:5928` and `6062`. A
counter defect would make the local RPC's `txouts` result inconsistent. The
rejected-block check targets state leakage across chainstate transitions.
Peer-supplied invalid block bytes do not directly invoke this statistics
routine in normal Core validation. No production bug was established on
master. Severity is informational/Low oracle hardening, not High or
Critical: this audit does not demonstrate a remotely triggerable consensus,
memory-safety, or invalid-block vulnerability.

### Corpus and clean replays

The frozen corpus was
`/tmp/bitcoin-utxo-total-supply-20260720/frozen`: 1,146 files, 716,166 bytes,
manifest
`8f7bca1b4c67f686008d09d1f285788c510dd7773cd205da547ef9bcfa26beb2`.

The original fuzzer source SHA-256 was
`23a7b4e91605c6d07c9fae4ffbd67dc20b0c916ecef8a909449ba0c80f23ca82`.
The enhanced source SHA-256 is
`f0ab4fdb2eb73531a7bb19a20da2c5c9265778f6a3ec7e886799c1136d36c340`.
The final fuzz binary SHA-256 is
`d53656f8a61924007e8f019dcacd324169e630c322a2716b21584d3f29578e0f`.

Both full replays used `-merge=0 -runs=1146 -timeout=60 -rss_limit_mb=0
-use_value_profile=1 -print_final_stats=1`. The original replay exited 0
after 1,149 executions, added zero units, and peaked at 454 MiB; its log
SHA-256 is
`3a0da9bd84a8d6c152c548281fa764a88cc4f9ac0dc3d7ed1eab3f59e9c79d72`.
The enhanced replay exited 0 after 1,149 executions, added zero units, and
peaked at 446 MiB; its log SHA-256 is
`cc4e777c2ff6cdddc9191bb148e5d057b74d61a3f159fb549f0affa4fe9dc2eb`.
Neither replay produced an artifact.

A four-worker run used `-jobs=4 -workers=4 -max_total_time=60` with the same
timeout, RSS, value-profile, and final-stat options. Jobs 0, 1, 2, and 3
each executed 1,149 inputs, added zero units, exited 0, and peaked at
459/458/453/470 MiB respectively. The worker log SHA-256 values, in job
order, are:

    84ae5d5c7530a9eda1a4f3a14bde5a01cda452e6a375c3d85140c06a10b04c09
    b17406ed8a7429bdde0d1d81185ceff8386d0a28d0e02ac48098763f0ceca6e1
    695050a8620eb3caa93de1e617f7485479785f64cce5990cb08604c877f078a1
    7882db03c163c11dda8624e67db44ae42c40a9131e2858bde89482e63109b85

The worker launch log SHA-256 is
`801791ec4272c19daecaaa5c62369285b383f1ab3d29328f887e9f7a78b0bf07`.
No worker artifact or orphan process remained.

### Differential proof

The exact one-byte corpus input used for the production mutation was
`/tmp/bitcoin-utxo-total-supply-20260720/frozen/241cbd6dfb6e53c43c73b62f9384359091dcbf56`:
bytes `ad`, SHA-256
`22adaf058a2cb668b15cb4c1f30e7cc720bbe38c146544169db35fbf630389c4`.

A temporary production mutation removed only `stats.nTransactionOutputs++`
from `src/kernel/coinstats.cpp`. The mutated source SHA-256 was
`b12f8b4d766413f0c61e97dcf73f7bd55cf59e2864bc2eb4afefda37c714e803`.
With the original harness source, whose SHA-256 was
`23a7b4e91605c6d07c9fae4ffbd67dc20b0c916ecef8a909449ba0c80f23ca82`, the
mutated binary SHA-256 was
`9d82a4b1b41040c925fdef3f8827a29750abbaa866e9b3442f23cee1e92619fd` and
the exact input exited 0. The control log SHA-256 was
`7c904554738d69ef01743cfefd1e0cfac9d9ceaecc27eab2e60d7d35d4e41554`.

With the enhanced harness and the identical production mutation, binary
SHA-256 was
`0414fcdce332e581333c8f7dcefe9b0d595fb009ff122ed65b8d692174f17eb4`.
The exact input exited 134 at `utxo_total_supply.cpp:108` on
`nTransactionOutputs == coins_count`; the diagnostic log SHA-256 was
`16adab78b5c0a6295914913857c6e873f00628f49a8e566b2b99d08bc046e05f`.
Restoring production and rebuilding reproduced the final binary hash above;
the same seed exited 0 with no artifact, with log SHA-256
`dd80088dead2cb2f0b92848cd92e9b71ec273a9a2540c072d1ba6446b286ad19`.

This is a differential proof that the postcondition closes the original
silent acceptance gap. The mutation is synthetic and does not prove that
master contains a counter defect. Existing findings remain rated against
their actual Bitcoin Core callers and input origins; this target adds no
new production finding.

### Verification

`cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j 8`,
`git diff --check`, clean-master replay, enhanced replay, four-worker replay,
and both sides of the exact differential test passed as recorded above.
`clang-format --dry-run --Werror` still reports two pre-existing violations
at `utxo_total_supply.cpp:59-60` in the untouched `PrepareBlock`
initializer; unrelated lines were not reformatted. The temporary source
mutation was reverted before the source commit, and no fuzz or mutation
process remains running.

## TxOrphanage simulation oracle audit (2026-07-20)

Source commit: `302a530ac76bcd7d270819a6dce632b9eb5e7396` (`fuzz: enforce
txorphanage model ordering and state`). The audit base was Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; this was also the `l0rinc/master`
tip. The source parent was
`7935fe43242bdb4a79d392e417484ca974db0421`. No l0rinc pull-request commit
applied to this target, and the prior ledger contained no
`txorphanage_sim` source audit.

### Oracle changes and Core boundary

`txorphanage_sim` already compared a real `TxOrphanage` with a vector model,
but most public state was checked only after the whole scenario. The new
`AssertSimState` runs after every mutator and trim cycle. It compares
announcement and unique-orphan counts, deduplicated usage, latency score,
global limits, per-peer usage/latency/counts, work availability, witness-id
lookups, and peer announcements. A later erase or trim can no longer hide an
intermediate accounting mismatch.

The old `GetTxToReconsider` model accepted any reconsiderable transaction.
The new check requires the first simulation entry for the peer, matching the
production `ByPeer` ordering of peer, reconsiderable state, and entry sequence.
This is the oldest reconsiderable announcement for that peer.

Bitcoin Core reaches this API through
`TxDownloadManagerImpl::GetTxToReconsider()` at
`src/node/txdownloadman_impl.cpp:574-576`, and consumes it in
`PeerManagerImpl::ProcessOrphanTx()` at `src/net_processing.cpp:3322`.
Remote peers can create multiple missing-input orphans, but an ordering
mistake would only change which peer work item is retried first. Clean master
has the correct implementation and no production bug was established.
Severity is informational/Low oracle hardening. A hypothetical wrong-order
implementation is reachability-limited peer-work scheduling/availability,
not a consensus, invalid-block, memory-safety, or cryptographic issue, so it
is not High or Critical. Existing findings remain rated against their actual
Bitcoin Core callers and input origins.

### Corpus and replays

The frozen corpus was
`/tmp/bitcoin-txorphanage-sim-20260720/frozen`: 1,123 files, 313,410 bytes.
The sorted manifest SHA-256 is
`4143e0e09efe651ea052a865279dc30a8d6799e59c12954cd915c75e35105c46`.

The original `src/test/fuzz/txorphan.cpp` SHA-256 was
`0689cbe0fa6babed429447dde57541b48f60d557a0564f39c4738c4415933e39`.
The enhanced source SHA-256 is
`79f0928611065ff8f90e558abc7d5e7b36a36ffc657b1ddae3c0bd1d13f217d5`.
The restored production `src/node/txorphanage.cpp` SHA-256 is
`7428f6933303e8237711a37cdabb3bba71745ccc7dd4b8cc0f5e39127847203a`.
The final sanitizer fuzz binary SHA-256 is
`c629d36231989a4889b2d6acf744b5cb17e311b0d7ccb801dca69427532c6b9f`.

The original replay used `-merge=0 -runs=1123 -timeout=60 -rss_limit_mb=0
-use_value_profile=1 -print_final_stats=1`, exited 0 after 2,126 executions,
added zero units, and peaked at 536 MiB. Its log SHA-256 is
`01f6f9fb08698356b6959235f65ab11bc267be2ed43bdf9706af8834b31d286d`.
The final enhanced replay used the same command, exited 0 after 2,126
executions, added zero units, and peaked at 540 MiB. Its log SHA-256 is
`59f39295ccf360e7ab3e42e07e925eedc249f8ca8b4cce8f5883cda044b75309`.
Neither replay produced an artifact.

A four-worker run used `-jobs=4 -workers=4 -max_total_time=60` with the same
timeout, RSS, value-profile, final-stat, and frozen-corpus arguments. Jobs 0,
1, 2, and 3 each executed 2,126 inputs, added zero units, and exited 0; peak
RSS was 540 MiB for every job and elapsed times were 164, 164, 164, and 165
seconds. Worker log SHA-256 values, in job order, are:

    8d38a0c956819ea52cc98a1400def2cd36fa54dbe489aaf12f6f32d6401c31d2
    b429274483c3c299277492226c68ade276c6890feb7e2f951f5b1312ae36cd77
    5668cd0b288b1065713f7806c8f4bdc6f573c991fbc2dce63ffa591e4a4b09d4
    cbd0e86cd8871d18023e9b9dea82465db87e2b159b13158339837d6073f36172

The worker launch log SHA-256 is
`b175ba634bd7490b25bd4a814ac0d5739ba7f8a52b29dbe9d176a98ccbd64d1b`.
No worker artifact or orphan process remained.

The final binary contains `__asan_init` and UBSan handlers. With
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1`, the frozen
corpus replay exited 0 after 2,126 executions, added zero units, and peaked
at 553 MiB. The sanitizer log SHA-256 is
`9c9f316ec9cad7a6b1e3ffe0113f34b6412905841cd4ee8fc37e0b493fc4b827`.
No sanitizer report or artifact was produced.

### Differential proof

A temporary production mutation changed
`GetTxToReconsider()` from
`lower_bound(ByPeerView{peer, true, 0})` to
`upper_bound(ByPeerView{peer, true, UINT64_MAX})` followed by decrement. It
therefore selected the newest reconsiderable announcement rather than the
documented oldest one. The mutated production source SHA-256 was
`9d305138b91037f88b8d28209da665fb25f8a0571bc2e6d583fd119d1e0fe90a`.

The exact corpus input was
`/tmp/bitcoin-txorphanage-sim-20260720/frozen/009d60a071cfdfd8d1024e28880ba9b153824743`:
431 bytes, SHA-256
`545ff4bf8cbc00948733f232f808d55cd938936c055943e53560b97a653fdcb0`.
With the enhanced harness and this mutation, binary SHA-256 was
`6589ca1dffb5d8fd4eab4d46748e1bec046d4a04bc2a82db302f9533ba4c7c55` and the
full replay exited 134 at `txorphan.cpp:731` on the exact ordering assertion.
The full mutation log SHA-256 is
`ca629385cd7dd4ab539ecc33242c695095ddb79367be1f82f284099339fd7df8`.
The one-input diagnostic log SHA-256 is
`70361e74ba8b14e41ee7a8c3683834445f2693b332a41f6cfb570edf8434a1bb`.

With the original harness source and the identical production mutation, the
same input exited 0. The control binary SHA-256 was
`755a98ba68835c5cbc4f7e8fcdba032c1c6645b1fea21271751471f39e7e6442` and the
control log SHA-256 was
`1d9114130f53f734c3e37ee9d86fdd257e4bc8182b69bd240045fbfd552ade97`.
This proves the old model accepted a wrong-order implementation while the
new assertion detects it. It is an oracle differential proof, not a claim
that clean master is wrong. Production was restored before the source
commit, and the exact seed then exited 0 with final log SHA-256
`312358193b758843adff84632bc56dc673a03ec2da299fe27c3b7495d780806a`.

### Verification and test gap

`cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j 8`,
`git diff --check`, baseline and enhanced corpus replays, the four-worker
run, sanitizer replay, and both sides of the exact differential test passed
as recorded above. The configured fuzz-only CMake build has no `test_bitcoin`
target, so the dedicated orphanage unit suite could not be executed in this
build; no production behavior was changed and no deterministic production
regression test was required.

`clang-format --dry-run --Werror` reports pre-existing violations at
`txorphan.cpp:212, 316, 331, 356, 484, 489, and 754`; unrelated lines were
not reformatted. No fuzz, sanitizer, or mutation process remains running.

## P2P headers presync block-index state oracle audit (2026-07-20)

Source commit: `515620e10c51afdf26b63cc5f033527aca4acdfd` (`fuzz: assert
low-work headers preserve block-index state`). The source parent was
`302a530ac76bcd7d270819a6dce632b9eb5e7396`; the audit base was Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b`, also the
`l0rinc/master` tip. No l0rinc pull-request commit applied to this target.

### Core boundary and severity

`p2p_headers_presync` exercises Bitcoin Core's
`PeerManagerImpl::ProcessHeadersMessage()` and
`PeerManagerImpl::TryLowWorkHeadersSync()` path at
`src/net_processing.cpp:2813, 3007, and 3149`. The fuzzer supplies untrusted
peer HEADERS, CMPCTBLOCK, and BLOCK messages. The generated chains are kept
below `MinimumChainWork`, so this path must discard them without mutating the
local block index or its candidate bookkeeping.

The old oracle only compared the final block-index size. The new oracle
snapshots each existing `CBlockIndex`, including its map object and hash
pointer, predecessor and skip links, header and storage fields, chain work,
transaction counters, status, sequence/time metadata, the best-header
pointer, and the active-chain candidate set. It checks this state after every
message and after the complete input, preventing an unchanged final size from
hiding a transient or metadata-only state transition.

Severity on master is informational/Low oracle hardening. Clean master
reproduced no production failure. This target does not expose consensus
failure, invalid-block acceptance, memory safety, or cryptographic impact;
invalid peer blocks remain below the work threshold. A hypothetical mutation
of best-header metadata could affect synchronization state, but that is
mutation evidence rather than a master finding and is not High/Critical. No
production fix or deterministic regression test is claimed.

### Source and corpus identity

The original harness source SHA-256 was
`4a0ae1131b69c79bd8342aa3170371d62de55902ddcb4d390f7e3556eee355a3`.
The strengthened harness source SHA-256 is
`40df280c9922654b4aa9fab597d2d596c00918230f9d6bb038ea95057dc2b630`.
The restored production `src/net_processing.cpp` SHA-256 is
`afc14cf644760b60670fa82fb088b03ffa792d421a52c5f6c73b2e67672cf419`.

The frozen manifest is
`/tmp/bitcoin-p2p-headers-presync-20260720/manifest.entries`: 602 files,
2,037,636 bytes, minimum 1 byte, maximum 46,865 bytes. Its sorted manifest
SHA-256 is
`4a8b3262415f96c956cac3e8a252f600ac255cb78586f749a6ec93e61072fcd3`.

LibFuzzer appends discovered inputs to a corpus directory. The shared working
directory consequently grew from the manifest's 602 files to 1,435 during
the campaign. All authoritative normal, sanitizer, and worker replays used
separate 602-file copies reconstructed from the manifest; the mutable working
directory is not evidence.

### Corpus and sanitizer replays

The non-sanitized fuzz binary was built with `BUILD_FOR_FUZZING=ON`,
`BUILD_FUZZ_BINARY=ON`, and an empty `SANITIZERS` setting. Its SHA-256 is
`7f162170dd16e6bbead51d50b5e930518963edf1c4a6118b0e804f47d0ca1b1f`.
The custom non-sanitized driver does not accept libFuzzer flags: the exact
input was replayed by passing only its path and exited 0, with log SHA-256
`76163b77031e432cdffce95fcdf014d394f841c6dbfac5ea50dd15cca9ba2cda`.
The isolated 602-file corpus exited 0, with log SHA-256
`93b1129e848e131a7e9af82e28274955a93bda2878dc9b821cb5e0560db7647f`.

The sanitizer fuzz binary was built with `undefined,address,fuzzer`; its
SHA-256 is
`ce1cc7451fa815d06c71b9c6a2bbd63c238895cf24decbf52a12d8db1d5054d6`.
With `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1`, the
isolated corpus replay exited 0 after 714 executions, added zero units, and
peaked at 501 MiB. The sanitizer log SHA-256 is
`db0332428f7ad26f8dfebec633636897ea3f320f83ab90aec84abe8090bd3fbe`.
No sanitizer report or artifact was produced.

A four-worker sanitizer run used `-jobs=4 -workers=4 -max_total_time=60`
with timeout, RSS, value-profile, final-stat, and isolated-corpus arguments.
It started from 602 inputs, completed 7,724 executions, added no artifacts,
and peaked at 503 MiB. The parent log SHA-256 is
`79102f162a402413c1c4a1e5b905bc88b504ca1b67408c91f64fefc7e499cf59`.
Worker log SHA-256 values are:

    fuzz-0 b21360dca73d8abdff349d3fce5a38343be3fa99d1f83a15d2c5829f57d17156
    fuzz-1 3e6a86ba931f4a97200e1508d943df158bd54c534be44f53f12acccfc5fd5a66
    fuzz-2 9b2d7f4eddd1d9565f1d7ac58375a84dcec31c67415ce115e3f04486a1657a5f
    fuzz-3 eb7000a72c7a1144c655c56c928c702e6a52028bb23d691a406b4fea790eb6e3

No worker artifact or orphan fuzz process remained.

The pre-change full-corpus baseline used the original size-only harness,
exited 0 after 710 executions, added zero units, and peaked at 500 MiB. Its
log SHA-256 is
`3b25120b2af5d743cb3d0373462c59eb963ff2dd3d14b2f2a8216c78964b3e6e`.

### Differential proof

The exact input was
`/tmp/bitcoin-p2p-headers-presync-20260720/first-failing-input`: 235 bytes,
SHA-256
`23ef66c19b8d4e22a53266e32bd6362a061ddc7383147d8a358d13901dfdf2e8`.

A temporary production mutation inserted
`WITH_LOCK(cs_main, ++m_chainman.m_best_header->nTimeMax);` in the low-work
branch. The mutated production source SHA-256 was
`bfddbac97e4710c81081a9814b002cbefde5bba89a88cb22a1a8483036f90722`.
With the original harness, the mutated binary SHA-256 was
`60fd815941c192237b60c9a529508bdc88e286bf9284860ce5a60af9e6fa6778`; the
input exited 0 and the control log SHA-256 was
`f68b57d0bde1e436d99166435d30b98c5ce0e30ccb76e9f19e0f7d8adc9c3d58`.

With the strengthened harness and the identical mutation, the binary SHA-256
was `dd3ba40fcc6175e5b20e925fbb7c9164e14ebeb643acaae6452221e3c0a5e129`.
The exact input exited 134 at `index.nTimeMax == snapshot.nTimeMax`; the
diagnostic log SHA-256 was
`b2aaf163d5789ef3fffcbe899c2639255d906da0ef331d5efbd8cf0143c4d74f`.
This proves the old size-only oracle accepts unchanged-size metadata
corruption while the new assertion detects it. The mutation is synthetic and
does not prove that clean master contains a production defect. It was removed
before the source commit.

Restoring production and rebuilding produced sanitizer binary SHA-256
`ce1cc7451fa815d06c71b9c6a2bbd63c238895cf24decbf52a12d8db1d5054d6`; the
exact input exited 0 with log SHA-256
`f45c78dfc4e43509d1be69de556091858d87aca1f172c0462e45177db94316c1`.

### Verification and test gap

The normal build used:
`cmake -S /tmp/bitcoin-secp256k1-audit-current -B
/tmp/bitcoin-secp256k1-audit-current-normal-build -DCMAKE_BUILD_TYPE=RelWithDebInfo
-DBUILD_FOR_FUZZING=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_TESTS=ON -DSANITIZERS=
-DWITH_ZMQ=OFF`, followed by
`cmake --build /tmp/bitcoin-secp256k1-audit-current-normal-build --target fuzz
--parallel 8`. The sanitizer build used
`ninja -C /tmp/bitcoin-secp256k1-audit-current-build fuzz`.

`git diff --check` passed. `clang-format --dry-run --Werror` still reports
the pre-existing missing trailing comma at
`src/test/fuzz/p2p_headers_presync.cpp:80`; unrelated lines were not
reformatted. The configured fuzz-only build has no `test_bitcoin` target, so
the dedicated unit suite could not be executed in that build. No production
behavior changed and no deterministic production regression test was added.
No fuzz, sanitizer, or mutation process remains running.

## `threadpool` lifecycle and shutdown oracle audit (2026-07-21)

Source commit: `d490ea2da497d73bcfb9dc89e5c0e92a3dacb434` (`fuzz: exercise
threadpool lifecycle contracts`). Its parent is
`9876b0a9635fd794c3988e319067e1a811f2ee05`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`l0rinc/master`. The l0rinc path query for `src/test/fuzz/threadpool.cpp`,
`src/util/threadpool.h`, `src/validation.cpp`, and `src/httpserver.cpp` was
empty. No l0rinc commit, later fix, or cherry-pick was used to mask this
clean-master behavior.

### Target and Core boundary

`FUZZ=threadpool` exercises the internal `ThreadPool` used for parallel
prevout fetching and the HTTP worker lifecycle. `CoinsViews::InitCache` creates
the `prevout` pool in `src/validation.cpp:1863-1871`;
`CoinsViewOverlay::StartFetching` in `src/coins.cpp:369-404` submits ranged
`ProcessInput` tasks while `ConnectTip` validates a candidate block, and falls
back to single-threaded fetching when submission is rejected. The HTTP server
uses the `http` pool in `src/httpserver.cpp:77`, submits REST/RPC work at
`190-196`, interrupts it at `1266-1275`, and stops it at `1278-1283`.
The fuzzer input is an internal task/lifecycle command stream, not a
serialized block or peer message. No invalid block by itself triggers this
finding on clean master.

### Oracle and severity

The old harness checked task completion and exception propagation but did not
exercise `Interrupt`, shutdown rejection, restart, controller-thread
`ProcessTask`, or ranged submission. The enhanced harness occupies every
worker with a latch/semaphore, verifies that `ProcessTask` drains exactly the
queued tasks on the controller thread, queues work before `Interrupt` and
verifies accepted work completes while new single and ranged submissions
return `SubmitError::Interrupted`, stops the pool and verifies the `Inactive`
error, restarts it, and verifies every ranged future executes exactly once.
The production header asserts the worker-count postcondition after `Start`,
the interrupt flag immediately after `Interrupt`, and empty worker/queue plus
a cleared interrupt flag after `Stop`.

Clean master reproduced no production failure. This is Informational/Low
oracle hardening only, not a confirmed production bug and not a consensus,
cryptographic, memory-safety, or Critical finding. If the modeled mutation
existed in production, accepting new tasks after `Interrupt` could extend HTTP
shutdown work or prevent `CoinsViewOverlay` from taking its documented
single-threaded fallback. The current prevout path handles a rejected
submission, and the HTTP path returns service unavailable for rejected
submissions. The proof input does not establish a remotely triggerable master
defect or an invalid-block vulnerability. No production fix or deterministic
regression test was added because clean master did not fail.

Existing findings are unchanged and remain rated against master and actual
Bitcoin Core callers: private-broadcast failed-send retention is Medium and
feature-conditional; empty HEADERS initial-sync handoff is Medium
availability/IBD; peer transaction activity refresh and process-message local
block-storage failure are Low; oversized transport types are Low in current
callers; ecmult scratch wrapping, forced 10x26 magnitude-32 normalization,
and SHA/HMAC/RFC6979 retention are Medium reachability-limited correctness or
hygiene findings; banman invalid-subnet/unban integrity is Low/nice-to-have.
The previously audited addrman, coins-cache, txgraph, txdownloadman,
txrequest, connman, eviction, handshake, compact-block, headers-sync, UTXO
snapshot, mempool-persistence, package-evaluation, RPC, and descriptor-cache
paths add no clean-master production bug to that ledger. A nonce with no
cryptographic meaning is not Critical merely because it is not cleared.

### Exact artifacts and replay

The original harness SHA-256 was
`2adfd0f70a470baa074ce8013aa8aec70ae07f3befb92f7e9199fcca4d2841b`; the
enhanced harness SHA-256 is
`73d44ba532fb0849892284ea2f20c23c67fd3fe54d9969e33deb6b9119d070ca`.
The original clean production header SHA-256 was
`c868c908500e1cb5bef2f6c4a78b1113c69df4f408e60e95e2c3e1c6b394eea6`; the
final clean production header SHA-256 is
`e06187fd24011f4de97c39d4ba2b07f58607e4f7e976ea3849e8d2763f680f5b`.

The frozen corpus is `/tmp/bitcoin-threadpool-20260721/frozen`: 45 files,
6,722 bytes, minimum 1 and maximum 2,047 bytes. Its filename-list SHA-256 is
`4b250c1eb56e61f441602a4dd9570c2bf548eeaa9a636a05db93f2cc099c123c`; its
per-file manifest SHA-256 is
`c58aa8556c888dd04fbf7804a1a5f9e47df1c4d6be1513dde943b1769571542f`.

The original sanitizer baseline used binary
`193b2eb1e96d9e9cfaef355d756e7df72340ebfecf98b43f7f3e2d99157a6419` and log
`9b9f5d4d6496708dbbf4d3dccb8d4aa4bff1ac021238f8c455331a93531aeda9`; it
executed 49 corpus inputs and exited 0 with no artifact. The final enhanced
sanitizer binary is
`1bab0c98388bf2933366f7002b1424210e8de018428464d583c4df4f9c9520a7`.
Its 45-file replay log is
`ac2cd070d6e0ee59d028d8fec86364d588a33b5232c7c727e323e137e27671a8`; its
exact zero-byte replay log is
`2483dad6348e533933a96054e9322faaee621c8b975f1927a4b83eadc97893ab`.
Both exited 0 with no artifact; the corpus replay completed 62 executions.
The final normal binary is
`693e9b7dde2ab71ada4254d6d11bd1e690460c061025be4d36c5579085c3197d`; its
normal one-file-per-invocation log is
`15b39c9ef80aa77e27d45f225a837a516c69d7d1e874ad8bee8df4630e4e0739`, and
all 45 files exited 0.

Four isolated enhanced sanitizer workers all exited 0 with no artifacts. The
worker log SHA-256 and summary were:

- `9da33f542c719b06e5cdd1db3900ac21586b16225eaecc9678a30a28cb68049a`: 63 executions, 112 MiB peak RSS.
- `b614674c5c6a908f92b138369a66bdba206a4d42923b68fab4fbcb01726fff78`: 62 executions, 113 MiB peak RSS.
- `a401a45bb5459f980874bd6a24e2191c4fcf872711665ed87e364affb0e23a78`: 60 executions, 112 MiB peak RSS.
- `60a2edf8479489595a1feee189422cfb69ef0e232e71379b63ad496dded25e2b`: 60 executions, 112 MiB peak RSS.

Each isolated corpus remained at the original 45 files. Their aggregate
worker listing SHA-256 was
`8e71e7645a9429b510c0bdf54a9ce75640601672dfee86cba36209c5c8bdbd93`.

### Mutation differential

A temporary production mutation changed the assignment in
`ThreadPool::Interrupt` from `m_interrupt = true` to `m_interrupt = false`
while retaining the production assertion. The enhanced mutated production
header SHA-256 was
`0abc9cdbfcba3c832e3eb7e8e50c9ba986efae966475702f6a5227215238ada9`; the
mutated sanitizer binary SHA-256 was
`6651bc87276efa99b7c7aa342708a6556a4b2e688fb607dad0038015c8de8b21`.
The exact reproducer was the zero-byte input
`/tmp/bitcoin-threadpool-20260721/empty`, SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
The enhanced mutation log SHA-256 was
`6ad14508462d4b0fb8c6082e478507f7db78d80cd165f7975f7e587d2a347b65`; it
exited 134 at `src/util/threadpool.h:280` on `assert(m_interrupt)` during
`Interrupt()`.

The original harness with the same mutation and exact input exited 0. Its
control binary was built from the original harness and unformatted production
header with the same mutation: production header SHA-256
`15c6ffa52b5367041ce29c9f1a3b6592157b5cca3fd04a1d84e8344b19ffefb1`,
harness SHA-256
`2adfd0f70a470baa074ce8013aa8aec70ae07f3befb92f7e9199fcca4d2841b`, and
control binary SHA-256
`68e65458cbdbf79faa5f6395976c7a681309ecdec3c018c9e97e873f83610b89`.
The control log SHA-256 was
`9e379dfc6ff201b47eccf185bf036965b3fbb874abfd139aa3c2daaac833612b`.
This establishes that the new lifecycle oracle, rather than the old
coverage-only target, observes the modeled shutdown regression. The mutation
was removed and the clean sanitizer replay was repeated before commit.

### Verification and test gap

`git diff --check` and
`clang-format --dry-run --Werror src/test/fuzz/threadpool.cpp
src/util/threadpool.h` passed. The sanitizer and normal fuzz builds
completed. The configured fuzz-only trees did not provide a `test_bitcoin`
target, so no unit-suite result is claimed. No deterministic production
regression test was needed because no clean-master bug was confirmed. The
temporary control worktree and build were removed, and no fuzz, sanitizer,
compiler, or mutation process remains running.

Any later potential fix, minor fix, oracle change, or cherry-pick must state
whether it preserves, changes, or masks this clean-master behavior and amend
the relevant commit message and ledger with its exact proof.

## `script_descriptor_cache` cache-state and merge oracle audit (2026-07-21)

Source commit: `9876b0a9635fd794c3988e319067e1a811f2ee05` (`fuzz: model
descriptor cache merge contracts`). Its parent is
`ebcd9cdc6bf6639db4c35c89b7d28dec88e01987`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`l0rinc/master`. No l0rinc pull-request commit was relevant to this target,
and no later fix or cherry-pick was used to mask clean-master behavior.

### Target and Core boundary

`FUZZ=script_descriptor_cache` exercises `DescriptorCache`, used by descriptor
expansion and wallet descriptor-cache persistence. The relevant Bitcoin Core
callers are `WalletBatch::WriteDescriptorCacheItems` and the corresponding
load path in `src/wallet/walletdb.cpp`, plus descriptor expansion through
`src/wallet/scriptpubkeyman.cpp` and `src/script/descriptor.cpp`. The cached
values are public extended keys and local wallet state. They are not a peer
consensus or block-validation boundary, so arbitrary invalid block bytes cannot
reach this finding through the normal Core path.

### Oracle and severity

The harness now keeps an independent model of the parent, derived, and
last-hardened maps. It checks miss/hit results before every cache operation,
overwrites, complete getter maps after every transition, `MergeAndDiff`
additions and diff contents, and conflicting-key `runtime_error` behavior with
the primary cache unchanged. Arbitrary `CExtPubKey` bytes remain valid test
values because the cache API stores values and does not make key validity a
cache invariant.

Clean master reproduced no production failure. This is Informational/Low
oracle hardening only: a hypothetical cache inconsistency could make local
wallet descriptor expansion or persistence incorrect, but it is not a
consensus, invalid-block, memory-safety, concurrency, or cryptographic
finding, and is not Critical. No production fix or deterministic regression
test was added because no clean-master production bug was confirmed.

### Exact artifacts and replay

The original harness SHA-256 was
`9736ac000f088154f7a8285e9d8917b505819202d5718d9c9777f93885cc2a0e`; the
enhanced harness SHA-256 is
`f62b94e597506899a5238e3ddf932ad5d356fc6496bf19a7e9f52a86c7a1668d`.
Restored production `src/script/descriptor.cpp` is
`55cad35197c4757b452b2693500c7ea9d4999360ade65b93da7f75265a320743`.

The frozen corpus is
`/tmp/bitcoin-script-descriptor-cache-20260721/frozen`: 164 files,
5,407,791 bytes, minimum 1 and maximum 800,345 bytes. Its per-file manifest
SHA-256 is
`ad2f61a488c760da28c141474d770036e9ec483242f334e98de4a273b520054b`; its
filename-list SHA-256 is
`5b9e762072cc3b0daeb987efec4a402bcaadc6675faa3c9ec6a167dddb896fbe`.

The pre-change sanitizer baseline used binary
`4a0c17aaff79ca2cc8469a8f5cca44a93501f76503f9f6f6416e802af037c2e0` and log
`9db5960067f350bdc037cef8b3681018ecaa40bc50483a23346c627668bfc888`;
it exited 0 after 165 executions with no artifact. The final sanitizer
binary is
`193b2eb1e96d9e9cfaef355d756e7df72340ebfecf98b43f7f3e2d99157a6419`; its
clean replay log is
`e2021dc7b0975330e62a2ee5d714c8eb8b9faf9f5a6683fd293044a2cb846dc4`.
It exited 0 after 165 executions, with no artifact and no corpus change. The
final normal binary is
`b2c68f906c9a347718de43eab46d8319f73bb62042de59c37f21973732896e66`; its
all-file replay log is
`4f670bff0115bfbfbb8f7c97f97123b64566e436230fa5e9ee8451c914d6ba32`.
The normal run processed all 164 files and exited 0. The exact zero-byte
clean-seed log is
`b31c6dd00dd59ce1623b2955bf65ebea74f76d650686fd3f9fb8d17576141a40`.

Four isolated sanitizer workers all exited 0 without artifacts or source
corpus changes. Their log SHA-256 and summary were:

- `16388cf2d17579e73ac81f6ea1f4b3e246abbe05272faf3a5bdb06564505edc1`: 9,149 executions, 631 MiB peak RSS, 581 new units.
- `62f60ad810455642ccc5ba9b3542253091063dc2d4f4392bbcf2f2a0c8895622`: 9,232 executions, 634 MiB peak RSS, 560 new units.
- `105382abd24089fc9a2e7dd16f366385da12663c716d26c84d1ac834d1599006`: 9,996 executions, 636 MiB peak RSS, 577 new units.
- `4c6627ec8d8732647a5f4e300a4de49ce7b8fe1054a28e56e765a89867d94e67`: 9,280 executions, 634 MiB peak RSS, 545 new units.

### Mutation proof and control

A temporary production mutation changed the loop in
`src/script/descriptor.cpp` that copies
`other.GetCachedLastHardenedExtPubKeys()` during `MergeAndDiff` to a loop that
is never entered. This skipped last-hardened entries during merge. The mutated
production SHA-256 was
`acab5b4be0255bf0cddbc597bb9817d202b64601505ddcbcae2b6dac848fd046`; the
mutated sanitizer binary SHA-256 was
`f2920b7881fc27d4747dfc2956cea7b04445f9158849e43db75638d17f2a4624`.

The exact reproducer was the zero-byte input
`/tmp/bitcoin-script-descriptor-cache-20260721/mutation-empty`, SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
The enhanced mutated log SHA-256 was
`b5301fbbbbead866f6b8a45f00e108a89f3928d30da68844a928ee02c9e4d78c`; it
exited 134 at
`src/test/fuzz/script_descriptor_cache.cpp:63`, where the modeled
`last_hardened` map detected the missing entry. No artifact was emitted.

The original coverage-only harness, with the identical mutation and input,
exited 0 without an artifact. Its mutated-control binary SHA-256 was
`d780e4714c644316bc550213dc9b06a1361f06b6f6ca36fce287c082f5b5f41c`; its
control log SHA-256 was
`b97d588907134221aed352dcd5b39acf04311eb2f7d18ed494be3992e88008d8`.
This differential proves that the new contract assertion is what makes the
regression observable; it does not claim that clean master contains this
production defect. The mutation was removed before the source commit.

### Verification and test gap

`git diff --check` and
`clang-format --dry-run --Werror src/test/fuzz/script_descriptor_cache.cpp`
passed. Production was restored to the clean source before commit. The
configured fuzz-only audit tree had no `test_bitcoin` target, so no dedicated
unit-suite result is claimed. No deterministic production regression test was
needed because no clean-master bug was confirmed. No fuzz, sanitizer, or
mutation process remains running.

## `rpc` block-index and mempool postcondition audit (2026-07-21)

Source commit: `ebcd9cdc6bf6639db4c35c89b7d28dec88e01987` (`fuzz: assert
Core state after RPC calls`). Its parent is
`7ced372e5aa31981c47310585f62d252dba5c83c`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`l0rinc/master`. No l0rinc pull-request commit was relevant to this target,
and no later fix or cherry-pick was used to mask the result.

### Core boundary and severity

`FUZZ=rpc` executes the existing safe RPC command set against
`RPCFuzzTestingSetup`. These commands reach local block and chainstate,
mempool, policy, address, and validation paths through `src/rpc/*.cpp`.
File-writing, file-reading, DNS/network, and shutdown RPCs remain excluded by
the existing target. The fuzz boundary is an RPC method and generated
arguments, normally a local authenticated control path, not a peer message or
direct invalid-block consensus input.

The old harness executed one RPC and stopped checking at the exception
boundary. The strengthened `CallRPC` synchronizes the validation-interface
queue and invokes production `ChainstateManager::CheckBlockIndex()` plus
`CTxMemPool::check()` after every successful RPC and after every RPC exception.
This checks structural block-index and mempool contracts immediately after
partial work. It does not assert that malformed RPC arguments are valid and
does not claim full RPC output equivalence.

Severity on master is Informational/Low oracle hardening, not a confirmed
production bug. Clean master reproduced no production failure, consensus or
invalid-block behavior, peer-triggerable acceptance issue, memory or
concurrency fault, or cryptographic issue. RPC callers are local policy and
control paths rather than peer block validation. A hypothetical RPC-induced
block-index corruption could be a serious node-integrity problem, but the
mutation below is synthetic and master did not reproduce it; no High/Critical
production finding is claimed.

### Source and corpus identity

The original `src/test/fuzz/rpc.cpp` SHA-256 was
`92a5092a3c34742ee707a3289f4154a0c588d231be029adb7ca134d85b530ae2`; the
enhanced source SHA-256 is
`4ae7bdae804c01a3d5080c4d45ab92a59364a496ab86689569975dcb2f112bcb`.
Restored production `src/rpc/blockchain.cpp` is
`6c1bc1260dfa0099161892d95a407ab50f6e9162ecd1bca1cff211d6eea3345f`.

The frozen corpus is `/tmp/bitcoin-rpc-20260721/frozen`: 13,390 files,
152,022,478 bytes, minimum 1 byte, maximum 1,048,576 bytes. The per-file
`sha256sum` manifest SHA-256 is
`8c2ca5f417c71c5a792c7792f5a02283ba9f84f084a183ff6a768ce5c34a0a09`; the
sorted filename-list SHA-256 is
`3f483973065efb3642eed401d8f6250bcd771f5ffa0e52e4973e89f8ecb219f2`.
Authoritative runs used isolated copies so worker corpus state could not
alter the frozen evidence.

### Replay evidence

The pre-change sanitizer baseline used binary SHA-256
`677da14117f700461e315172280f2075419db1c0e83b951c9bffe40db37263a2`; its
log SHA-256 is
`34c805a5143e0a0e204e086f7d55be4aedd8fc0049d27751f10e6fc1cb632a55`.
It exited 0 after 13,531 executions with no artifacts and a 786 MiB peak RSS.

The final enhanced sanitizer binary SHA-256 is
`4a0c17aaff79ca2cc8469a8f5cca44a93501f76503f9f6f6416e802af037c2e0`; its
full-corpus log SHA-256 is
`d3ae76b9ec46e1074a86dda616923bc782bdb04314cfd2b26add6dc18e276337`.
The replay exited 0 after 13,534 executions, produced no artifacts, left the
13,390-file corpus unchanged, and peaked at 807 MiB. The final normal binary
SHA-256 is
`241c669f4b3a08b46b2daba33c62f6f9bcf77411f980502c32a058a149ba9068`; its
log SHA-256 is
`db99d42dafe1be51f9f720e5716f0abb0fb47925bb49974dc715f38ec4b7c087`.
It exited 0 after all 13,390 files in 6 seconds.

Four sanitizer workers used
`-max_total_time=60 -timeout=120 -rss_limit_mb=4096 -use_value_profile=1`,
with isolated corpus and artifact paths. All exited 0, produced no artifacts,
and left their 13,390-file corpus copies unchanged. Executions were 13,534,
13,532, 13,531, and 13,533; peak RSS was 755, 800, 751, and 752 MiB. Worker
log SHA-256 values were:

    fuzz-0 0c0e70a029ae62d17f7783fc1fbf979b307db9cb56f9b2c173d2069b0ed4d6c1
    fuzz-1 e44062cdf6681f36d54d7d945d7f105a6f041bf0705d32fc6191b50b5ef2bc21
    fuzz-2 48c8ac6f8265d78e595ec57175475249235e517ad2a3419aa8e440d3fd23c7c2
    fuzz-3 f1ed22ea69afcb4422028c5d15e35c826a6c5d1cc0fb0f232f92ad9e0c27cf56

### Differential oracle proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation inserted
`++chainman.ActiveChain().Tip()->nHeight` in the `getblockcount` RPC handler
immediately before its normal return. It models a read-only RPC corrupting the
active block-index height while leaving the old harness with no postcondition
to inspect. Mutated production `src/rpc/blockchain.cpp` SHA-256 was
`77add976dffcc86784f2a2490d7bff73c87f5a417fd51a2877cb1deb3bf7a344`; the
mutated sanitizer binary SHA-256 was
`fad5f864778cb7c8502ba849e36272458fb9a4468b8951e0272a86fdece3106d`.

The frozen corpus reached the mutation with the exact 17-byte artifact
`/tmp/bitcoin-rpc-20260721/mutation-artifacts/crash-17a78f476a7e15300670119563801dc80e79abed`.
It contains the ASCII bytes `getblockcount\\ttf`, SHA-256
`d5ce33c0353aa0283e716556728a00d8b2fc5c6d0289efca3c03c56ba9118af5`, and
Base64 `Z2V0YmxvY2tjb3VudFx0dGY=`. Enhanced replay exited 77 after 37
executions at `validation.cpp:5195` in `ChainstateManager::CheckBlockIndex()`,
called from `rpc.cpp:92` `AssertCorePostconditions`; the mutation log SHA-256
is `928f554f4e6a1d95abb3d3ca34ef0cc09fe0d66f7b41bfec045ec0da61e0b87`.

With the original harness, the identical mutation and artifact exited 0 with
no artifact. The control binary SHA-256 was
`cdec1b68a9599e88f1e0dc36d0a09132a5caeb07b81ae2af7f0e7b7b57509785`; the
control log SHA-256 was
`eb92ff239c163cbd0e0c136c91ac88c355caa0062bc6cee4fbe89156b82c34f7`.
After restoring production and the enhanced harness, the exact artifact
exited 0 with no artifact in the final sanitizer binary; the final-seed log
SHA-256 was
`7b3061b5f6ae10095b0016420001061da6ea48034f29032a6a9649c98ebf0b38`.
This proves the old harness did not observe a block-index corruption caused
by an RPC while the new postcondition did. No production fix or deterministic
regression test is claimed because clean master did not fail.

### Verification and test gap

`git diff --check` passed, and
`clang-format --dry-run --Werror src/test/fuzz/rpc.cpp` passed. The sanitizer
and normal fuzz targets were rebuilt after restoring production. The configured
fuzz-only build has no `test_bitcoin` target, so the dedicated unit suite was
unavailable. No production behavior changed, no production bug is asserted,
and no fuzz, sanitizer, or mutation process remains running.

## `validation_load_mempool` dump/load round-trip oracle audit (2026-07-21)

Source commit: `7ced372e5aa31981c47310585f62d252dba5c83c` (`fuzz: verify
mempool dump/load round trips`). Its parent is
`815fe5bf267815bbf70813314b7a0ea261d328e5`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`l0rinc/master`. No l0rinc pull-request commit was relevant to this target.

### Core boundary and severity

`FUZZ=validation_load_mempool` exercises `node::LoadMempool` and
`node::DumpMempool`. Bitcoin Core calls these through startup/shutdown
persistence in `src/init.cpp` and through the `importmempool` and
`savemempool` RPC paths in `src/rpc/mempool.cpp`. The fuzz input is arbitrary
local-file data and fuzzed open/read/write behavior. It is not a peer block,
witness, or consensus-validation boundary, so malformed bytes alone do not
justify a High or Critical rating.

The old harness checked the live pool after fuzzed I/O, but it could not see
whether a successful dump could be read back. `FuzzedFileProvider` deliberately
discards writes, so the enhanced harness keeps that failure-path coverage and
also performs a real-file round trip when `DumpMempool` succeeds. It reloads
the file into a fresh `CTxMemPool`, requires `LoadMempool` to succeed, invokes
the production mempool consistency check, verifies every unbroadcast
transaction exists, and compares persistent transaction IDs, fee deltas,
unbroadcast IDs, total transaction size, total fee, and `load_tried`. The fresh
pool's sequence counter is excluded because it is runtime identity rather than
serialized state.

Severity on master is Informational/Low oracle hardening, not a confirmed
production bug. Clean master reproduced no dump/load failure, consensus or
invalid-block behavior, peer-triggerable acceptance issue, memory or
concurrency fault, or cryptographic issue. A hypothetical dump-format defect
could lose mempool persistence across restart or RPC export/import, but this
audit did not reproduce one on master and found no Critical path.

### Source and corpus identity

The original harness source SHA-256 was
`4de679169905895d57cefca73f3322e1afe975e2cfcdbb8b871581cf33fb7e7e`; the
enhanced source SHA-256 is
`b977739929bc138ceaad1d1a0317dc0e45153a19871127490de8c7e24bb4750e`.
Restored production `src/node/mempool_persist.cpp` is
`3ac7d1c297009783f55fd4ff5058c5efbd0896bc4dd04d342cc9d86807c6379a`.

The frozen corpus is
`/tmp/bitcoin-validation-load-mempool-20260720/frozen`: 1,425 files,
112,122,591 bytes, minimum 1 byte, maximum 1,048,229 bytes. The per-file
manifest SHA-256 is
`234306bc52f95dd0830a42220cb9dd2ed858c15f0409ef86e4114278a3415e4d`; the
sorted filename-list SHA-256 is
`793d7e314b30622ee8d325037d7ca792dd7dbb2f881ced709d1c04c130e2ef40`.
Authoritative runs used isolated copies so worker corpus growth could not
change the frozen evidence.

### Replay evidence

The pre-change sanitizer baseline used binary SHA-256
`5a71a991479d01a285e2ba4cf7eb783e09927b8610403602c262d980e31e1d0f` and
log SHA-256
`ce8560397d98a4d002be55d058f03513f4adf7b2b8955c0f0f3b21f05e189861`.
It exited 0 after 1,426 executions with no artifacts and a 547 MiB peak RSS.

The final enhanced sanitizer binary SHA-256 is
`677da14117f700461e315172280f2075419db1c0e83b951c9bffe40db37263a2`; its
full-corpus log SHA-256 is
`fdfb49e796fd609e37179875e6608d9a271ad1d8b74381dc536a7583bc20ac34`.
The replay exited 0 after 1,426 executions, produced no artifacts, left the
1,425-file corpus unchanged, and peaked at 706 MiB. The final normal binary
SHA-256 is
`03b83e5cf1a85f495a19fa2b74c2cc655c1bbb4794664ca38628e3bbd742e393`; its
log SHA-256 is
`9bd2ffced509169ded83e38327817ddaa9a19aec57ae387733946e3831b00e17`.
It exited 0 after all 1,425 files in 8 seconds.

Four sanitizer workers used
`-max_total_time=60 -timeout=120 -rss_limit_mb=4096 -use_value_profile=1`,
with isolated corpus and artifact paths. All exited 0, produced no artifacts,
and peaked at 738, 727, 716, and 702 MiB. Their log SHA-256 values were:

    fuzz-0 cf90708cb20dbc56e5777b6c8fcacd5167f62a27e07a1d0d73f3ce8c812d62fb
    fuzz-1 7e3763d313c61ff74cd452a1410d3b78efeeeb25a0d1b0639cd45e15c22d57f8
    fuzz-2 b29684be91bf70e0432028ae67212172f80fafcc80ece6f6821b037d901bb503
    fuzz-3 e92d44b8d0d8695c5a74ed85593804aecefb73a2322ea8cc992952441b73db3c

### Differential oracle proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation changed `file << version` to
`file << version + 1` in `DumpMempool`, writing an unsupported format version
while leaving the live pool unchanged. The mutated production source SHA-256
was
`29cb4de101abf553c5b12c38fb2562371c8691ba54ec8166776ff8beb780137e`.

The exact replay input was
`/tmp/bitcoin-validation-load-mempool-20260720/mutation-version-empty`: 0
bytes, SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
With the enhanced harness, mutated binary SHA-256
`7fa27b53b9d8996515a01908d316bb7b214ba0cadf13445ef52f5be0f9759342`
aborted at `validation_load_mempool.cpp:133` in `Assert(loaded)`. The fixed
input log SHA-256 is
`a4140de1a1c94c4d8e8ff613c3992e486359f192a27af9c458cb021eb1700f1a`.
The full frozen-corpus replay also aborted on the empty generated seed; its
log SHA-256 is
`d88acd5f9a472780eb765acf4392f8473dc33de8523723a8b16d64f43cb0f498`.

With the original harness, the identical mutation and exact input exited 0
with no artifact. The original-harness mutated binary SHA-256 was
`9e9a30de84e6f5661fe90b16af16b6c73b64fe430dbc57e16ed77798aec4f283`; the
control log SHA-256 was
`08dfb545670f2cccb488c6b89a87b1f41da2b0c6eb758a9c3ba1ed8610d80c38`.
After restoring production and the enhanced harness, the exact input exited
0 in the final sanitizer binary; the log SHA-256 was
`7e1d2a8e7b8cc6f31a9f5e6609694bfadf32892bed83e89967aeb1e8501de903`.
This proves the old harness did not observe a successful dump becoming
unloadable, while the new round-trip assertion does. No production fix or
deterministic regression test is claimed because clean master did not fail.

A first mutation that removed one entry from the serialized `vinfo` snapshot
was deliberately discarded. Its source SHA-256 was
`3a0bad134fb575d9d8a73f6da859b5d98484e06439a967005ae6d5da316a4bd1`, binary
SHA-256 was
`c434b5ceaa4df7cdb79a33469a67264324fef724e4ac27ec37d00de8ed5fe68e`, and
corpus log SHA-256 was
`776e11d6d9d2ffb9a8dd1dde476396591565d4c7127bf2f61e9e613d57218587`.
It exited 0 because the frozen inputs did not reach a non-empty reloadable
pool; no finding is claimed from that mutation.

### Verification and test gap

`git diff --check` passed. The sanitizer and normal targets were rebuilt after
restoring production. The configured fuzz-only build has no `test_bitcoin`
target, so the dedicated unit suite was unavailable. `clang-format` reports
only the pre-existing include ordering at
`src/test/fuzz/validation_load_mempool.cpp:5`; no unrelated formatting was
changed. No production behavior changed, no production bug is asserted, and
no fuzz, sanitizer, or mutation process remains running.

## `script_sign` MuSig2 secret-nonce lifecycle oracle audit (2026-07-22)

Source commit: `e91e0f9a61f5b9432d5d4b805a7764672eec737f`
(`fuzz: exercise MuSig2 signing and secret-nonce lifecycle`), parent
`fad946a20252fb2f28270ba0234f260d38f9c669`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
source branch remains based on latest Bitcoin Core `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`; that commit is an ancestor of
the source checkpoint. `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master.
The target-scoped query over `src/test/fuzz/script_sign.cpp`,
`src/script/sign.cpp`, `src/script/sign.h`, `src/script/signingprovider.cpp`,
`src/script/signingprovider.h`, `src/musig.cpp`, and `src/musig.h` was empty,
so no unique l0rinc commit applies or was cherry-picked. The enhanced source
SHA-256 is
`872e6f1904bfa6c047de9e91ecaa331615075221d13944b6c609bcb51e25584b`.
Production `src/script/sign.cpp` and `src/musig.cpp` were restored unchanged
at SHA-256 values
`f47faa2ecc0e5bd43d49c83f491dbd2fa3d9404c3149d65412c2bcdd1faade48` and
`bed3eb17a1c14553bd14d0de9cd7dfb98130a5d49839361970f364581aaac883`.

### Oracle and Core boundary

The old `FUZZ=script_sign` target discarded signing results and never built a
valid MuSig2 session. The new test-only path constructs two fixed-valid
participants and a valid synthetic transaction, creates both public and
provider-held secret nonces, consumes one session per partial signature,
requires provider session storage to shrink to zero, rejects a retry with a
consumed nonce, aggregates the partial signatures, computes the same BIP341
Taproot sighash, and verifies the final 64-byte Schnorr signature under the
aggregate x-only key. Fixed fallback keys ensure every corpus input reaches
the contract even when random `SignatureData` cannot describe an aggregate.

This is the real MuSig2 secret lifecycle: `src/musig.cpp:237-239`
invalidates `MuSig2SecNonce` after partial signing, while
`src/script/sign.cpp:186` removes the provider session. The wallet/local
signing boundary is reached through `src/script/sign.cpp:282-373` and
`:745`, `src/wallet/scriptpubkeyman.cpp:1328-1339`,
`src/wallet/rpc/spend.cpp:937`, and `src/wallet/wallet.cpp:2196`. An invalid
network block cannot trigger this synthetic signing sequence. Clean master
produced no production failure, invalid-block behavior, sanitizer report, or
concurrency failure. The current-tree rating is **Informational/Low to
Medium** oracle and lifecycle hardening, not a production vulnerability,
fix, or deterministic regression test.

The modeled mutation below removes provider-session deletion but leaves
libsecp256k1's secret-nonce invalidation intact. It therefore demonstrates
stale invalid-session retention, not nonce reuse or private-key leakage. It is
not Critical on the actual caller path. A production defect that retained a
valid MuSig2 secret nonce for reuse, exposed it, or caused nonce/key
compromise would be **Critical** for wallet funds. A nonce without
cryptographic meaning is not Critical merely because it is not cleared; these
MuSig2 secret nonces do carry cryptographic meaning, so valid-secret reuse
would receive the stronger rating.

### Existing findings reiterated

The master-relative ledger remains: feature-conditional private-broadcast
failed-send retention is **Medium**, and the empty-HEADERS initial-sync
handoff is **Medium** for availability/IBD impact. Peer transaction-activity
refresh, local `ProcessMessage` block-storage failure, oversized transport
types, compact-block diagnostics, cache/index, storage, serialization, and
container findings remain **Low** or hardening under current Bitcoin Core
callers. Ecmult scratch wrapping, forced 10x26 magnitude-32 normalization,
and SHA/HMAC/RFC6979 retention remain **Medium but latent/reachability-
limited** correctness or hygiene findings; Banman invalid-subnet/unban
integrity remains Low/nice-to-have. Txrequest, txdownloadman, connman,
eviction, handshake, headers-sync, UTXO snapshot, mempool-persistence,
package-evaluation, RPC, descriptor-cache, and other prior audits have no
additional clean-master production bug to promote here. Later fork commits,
minor fixes, and master changes must be checked against clean master and
identified as masking, preserving, or changing stronger behavior.

### Corpus and replay evidence

The frozen corpus came from
`/mnt/my_storage/qa-assets/fuzz_corpora/script_sign` and was copied to
`/tmp/bitcoin-script-sign-audit-20260722/frozen`. It contains 5,310 files and
61,039,868 bytes, sizes 17..938,133. The per-file manifest SHA-256 is
`ece68043491ff67e92c69399ab5b34098769b78afc962cbc409edce93e1cbf48`; the
sorted filename manifest SHA-256 is
`ac1f5d9b94d73a2883677a4f7ebb4eed8136c9695437eced00ebb5a519c29504`.

The final normal fuzz binary SHA-256 was
`248db38643782f3a505f3b4a85f5b7c7fa6076a1701963971eafb714da00c6a9`.
The corpus replay exited 0 after 5,311 executions, reached coverage/features
4,634/26,303, peaked at 127 MiB, and produced no new units or artifacts; log
SHA-256: `b491a6be9986b603cef177f0a4204f3cf70942179892f3d1dc98ea44929cfef0`.
The final ASan/UBSan binary SHA-256 was
`2cf38531c6312688ad447048133e65d6175a567fcb80f346f5d8f11eb764439f`.
It exited 0 after 5,312 executions, reached coverage/features 13,173/76,730,
peaked at 860 MiB, and produced no new units, artifacts, or sanitizer
diagnostics; log SHA-256:
`f88701af759b4307103d45885c5e57eed0b1033e1c82c011b4cd6c18974cc15f`.

Four isolated ASan workers replayed shards of 1,328/1,328/1,327/1,327 files
and executed 1,330/1,330/1,329/1,329 units. Coverage/features and peak RSS
were 13,105/66,716 at 817 MiB, 13,033/66,612 at 668 MiB,
13,107/67,070 at 642 MiB, and 13,069/67,767 at 714 MiB. Each exited 0
without artifacts or diagnostics. Worker log SHA-256 values were
`d5a87bcdf9b9113d7d7a69a8ffc6162b658b649243a273cb2455aff18a0f1155`,
`12f254ec9313fbfe16cd2359523c000a1c6e21a697e0cdaca3cb35ce49bc3710`,
`92ebff638e44816c35cc5f61cfe5944698219106f50d34513d4a38c81d9e6b0d`, and
`7b89aeff3d02da793646810d9c187c1f1ad7244afa7cc9a6ff6059f77a032fa4`.
The worker filename union matched the frozen filename manifest exactly.

### Mutation proof and test gap

The exact 17-byte proof witness is
`/mnt/my_storage/qa-assets/fuzz_corpora/script_sign/2957816cc9a07a94a14c469295ab655ce08396a8`,
with SHA-256
`90654b2c37437c6a273c83d3f19c1094736c915a81c6a33157464fb6fce26868`.
A temporary mutation at `src/script/sign.cpp:186` commented out
`provider.DeleteMuSig2Session(session_id)` while leaving the underlying
secnonce invalidation in place. Enhanced normal and ASan/UBSan runs reached
`src/test/fuzz/script_sign.cpp:204` and failed the assertion that one session
remained after the first of two partial signatures. Their logs reported the
assertion and libFuzzer deadly signal; SHA-256 values were
`c29f789afbd5d1a2a6f16a470474cabce3aa148874403ac940faba35c51d9680` and
`e1e7cef09870df118d9b967f6a855f7b6bd4229dfa3ca5b0833e271767bc54b3`.
The wrappers were interrupted after libFuzzer's signal handler did not
return; no artifact was produced.

Matched old-harness controls removed only the new MuSig2 block and its
includes, used the same production mutation and exact witness, and exited 0
after one execution with no artifacts. Control log SHA-256 values were
`83d3198193bec112e0e39e73b13e3e4231c5965a3052c566035acb6f5f767f7b` and
`c8222c327727c19cdb58c2c86e9a7f1e9a138877190809825a68a74849ac1b0d`.
This proves the new oracle detects a secret-session lifecycle regression the
old target accepted; it does not upgrade the modeled mutation to a
clean-master vulnerability or Critical wallet finding.

After restoring production and rebuilding, the exact witness exited 0 in
normal and ASan/UBSan final replays; log SHA-256 values were
`10df0ee9b5bdb18e6f306e60ca8305a16c196a6d1dee7147465a78f1dffec9ad` and
`2a93367f0d53421d580d02cad6e7f8ab1cd491f3bfaf90e0cb858f833dc598a7`.
The focused command
`/tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=script_tests,txvalidationcache_tests,validation_tests,bip328_tests,key_tests --log_level=test_suite`
exited 0 with `*** No errors detected`; log SHA-256 is
`6f09c2ceb1b4f080e93707e8759c3e9e8f6927478b2b7b3e411e77f8186a5503` and
the test binary SHA-256 is
`f744420ce0ce9bc48f20bede52e6bc6358f327ce957e6735745619b17f1ae5f0`.
`git diff --check` passed, all temporary production mutations were restored,
and no fuzz, sanitizer, mutation, build, or test process remains. Any later
l0rinc cherry-pick, fork/minor fix, or master change affecting MuSig2,
signing providers, secret-nonce invalidation/deletion, or wallet callers must
be amended into the source commit and this note with target, caller, corpus
or mutation, assertion, failure mode, master-relative severity, and whether it
masks, preserves, or changes the result.

## `script_interpreter` same-transaction precomputation oracle audit (2026-07-22)

Source commit: `0413a4bedb` (`fuzz: assert precomputed sighash equivalence`),
parent `e91e0f9a61f5b9432d5d4b805a7764672eec737f`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
source branch is based on latest fetched Bitcoin Core `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`; `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master.
The exact target-scoped query

    git log origin/master..l0rinc/master -- src/script/interpreter.cpp src/script/interpreter.h src/test/fuzz/script_interpreter.cpp

returned no output. No unique l0rinc commit applied and no cherry-pick was
needed. The enhanced harness SHA-256 is
`284360cfef1e08041f4854996a3de22bfbdedfecd1a90d205df4d0d12732083c`; clean
production `src/script/interpreter.cpp` is
`57a6f328c7e6202f499e48366a86eb26e09c8ebb78e92eef7e839207230be333`.

### Oracle and Core boundary

The first `FUZZ=script_interpreter` block used to pair the transaction being
signed with a `PrecomputedTransactionData` built from a different random
transaction and discarded both results. That is invalid fuzzer-domain
construction and provides no postcondition. The new oracle keeps the generated
transaction and input index as the common domain, compares `SignatureHash`
without precomputation against a cache built from that same transaction, then
repeats the comparison with that precomputation and a fresh `SigHashCache` and
with a cache hit. This covers the real BIP143 precomputation plus per-checker
sighash-cache composition. The existing `sighash_cache` target remains the
cross-script cache-replacement oracle, so this does not rediscover that check.

Bitcoin Core reaches this contract when `validation.cpp:2030` constructs
`CScriptCheck` with `CachingTransactionSignatureChecker`; the checker passes
both `PrecomputedTransactionData` and `SigHashCache` into `SignatureHash` at
`interpreter.cpp:1718`. `VerifyScript` reuses the checker at
`interpreter.cpp:2029`, `:2034`, and `:2080` across `scriptSig`,
`scriptPubKey`, and P2SH redeem-script evaluation. The witness path is part of
consensus script validation and block checking.

Clean current master produced no mismatch or production failure. The
current-tree rating is **Low/informational oracle hardening**, not a
production vulnerability, fix, or deterministic regression test. If master
used a wrong precomputed hash and accepted or rejected an invalid block, the
impact would be **High/Critical** because block script validation could be
incorrect. Malformed fuzzer input alone is not Critical. No nonce-clearing or
cryptographic-nonce claim is involved; a nonce without cryptographic meaning
is not Critical merely because it is not cleared.

### Existing findings reiterated

The master-relative ledger remains: feature-conditional private-broadcast
failed-send retention and empty-`HEADERS` initial-sync handoff are **Medium**;
ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
SHA/HMAC/RFC6979 retention are **Medium but latent/reachability-limited**;
peer transaction-activity refresh, local `ProcessMessage` block-storage
failure, oversized transport types, compact-block diagnostics, cache/index,
storage, serialization, and container findings remain **Low** or hardening
under current Bitcoin Core callers. Txrequest, txdownloadman, connman,
eviction, handshake, headers-sync, UTXO snapshot, mempool-persistence,
package-evaluation, RPC, descriptor-cache, and other prior audits have no
additional clean-master production bug to promote. Later fork/minor/master
changes must be checked for masking, preserving, or changing these ratings.

### Corpus and replay evidence

The frozen corpus came from
`/mnt/my_storage/qa-assets/fuzz_corpora/script_interpreter` and was copied to
`/tmp/bitcoin-script-interpreter-audit-20260722/frozen`: 532 files,
59,274,390 bytes, sizes 4..1,042,390. The per-file manifest SHA-256 is
`8a6ef2ee9dd4d4261f0089c0aa4fb76359aff8dd1453155720e1d1d744122d29`; the
sorted filename manifest SHA-256 is
`68c6f776fac96ee0d0304a51ac52d1550dbdcd11ca5b7d20d8c5c36783011ffc`.

The final normal fuzz binary SHA-256 is
`4cac5a05249cf2c134b7402bd078692aad503d48438bea4718630e97a775459d`.
The full replay exited 0 after 533 executions, reached coverage/features
`660/2,991`, peaked at 176 MiB, added no units, produced no artifacts, and
has log SHA-256
`b8887aff70ee83bc02b3764c5a980de87206420281c89b1bedf9afd9b1701aee`.
The final ASan/UBSan binary SHA-256 is
`a9a0f479b5690fd2673f3abca9ec2f24090d8ea2693882b37f1afb0f4ed12bab`.
Its replay exited 0 after 533 executions, reached coverage/features
`1,026/5,223`, peaked at 515 MiB, added no units, produced no artifacts or
sanitizer diagnostics, and has log SHA-256
`6cd3247f6069fd2f555466201aa422dbd9c1ce63ec5ffe341001600cd199e5a5`.

Four independent ASan workers replayed 133-file shards and executed 134 units
each. Coverage/features and peak RSS were `993/4,623` at 490 MiB,
`992/4,583` at 503 MiB, `1,015/4,734` at 473 MiB, and `996/4,609` at
478 MiB. Every worker exited 0 without artifacts or sanitizer diagnostics;
log SHA-256 values were
`6ec48d0bfafc2538696a2a764d30ecc80da0703e7949ff853ef90a8d591bc284`,
`44130f9397d220b61a1573ec2b36600c50c0e62def63845cb64853091dceaefe`,
`a47cdbe0f4788bd0854da068d294cd613f613440ac44f128d79a75bc4b453fcd`, and
`ac0b88f7d46389d3b28e5cebd635e1401d17038520c5dad30e2aaa54a8ee85b`.
The worker filename union matched the frozen filename manifest exactly.

### Differential proof and verification

This is an oracle differential proof, not a clean-master production finding.
A temporary mutation at `src/script/interpreter.cpp:1637` changed
`cache->hashPrevouts` to `cache->hashSequence`, modeling a wrong BIP143
precomputed component. Enhanced normal and ASan/UBSan corpus replays failed at
`src/test/fuzz/script_interpreter.cpp:37` after 136 executions with internal
libFuzzer exit 77. Mutation log SHA-256 values were
`4d3ee51b433ead8346cca3660e462993765e25da2fa5eb28ad82319c49914dd2` and
`e4696fbf7d1b7bd95e3d43d3b67a29c4b61a5acfb6ddd59da3cf1d1f1ce1d8bc`.
Both produced the same 152-byte artifact
`crash-4ae0d2d379b95a98ab37598cfad45b5450bd4cba`, SHA-256
`5fee4509f363c6e6bf56cbc5e6ff296d962661481df9b863089b4d5600350ce9`.

Matched old-harness controls removed only this new same-transaction
differential, retained the existing `sighash_cache` target, used the same
mutation and artifact, and exited 0 after one execution. Normal and ASan
control log SHA-256 values were
`0cc125dab4c77c440f544b35029b277e0e114dd210e2106f5c823c28e2fe0dc2` and
`d86adbee2d4b9e982df23f68f0f03073a37eb213d24382a53df91f08c5e93397`.
This proves the new oracle catches a precomputed-sighash regression the old
`script_interpreter` harness accepts. After restoring production, the exact
artifact exited 0 in normal and ASan/UBSan builds; final replay log SHA-256
values were
`66034167075c4f2dc06ab1b9b2bfa10f8376fa9dd11efac416c0eb9ef3f4046d` and
`8e71aff5c17d06710fd53943647e881a72ac8732efaf640cd20cca52396eefdb`.

The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=script_tests,sighash_tests,txvalidationcache_tests,sigopcount_tests,validation_tests --log_level=test_suite

exited 0 with `*** No errors detected`; test binary SHA-256 is
`3d3ec37a6ee64c1c2678a16b832fcc87b0e39d85d2850a336feb39f2bd6709cd` and log
SHA-256 is
`c28d737dfd40ea49656f8c74b82c414da157dd26f8db8b4fd4b0bd6d3a994986`.
`git diff --check` and clang-format validation passed. All temporary
production mutations were restored; no production behavior changed, no
production bug or deterministic regression test is claimed, and no fuzz,
sanitizer, mutation, build, or test process remains.

Any later l0rinc cherry-pick, fork/minor fix, or master change affecting
`SignatureHash`, `PrecomputedTransactionData`, `SigHashCache`, or these Core
callers must be amended into the source commit and this note with the target,
caller, corpus or mutation, assertion, failure mode, master-relative severity,
and whether it masks, preserves, or changes the finding. Every production
claim still requires clean-master reproduction or a minimal production
mutation plus the strongest deterministic proof available.

## `script_flags` fresh-checker replay oracle audit (2026-07-22)

Source commit: `fad946a20252fb2f28270ba0234f260d38f9c669` (`fuzz: replay
script flag checks with fresh checkers`), parent
`d0d7cb6c97fd3266acc62a22b792265bf737486e`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
source branch remains based on latest Bitcoin Core `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`; that commit is an ancestor of
the source checkpoint. `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master.
The target-scoped query over `src/test/fuzz/script_flags.cpp`,
`src/script/interpreter.cpp`, and `src/script/interpreter.h` was empty, so no
unique l0rinc commit applies or was cherry-picked for this target. The
enhanced source SHA-256 is
`320647133eb1a4b34804244c3db29a55bbf93b11ff1d64bb767dd1c0d32d81cb`.
Production `src/script/interpreter.cpp` was restored unchanged at SHA-256
`57a6f328c7e6202f499e48366a86eb26e09c8ebb78e92eef7e839207230be333`.

### Oracle and Core boundary

The old `FUZZ=script_flags` target reused one mutable
`TransactionSignatureChecker` after changing verification flags and only
compared boolean results. The new oracle snapshots the original flags,
initializes both `ScriptError` values, and replays both the original and
mutated flag sets with fresh identical checkers. It requires both the result
and the exact `ScriptError` to match. This exposes checker state, including
`SigHashCache`, that might leak from one `VerifyScript` call into the next.

Bitcoin Core reaches this boundary from `src/validation.cpp:2030` through
`CScriptCheck` during transaction and block validation. `VerifyScript`
reuses the checker across `scriptSig`, `scriptPubKey`, P2SH redeem-script,
and witness paths at `src/script/interpreter.cpp:2029`, `:2034`, `:2080`, and
around `:1874`. The current-master rating is **Informational/Low** oracle
hardening: clean master produced no mismatch, production failure,
invalid-block acceptance, consensus divergence, sanitizer report, or
concurrency failure. A real production divergence that accepted an invalid
block would be **Critical**; a demonstrated valid-block rejection or
consensus DoS would require High/Critical treatment according to its actual
Core reachability and impact. Stale diagnostics or malformed fuzzer input
alone are not Critical. No nonce-clearing or cryptographic-nonce claim is
made; a nonce without cryptographic meaning is not Critical merely because it
is not cleared.

### Existing findings reiterated

The master-relative ledger remains: feature-conditional private-broadcast
failed-send retention is **Medium**, and the empty-HEADERS initial-sync
handoff is **Medium** for availability/IBD impact. Peer transaction-activity
refresh, local `ProcessMessage` block-storage failure, oversized transport
types, compact-block diagnostics, cache/index, storage, serialization, and
container findings remain **Low** or hardening under current Bitcoin Core
callers. Ecmult scratch wrapping, forced 10x26 magnitude-32 normalization,
and SHA/HMAC/RFC6979 retention remain **Medium but latent/reachability-
limited** correctness or hygiene findings; Banman invalid-subnet/unban
integrity remains Low/nice-to-have. The txrequest, txdownloadman, connman,
eviction, handshake, headers-sync, UTXO snapshot, mempool-persistence,
package-evaluation, RPC, descriptor-cache, and other prior audits have no
additional clean-master production bug to promote here. Later fork commits,
minor fixes, and master changes must be checked against clean master and
identified as masking, preserving, or changing stronger behavior.

### Corpus and replay evidence

The frozen corpus came from
`/mnt/my_storage/qa-assets/fuzz_corpora/script_flags` and was copied to
`/tmp/bitcoin-script-flags-audit-20260722/frozen`. It contains 2,171 files
and 39,654,526 bytes, sizes 7..100,001. The per-file manifest SHA-256 is
`9c1c6cf5cb38f358a60c300a24bd54ffad64380de6087c553c7bee6eee861013`; the
sorted filename manifest SHA-256 is
`006e92261683af3032681c65e63ee6b76fa788902c9870e9cfb1971e6ede7ba4`.
2,170 files are within the existing 100,000-byte guard; one 100,001-byte
file is loaded by libFuzzer but intentionally returns at that guard.

The final normal fuzz binary SHA-256 was
`2a2747053e84a112adf128c6d36083d609611788d8e41051ebef3e3513ea780a`.
The replay exited 0 after 2,172 executions, reached coverage/features
1,878/11,581, peaked at 84 MiB, and produced no new units or artifacts; log
SHA-256: `71b224c4dc83b58af3083397ddb6f1704ebd7f28fc439967be35d02a3836e7e0`.
The final ASan/UBSan binary SHA-256 was
`bb37f03a80012132bf0c1f114241dcb690e932f998c3a8e8a5cf40370c5e7fc6`.
It exited 0 after 2,173 executions, reached coverage/features 6,654/44,844,
peaked at 675 MiB, and produced no new units, artifacts, or sanitizer
diagnostics; log SHA-256:
`8db5d28de894cfea126a3322abeaf7b45557fb7e7c4726a26732326d48544393`.

Four isolated ASan workers replayed shards of 543/543/543/542 files and
executed 545/545/545/544 units. Coverage/features and peak RSS were
6,630/37,695 at 630 MiB, 6,621/40,273 at 608 MiB, 6,625/40,409 at 600 MiB,
and 6,615/40,269 at 631 MiB. Each exited 0 without artifacts or diagnostics.
Worker log SHA-256 values were
`3560c566b2afd38b364348e73eca65151edcf0c503fa1e46fc92912ff2cb8e47`,
`1af7febf2a7cdeb1934d3c0796d923c7bf08e9ef9dd933917548d2cfa1137c8a`,
`8ef884f205588b0d5e012d477b12cae1e7816f1de99247986551b418a68e9d2a`, and
`24eb24204638601d2445af94dd64b0ec9ed604601648f007423c8dba8b0c569e`.
The worker filename union matched the frozen filename manifest exactly.

### Mutation result and test gap

The stateful production condition under test was a cache hit after a prior
`VerifyScript` call. A temporary mutation at
`src/script/interpreter.cpp:1595` inverted
`if (script_code == entry->first)` to `if (script_code != entry->first)`,
modeling reuse of a cached sighash for the wrong script code. It was removed
before commit and is not a claim about current master. Enhanced normal and
ASan/UBSan corpus replays both exited 0 with no artifact; their log SHA-256
values were `85a2b53371d4b291c59387bc0264e7a4fbb2304f19cad88fc4d8e35050443a17`
and `ef5b401a96d090ff66a69c2ae66b37a22b0913d00873c43b38d965b2cb8153df`.
A 90-second normal generation search executed 52,681 units, reached
coverage/features 1,878/11,774, peaked at 115 MiB, exited 0, and produced no
artifact; log SHA-256:
`43ea129f918239619a7de1cb49f3b3866f7f4405eab33d685fe1b7f495bec1c2`.

This is a deliberate negative result: the exact distinct-script cache-hit
condition was not reached, so no differential finding or severity escalation
is claimed. The unhit mutation is a follow-up coverage target, not evidence
of a master bug. The earlier `sighash_cache` oracle already covers the direct
wrong-script cache mutation, so it must not be rediscovered as a new finding
here. No production fix or deterministic regression test is added.

The focused command
`/tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=script_tests,sighash_tests,txvalidationcache_tests,sigopcount_tests,validation_tests --log_level=test_suite`
exited 0 with `*** No errors detected`; log SHA-256 is
`de0a1fb2b75e8b249a3a0f90d769142b46871f885a95f55379e8459cb4cca174`.
`git diff --check` passed, production was restored and rebuilt after every
temporary mutation, and no fuzz, sanitizer, mutation, build, or test process
remains. Any later l0rinc cherry-pick, fork/minor fix, or master change
affecting script flags, `VerifyScript`, checker state, or Core validation
callers must be amended into the source commit and this note with its target,
caller, corpus or mutation, assertion, failure mode, master-relative
severity, and whether it masks, preserves, or changes the result.

## `signature_checker` deterministic callback and result oracle audit (2026-07-22)

Source commit: `d0d7cb6c97` (`fuzz: assert signature checker contracts`),
parent `2027789f721830debc40d19a6600ae083706d3f5`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
source branch is based on latest Bitcoin Core `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`. `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master.
The target-scoped query
`git log origin/master..l0rinc/master -- src/script/interpreter.cpp
src/script/interpreter.h src/test/fuzz/signature_checker.cpp` was empty, so
no unique l0rinc commit applies. Production
`src/script/interpreter.cpp` was restored unchanged; only
`FUZZ=signature_checker` changed.

### Oracle and Core boundary

The old checker consumed a new fuzzer bit for every callback and discarded
both EvalScript and VerifyScript results and ScriptError values. That made
replay stateful and could not expose stale errors. The new checker captures
fixed ECDSA, Schnorr, CHECKLOCKTIMEVERIFY, and CHECKSEQUENCEVERIFY outcomes;
sets a Schnorr error when the modeled callback rejects; and runs each script
under BASE, WITNESS_V0, and TAPSCRIPT with initialized Tapscript validation
weight. It requires `result == (error == SCRIPT_ERR_OK)`, then replays with a
fresh identical checker and compares result, error, final stack,
code-separator position, and validation-weight state.

For valid flag combinations, VerifyScript receives the same result/error
postcondition. The harness compares a null witness with an explicit empty
witness and repeats the call with a fresh checker. This covers the real
stateless callback contract without treating arbitrary script acceptance as
semantic validity.

Bitcoin Core reaches VerifyScript through `src/validation.cpp:2030` and
`CScriptCheck` while validating transactions and blocks. The interpreter
evaluates scriptSig, scriptPubKey, and P2SH redeem scripts at
`src/script/interpreter.cpp:2029`, `:2034`, and `:2080`; witness execution
reaches EvalScript at `:1874`. The fuzzer therefore exercises a consensus
validation boundary, even though arbitrary fuzzer input is not itself proof
of a peer-triggerable block problem.

### Severity and reiterated findings

Clean current master produced no mismatch, production failure, invalid-block
acceptance, consensus divergence, sanitizer report, or concurrency failure.
The master-relative rating is **Informational/Low oracle hardening**, with no
production vulnerability, fix, or severity-raising claim. A real boolean or
ScriptError divergence at this consensus boundary that accepted an invalid
block would be High/Critical according to demonstrated impact. Malformed
fuzzer input alone is not Critical. A nonce without cryptographic meaning is
not a critical-clearance issue.

The txrequest and txdownloadman transition-model audits were already present
in source history and were not repeated here. The reiterated master-relative
ledger remains feature- and caller-dependent private-broadcast and
empty-HEADERS findings at Medium; cache/index, compact-block diagnostics,
storage, serialization, and container findings at Low or hardening unless
actual Bitcoin Core reachability and impact proves otherwise. Later fixes,
minor adjustments, fork commits, and master changes must be checked against
clean master and recorded as preserving, changing, or masking a stronger
behavior rather than silently replacing its severity.

### Corpus and final replay evidence

The frozen corpus is
`/mnt/my_storage/qa-assets/fuzz_corpora/signature_checker`, copied to
`/tmp/bitcoin-signature-checker-audit-20260722/frozen`: 1,495 files,
1,530,040 bytes, sizes 1..217,149. The per-file SHA-256 manifest is
`0c68a4a1a70225f6729dc4880eba216b630a2da5c52e95a757fb2a205db1f156`; the
sorted filename manifest is
`e04e19817fc436f1606352e6106fa6c1feb9285db5456a4161d07e228b518db1`.

The final normal fuzz binary SHA-256 is
`c383059b2882f6a585f34ff0143375000ecd5958479039094563cb2a940e5bdb`.
The frozen replay exited 0 after 1,496 executions, reached
coverage/features `1125/12171`, peaked at 68 MiB, added no units, produced
no artifacts, and has log SHA-256
`c8fc8e95f75e198e9acd602e1993fe0751aec837fd640c0cc76dbb468bd80a07`.
The final ASan/UBSan binary SHA-256 is
`bd89d4ce559616973769861e1da1ce543c52b40fbe2faacbf1c0f1dac9cc7069`.
Its replay also exited 0 after 1,496 executions, reached
`1758/20211` coverage/features, peaked at 412 MiB, added no units, produced
no artifacts or sanitizer diagnostics, and has log SHA-256
`660d5d2c5de4b35c5a265cd8e130f02205a9b96d26fc20b186380c9fa1876f8a`.

Four isolated ASan workers processed 374/374/374/373 files and executed
375/375/375/374 units. Their coverage/features and peak RSS were
`1685/16917` at 199 MiB, `1656/17116` at 214 MiB, `1655/16908` at 153 MiB,
and `1667/17383` at 161 MiB. Worker log SHA-256 values were:

    worker-0: 475710319a536e698312df64ee42a2969963f997ca7ed3e99dfef66ca2c0a692
    worker-1: a776519f0e58013c45a7de80e59752baf2dd3ee0dc48ab43180a1a6e39941c8f
    worker-2: ac27daa40b34bc3656419287525f5141518ac7af37b5b8c8a8ee936df26f0171
    worker-3: 3aebf2ce1a720d4afb0e6fad6c1d7dc5654cc20d0dfa214efb3d6464c4c8c498

The worker filename union matched the frozen manifest exactly.

### Differential proof and controls

A temporary production mutation at `src/script/interpreter.cpp:1248` changed
`return set_success(serror)` to `return true`. Enhanced normal and
ASan/UBSan runs failed at `signature_checker.cpp:80` on the reduced empty
input with internal exit 77. Mutation log SHA-256 values were
`9fcfedc52a854535914bee46c2612d3c2eb4807cdc809a8ecc4878fc70818758` and
`db3c6361e28eb63ba8b9e9d77f93524cf78e30fb0a2ca365fef1f36582025f9b`.
The libFuzzer artifact identifier was
`da39a3ee5e6b4b0d3255bfef95601890afd80709`; the zero-byte artifact SHA-256
was `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.

Matched old-harness controls removed only the new oracle, used the same empty
input and production mutation, and exited 0 after two executions. Control
log SHA-256 values were
`a9313aea0472146cacca3fb6b3f676c15edc24956f4fa591cf341c5db507475e` and
`f1343d836d55ec7d905fea8bbf523111ba64eb7f662a953f0858c4ec7541db0d`.
Restored clean production accepted the same empty witness in normal and
ASan/UBSan final runs; witness log SHA-256 values were
`d2b471533d95404d672f8c343db6b5ca3b81b11ee3130ba1d2c8a1d5f5647f20` and
`26bdf3ed8249a17c0c0e39b1bb5237a43fad549c3a8b76d68174521735d62b2`.

The exact production mutation also caused four deterministic
`script_tests/script_PushData` failures at `script_tests.cpp:991`, `:996`,
`:1001`, and `:1006`; mutation-focused test log SHA-256 is
`b4619f0f93fcc19ce22f38473f036f303d607a32fb09285a3a9804d016d40354`.
This is defense-in-depth over callback and Tapscript state, not a
clean-master production bug or test-gap claim.

A separate temporary inversion of the production `CheckSequence` condition
at `src/script/interpreter.cpp:599` was not reached by the frozen corpus or
bounded generation searches, so it is not claimed as a finding or severity
result. The normal search executed 8,846 units in 93 seconds with log
SHA-256 `97f23ef1ab4d3e65a7f8fd6cb0eb6d01835e4f2656c36e6d33ebde705d6fd4f6`;
the ASan search executed 23,124 units in 31 seconds with log SHA-256
`8cfaafc88aeb6f87c9b0645dec837d874d9cff2f5479278891eb8f542b6d877`.
Both exited 0 with no artifacts. This is a coverage gap to target separately,
not evidence of a master bug.

### Verification and follow-up

The final focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=script_tests,sighash_tests,txvalidationcache_tests,sigopcount_tests,validation_tests --log_level=test_suite

exited 0 with `*** No errors detected`; final test log SHA-256 is
`6b08cbfbe871510f6da0d54196160e3f7e3870a46314efe5f83412009af43002` and
the test binary SHA-256 is
`d390bdc15c8b4acf19fa775c592cfda9167c4a32e920f9b999aff3dd0e1df25e`.
`git diff --check` and clang-format validation passed, production source was
restored, and no fuzz, sanitizer, mutation, build, or test process remains.
No production fix or regression test is added because clean master is
correct.

Any later l0rinc cherry-pick, fork/minor fix, or master change affecting
EvalScript, VerifyScript, ScriptError, or signature checker callers must be
amended into the source commit and this note with target, caller, assertion,
corpus or mutation, failure mode, master-relative severity, and whether it
masks, preserves, or changes the result. Every future production claim
requires clean-master reproduction or a minimal production mutation plus the
strongest deterministic proof available.

## `eval_script` result/error/state oracle audit (2026-07-22)

Source commit: `2027789f72` (`fuzz: assert EvalScript result and state
contracts`), parent `fc42f885bee7f0f4ca4afaccbce8e3b1e26835e6`, on source
branch `codex/fuzz-oracles-current` in
`/tmp/bitcoin-secp256k1-audit-current`. The source branch is based on latest
Bitcoin Core `origin/master` `32eb52100296718f7c0469e3210ce1db73694793`.
`l0rinc/master` is `d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of
current master. The target-scoped query
`git log origin/master..l0rinc/master -- src/script/interpreter.cpp
src/script/interpreter.h src/test/fuzz/eval_script.cpp` was empty, so no
unique l0rinc commit applies to this target. Production
`src/script/interpreter.cpp` was restored unchanged; this commit changes only
`FUZZ=eval_script`.

### Oracle and Core boundary

The old target evaluated arbitrary scripts for `SigVersion::BASE` and
`SigVersion::WITNESS_V0` but discarded the return value, `ScriptError`, and
resulting stack. The new oracle retains `ScriptError`, requires the production
contract `result == (serror == SCRIPT_ERR_OK)`, then replays the same script
from the same empty stack with the stateless `BaseSignatureChecker`. It
requires identical result, error, and final stack. This detects stale error
outputs, stateful or nondeterministic transitions, and partial stack
corruption without assuming that every accepted fuzzer input is semantically
valid.

Bitcoin Core calls this code through `VerifyScript`: scriptSig and
scriptPubKey are evaluated at `src/script/interpreter.cpp:2029` and `:2034`,
P2SH redeem scripts at `:2080`, and witness execution reaches `EvalScript` at
`:1874`. `src/validation.cpp:2030` reaches the boundary through `CScriptCheck`
while validating transactions and blocks. The replay therefore checks a real
consensus-validation contract, although the fuzzer's arbitrary script is not
itself proof of a peer-triggerable block problem.

### Severity and reiterated findings

Clean current master produced no mismatch, production failure, invalid-block
acceptance, consensus divergence, sanitizer report, or concurrency failure.
The master-relative rating is **Informational/Low oracle hardening**, with no
production vulnerability, fix, or severity-raising claim. The modeled defect
below leaves a success result with a stale error while Core normally branches
on the bool. If a real boolean divergence reached block validation, wrong
script acceptance or rejection would be High/Critical according to the
demonstrated invalid-block impact. Malformed fuzzer input alone is not
Critical. A nonce with no cryptographic meaning does not create a critical
clearance requirement.

The reiterated master-relative ledger is unchanged: feature- and
caller-dependent private-broadcast and empty-HEADERS findings remain Medium;
cache/index, compact-block diagnostics, storage, serialization, and container
findings remain Low or hardening unless actual Bitcoin Core reachability and
impact prove otherwise. Any later fix, minor adjustment, cherry-pick, or
follow-up commit must be evaluated against clean master before its severity is
assigned. If it masks a stronger master behavior, that masking must be
recorded rather than treated as proof that the stronger bug never existed.

### Corpus and replay evidence

The frozen corpus is
`/mnt/my_storage/qa-assets/fuzz_corpora/eval_script`, copied to
`/tmp/bitcoin-eval-script-audit-20260722/frozen`: 1,675 files, 897,350 bytes,
sizes 1..10,010. The per-file manifest SHA-256 is
`ef0f0642e604180b210bfb6d6156f034f8a7b494d33d5b3ecf2f60ace539b6d6`; the
sorted filename manifest SHA-256 is
`f5aabd2466ecd30b810f1bb323f0294abe83d27720f59b0572ea8fcf8271a6ff`.

The final normal fuzz binary SHA-256 is
`495c628d20fb73c6d77d8a268388e9db678f7f4c29c887afabfa63fee4226420`.
The frozen replay exited 0 after 1,676 executions, reached coverage/features
`879/9,169`, peaked at 54 MiB, added no units, and has log SHA-256
`1f8016557aba475039cbee56571a7871723972dc9dcbca601136cd9223b10055`.
The final ASan/UBSan binary SHA-256 is
`0139efd97eb400d60cc41f516e78066c9dfe657aaecc274495e0f953ea5de598`.
Its replay also exited 0 after 1,676 executions, reached
`1,422/15,635` coverage/features, peaked at 367 MiB, added no units, and has
log SHA-256
`460807d7147b11b893f90223de4a2d4214f2133d590c2fa3ac422f0926c977c5`.

Four isolated workers processed 420/420/420/419 inputs and all exited 0 with
no artifacts. Their coverage/features and peak RSS were
`1395/14347` at 171 MiB, `1390/14237` at 177 MiB, `1401/14373` at 163 MiB,
and `1388/14364` at 166 MiB. Worker log SHA-256 values were:

    worker-0: 2bce0c3ff7a20cf51cd19e7c81e9244aae1738a58d7f2b834090e4813100d1e7
    worker-1: 463cf6883aceef8d47973f22216c401a4dfbe6275f79382b250c28587cd8da44
    worker-2: 83707f23cfb248518d589ff438d2beaa7f58a89f78822466ab03ea7725261ef1
    worker-3: b400650914982b44bbe2e4178e06282ed04aa24242f52cdda47c78dd6b7dbb54

### Differential proof and existing coverage

A temporary production mutation changed `return set_success(serror)` to
`return true` at `src/script/interpreter.cpp:1248`. The enhanced normal and
ASan/UBSan corpus runs failed at the new `eval_script.cpp:32` assertion. Their
log SHA-256 values were
`b054b09f10ea838dd1e9d78caad13271387c2dae711d7c7af411800753d4e994` and
`3da9699746527ca7548ee94463d3a0f5c21d6d7355de874d29a9fe988baf1db9`.
Both reduced to the empty input, libFuzzer identifier
`da39a3ee5e6b4b0d3255bfef95601890afd80709`, artifact SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`, and
internal exit 77.

Matched old-harness controls removed only the new assertions, used the same
mutation and empty input, and exited 0. Normal and ASan/UBSan control log
SHA-256 values were
`9cc422505bc930868970e08e76a3397abc2030deaacc23308d73b1cbf6a46bc1` and
`30e2a64de7520fd37304440cc04853422b1a85b3fa1abc02ac9e22ce9ef623ba`.
After restoring production, the exact empty input exited 0 in both builds;
final witness log SHA-256 values were
`7871f63125f53137317aa6ef835a5be13df8e8d4f4cb23ca221207abbb2e136f` and
`ff64406f6a7b6adb1270c8bfd741da2c97e62196989ebe7b4db1ddf367b1306e`.

The existing `script_tests/script_PushData` deterministic tests also caught
the exact production mutation with four failures; the mutation-focused test
log SHA-256 is
`8955b245cd2372a3fa6e8ba084cffe89e2cf72ace7602c37c5ee6e1ba43f51f5`.
This is defense-in-depth over real corpus and state diversity, not a claim
that clean master contained a production bug or that existing deterministic
tests missed this mutation. No production fix or regression test is claimed.

### Verification and follow-up

The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=script_tests,sighash_tests,txvalidationcache_tests,sigopcount_tests,validation_tests --log_level=test_suite

exited 0 with `*** No errors detected`; its log SHA-256 is
`889533826e8fde83ee9c98f93b3825561009b570c92ce21bb07aa2e50a493966` and the
test binary SHA-256 is
`709d7540dac6f5bf9abb2d7620f6ecc38437190a496d5e363a83d1451b2e21bf`.
`git diff --check` and clang-format validation passed. All mutation sources
were restored, and no fuzz, sanitizer, mutation, build, or test process
remains running.

Any later l0rinc cherry-pick, fork/minor fix, or master change affecting
EvalScript, VerifyScript, or ScriptError must be amended into the source
commit and this note with the target, caller, assertion, corpus or mutation,
failure mode, master-relative severity, and whether it masks, preserves, or
changes the finding. Every future production claim requires clean-master
reproduction or a minimal production mutation plus the strongest deterministic
proof available.

## `sighash_cache` cross-script oracle audit (2026-07-22)

Source commit: `fc42f885bee7f0f4ca4afaccbce8e3b1e26835e6` (`fuzz: check
sighash cache across script codes`), parent
`6bcb8be04416f248ef94ff09a32c4057f25db651`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`.
The source branch is based on latest fetched `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`. `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master.
The target-scoped query
`git log origin/master..l0rinc/master -- src/script/interpreter.cpp
src/script/interpreter.h src/test/fuzz/script_interpreter.cpp` was empty;
no unique l0rinc commit applied. Production source is unchanged.

### Oracle and Core boundary

The existing `sighash_cache` target compared cached and uncached
`SignatureHash` results, but always used one `scriptCode`. The new oracle
keeps one `SigHashCache` alive across 100 generated hash types and two
guaranteed-distinct script codes. The alternate is a minimal `OP_NOP`
script, changed to `OP_1` only if the generated script is identical, so
large fuzzed scripts are not copied or hashed twice. For every generated
transaction/input/amount combination and both `BASE` and `WITNESS_V0`, the
harness compares cached and uncached results for both scripts, exercising
cache hits, misses, hash-type buckets, and script-code replacement.

Bitcoin Core reaches this contract through `validation.cpp:2030`, where
`CScriptCheck` constructs `CachingTransactionSignatureChecker`;
`interpreter.cpp:1718` passes that checker's `SigHashCache` into
`SignatureHash`. `VerifyScript` reuses the same checker while evaluating
`scriptSig`, `scriptPubKey`, and a P2SH redeem script at
`interpreter.cpp:2029`, `:2034`, and `:2080`. Cross-script cache reuse is
therefore a real caller boundary, not an impossible malformed-fuzzer state.

### Severity and existing coverage

Clean current master produced no mismatch or production failure. The
current-tree rating is **Low/informational oracle hardening**, not a
production vulnerability or fix. If the modeled defect existed on master,
a wrong sighash could change consensus script validation and potentially
accept or reject an invalid block; severity would be High/Critical according
to the demonstrated invalid-block impact. No nonce-clearing or
cryptographic-nonce claim is involved, and a nonce without cryptographic
meaning is not Critical.

The repository's `src/test/sighash_tests.cpp:sighash_caching` already
deterministically checks a different script code through one cache and
catches this exact mutation. The focused suite passed. This audit does not
claim an existing test gap or a current production bug; it broadens the
same contract to 585 real corpus inputs and diverse generated state. The
prior ledger is unchanged: feature/caller-dependent private-broadcast and
empty-`HEADERS` findings remain Medium, while cache/index, compact-block
diagnostics, storage, serialization, and container findings remain Low or
hardening unless Core impact proves otherwise.

### Corpus and final replay

The frozen corpus is `/mnt/my_storage/qa-assets/fuzz_corpora/sighash_cache`:
585 files, 3,571,029 bytes total, with sizes 2..621,249. The per-file
SHA-256 manifest is
`5e2fa98613a257255d2da7fd9a160f1347f0aa4ddb180aaa01f66c80799b5f31`.
The sorted filename manifest is
`7a22bcb957c50657ddc942867439f7ef01613c90e2b771de2d2e36bd9d028e57`.
Final and worker unions matched it exactly.

The final normal fuzz binary SHA-256 was
`3b65aebd0bff2499aa85f92227ab8e5a3a80e4d17e478db34a926785bbc3f33b`.
The full replay exited 0 after 586 executions, reached coverage/features
`636/8,522`, peaked at 110 MiB, emitted no diagnostics or new units, and
produced log SHA-256
`13109056f5ffa14fbb4ab82ef7e72dd2b93beb332c15ffb6dc152e60923cc69d`.

The final ASan/UBSan fuzz binary SHA-256 was
`4e618583b6551bd19aa1c7853197f2dc9c8885146afaed3bdc2a526824bde14e`.
The full sanitizer replay exited 0 after 586 executions, reached
coverage/features `1,038/14,690`, peaked at 579 MiB, emitted no sanitizer
diagnostics or new units, and produced log SHA-256
`c24a1eef8863ace6199799a6f01405c7e16c7395cb02c9f4bbe11c07f38c37dd`.

Four strict ASan workers replayed shards of 147/146/146/146 inputs. They
executed 148/147/147/147 units, with coverage/features
`1000/13334`, `1030/13719`, `997/13235`, and `1030/13683`; peak RSS was
254/496/224/496 MiB. Worker log SHA-256 values were:

    worker0: f59c1dadef9a261404586c2979b71620099b9b529aaca0dd0b808d3cf5cc5868
    worker1: 164f0310b85583b37aae46888c3f2c6d148a46172f1c28b263f96e5f26773e06
    worker2: de3a22a2b84671d463c3fc12bd54541d8449444d6718eb8a8c2dbb2ea4291e59
    worker3: 54699462edc98a2541b8178fa16f10d72df50a14ecd56ae9924fe63a669baf62

The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=sighash_tests,script_tests,txvalidationcache_tests,sigopcount_tests,validation_tests --log_level=test_suite

exited 0 with `*** No errors detected`; log SHA-256 is
`7f5d81527009a7edf50efbdbe42d96afa442853af43ad5384893ba760d704d05`.
The test binary SHA-256 is
`5790c7452a12dd9a0fef7caca988f673f6603c938f317b4f77fb7a15d16224a2`.

### Differential proof and controls

A temporary production mutation at `src/script/interpreter.cpp:1595`
changed `if (script_code == entry->first)` to `if (true)`, modeling cache
reuse across different script codes. Enhanced normal and ASan/UBSan corpus
runs failed at `src/test/fuzz/script_interpreter.cpp:81`; the internal
libFuzzer exits were 77. Their log SHA-256 values were
`6f82ecfe4161eecbedd04905922244a9623d93fdc7c0d1da5a2588ed1e4ecb74` and
`a905c7aa9532d1aa12aaf8ec7e8fe777b4425d03f3e0cd058d4221f02c97dc71`.

Both runs reduced to the same 11-byte witness:
`43 ff 43 43 43 43 43 da 4d f3 95`, SHA-256
`6bcb62fc931a028b4b59c67fe9583d37e59b0a5c06a90982a21cec49cc7de3b6`.
The normal and ASan crash artifacts were byte-identical. The libFuzzer
artifact identifier was `427f1cffe1e6bef6c1439e1bfc6662e9e772770a`; the
artifact SHA-256 was the witness hash above.

Matched old-harness controls removed only the new two-script loop, used the
same production mutation and witness, and exited 0 after one execution.
Normal and ASan control log SHA-256 values were
`269279767072043ce5dcf927624482122c2304d1fb4ccd7016630e5baf7f7c71` and
`dc58c705ee4aede8a768855d5a83697fcbf5db811268d8d346e2f0f05bb4ede5`.
After restoring production, the exact witness exited 0 in normal and
ASan/UBSan builds; final witness log SHA-256 values were
`69406050b24bffa30f74bcbe24548796b069ac4dfc50685acd1022b2d67a2b81` and
`021476a6ab2fba05732ed45097893b5c778b3b625a5bfd201b8e4176e0185b66`.

An initial implementation copied the full script into the alternate and
timed out under ASan at 68 seconds on frozen witness
`40612e90cc70779e70f10b5f00ab76c230937e11`, input SHA-256
`79e47f1d0850c0c4eb08ec45d3a4f93c273454326a2ab3760a205411f912f1c1`;
the timeout log SHA-256 was
`66512e463aae0be24550a755e67520775cbfe77067b3a054cf1482bba21528ad`.
The final bounded one-op alternate reduced the slowest ASan unit to 52
seconds. This was a harness performance correction, not a production
finding.

The production equality check was restored before the final replay. No
production bug, deterministic regression test, or fix is claimed. Any later
l0rinc cherry-pick, fork/minor fix, or master change that alters
`SigHashCache` or its caller behavior must be amended into this commit and
this note with target, caller, corpus or mutation, assertion, failure mode,
master-relative severity, and whether it masks, preserves, or changes the
finding. Every future production claim requires clean-master reproduction or
a minimal production mutation plus the strongest deterministic proof
available. `git diff --check` and
`clang-format --dry-run --Werror src/test/fuzz/script_interpreter.cpp` pass;
no fuzz, sanitizer, mutation, build, or test process remains running.

## `key` Schnorr/Taproot oracle audit (2026-07-22)

Source commit: `6bcb8be04416f248ef94ff09a32c4057f25db651` (`fuzz: assert
Schnorr and Taproot key contracts`), parent
`9a2f9cedf76cd6f4c16b4e692fd65be3be17ef9d`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`.
The source branch is based on latest fetched `origin/master`
`32eb52100296718f7c0469e3210ce1db73694793`. `l0rinc/master` is
`d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current master;
no unique l0rinc commit applies to `key.cpp`, so no fork commit was
cherry-picked for this target. Production source is unchanged by this
commit; it adds test-only assertions to `FUZZ=key`.

### Oracle and contract

The existing key fuzzer covered ECDSA, BIP32, EllSwift, and ECDH, but did not
make the BIP340/BIP341 transitions observable. The new oracle exercises
`CKey::SignSchnorr` and `CKey::ComputeKeyPair` for all three merkle-root modes:
`nullptr` (no tweak), a null `uint256` (the BIP341 no-script form), and a
nonzero script-tree root.

For each mode, the harness independently computes the expected x-only output
through `XOnlyPubKey::CreateTapTweak`, requires the wrapper and `KeyPair`
validity results to agree, and requires the wrapper and direct `KeyPair`
signatures to be byte-identical. It verifies the signature with the output
key, checks fully-valid and even-parity output keys, checks both full-key to
x-only conversions, and checks `CheckTapTweak` for nonzero roots. It also
constructs arbitrary x-only keys from fuzzer bytes and requires invalid keys
to reject Schnorr verification and tap tweaks. These are narrow production
contracts; the harness does not treat arbitrary acceptance as validity.

### Bitcoin Core boundary and severity

Bitcoin Core signing reaches `key.SignSchnorr` through
`src/script/sign.cpp:112`. Consensus verification reaches
`XOnlyPubKey::VerifySchnorr` through `src/script/interpreter.cpp:1696-1698`,
and Taproot control-block validation reaches `CheckTapTweak` at
`src/script/interpreter.cpp:1920-1924`. The null-root distinction is also
used by tap-tweak construction at `src/script/sign.cpp:334-338`.

Clean current master produced no production failure. The current-tree rating
is therefore **Low/informational oracle hardening**, not a confirmed
vulnerability and not a production fix. The signing path is local, but a
real verifier or tweak mismatch reachable from an invalid block could change
Taproot consensus validation or invalid-block acceptance; that finding would
be rated High/Critical according to the demonstrated impact. Fuzzer bytes
being malformed is not, by itself, Critical. No nonce-clearing assertion was
added: the auxiliary signing input here has no independent cryptographic
secret-lifecycle contract, and a nonce without cryptographic meaning is not
Critical.

This reiterates the existing master-relative ledger rather than upgrading it:
the private-broadcast failed-send retention and empty-`HEADERS` initial-sync
findings remain Medium and feature/caller dependent; package-evaluation,
addrman, cache/index, compact-block diagnostic accounting, storage,
serialization, and container-transition findings remain Low or hardening
unless a Bitcoin Core caller demonstrates stronger impact. The current
compact-block section below records the historical `extra_count` condition
and current-master fix separately. No clean-current-master Critical finding
is claimed by this key audit.

### Corpus and clean replay

The frozen corpus is `/mnt/my_storage/qa-assets/fuzz_corpora/key`, with 1,085
files, 34,658 bytes total, and file sizes 1..32. The per-file SHA-256
manifest is
`0037489243e76f4fc0653e0e5fd69705019c861dd730d96b387761a18bd8b25a`.
The sorted filename manifest is
`d61e480c6b097cc876feabdd81b99363767147596b1e9e5fe7b1ba7a3f6fa442`, and
the four-worker union matched it exactly. The exact witness was
`00235b297abea064ae2e346e492ba338f52a048b`, 32 bytes, SHA-256
`92d12a2e5eeada32fa85804fc9e77a89b50625341cca39498d100fa0d67df638`.

The final normal fuzz binary SHA-256 was
`21df6e94ff7feffcdfa7a1cbfb4910abb2c2f9b43b199ccae5f5e8485d81fb9c`.
The corpus replay exited 0 after 1,086 executions, reached
coverage/features `1,308/11,807`, peaked at 54 MiB, emitted no diagnostics
or new units, and produced log SHA-256
`5978e894f330b7978f51d5b076a9cc7e7ca4370474ab7b9852ac0616157c5de5`.

The final ASan/UBSan fuzz binary SHA-256 was
`8f512d7327c70ffdf12cac7fafb697492397cdd62e23f6dd181eef7f2c09dcf3`.
Its corpus replay exited 0 after 2,088 executions, reached
coverage/features `7,501/28,108`, peaked at 163 MiB, emitted no sanitizer
diagnostics or new units, and produced log SHA-256
`a38a7fc3cc4b44c9fbfeefac96b43c975eca4317802632d1cc225135a7b0a8d9`.

Four strict sanitizer workers replayed shards of 272/271/271/271 inputs.
Each exited 0 with no diagnostics or new units and peaked at 123 MiB. Their
log SHA-256 values were:

    worker0: b7989067495c5e265878b31a91c305a998252c9055f4015a26ecd46b0c4cad4b
    worker1: e878497fb73c4e88baa53b770d357230cfa070419a7a2a89b94ac9761805e192
    worker2: f2363bf481feb992cf3fa6f42bc4525816de15775ae8c1d30b9fec9a0ac9ce80e
    worker3: c96b4389949f608b55b609b2f85d864fd4e35795109ec9b0ee70b0ebc872d37e

The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=key_tests,script_tests,descriptor_tests,bip324_tests --log_level=test_suite

exited 0 with `*** No errors detected`; log SHA-256 is
`ab7729954696a2eb0c5d8cd5afd57c8d648c0d037b19f89b2d3da3c897ae49df` and
the test binary SHA-256 is
`3082013e63c0b7fe6b6d3484147b5e1396767ea907e25b3789e84c9d93d5723f`.
No production, fuzzer, sanitizer, mutation, or test process remains
running.

### Differential proof and follow-up policy

The strongest proof available here is a matched production mutation, not a
claim that clean master is broken. A temporary mutation at `src/key.cpp:433`
changed
`ComputeTapTweakHash(merkle_root->IsNull() ? nullptr : merkle_root)` to
`ComputeTapTweakHash(merkle_root)`. It models treating the null root as an
all-zero script tree instead of the BIP341 no-script form. The mutation was
removed before the source commit, and current master was rebuilt and
replayed.

With the enhanced oracle and exact witness, the normal mutation replay
aborted at `src/test/fuzz/key.cpp:157` in the output-key Schnorr verification
assertion with exit 134; log SHA-256
`da800fc6e3bb27d013330778e799e9afbab4bcd3386c99c903cbe7d31fe88eb5`.
ASan/UBSan aborted at the same assertion with exit 134; log SHA-256
`3a8bad60388d7ce7478fd07b2bedec24edb62333e289eb4cc49cd4de691f6be5`.
Matched old-harness controls removed only this new 64-line oracle block,
used the same witness and production mutation, and exited 0 in normal and
ASan/UBSan builds; log SHA-256 values were
`abacbe4da5f75b0382fd4d76fe485f04a3fd87b667b281bc36326393d4d97663` and
`813420acbd9fc3bfd73a1d1665403f3f57f0af38cf20e4c7a882b5937b43187f`.
This proves that the added oracle catches the modeled regression missed by
the prior harness. It does not claim a current-master production bug, so no
deterministic production regression test is asserted by this commit.

If a later l0rinc cherry-pick, fork commit, minor fix, or current-master
change alters this null-root or Taproot behavior, amend the same commit and
this note or merge the changes with a complete record of target, caller,
corpus or mutation, assertion, failure mode, master-relative severity, and
whether the change masks, preserves, or changes the finding. Every future
production claim requires clean-master reproduction or a minimal production
mutation plus the strongest deterministic proof available. `clang-format --dry-run --Werror`
still fails on the file's pre-existing include grouping;
the parent fails similarly, so unrelated whole-file formatting was not
changed.

## `cmpctblock` collision-counter oracle audit (2026-07-22)

Source commit: `9a2f9cedf76cd6f4c16b4e692fd65be3be17ef9d` (`fuzz: guard
compact-block collision counters`), parent `6cfa9a776b`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`.
After the previous checkpoint, the 95-commit source audit branch was rebased
cleanly from the old base
`18c05d93016b28a9afd4c716dfe00b6e0accb30b` onto the fetched latest
`origin/master` `32eb52100296718f7c0469e3210ce1db73694793`. The l0rinc branch
is `d1d85263f8ebb47ad4d6126ff992d4915dda026b`, an ancestor of current
`origin/master`; no unique l0rinc commit was applicable or cherry-picked for
this target. Older sections retain their original base and evidence labels;
this section is the first current-master replay after the rebase.

### Upstream fix and target contract

Current master already contains upstream commit
`6aa5d8d9481f5e06b10095df7f46f0532f7ecdb7` (`blockencodings: fix extra
transaction count`). Before that fix, a short-ID collision after an
extra-pool transaction could decrement `extra_count` even when the
invalidated slot came from the mempool. The current implementation tracks the
slot source and decrements the counter only for an `EXTRA` slot; it also marks
collided slots terminal.

The existing `cmpctblock` fuzzer checked broad peer state, block-index, and
mempool postconditions but could not observe the protected
`PartiallyDownloadedBlock` counters. The new test-only subclass exposes those
two fields without changing the production API. Every input additionally
constructs an isolated mempool and a four-transaction compact block, provides
one extra-sourced transaction, places another transaction in the mempool, and
then supplies a mismatched extra entry with the mempool transaction's short
ID. The expected current-master state is `mempool_count == 1`,
`extra_count == 1`, the extra slot available, and the collided/missing slots
unavailable.

### Bitcoin Core boundary and severity

Peer compact blocks reach `PartiallyDownloadedBlock` through
`PeerManagerImpl::ProcessCompactBlockTxns`. `FillBlock` uses `extra_count`
only in the `BCLog::CMPCTBLOCK` success message at
`src/blockencodings.cpp:221-225`; it does not use the value to accept a block,
update the UTXO set, select transactions, or move funds. The collision is
peer-reachable, but the affected behavior is diagnostic accounting.

The master-relative rating is **Low historical diagnostic-accounting
correctness / oracle hardening**, not Critical. Current master has the fix and
the deterministic `blockencodings_tests` regression coverage. No new
clean-master production bug or production fix is claimed by this commit. A
malformed compact-block object alone is not Critical; severity follows the
actual Bitcoin Core caller effect. The compact-block nonce is a SipHash
selector; this audit asserts no nonce-clearing contract and makes no
cryptographic-nonce severity claim.

### Corpus and restored replay

The frozen corpus is `/tmp/bitcoin-cmpctblock-20260720-frozen`, copied from
the project corpus. It contains 1,435 files and 3,705,961 bytes, with sizes
5..31,412. The per-file SHA-256 manifest is
`b0cfeef40252982a88db251b9114df589491a212960dca46ad6ba6b79bf8b881`; the
sorted filename manifest is
`0b79ef0e1bd7824dd6a8d4edae2a6225e7422eca0b4ff32edeb2df6eb42e7e00`.

The final normal fuzz binary SHA-256 is
`1fb0690035b57848a0479a2013d34614454bb42362440e41a38f4a3c3a7ce489`.
The restored normal replay exited 0 after 5,889 executions, reached
coverage/features 9,941/44,680, peaked at 114 MiB, and produced log
SHA-256
`0f44194b892c25441ce7c403d8cae678d2a2f9296df6c2c27318057cec62f56c`.

The final ASan/UBSan binary SHA-256 is
`66bdb502e0ca8c4f5be7cef4a9878153d9416675d296641ae6bd2b143c09c909`.
Its timestamp-verified replay exited 0 after 1,437 executions, reached
coverage/features 20,474/89,105, peaked at 511 MiB, emitted no sanitizer
diagnostics, and produced log SHA-256
`e1a5eb1d2d145c4dc1d1b72517f6609f8deb99a41738e4c3eabbbbadf94637d7`.

### Differential mutation proof

The exact witness was frozen file
`000bd1e24ae7350a08b76587ebf6c8f487cc131f`, 1,493 bytes, SHA-256
`75da0ce2528c17158ab2156bc984771a300bb0949485420033da161d0f8a63b3`.
A temporary production mutation replaced
`extra_count -= (tx_source[idit->second] == TxSource::EXTRA);` with
`extra_count--;` in `src/blockencodings.cpp`. This is the minimal historical
pre-fix condition and was removed before committing.

With the enhanced normal fuzzer, the mutation failed at
`AssertExtraTransactionCounterContract` with wrapper rc 77; the log SHA-256
is `dd2c67aa5f7f092cdcd8c7f47e900d405b79eeb9b3977b3345aa6afb276adeae`.
The matched old-harness normal control removed only the new helper call,
processed the same witness under the same mutation, exited 0, and has log
SHA-256
`938088d15c6a7e712aff16a1e377b290624ffda0e53404966ee1065f9098989c`.

Under ASan/UBSan, the enhanced mutation failed at the same assertion with
wrapper rc 77; log SHA-256
`f4b16632d65f331727cfdc6b41f64b5711e5d17344b10e6da14e06727ed4c7ea`.
The matched sanitizer control exited 0; log SHA-256
`3a1234cce830a5dec3ce3ed38ae201ce8f40d6f17fac6446001ba94ef096275d`.

The proof shows that the new oracle detects the historical counter condition
the old fuzzer missed. It does not turn a log-only historical bug into a
consensus or security finding, and the upstream current-master fix is not
being presented as proof of a new defect.

### Worker and focused-test evidence

An initial time-based four-worker sanitizer pass was excluded from the
manifest evidence because libFuzzer grew each isolated corpus. The
authoritative corpus-only replay used `-runs` equal to each shard size:
361/361/361/360 executions, zero new units, no diagnostics, and peak RSS
491/483/493/494 MiB. Worker log SHA-256 values are:

    worker0: 873853192baccd91fd7f7eccbdab5c80dd62b0adca001fab379e67443123a1df
    worker1: b453fe9ab6075425032886084114f1ab449b379989e7afd11a7537de54c5855e
    worker2: 7967b3d76c39e9fb955c64667d500ab3416835e094691750a75c64267dc8474e
    worker3: 247fab65006de2599391487cc5b14269f7d59592959cb0e7a42fbad6fc87b74a

The 1,435-file worker union exactly matched the frozen filename manifest.

The current-master focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=blockencodings_tests,net_processing_tests,validation_block_tests,validation_tests,serialize_tests,blockmanager_tests,net_tests,private_broadcast_tests --log_level=test_suite

exited 0 with `*** No errors detected`; log SHA-256 is
`43cec9b034ebc991084aace546ae4f5913d4159bb5a7aaa45f36c097bde0d2f4`.
No mutation, control target, fuzz, sanitizer, build, or test process remains
running.

### Findings carried forward and masking policy

This current-master replay reiterates rather than upgrades the existing
ledger: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability remain Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban remain Low or hardening. Earlier addrman, coins-cache, txgraph,
txdownloadman, txrequest, connman, eviction, headers-sync, UTXO snapshot,
mempool persistence, package, handshake, `BufferedFile`, block-index,
compact-block, Merkle, wire, snapshot metadata, Bloom, compressed amount,
scalar/field/group, DER, EllSwift, wallet, PSBT, scriptpubkeyman, BIP324,
CMessageHeader, CInv, CBlockLocator, CBlockFileInfo, and related audits found
no additional clean-master production bug unless their notes say otherwise.
Reachability-limited ecmult scratch wrapping, 10x26 magnitude normalization,
and SHA/HMAC/RFC6979 retention remain non-findings. A nonce without
cryptographic meaning is not a Critical clearing finding.

The upstream fix changes the behavior guarded here and must be acknowledged
in any follow-up rather than treated as proof. If a later cherry-pick, caller-
side change, or minor fix can mask a follow-up failure, amend that same commit
and this note, or merge the evidence, with the exact target, caller,
corpus/mutation, assertion, failure mode, master-relative severity, and
whether the change masks, preserves, or changes master behavior. Every new
production claim still requires clean master or a minimal production mutation
plus the strongest deterministic proof available.

## `cmpctblock` state-transition oracle audit (2026-07-21)

Source commit: `815fe5bf267815bbf70813314b7a0ea261d328e5` (`fuzz: check
compact-block state transitions`). Its parent is
`54c1ca5f5ae3cdedb0eaf206114b6fcb44a8fe8e`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`l0rinc/master`. No l0rinc pull-request commit was relevant to this target.

### Core boundary and severity

`FUZZ=cmpctblock` drives Bitcoin Core's compact-block peer path through
`PeerManagerImpl`, including `CMPCTBLOCK`, `BLOCKTXN`, `HEADERS`,
`SENDCMPCT`, and `TX` messages. It exercises compact-block reconstruction,
partial block requests, block-index updates, mempool removal/reinsertion,
validation callbacks, and BIP152 high-bandwidth state. The existing harness
already checked local counters, timestamps, mempool sequence monotonicity,
and selected peer-state transitions, but did not run the complete production
block-index and mempool consistency contracts after each processing attempt.

The new harness synchronizes validation callbacks and invokes production
`ChainstateManager::CheckBlockIndex()` and `CTxMemPool::check()` under
`cs_main` after each `ProcessMessagesOnce()` in both the guaranteed rejected
transaction setup and the main transition loop, plus at final cleanup. The
test setup fixes `check_block_index=1` and mempool `check_ratio=1`, so these
otherwise optional checks are deterministic.

Severity on master is Informational/Low oracle hardening. Clean master
reproduced no production bug, invalid-block acceptance, consensus failure,
memory/concurrency fault, or cryptographic issue. Compact-block data is
untrusted peer input, but this audit found no master behavior that warrants a
High/Critical rating. No production fix or deterministic regression test is
claimed.

### Source and corpus identity

The original `cmpctblock` harness source SHA-256 was
`47a590596e1c445a4ba83e50a98c81a678bfece307723d33c83fd28f29000630`; the
enhanced source SHA-256 is
`9caa805f289a50986c51a34de424a59b3629147b5339d0b862fcbb93a9f7652c`.
Restored production `src/net_processing.cpp` is
`afc14cf644760b60670fa82fb088b03ffa792d421a52c5f6c73b2e67672cf419`.

The frozen corpus is
`/tmp/bitcoin-cmpctblock-20260720-frozen`: 1,435 files, 3,705,961 bytes,
minimum 5 bytes, maximum 31,412 bytes. The per-file `sha256sum` manifest
SHA-256 is
`b0cfeef40252982a88db251b9114df589491a212960dca46ad6ba6b79bf8b881`;
the sorted filename-list SHA-256 is
`0b79ef0e1bd7824dd6a8d4edae2a6225e7422eca0b4ff32edeb2df6eb42e7e00`.
All authoritative runs used isolated copies so libFuzzer corpus growth could
not alter the frozen evidence.

### Replay evidence

The original-harness sanitizer baseline used binary SHA-256
`bbd23e03a665870c4a98e6ef804deed4ed173d3dcdab810614168ccec4626ce7`, exited
0 after 1,437 executions, added no units, produced no artifacts, and has log
SHA-256
`a063219a725de3d1f041541bf2471a85c9a65a7c912487f35929e27ce023365d`.
The enhanced sanitizer binary SHA-256 is
`5a71a991479d01a285e2ba4cf7eb783e09927b8610403602c262d980e31e1d0f`; the
1,435-file replay exited 0 after 1,437 executions, produced no artifacts,
and left its isolated corpus unchanged. Its log SHA-256 is
`483569cc742111116c958025135631f2c410889f1f82d50f644bf93639013d61`.

The normal standalone fuzz binary SHA-256 is
`6a07edad172e19761135fbe9e07104b3094af37964021e0ad3df4f1560fa23a4`; it
passed all 1,435 files in 7 seconds with no artifacts. Its log SHA-256 is
`df920458cf245ae9f651680d7724f111fe360057e9f9bcc36ae24024cbab813e`.
The standalone driver was invoked with only the corpus directory because it
does not parse libFuzzer flags.

Four independent sanitizer workers used
`-max_total_time=60 -timeout=60 -rss_limit_mb=0 -use_value_profile=1
-print_final_stats=1`, isolated corpus copies, and isolated artifact paths.
All exited 0 after 1,437 executions, left 1,435-file copies unchanged,
produced no artifacts, and peaked at 513--519 MiB. Worker log SHA-256 values
were:

    fuzz-0 8e9a3100ec490d88f69d9103d363ea50f8ce4689da6b73dfae3326ed043084ea
    fuzz-1 0cdbfe6a2f90b8b3c57621c03a04d4814aedde5f9c0bd0701ce9e33935756944
    fuzz-2 b4f5fcc76739c119f854692637136ef8041cf3feb0152dcf7e25df900cbb8dd1
    fuzz-3 4a57ef584ab55df5661f07a3f363f0379e5d713e866dca89da01a50bb23b4076

### Differential oracle proof

This is an oracle differential proof, not a clean-master production
finding. A temporary production mutation inserted
`WITH_LOCK(cs_main, ++m_chainman.ActiveChain().Tip()->nHeight);` at the start
of `PeerManagerImpl::ProcessCompactBlockTxns()`. The mutated production
source SHA-256 was
`7228bdff1c4c807258e540907caef15b1d57c486f6637130334aa0840478faa0`.

The exact frozen input was
`/tmp/bitcoin-cmpctblock-20260720-frozen/277bb4e0a988575b6e5bbe0a3898e13b5f0577c5`:
524 bytes, SHA-256
`edf2326bb9d72cf71ada8f6cd3ce10452effb4ac7bf6d1cddedc7d0900904a6f`.
With the enhanced harness, mutated binary SHA-256
`635d6017392c599828b392151cac53b476978b0c408450d6eb98285cc996fe7f`
exited 134 after 148 executions at `validation.cpp:5195` in
`CheckBlockIndex()`. The mutation log SHA-256 is
`c5077736d9047f3728a5ad8528d064e9181aa4cf9f6df95c92d6889eefd1773e`;
the saved artifact has the input SHA-256 above.

With the original harness and identical production mutation, binary SHA-256
`de684c36ce03021710870c92df805aa560da95ad9d981bab1abb87b09443d03c`
exited 0 after one fixed execution with no artifact; control log SHA-256 is
`0480566eb7d6f581436428e89afd874ca450a679f0ca7e957b24b9f2a4a7536d`.
This proves the old size/data-count oracle can accept unchanged-size index
metadata corruption while the new production contract detects it. The
mutation was removed before the source commit. With restored production, the
exact seed exited 0 in the final sanitizer binary; log SHA-256 is
`631ea378b353bdad583577da1d05e666e38aec8d050f6b0fc68b771da91d8f70`.

### Verification and test gap

`git diff --check` passed. `clang-format --dry-run --Werror` reports only
pre-existing brace-spacing violations at `cmpctblock.cpp:101` and `:105`;
unrelated lines were not reformatted. The sanitizer and normal targets were
built with:

    cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2
    cmake --build /tmp/bitcoin-secp256k1-audit-current-normal-build --target fuzz -j2

The configured fuzz-only build has no `test_bitcoin` target, so the dedicated
unit suite was unavailable. No production behavior changed, no production
bug is asserted, and no deterministic regression test was required. No fuzz,
sanitizer, or mutation process remains running.

---

## descriptor_parse and mocked_descriptor_parse audit

Source commit: `41ff6a5c63` (`fuzz: harden descriptor serialization and
multipath oracles`). The audited source worktree was
`/tmp/bitcoin-secp256k1-audit-current`, branch `codex/fuzz-oracles-current`.

### Provenance and l0rinc comparison

The descriptor work started at `6909f22d742dabd4350c54b369c3a1d55d0000a3`,
whose parent was the preceding block-index oracle commit
`4a06bf618d09d6f5b0b26d1fd5b800b4c41338d4`. Both `origin/master` and
`remotes/l0rinc/master` resolved to
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`.

The exact target-scoped comparison was:

    git log origin/master..remotes/l0rinc/master -- \
      src/script/descriptor.cpp src/script/descriptor.h \
      src/script/miniscript.cpp src/test/fuzz/descriptor_parse.cpp \
      src/test/descriptor_tests.cpp src/rpc/output_script.cpp \
      src/wallet/scriptpubkeyman.cpp src/wallet/export.cpp \
      src/wallet/rpc/backup.cpp

It returned no output. No l0rinc commit was cherry-picked for this target,
so no fork change masked, weakened, or altered the clean-master behavior.
Any later cherry-pick or fix that changes a follow-up finding must be added
to the same commit and note, explicitly stating whether it masks, preserves,
or changes the master behavior. A nonce with no cryptographic meaning is not
Critical merely because it is not cleared.

### Oracle changes

`src/test/fuzz/descriptor_parse.cpp` now checks strict checksummed public,
private, and normalized serialization round trips. Private strings are
compared with `ToPrivateString`, not public `ToString`; multipath parsing is
required to contain a matching result rather than being incorrectly assumed
to return exactly one descriptor. Every inferred script descriptor must be
non-ranged and expand with an empty provider to exactly the original script.
Both `descriptor_parse` and `mocked_descriptor_parse` use these checks.

Production `src/script/descriptor.cpp` now makes `AddressDescriptor` and
`RawDescriptor` populate the public descriptor when `ToPrivateString` returns
false, as required by the `Descriptor` API. It also gives `InferDescriptor`
the documented non-null/non-ranged `Assert` contract. During miniscript
parsing, `KeyParser` tracks the active multipath alternative for comparison
and serialization, and duplicate-key sanity is checked for every expanded
path before descriptors are returned.

### Findings and Bitcoin Core severity

Two real clean-master production bugs were confirmed. Both are **Low
severity correctness/availability findings on master**, not Critical
Bitcoin Core vulnerabilities:

1. `AddressDescriptor::ToPrivateString` and `RawDescriptor::ToPrivateString`
   returned false without writing `out`. Bitcoin Core reaches this through
   `DescriptorScriptPubKeyMan::GetDescriptorString(priv=true)` at
   `src/wallet/scriptpubkeyman.cpp:1554-1565`; `src/wallet/export.cpp:30`
   treats false as an export failure. `addr()` and `raw()` have no private
   key to expose, so the fix is the public fallback. This is local wallet
   export correctness/availability only: no fund loss, consensus effect,
   memory safety, or cryptographic failure, and no peer invalid block or
   transaction can directly trigger this path.

2. Miniscript duplicate-key sanity was checked only for multipath path zero
   before one-element key vectors were cloned. A later path could contain
   duplicate public keys, then be emitted by `ToString()` as a descriptor
   that strict parsing rejected. `getdescriptorinfo` parses local input and
   returns `descs.at(0)->ToString()` at `src/rpc/output_script.cpp:199-205`;
   wallet scriptpubkeyman inference paths at `src/wallet/scriptpubkeyman.cpp:
   699-712` and `794-810` serialize and reparse descriptors, skipping those
   that fail. This is local descriptor correctness/availability, not an
   invalid-block consensus boundary. It is not High/Critical without proof
   of a different Core caller or impact.

The fuzzer also adds production assertions for the `InferDescriptor`
contract, but clean master did not produce a null or ranged inference. No
additional production bug is claimed from that hardening.

### False oracle corrections

The first private round-trip assertion was too strong: it compared a private
`tr()` descriptor with public `ToString()`. The 56-byte witness had SHA-256
`c7112224f70ab81cebc81061d918273d8809fb4ec307aa9b93b61696b91cb2bb`. The
oracle now reparses and compares `ToPrivateString` output.

The first public round-trip assertion also incorrectly required one Parse
result. The 439-byte multipath witness
`bdadb00bf32e7c6f0130833f06fd9be01cdca743` has SHA-256
`18b3d8236f8c64b1f0190a390700734a53696717f0a3c108d5f7b760e86a875f` and
legitimately expands to multiple descriptors. After that oracle correction,
the same input exposed the real clean-master later-path duplicate-key bug.

### Corpus identity and replay evidence

The existing `descriptor_parse` corpus contains 3,281 files and 10,393,136
bytes. The frozen copy is
`/tmp/bitcoin-descriptor-20260721/frozen/descriptor_parse`, with minimum and
maximum file sizes 1 and 528,799 bytes. Its sorted filename manifest SHA-256
is `4560424f4b98846ae71ad0ffadd7a0dcedb9d60610ce5df3c4d2eece2193a715` and
its filename/size manifest SHA-256 is
`aa0d36472f6e7c68d00cb1e69e2e22f8d038865f967c827fcfd5d7dbd7f7f772`.

The existing `mocked_descriptor_parse` corpus contains 3,798 files and
4,620,231 bytes. The frozen copy is
`/tmp/bitcoin-descriptor-20260721/frozen/mocked_descriptor_parse`, with
minimum and maximum file sizes 1 and 510,318 bytes. Its sorted filename
manifest SHA-256 is
`74cd28919252b5f6d7db86e061c0847e0aeb695cd2ef229e7f5fce4e4117aba3` and
its filename/size manifest SHA-256 is
`779078568d9e6360030b5a217a8293350c97648649c862b45c6a93879e7d8cd1`.

Baseline and final full-corpus results:

    target                    run       executions cov    features  RSS
    descriptor_parse          normal    3282       7460   43245     94 MB
    descriptor_parse          final     3282       7523   44170     93 MB
    descriptor_parse          ASan     3282       16691  111005    778 MB
    descriptor_parse          final ASan 3282       16758  111585    776 MB
    mocked_descriptor_parse   normal    3799       7529   46877     80 MB
    mocked_descriptor_parse   final     3799       7565   47389     80 MB
    mocked_descriptor_parse   ASan     3800       16780  116643    768 MB
    mocked_descriptor_parse   final ASan 3800       16805  116872    850 MB

Every listed run exited 0 with no crash, OOM, ASan, or UBSan artifact. The
baseline/final log SHA-256 values, in the same order, are:

    93454e0b6c3054cdbf1addf008238556b94e2ef96752767533c0036516554edf
    7fad2e50b910dcc2797bda8fe8d24622da2cb781f572fd8d4cd83f6e10270a9e
    94d0b5f0fe295f36cd09654071b140e6379d0227347326e45587a9726b7441c1
    4d64d39f4a71c0990f9b2c4ded7b7bbe3a81a9f02441ec1d01c3e451ccd19197
    c9f250176ea04beb74a7b646ebf99732d3f12e347980807a3e3685acb44c2e9a
    edf91983af315299fc81ac2334b26717a52a4b2b72c8f8103851166caca1d808
    2ed705ddbd2dfb9b7ee3ad0aacebe76eb54c8bec0609c76dc66a226e9826e77e
    d8b20917bacc2e50e8cadd6bf67c08677d11511991e797e266cd2b7ffc8b5340

Four independent ASan/UBSan descriptor workers used 821, 820, 820, and 820
files, executed 823, 822, 822, and 822 units, and all exited 0 with only
slow-unit files. Peak RSS was 677, 670, 606, and 682 MB. Their log SHA-256
values were:

    0becf876b473d2ae82f82af106a11a7521fa510be8e0ff960d0e1b5c1ca76d91
    f74b456b15200d7d3f3a627ea6a26d0c9f563f0b630f025c9d39f4a6392604c9
    58a20515aeaa70ffcd99d758224ef118e61be2198a5bc3f691bf6605bb06f0fc
    7ebb4dd47a73970604ede28afb7ea1db4f8894181c1669a0238fb985732e9b93

Four independent ASan/UBSan mocked workers used 950, 950, 949, and 949
files, executed 953, 953, 952, and 952 units, and all exited 0 with only
slow-unit files. Peak RSS was 641, 604, 681, and 675 MB. Their log SHA-256
values were:

    9f9b4e4137d3ee502e0320f4975eeeaa14b1f9b5ee1312a8c51b272660ffa2c8
    b36ae58f69c862f76892d6f3fc763e392b6f41f8f1fe736722cd31a00c4c6856
    33d43f615fec02d5ed7e420f7dbe6ceacba35841946386ed23ed21ac1bf6ba21
    c4ce404104545e3d313b0fe65aae88d03c0797dc99e70c43e2766b87413f6234

### Mutation and matched-control proof

For the multipath bug, the exact production mutation changed the new loop
from `for (size_t i = 1; i < num_multipath; ++i)` to
`for (size_t i = num_multipath; i < num_multipath; ++i)`, skipping every
nonzero-path duplicate-key check. The enhanced target hit
`AssertDescriptorRoundTrip` (`!reparsed.empty()`) on the exact 439-byte
frozen witness above. The enhanced log SHA-256 is
`898021e47d5ab4f4a99e8130e8a79f32aa14c829902a374146441a8429c50603`.
The matched control removed only the new round-trip/inference assertions and
exited 0 on that witness; control log SHA-256 is
`71f4ba2fc1b33eed4051004e08abf396d5bf21cc906228f8b549c27343ef829e`.
The assertion signal handler emitted no artifact, so the log hash and clean
control are the recorded proof. The mutation and temporary control target
were removed before commit.

For the serializer bug, the exact production mutation removed only
`out = DescriptorImpl::ToString(/*compat_format=*/false)` from
`RawDescriptor::ToPrivateString`, leaving `return false`. The corpus witness
is the 11-byte input `raw(0181ae)` at
`/tmp/bitcoin-descriptor-20260721/frozen/descriptor_parse/ff7979808a09646a4a6b89d8f5962d29706cdbfd`,
SHA-256 `2dcd8fb2a0c628decdbefae2a67504d5edc74760464f2782b4be4f19cfc8a082`.
The enhanced target hit `!private_string.empty()`; its log SHA-256 is
`02f52183b15a465c5715b1b07d8dfd6243a8397d56f1e026f5fa7d058d6767ab`.
The matched control exited 0; its log SHA-256 is
`9deed9990ce57b5bf652d5aa3304ec4c22d20d6e8ab4987075d11391116044c7`.
The mutation and temporary control target were removed before commit.

After restoration, both exact witnesses exited 0. The final multipath log
SHA-256 is `995aaf68a008ce0da8170f53549f08ec16bb6b76cf9f573416dbf76ab9151440`
and the final raw log SHA-256 is
`609ca21d2dbaf3407238db2d644832c5c632e7ac71afd653fbc4a2077db5266d`.

### Deterministic tests and verification

The separate non-fuzz configuration
`/tmp/bitcoin-descriptor-test-build` built `test_bitcoin`. The command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin \
      --run_test=descriptor_tests --log_level=test_suite

passed all six descriptor cases, including
`descriptor_private_string_without_private_keys` and
`descriptor_multipath_duplicate_key_rejected`, with exit 0. The test log
SHA-256 is `0b920eec499993b62be60d9673c2adeda27da7fbe261291fbb65ca8ee2b18764`.
`git diff --check` passed. Final source SHA-256 values are:

    src/script/descriptor.cpp             b0686563e0246bea07518235c87f0291eda8578e3e520e69f5eba380dcf5efe9
    src/test/descriptor_tests.cpp         bd20ee3e6c8e402ff6f20874d7ba0a3831ae2826fbf70e8eee3bf93549e9f0e3
    src/test/fuzz/descriptor_parse.cpp    428a869e8104b806196e939895bb5ad721d475aa818863b50ecbb216bd69a5a2

### Findings carried forward

Existing audit findings remain rated against clean master and actual Bitcoin
Core callers. Generic raw `finalizepsbt` with invalid nonempty
`final_scriptSig` remains Low private-RPC correctness/availability;
`walletprocesspsbt` rechecks through `CWallet::FillPSBT`. Feature-conditional
private-broadcast failed-send retention and empty HEADERS initial-sync
availability remain Medium. Peer activity refresh, block-storage failure,
oversized transport types, and banman invalid-subnet/unban remain Low or
hardening findings. Earlier BIP324, EllSwift, key, scriptpubkeyman, wallet,
PSBT, tx_pool, block-index, scalar/field/group, DER, and related audits found
no additional clean-master production bug. Latent ecmult scratch wrapping,
10x26 magnitude normalization, and SHA/HMAC/RFC6979 retention concerns are
reachability-limited, not Critical Bitcoin Core vulnerabilities. An isolated
malformed input or a nonce without cryptographic meaning does not raise
severity; invalid blocks or transactions are Critical only when the real
Bitcoin Core caller can actually reach an impactful failure.

## `snapshotmetadata_deserialize` untrusted-file round-trip oracle audit (2026-07-21)

Source commit: `baa8fcc2c1` (`fuzz: strengthen snapshot metadata round-trip
oracle`). The source worktree was `/tmp/bitcoin-secp256k1-audit-current`,
branch `codex/fuzz-oracles-current`, and the documentation worktree was
`/tmp/secp256k1-oracles-next`, branch `codex/fuzz-oracles`.

### Provenance and l0rinc comparison

This pass started from source commit `41ff6a5c6351563700064e12fd1aca40df93e047`.
The source branch is based directly on
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both `origin/master` and
`remotes/l0rinc/master` resolved to that commit at audit time. The exact
target-scoped comparison was:

    git log origin/master..remotes/l0rinc/master -- \
      src/node/utxo_snapshot.h src/validation.cpp \
      src/rpc/blockchain.cpp src/test/fuzz/deserialize.cpp \
      src/test/validation_chainstatemanager_tests.cpp

It returned no output. No l0rinc commit was cherry-picked for this target, so
no fork fix masked, weakened, or changed the clean-master behavior. Any later
cherry-pick or potential fix that changes a follow-up finding must be amended
into the same commit and note, explicitly stating whether it masks, preserves,
or changes the behavior on master. A nonce with no cryptographic meaning does
not need clearing and is not Critical.

### Oracle and Core call path

`src/test/fuzz/deserialize.cpp` now adds a postcondition after accepted
`SnapshotMetadata` parsing. It serializes the accepted object, reparses it
with the active network parameters, requires the serialized stream to be
fully consumed, and compares the base block hash and coin count. The input
buffer itself is intentionally not required to be empty: in production the
metadata header is followed by serialized coins in the same snapshot file.
Expected invalid-file exceptions remain part of the harness domain.

The production type is `node::SnapshotMetadata` in
`src/node/utxo_snapshot.h`. Its fields are explicitly untrusted and are
validated by `Unserialize`; the fuzzer now also checks that an accepted state
can reproduce the same serialized metadata. The principal Bitcoin Core
callers are:

* `loadtxoutset` in `src/rpc/blockchain.cpp:3528-3537`, which parses metadata
  from a local snapshot file and passes it to
  `ChainstateManager::ActivateSnapshot` in `src/validation.cpp:5614`.
* `PopulateAndValidateSnapshot` in `src/validation.cpp:5780`, which validates
  the base header, assumeutxo height/work, coin count, coin heights, and
  `MoneyRange` while loading the snapshot.
* `dumptxoutset` in `src/rpc/blockchain.cpp:3395-3397`, which writes the same
  metadata format.

No clean-master production bug was confirmed. Against master this is an
**Informational/Low hardening finding** for local snapshot correctness and
availability. Snapshot metadata is a local untrusted-file/RPC boundary, not a
peer block or transaction validation boundary. It cannot be rated High or
Critical merely because malformed bytes are accepted by the fuzzer; an invalid
block or transaction is Critical only when an actual Bitcoin Core caller can
reach an impactful failure. Existing deterministic chainstate tests already
cover malformed snapshot counts and base hashes. Since there is no production
fix or reproducible clean-master failure, this commit adds no redundant unit
test; the existing caller-level tests remain the strongest applicable proof.

### Corpus and replay evidence

The existing `snapshotmetadata_deserialize` corpus has 16 files and 208 total
bytes, with minimum/maximum sizes of 1/51 bytes. The frozen copy is
`/tmp/bitcoin-snapshotmetadata-20260721/frozen/snapshotmetadata_deserialize`.
The sorted filename manifest SHA-256 is
`b1664d472369c2f2485bcb2031dc27f0df404a11a1180c1bb98ef8922f4e6ae5`; the
filename/size manifest SHA-256 is
`edddea25ed434fe4d7ebc689da9d5214ce20b3c0cc079148c52477081dd7c63e`.

The pre-change normal replay ran 17 executions, reached coverage 630 and
1,154 features, peaked at 55 MB, exited 0, and produced no artifact. Its log
SHA-256 is `4755395134ff49b26be8e5fe89e832b3399a028e7eca945d4e275c85a464c8b3`.
The final normal replay ran 17 executions, reached coverage 641 and 1,177
features, peaked at 55 MB, exited 0, and produced no artifact. Its log
SHA-256 is `b9baceafdac8dc3874fe7267723a2ab8a68d2387de9cd0729c01c9d52a11139e`.
The final ASan/UBSan replay ran 17 executions, reached coverage 1,110 and
2,042 features, peaked at 105 MB, exited 0, and reported no sanitizer or
artifact fault. Its log SHA-256 is
`ed5de1a1275a79dca99cfa907062f125ccf2f02bdc4336ee772913e47ba72aa9`.

Four disjoint ASan/UBSan workers each ran four corpus files plus one seed, for
five executions. All exited 0 with no sanitizer/artifact failure and peaked
at 104 MB. Their log SHA-256 values were:

    worker0 ee1e9428104c7169558eaecbcd6269bb8a2460d6293bf6d4f1e1073cd6a7df9
    worker1 6f5508a818cd06748be7d349c607dc2d9291014e9827355c8b75a6f6599e5c83
    worker2 62eb248536d997322d603e045466fdeed02b4355929ea817e17144c6830fab53
    worker3 465f3ffc3b228ffb3069afcaac0ef45928e583424d7f92eed0f033c0f85dd07e

The union of worker filenames exactly matches the frozen filename manifest;
the union SHA-256 is `b1664d472369c2f2485bcb2031dc27f0df404a11a1180c1bb98ef8922f4e6ae5`.

### Mutation and matched-control proof

The exact valid corpus witness was
`09f89addb7c67c8a5caf9172065e458878a579e9`, 51 bytes, SHA-256
`9f34b5aba95f3313fe838a17ee2f39b2aed04f7fe0afd82ac91e8979f4b9c080`.
The production mutation removed only `s << m_coins_count` from
`SnapshotMetadata::Serialize`. With the enhanced harness, the witness failed
immediately while reparsing with `DataStream::read(): end of data`; the
wrapper exited 125 after one execution. Mutation log SHA-256:
`23d5b177df8650eae3a6fc5d401bb52a8f5b23ee3fe251b9c2fd7ab4df58bb1f`.
The matched temporary control target retained the old execution-only
behavior and exited 0 on the same witness. Control log SHA-256:
`51eb970117f9fd006622e98ae9783fa4b83f7c2ec2611f07a03c8cfd8211126c`.
This is proof that the oracle detects the modeled serializer regression, not
proof of a clean-master production bug. The mutation and control target were
removed before the source commit.

### Verification and findings carried forward

The final normal fuzz binary SHA-256 is
`93615c3c3dd31a3560300ec810da75d573b9346c57cc3da05680be5ad2fb1249`; the
ASan/UBSan fuzz binary SHA-256 is
`dec4519a21ae324a868171cb5b7789af5e71573cc6852f75a62927be25e42b36`.
The final `src/test/fuzz/deserialize.cpp` SHA-256 is
`004a618e444234afe49e7fe96231d5ffe70217c28ee010406cb8c0c8cb24b519`.
`git diff --check` passed.

The relevant `validation_chainstatemanager_tests` command was launched from
`/tmp/bitcoin-descriptor-test-build/bin/test_bitcoin`. It reached the existing
snapshot assertion-based negative cases, then hung in
`chainstatemanager_snapshot_completion_hash_mismatch` with the process blocked
in `futex`; it was stopped with exit 130. Log SHA-256:
`21003b72a443b90cf68715cd2aedc66f0b967fc3a7d2a088d4c3e55b527a440c`.
This is a test-environment limitation, not a fuzz failure.

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, scalar/field/group, DER, and related audits
found no additional clean-master production bug. Latent ecmult scratch
wrapping, 10x26 magnitude normalization, and SHA/HMAC/RFC6979 retention remain
reachability-limited. Any later cherry-pick that alters a follow-up finding
must be amended into the same commit/note with whether it masks, preserves, or
changes master behavior. No fuzz, sanitizer, mutation, or test process remains
running.

## `blocktransactions_deserialize` BLOCKTXN input-prefix oracle audit (2026-07-21)

Source commit: `1a71283cfe` (`fuzz: strengthen BLOCKTXN deserialization
oracle`). The source worktree was `/tmp/bitcoin-secp256k1-audit-current`,
branch `codex/fuzz-oracles-current`; the source branch is based on
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, which was both
`origin/master` and `remotes/l0rinc/master` at audit time.

### Provenance and target selection

The separate `chain` fuzzer already round-trips every serialized
`CDiskBlockIndex` field, so `diskblockindex_deserialize` was deliberately not
duplicated. `blocktransactions_deserialize` was selected because it is a
peer-facing parser whose harness only deserialized `BlockTransactions` and
then exited.

The target-scoped comparison was:

    git log origin/master..remotes/l0rinc/master -- \
      src/blockencodings.h src/blockencodings.cpp src/net_processing.cpp \
      src/test/fuzz/deserialize.cpp src/test/fuzz/chain.cpp \
      src/test/blockencodings_tests.cpp src/test/net_tests.cpp

It returned no output. No l0rinc commit was cherry-picked, so no fork change
masked or altered the clean-master behavior. Any later cherry-pick or fix that
changes a follow-up finding must be amended into the same commit and note,
stating whether it masks, preserves, or changes master behavior. A nonce with
no cryptographic meaning does not need clearing and is not Critical.

### Oracle and Bitcoin Core callers

The enhanced target uses a target-specific `SpanReader` path. For an accepted
object it:

* requires every deserialized `CTransactionRef` to be non-null;
* reserializes the object and requires the bytes to equal the exact consumed
  prefix of the original input, with the `SpanReader` remainder accounting for
  the rest; and
* reparses the canonical serialization and compares the block hash, transaction
  count, and each transaction witness identity.

Trailing bytes are intentionally allowed, matching the generic deserializer
harness. The block hash is not required to be non-null or semantically valid at
this parser boundary: `ProcessCompactBlockTxns` matches it to an outstanding
request, and `PartiallyDownloadedBlock::FillBlock` checks compact-block
consistency.

Bitcoin Core deserializes a peer `BLOCKTXN` message in
`PeerManagerImpl::ProcessMessage` at `src/net_processing.cpp:4876-4887` and
passes it to `ProcessCompactBlockTxns` at `src/net_processing.cpp:3536-3620`.
That caller handles invalid reconstruction by removing the request and
misbehaving the peer, handles failed reconstruction by falling back to
`GETDATA` or waiting for another download, and only sends a successful block
to `ProcessBlock`.

No clean-master production bug was found. This is **Informational/Low
hardening on master**. The modeled regression causes compact-block
reconstruction failure and extra-download/availability behavior, not invalid
block acceptance, fund loss, consensus failure, memory safety, or cryptographic
compromise. A malformed peer block or transaction is not Critical without a
reachable impactful Core failure.

### Corpus and replay evidence

The existing corpus has 220 files and 20,081,783 bytes, with minimum/maximum
sizes of 1/1,043,992 bytes. The frozen copy is
`/tmp/bitcoin-blocktransactions-20260721/frozen/blocktransactions_deserialize`.
The sorted filename manifest SHA-256 is
`32275e3e880aadc181bddc4d688ee673ad123d4f06be7b8d6074f22c03f4891b`; the
filename/size manifest SHA-256 is
`f8c98fc97d518fa9168569796e442ba7610a223fc9c6e488d9b1aa4033e143fc`.

The pre-change normal replay used binary SHA-256
`93615c3c3dd31a3560300ec810da75d573b9346c57cc3da05680be5ad2fb1249`, ran
221 executions, reached coverage/features 337/1,981, peaked at 153 MB, and
exited 0. Its log SHA-256 is
`d725b28e95087dceb502d3ab726a0831926eba8ac5f0f5c8a3691ec5c993ddac`.

The final normal binary SHA-256 is
`c6418f87b9b3f7fd517f73664bc927f0c4ad6a510355e78035b93b29c3444626`.
The final replay ran 221 executions, reached coverage/features 417/2,339,
peaked at 155 MB, exited 0, and produced no artifact. Log SHA-256:
`661236011df432cbead1d376c506780af24bd115d4ee6b9af7c57953d0c6005f`.

The final ASan/UBSan binary SHA-256 is
`79f2a76b95821cc8a3efe833a032f898ceef7679b20c082fe581314e9953e6ef`.
Its replay ran 221 executions, reached coverage/features 605/3,555, peaked at
461 MB, exited 0, and produced no sanitizer or artifact fault. Log SHA-256:
`c9d738d66ab31e95c5a64459cda72858f7bdf39e0b394f5c818664c8360b3f1d`.

Four disjoint ASan/UBSan workers each ran 55 corpus files plus one seed, for
56 executions. All exited 0 with no sanitizer or artifact fault; peak RSS was
353, 174, 266, and 301 MB. Worker log SHA-256 values were:

    worker0 716ab30a2fe977f5384955beca5a27f99ca7a4f4f86ac72d28d0ea8ac3738707
    worker1 07a4dfe786799b4f15a607a51d1d8f0ef1cefe5b9de6e171a1242dfea5d1efdc
    worker2 60b9dc154b100465f6a9fc7c96af7690a4af2609d6c7ec42ce6ab301b5c1849f
    worker3 a552e3d9f802625449e49fdffde39ae436db22cb6edb33b4bafd9431ebe15403

The worker filename union exactly matches the frozen manifest and has SHA-256
`32275e3e880aadc181bddc4d688ee673ad123d4f06be7b8d6074f22c03f4891b`.

### Mutation and matched control

The temporary production mutation added `SER_READ(obj, obj.txn.clear())`
after `BlockTransactions` serialization. It models a read-side deserializer
that consumes valid transaction bytes but silently drops the parsed vector.
The enhanced full corpus reached the consumed-prefix assertion after 11
executions; mutation log SHA-256:
`27b576284feb7fe4993d94649e92a32ebde5244143c117487724f13607820b36`.

The exact witness was
`08a5b5c28c3614512b2ec9c1c63eb3458acd54d7`, 123 bytes, SHA-256
`a7f6641b68cbdeab1e3625987edcfe4a4aafd231db645778a42f96c21439f821`.
The enhanced mutated target failed at `deserialize.cpp:161` on the consumed
prefix contract; exact mutation log SHA-256:
`e2e3b6eb2467cbe1d682da3285ab509d62fd2795b730ff2f1c21889c283dba31`.
The matched temporary `blocktransactions_deserialize_control` target used the
same mutation but only deserialized and exited 0 on that witness; control log
SHA-256:
`633608a50afda991b13005022c3699861e2d02afa9795e68d9428d1ea9a3e7a8`.
The mutation and control target were removed before the source commit. This
is oracle-sensitivity proof, not a clean-master production bug.

### Verification and findings carried forward

The exact witness passed restored normal and ASan/UBSan replay. Exact log
SHA-256 values are `da9a671f26315e90aa3e42cfe4fd44b44d3728307197ba345bc2e5a7dd68af06`
and `d2a548552a88f89565c4476a6dc894f0601bfa9997837110bffbe108e017a837`.
The focused command
`test_bitcoin --run_test=blockencodings_tests --log_level=test_suite`
passed all eight blockencodings cases with exit 0. Its log SHA-256 is
`b3985523672478baa98f63e7fb5e0730b6cc51033b708e87a9ad87686d39fa4a`.
The final source file SHA-256 is
`ea2ac3e1bd8899204c52ac428d81f59208266c52833ca9f3ace77ade16de739f` and
`git diff --check` passed.

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, snapshot metadata, scalar/field/group, DER,
and related audits found no additional clean-master production bug. Latent
ecmult scratch wrapping, 10x26 magnitude normalization, and SHA/HMAC/RFC6979
retention remain reachability-limited. No fuzz, sanitizer, mutation, or test
process remains running.

## `merkleblock` reusable-output oracle audit (2026-07-21)

Source commit: `cf2cf0460e` (`fix: reset partial merkle match indices`),
parent `1a71283cfe` and audit base `18c05d93016b28a9afd4c716dfe00b6e0accb30b`.
The source worktree was `/tmp/bitcoin-secp256k1-audit-current` on
`codex/fuzz-oracles-current`; `origin/master` and
`remotes/l0rinc/master` were both that base, so no rebase was needed. The
target-scoped l0rinc comparison over `src/merkleblock.cpp`,
`src/merkleblock.h`, `src/test/fuzz/merkleblock.cpp`,
`src/test/merkleblock_tests.cpp`, `src/test/pmt_tests.cpp`, and
`src/test/bloom_tests.cpp` returned no output. No l0rinc commit was
cherry-picked and no fork change masked this master behavior.

### Finding and oracle

`CPartialMerkleTree::ExtractMatches` is an output-parameter API: each call
must overwrite the matched txids and their corresponding transaction indices.
On master it cleared `vMatch` but not `vnIndex`. Reusing the index vector
therefore retained stale entries, breaking pair alignment even when the tree
was valid. The production fix clears both vectors at the start of the
operation.

The `merkleblock` fuzzer now deliberately pre-seeds `vnIndex` with
`0xdeadbeef`, asserts equal output sizes, checks that extracted indices are
in range and strictly increasing, and calls extraction twice to require an
idempotent state transition. `merkleblock_extract_matches_reuse` adds a
deterministic regression test with a real block, a pre-seeded index, and a
second extraction. This is a real clean-master production correctness bug,
not a sanitizer-only or mutation-only finding.

### Bitcoin Core boundary and severity

The current Core callers are `verifytxoutproof` at
`src/rpc/txoutproof.cpp:147-166` and `importprunedfunds` at
`src/wallet/rpc/backup.cpp:61-87`; both construct fresh match and index
vectors before extraction. The `net_processing.cpp` merkleblock path only
constructs and sends filtered-block proofs; this tree is not an inbound
invalid-block acceptance path. Therefore the master-relative rating is
**Low** API correctness. A caller that reuses the public output parameters
could receive mismatched txid/index pairs, but current Bitcoin Core cannot
trigger it through an invalid peer block, consensus validation, fund loss,
memory corruption, or cryptographic compromise. No cryptographic nonce is
involved; a nonce without cryptographic meaning would not require clearing.
This is not Critical.

### Corpus and differential proof

The frozen corpus is
`/tmp/bitcoin-merkleblock-20260721/frozen/merkleblock`, copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/merkleblock`: 432 files,
45,510,755 bytes, minimum/maximum size 1/1,045,251 bytes. The sorted
filename manifest SHA-256 is
`cc200c7bc021933fcf9320e6ce09efbf6e1640e899a0df5b35e2afb1ed05e148`; the
filename/size manifest SHA-256 is
`2c85704dffd0235f968ed2ee142fb90284ac1ae71a86e86d6b9d5862b9c8371e`.

On the unmodified master behavior, the pre-seeded oracle failed at
`src/test/fuzz/merkleblock.cpp:24` while loading the corpus. The exact
witness is `003b7523bbe7233986b96c4b3cb1e9579155a9f4`, 5,304 bytes,
SHA-256 `5377bef9616e7447343946d18d29fe3fee3176fdb5dd3928216f2deea939c6b9`.
The pre-fix corpus log SHA-256 is
`ce317cad3d62c4c7dc3f184119a0e9f42f3fe67b1f5d1981fec0a41be4e51898`.
The failure is caused by the exact corpus condition plus the pre-seeded
index; no artificial production mutation is needed because master itself
reproduces it. The post-abort symbolizer helper was stopped after preserving
the assertion log.

The restored normal binary SHA-256 is
`0cb2fc1f40185873b4ffc0a0943a1d18b46a5dbb9bf1567f8e044f892781bda0`.
The restored corpus replay executed 433 units (432 corpus files plus the
seed), reached coverage/features 587/2,529, peaked at 152 MB, exited 0, and
left no artifact; log SHA-256:
`81fec82f151800338c5f25361d70f0ecc4bfb8393fd4e65b63309957ccb9f449`.
The restored ASan/UBSan binary SHA-256 is
`e7b9ff6f141df82c4bc5bec8bb46809ca0ab04a01aba7e1bd7abccb6dac5f8e9`.
Its 433-unit replay reached coverage/features 1,033/5,178, peaked at
523 MB, exited 0, and produced no sanitizer or artifact fault; log SHA-256:
`337b7ac331c1b6fe3700e36dfe93919666350fedcf3fc2bc6921fa8dbf42aab6`.
The exact witness passed restored normal and ASan/UBSan replays; their log
SHA-256 values are `d445c7a321f2a4da1d58af53903e106731868c059e5ad984b4767c355e0a704a`
and `7900321060d64d9f56d2d765e4b36580cf9f24ea709f69a697cff06720c32df3`.

Four disjoint ASan/UBSan workers covered all 432 files, 108 per worker, and
each exited 0 without sanitizer markers or artifacts:

    worker0: 110 executions, coverage/features 999/4,843, peak 450 MB, log 93eb0f5b80c3f79782f3079a1ade2507831cf01af37c317984406fe3da1aa2e7
    worker1: 110 executions, coverage/features 1000/4,715, peak 323 MB, log 01928b3b9eef7b68d73a17fb937945d0764242e17b5ff7906c505c3a145b8f54
    worker2: 110 executions, coverage/features 1016/4,854, peak 432 MB, log 077bcaaee1e5ec3b98552d22982ba84e882683a76f3e181202303fbf25b80029
    worker3: 110 executions, coverage/features 982/4,560, peak 259 MB, log 410f2e4c0fb47f647ae97f3e6549a02bfac13f90da2255c4dd48ec765b4663d5

The worker filename union has SHA-256
`cc200c7bc021933fcf9320e6ce09efbf6e1640e899a0df5b35e2afb1ed05e148`.
The focused `merkleblock_tests,bloom_tests,pmt_tests` command passed all 17
cases; log SHA-256:
`3071ff8cd04d91386d6daa177e1bd144a05711dd46ea866eab15c8e68ac258ed`.

### Findings carried forward and cherry-pick policy

Existing findings remain rated against clean master and actual Core callers:
generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, snapshot metadata, scalar/field/group,
DER, and related audits found no additional clean-master production bug.
Latent ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention remain reachability-limited.

The fixed behavior is intentionally at the library output boundary. A later
caller-side pre-clear or cherry-picked potential fix would mask the fuzzer
failure without repairing reusable API state; amend that commit and this
note, or merge the changes, and state whether it masks, preserves, or changes
the master-relative finding. Every follow-up claim still requires a clean
master or minimal-production-mutation reproduction and deterministic proof.
The source commit message contains the same corpus condition, severity,
caller analysis, fork comparison, and verifier requirement. No fuzz,
sanitizer, mutation, or test process remains running.

## Merkle deserialization prefix and extraction oracle audit (2026-07-21)

Source commit: `c759644fc4` (`fuzz: strengthen merkle deserialization
oracles`), parent `cf2cf0460e`, with source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; both
`origin/master` and `remotes/l0rinc/master` pointed there. The target-scoped
l0rinc history over the merkle headers, RPC callers, deserializer, and fuzz
utility paths returned no output. No fork commit was cherry-picked or allowed
to mask this result.

### Target selection and oracle

`merkle_block_deserialize` and `partial_merkle_tree_deserialize` previously
only deserialized an object and returned. The constructor-oriented
`merkleblock` fuzzer already checked the `ExtractMatches` state contract, so
that assertion was moved into `src/test/fuzz/util.h` and reused here rather
than recreated. The existing `blocktransactions_deserialize` prefix check was
also routed through the same canonical-prefix helper; this was a mechanical
reuse, not a behavior change, and its old corpus was replayed after the edit.

For every accepted object, `DeserializeAndAssertCanonicalPrefix` now requires
the serialized object to equal the exact bytes consumed by `SpanReader`, with
the remaining input explicitly accounted for. Trailing bytes remain allowed.
For both partial-tree targets the extraction oracle pre-seeds the index output
with `0xdeadbeef`, requires txid/index pair alignment, requires indices to be
in range and strictly increasing, and requires repeated extraction to return
the same root and vectors. This is an extraction/state oracle, not a proof
validity claim; caller-side root verification remains required.

### Bitcoin Core boundary and severity

The relevant production callers are `verifytxoutproof` at
`src/rpc/txoutproof.cpp:147-166` and `importprunedfunds` at
`src/wallet/rpc/backup.cpp:61-87`. They parse RPC or wallet-supplied proofs,
verify the partial-tree root, and allocate fresh output vectors. The
`net_processing.cpp` merkleblock path constructs and sends filtered-block
replies; it is not an inbound invalid-block acceptance path. No clean-master
production bug was confirmed. This is **Informational/Low** parser-oracle
hardening against master: a malformed proof can be rejected locally, but
these methods cannot turn it into consensus acceptance, fund loss, memory
corruption, or a cryptographic compromise. It is not Critical. No
cryptographic nonce is involved; a nonce without cryptographic meaning does
not require clearing.

### Corpus identity and baseline

The frozen `merkle_block_deserialize` corpus is
`/tmp/bitcoin-merkle-deserialize-20260721/frozen/merkle_block_deserialize`,
copied from `/mnt/my_storage/qa-assets/fuzz_corpora/merkle_block_deserialize`:
82 files, 3,344,447 bytes, minimum/maximum 1/806,511 bytes. Its sorted
filename manifest SHA-256 is
`3194351dce6ab8f40007aec463049ea199470502c46c9652eb5cbb5bf111e542`; its
filename/size manifest SHA-256 is
`98686a173143936f1414c7a1cd98e30f035bd985350bab2745da111197e4c0c4`.

The frozen `partial_merkle_tree_deserialize` corpus is
`/tmp/bitcoin-merkle-deserialize-20260721/frozen/partial_merkle_tree_deserialize`,
copied from `/mnt/my_storage/qa-assets/fuzz_corpora/partial_merkle_tree_deserialize`:
89 files, 2,598,778 bytes, minimum/maximum 4/524,365 bytes. Its sorted
filename manifest SHA-256 is
`b6e092a79d026c542c7ac0b9e9e0dd8106c50de672ba939ace25942735c57c84`; its
filename/size manifest SHA-256 is
`01bad4896092a7da3b4ad2642d8b8ff8b30434ac43702034154cba21cfc32632`.

Before the oracle, the normal replays ran 83 and 90 units with
coverage/features 166/332 and 155/335, peak RSS 62/60 MB, and log SHA-256
values `e8d642c53c2decf1fd374e754454bb4fb6518637f8910718bb83728af99ca9e2`
and `128e0bf8cc8b96939f5bf5dafdc720823d58992e0d44ca462859487d421d688a`.
The baseline ASan/UBSan replays ran 83 and 90 units with coverage/features
234/584 and 218/605, peak RSS 146/139 MB, and log SHA-256 values
`e618a6ca3f26806578fc6cf222c4e3c0fa1117d5ec4972dd549c12b7c2ed94ad` and
`3da334ceb8d42f629af76456c25962ebd2b382f3a582160b6e258223f576680e`.

### Mutation proof

The temporary production mutation was
`SER_READ(obj, obj.vHash.clear())` immediately after
`READWRITE(obj.nTransactions, obj.vHash)` in `src/merkleblock.h`. It models
successful consumption followed by silent loss of the parsed proof hashes.
The `merkle_block_deserialize` replay failed at
`deserialize.cpp:133` on witness
`01a1e3e1d36242b21b99f58d295a66e8e11993f1`, 106,239 bytes, SHA-256
`259412379d063ca77baac1219848b91a8f2ab59183764d8d3339c6803a5b231e`.
Its exact-input log SHA-256 is
`fb9756098bcaf5e020d21168b755ec799cf246c7c46e6d8c12d48abf075b9d6a`, and
the full mutation-corpus log SHA-256 is
`3d7ef09376ef776f141b5a2316b09a433a946c7e559172c83ded6a10874c0c92`.
The `partial_merkle_tree_deserialize` replay failed at the same assertion on
witness `00013359ebca06518ff1e224de58b5934f7490ef`, 3,509 bytes, SHA-256
`c7209f2b75e5a367fb8f7428f5c2b5f3c6724d0b79e922517e63b8c9fd5ec2ee`.
Its exact-input log SHA-256 is
`8d960d0ff1519d8d44d4df3f89db62e1183174dd781b6b53b81f1c2025c216c2`, and
the full mutation-corpus log SHA-256 is
`15c4403f7f61d815a26555e6932115511fcd9779c6adc45ca037b73c5bd68a6a`.
The mutation was removed before the source commit; no temporary production
behavior remains.

### Restored replay and workers

The final normal and ASan/UBSan fuzz binary SHA-256 values are
`4f55cc6fff5b3ce910d57628c6571777f15e83c19aa4b708c1fc4e5215f9d151` and
`67ffd3877b4928b39cd46407c6fe397f18bb589b4462e2479bbb7e8b4dd8d42c`.
Restored normal `merkle_block_deserialize` and `partial_merkle_tree_deserialize`
replays ran 83/90 units, coverage/features 188/356 and 172/352, peak RSS
62/61 MB; log SHA-256 values are
`94d68f3008a54c88c308b1db897b506c3138b476bc78735bd7a8e94edc8735f0` and
`4ad71d2e56d21c33d555f4ce7f652e65c5d1fd684f61a2c99fdcd4d628745f98`.
Restored ASan/UBSan replays ran 83/90 units, coverage/features 281/633 and
252/638, peak RSS 147/139 MB, and no sanitizer or artifact fault; log
SHA-256 values are
`1fbcf57327381831f2d73b535859f488c821c4a2c0886bb6b095a48dcd97f69e` and
`b45a7888138e7e41fc998b9eeb7270d0f73b57f669309ce44197416e221b10cd`.

Four disjoint ASan/UBSan workers were run for each target. The worker logs
and replay summaries are:

    merkle_block_deserialize worker0: 23 executions, coverage/features 248/491, peak 118 MB, log 3a131ba000494df46d2e6f126188130ccb135ca6c735dc57a532eceef9f167c7
    merkle_block_deserialize worker1: 23 executions, coverage/features 258/506, peak 116 MB, log dfc3474849925a4a3a44469f0ed2b71ee1834613da281e068dd47a93faf8cbd2
    merkle_block_deserialize worker2: 22 executions, coverage/features 259/488, peak 111 MB, log a78a0b96af22bd44b1aa038ee20d3fe56393f4e15b51b7f159af02164587e4a1
    merkle_block_deserialize worker3: 22 executions, coverage/features 266/490, peak 119 MB, log 5556de7cac56b5b9a253170c7960156ee8faf54b19bfae9b9c72522cbc1ae3f2
    partial_merkle_tree_deserialize worker0: 25 executions, coverage/features 244/552, peak 112 MB, log 4ef8843d9a3d28da0a4ead46feabb379c81c1e01f2a62624a95ed6b48e87d66c
    partial_merkle_tree_deserialize worker1: 24 executions, coverage/features 248/537, peak 111 MB, log 08134d5cf556236f1179b3dbe62b68effaf9d058d0268731187711b997d68cd1
    partial_merkle_tree_deserialize worker2: 24 executions, coverage/features 243/532, peak 112 MB, log e17c7f88525d3971b37ae8bbaa6c543240d67f0f5c0cbfd30de8995761642c55
    partial_merkle_tree_deserialize worker3: 24 executions, coverage/features 237/546, peak 120 MB, log b5ed95bd121cf46070e9945daecf1024d98ca8e8af23b4a77d3f3a21ed5ecef1

The worker filename unions exactly match the frozen manifests: SHA-256
`3194351dce6ab8f40007aec463049ea199470502c46c9652eb5cbb5bf111e542` for
`merkle_block_deserialize` and
`b6e092a79d026c542c7ac0b9e9e0dd8106c50de672ba939ace25942735c57c84` for
`partial_merkle_tree_deserialize`. The focused
`merkleblock_tests,bloom_tests,pmt_tests` command passed all 17 cases; log
SHA-256 `02fd698e4a6ebd51d2ae1f6f89b1893719229555144409963d75aa63e3fa72fc`.
The earlier `merkleblock` and `blocktransactions_deserialize` corpora were
also replayed after the shared-helper refactor: normal/ASan log SHA-256
values are `47920a806000493ca1faa8406d28b3940b036697e35107f749d9943cfb59095d`,
`8298c3583e8b54f754d5d20936fa2f118bb8d58d77ee866fd0014ee87df3d4e8`,
`6c139c6556aac5e6bee886d4f0c0e0433627809554a6e9853fe859a9392ac4b0`, and
`c0425d81edc201ebd7569e3dc958057f0b73083ac499597ebf2b5824ed635f1d`.

### Findings carried forward and follow-up policy

Existing findings remain rated against clean master and actual Core callers:
generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, snapshot metadata, scalar/field/group,
DER, and related audits found no additional clean-master production bug.
Latent ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention remain reachability-limited.

A later caller-side pre-clear or potential cherry-pick could mask a parser
oracle without fixing the production state transition. Amend that follow-up
commit and this note, or merge the changes, and state whether it masks,
preserves, or changes the master-relative behavior. Every claimed production
bug still requires clean-master or minimal-mutation reproduction and the
strongest deterministic proof available. `git diff --check` passed, and no
fuzz, sanitizer, mutation, or test process remains running.

## CBlockFileInfo deserialization oracle audit (2026-07-21)

Source commit: `7a3d572282` (`fuzz: strengthen CBlockFileInfo deserialization
oracle`), parent `f095258342`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit started from the latest Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` resolved to that same commit. A target-scoped query
over `src/node/blockstorage.h`, `src/node/blockstorage.cpp`,
`src/test/fuzz/deserialize.cpp`, `src/test/fuzz/block_index.cpp`,
`src/test/blockmanager_tests.cpp`, and the l0rinc branch found no relevant
post-base l0rinc commit, so no cherry-pick was applicable here.

### Contract and caller boundary

`block_file_info_deserialize` previously deserialized a `CBlockFileInfo` and
discarded it. The new oracle applies the shared canonical-prefix check, then
serializes and reparses the complete object and compares every field.
`CBlockFileInfo` has seven VARINT-encoded fields in this order:
`nBlocks`, `nSize`, `nUndoSize`, `nHeightFirst`, `nHeightLast`, `nTimeFirst`,
and `nTimeLast`. The round trip therefore checks canonical encoding,
truncation/leftover handling, and field preservation without assuming that a
successful parse is semantically valid.

The semantic boundary matters. `AddBlock` normally maintains the height and
time extrema for records created by Core, but `BlockTreeDB::ReadBlockFileInfo`
at `src/node/blockstorage.cpp:68-71` reads a local database record directly.
`BlockManager::LoadBlockIndexDB` at `:556-602` loads the records, logs the
last one, and uses `nHeightLast` to initialize block-file cursors. The same
metadata drives pruning and current-usage accounting at `:323-410`, block
file size updates around `:958-966`, undo allocation at `:968-980`, and undo
flush positions at `:758-765`.

This target intentionally does not assert
`nHeightFirst <= nHeightLast` or `nTimeFirst <= nTimeLast`. The raw production
DB reader does not enforce those relations, and the existing stateful
`block_index` fuzzer intentionally creates and writes arbitrary
`CBlockFileInfo` records. Adding those checks here would be an overbroad
fuzzer domain assumption rather than a proven production contract. A future
semantic finding must first identify the production check or caller that
requires the relation.

The master-relative rating is **Informational/Low local-persistence
hardening**, not a production vulnerability. This object is loaded from
Bitcoin Core's local `blocks/index` database; it is not a peer-supplied block
header, transaction, or consensus-validation input. A corrupt local record
could affect restart, pruning, usage accounting, or file-cursor bookkeeping,
but this deserializer cannot make an invalid network block acceptable, alter
consensus state, or move funds. No clean-master production bug, memory bug,
race, or resource-amplification issue was found. It is consequently not
Critical under the actual Bitcoin Core caller model, even though malformed
metadata can be constructed by the fuzzer.

This audit did not involve a cryptographic nonce. Clearing or retaining a
non-cryptographic nonce or version field is not Critical; severity follows the
reachable Bitcoin Core effect.

### Corpus and pre-oracle baseline

The frozen corpus is `/tmp/bitcoin-blockfileinfo-20260721-clean/frozen`,
copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/block_file_info_deserialize`:
39 files, 592 bytes total, minimum/maximum 1/74 bytes. The sorted filename
manifest SHA-256 is
`ac495faad424b2949ce68b704e8114965dd25a613e347a7715ad920c24f6f296`.
The filename/size manifest SHA-256 is
`83e8e0421b8bafd351a07c70cc813892c67bd21217bbc4acd4f108c158edb3fa`.

Before the oracle, the normal replay exited 0 after 40 executions, reached
coverage/features 110/207, peaked at 55 MiB, and produced log SHA-256
`af27880e9ea51a9b6ed919beb8e4ae910baa6a1637d6d5860e90732d7ba4a6f2`.
The pre-oracle ASan/UBSan replay exited 0 after 40 executions, reached
143/325, peaked at 104 MiB, and produced log SHA-256
`5bab00242eff4939e00b03d36cee0ecbd101eccd33fd68833b38dd870766d08c`.

Existing `blockmanager_tests` and `serialize_tests` cover constructed
objects and database workflows, and `block_index` exercises arbitrary
stateful records. The old raw-input target did not compare arbitrary decoded
bytes or fields against canonical reserialization, so those tests and the
success-only fuzzer did not establish this byte/field invariant.

### Differential sensitivity proof

A temporary production-code mutation was inserted after the final
`READWRITE` in `CBlockFileInfo`: on read only,
`const_cast<CBlockFileInfo&>(obj).nSize ^= 1`. A temporary
`block_file_info_deserialize_control` target retained the old
deserialize-and-discard behavior. Both temporary changes were removed before
the source commit.

With the exact frozen corpus, the enhanced normal target hit the round-trip
assertion after 12 executions. The wrapper returned 124 after the
assertion/symbolizer path; this is a harness termination detail, not a
clean-master failure. The mutation log SHA-256 is
`982521a92bcf65bf1b56d1858979738db15cb58a70f9a4330a9a45c01bb87148`.
The matched normal control exited 0 after all 40 executions; its log SHA-256
is `b40e854f9f27dc0704154cce677ee89ef3fa6746d8d834178f02839ea16b0e25`.

Under ASan/UBSan, the enhanced target hit the same assertion after 11
executions; its mutation log SHA-256 is
`df2e5506637fbc82b7cde17d96e5f858fef609bf15d14a6d7986f58533f39d81`.
The matched sanitizer control exited 0 after all 40 executions; its log
SHA-256 is
`c67de33f924bc34ad01732b411bd5f8a89941e8a9c7c6ceb77b7bc6665fd862a`.

This proves that the new oracle detects a modeled field-loss/state-corruption
condition that the old target missed. It is not evidence of a clean-master
production bug: the mutation was artificial, no source mutation remains, and
no production fix or deterministic regression test is claimed for this
harness-only hardening.

### Restored verification

After removing the mutation and control target, the final normal fuzz binary
SHA-256 is
`4fe30ad880de1d0d5eaa0c063374b7967f0efa273346c8123141de10f18f8052`.
The restored normal replay exited 0 after 40 executions, reached 126/220,
peaked at 54 MiB, and produced log SHA-256
`a43fdc2ba05b55b2f1867559d9eb77947e488c7b1c5eff161975009ef8d2d8b5`.

One initial post-restore ASan invocation was discarded: its binary timestamp
predated the restored `blockstorage.h`, and it still contained the temporary
mutation, so it hit the known mutation assertion. The fuzzer and symbolizer
were terminated. The ASan build was then cleaned (542 artifacts), rebuilt
with ccache disabled, and timestamp-checked before replay. The restored
source `blockstorage.h` was at 23:59:01 on 2026-07-21; the rebuilt
`deserialize.cpp.o` was at 00:30:09, `blockstorage.cpp.o` at 00:17:13, and
the fuzz binary at 00:37:02 on 2026-07-22. No control target or mutation
symbol remained. Only this clean replay is evidence.

The final clean ASan/UBSan binary SHA-256 is
`9656ed212df573c85cfb5a43ed72a6c92f440b4303068b7705886a4cfbb5ea12`.
Its replay exited 0 after 40 executions, reached 170/349, peaked at 104 MiB,
and produced log SHA-256
`e71b589773163235c4c5c4610c90ed1a9d5e29c7d7249d569ebfab9340887f37`.

Four disjoint ASan/UBSan workers processed the frozen corpus: 11/11/11/10
executions, with peak RSS 103/103/103/104 MiB. Worker log SHA-256 values are:

    worker0: 5f9ce1293221a9d603ef80ddd691f16d51d7c606d205960c39f5000549cf5c1f
    worker1: 650161ece78d931ff2008bf9c3e4789e88a070698e6bb93652a50b80430162ed
    worker2: 7b30adc1282daa8a92ba7ef50b368e8b00cff9aae1ba36fcd967927d319237e8
    worker3: 75cf7e73e2c80d1426b66a2b0c7e1a1bb26670dd8146d8cfe0a4f6dd792df766

The 39-file worker union exactly matches the frozen filename manifest:
`ac495faad424b2949ce68b704e8114965dd25a613e347a7715ad920c24f6f296`.
The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=blockmanager_tests,serialize_tests,net_tests --log_level=test_suite

exited 0 with `*** No errors detected`; its log SHA-256 is
`a559ab24e2589b41269f8b073f9ad3ff141b4978dcabd7850a93163a2fda0864`.
`git diff --check` passed after restoration, and no fuzz, sanitizer,
mutation, symbolizer, or test process remained running.

### Findings carried forward and follow-up policy

This audit reiterates the existing ledger rather than adding a production
finding. Generic raw `finalizepsbt` invalid `final_scriptSig` remains Low
local RPC correctness; feature-conditional private-broadcast failed-send
retention and empty HEADERS initial-sync availability remain Medium; peer
activity refresh, block-storage failure, oversized transport types, and
banman invalid-subnet/unban remain Low or hardening. Earlier addrman,
coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
headers-sync, UTXO snapshot, mempool persistence, package, handshake,
`BufferedFile`, block-index, compact-block, Merkle, wire, snapshot metadata,
Bloom, compressed amount, scalar/field/group, DER, EllSwift, wallet, PSBT,
scriptpubkeyman, BIP324, CMessageHeader, and related audits found no
additional clean-master production bug unless their notes say otherwise.
Latent ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention remain reachability-limited.

A later caller-side pre-clear or potential cherry-pick can mask a follow-up
oracle without fixing the production state transition. Any such follow-up
must amend its own commit message and this durable note, or merge the changes,
and state the exact target, caller, corpus/mutation, assertion/failure,
master-relative severity, and whether the change masks, preserves, or alters
the behavior being tested. A potential fix is not proof. Every claimed
production bug still requires clean-master or minimal-production-mutation
reproduction and the strongest deterministic regression/functional proof
available.

## CInv deserialization oracle audit (2026-07-21)

Source commit: `f095258342` (`fuzz: strengthen CInv deserialization oracle`),
parent `fd87dbdc73`, on source branch `codex/fuzz-oracles-current` in
`/tmp/bitcoin-secp256k1-audit-current`. The audit started from the latest
Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` resolved to that same commit. A target-scoped query
over `src/protocol.h`, `src/protocol.cpp`, `src/net_processing.cpp`,
`src/test/fuzz/deserialize.cpp`, the related fuzzers, and protocol tests found
no post-base l0rinc commits relevant to `CInv`, so no cherry-pick was applied.

### Contract and caller boundary

`inv_deserialize` previously parsed one `CInv` and discarded it. The new oracle
uses the shared canonical-prefix check and a fixed-field round trip. Successful
input must reproduce the exact serialized prefix, consume one `CInv` record,
and preserve both decoded fields: the `uint32_t type` and `uint256 hash`.

Truncated 1-byte and 4-byte inputs remain negative fuzzer inputs. Unknown
inventory types remain valid decoded values: `CInv` serialization has no
canonical enum restriction, and Core intentionally logs unknown types rather
than requiring `IsGenTxMsg()` or `IsGenBlkMsg()` to be true. The oracle does not
invent a domain constraint that the production contract does not have.

Bitcoin Core decodes inbound `INV` vectors in
`PeerManagerImpl::ProcessMessage` at `src/net_processing.cpp:4183-4185`, checks
`MAX_INV_SZ` at `:4186-4189`, and uses known block inventory to update block
availability and potentially trigger headers sync or block fetching at
`:4211-4269`. Known transaction inventory is converted to `GenTxid` and sent to
the transaction download manager at `:4219-4250`; unknown types are logged and
ignored.

The same wire record is decoded for `GETDATA` at `:4274-4283`, checked against
`MAX_INV_SZ`, then either compared with the private-broadcast transaction at
`:4289-4313` or queued for `ProcessGetData` at `:4316-4320`. `NOTFOUND` records
are decoded at `:5169-5181` and filtered to transaction inventory before
reaching `txdownloadman`.

The master-relative rating is **Informational/Low oracle hardening**, not a
production vulnerability. An arbitrary `CInv` can cause expected availability
lookups, announcements, requests, or peer bookkeeping, but cannot by itself
accept an invalid block, alter the UTXO set or consensus state, or move funds.
No clean-master memory, race, resource-amplification, or production failure
was found, and this audit makes no DoS claim. It is not Critical merely because
an invalid block hash may appear in an inventory announcement; severity would
rise only if the actual caller carried the value into invalid-block acceptance
or equivalent security impact.

This audit did not involve a cryptographic nonce. Clearing or retaining a
non-cryptographic nonce or version field is not Critical; severity follows the
reachable Bitcoin Core effect.

### Corpus and pre-oracle baseline

The frozen corpus is `/tmp/bitcoin-inv-20260721-clean/frozen`, copied
byte-for-byte from `/mnt/my_storage/qa-assets/fuzz_corpora/inv_deserialize`:
6 files, 82 bytes total, minimum/maximum 1/36 bytes. The sorted filename
manifest SHA-256 is
`c77d8c7f7b26da2c9e3b6129499f409659ba4cd210a338057f6c84044a444c0e`.
The filename/size manifest SHA-256 is
`afea795cc7f867d824be224d235dca7713003d8b57cb7305ab7cf072d6a8761c`.

Before the oracle, the normal replay exited 0 after 7 executions, reached
coverage/features 60/64, peaked at 55 MiB, and produced log SHA-256
`a0aff785247c4785ba9a03a1f562c8e6d5a4ffa821d9d9b86f2b7a3b05ed3717`.
The pre-oracle ASan/UBSan replay exited 0 after 7 executions, reached 98/104,
peaked at 103 MiB, and produced log SHA-256
`a2d1f32c0b56592855b12d2315422ebaca563cee803e371403cec6b550cd3db7`.

Existing `net_tests` and `serialize_tests` exercise constructed protocol
messages and Core processing, but the old fuzzer did not compare arbitrary
decoded `CInv` bytes or fields against canonical reserialization. Those tests
and the success-only fuzzer therefore did not establish this raw-input
invariant.

### Differential sensitivity proof

A temporary production-code mutation was inserted after `READWRITE` in
`CInv`: on read only,
`const_cast<CInv&>(obj).type ^= 1`. A temporary
`inv_deserialize_control` target retained the old deserialize-and-discard
behavior. Both temporary changes were removed before the source commit.

With the exact frozen corpus, the enhanced normal target hit the
canonical-prefix `memcmp` assertion after 6 executions. The wrapper returned
124 because libFuzzer remained in its symbolizer/termination path. The
mutation log SHA-256 is
`ecb53e82268bd2e95e5e4d6467c568d434fdcb32b3fa2d9b3a33ee111dd1af24`.
The matched normal control exited 0 after all 7 executions; its log SHA-256
is `7ea516413bc44eed4e1b630211ca6a7a1efc3cbaeaaa0595d6595a8a85bf7351`.

Under ASan/UBSan the enhanced target hit the same assertion after 6
executions; its mutation log SHA-256 is
`c30bcce462fde3e3ae879d437784dc45d32765c09fb8dd3238407333bf973d90`.
The matched sanitizer control exited 0 after all 7 executions; its log SHA-256
is `c4ecf45cd87a932b0f70c84128fa6ba8164ff305133e8c465d638a1859162a78`.

This proves that the oracle detects a modeled production state corruption that
the old target missed. It is not evidence of a clean-master production bug:
no source mutation remains, no production fix is claimed, and no deterministic
production regression test is claimed for this harness-only hardening.

### Restored verification

After removing the mutation and control target, the normal fuzz binary SHA-256
is `b523af95f969b3ca9e50e45fb2f16b3678da2be470be4727d17d22a959c39f9b`.
The restored replay exited 0 after 7 executions, reached coverage/features
67/72, peaked at 55 MiB, and produced log SHA-256
`916544dca8127c0315976448556518af198d64e8b8388d34ef347424accfb49e`.
The restored ASan/UBSan binary SHA-256 is
`1d4a7d6c5f2850641637c4ebf85290ab9523931a7e46ba650838e606143f5f2b`.
Its timestamp-verified replay exited 0 after 7 executions, reached 114/122,
peaked at 103 MiB, and produced log SHA-256
`563979e119239258154cd60aeedc096b94bebac1e90fa826d38a6125f43ce86a`.

One initial post-restore ASan invocation was discarded: a timestamp check
showed that it used the pre-restoration mutation binary and it hit the known
mutation assertion. The fuzzer and symbolizer were terminated, the ASan target
was rebuilt from restored source, object and binary timestamps were verified
newer than `protocol.h`, and only the resulting clean replay above is evidence.

Four disjoint ASan/UBSan workers processed 3/3/2/2 inputs. All exited 0
without sanitizer diagnostics and reported peak RSS 104/103/104/103 MiB.
Worker log SHA-256 values are:

    worker0: 3a8c99bbf9b8c45db63c553193d6bfcb4339ab1723309187e41c611e3779bae5
    worker1: 952e1d79cfc2d97bf730abaa5f63b3ecf7ca2b8e3576952247dec4a0853f96d0
    worker2: 804056f28bb95ce2c7b3106336fb6be76ae8ae6839ef7e3b334e504be9744eac
    worker3: a5e4dc4636275cadb1e8e2fe75acee25584ee43f85ad33b8e7948293dbd728c8

The 6-worker-file union exactly matches the frozen filename manifest:
`c77d8c7f7b26da2c9e3b6129499f409659ba4cd210a338057f6c84044a444c0e`.
The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=skiplist_tests,net_tests,serialize_tests --log_level=test_suite

exited 0 with `*** No errors detected`; its log SHA-256 is
`41865aeb836335b6d78727914d2e935716838e72127dd1916de1dae769caeb6c`.
`git diff --check` passed after restoration, no crash/leak/timeout artifact
remained in the source worktree, and no fuzz, sanitizer, mutation, symbolizer,
or test process remained running.

### Findings carried forward and follow-up policy

Existing findings remain rated against clean master and actual Core callers:
generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync handoff are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier addrman, coins-cache, txgraph,
txdownloadman, txrequest, connman, eviction, headers-sync, UTXO snapshot,
mempool persistence, package, handshake, `BufferedFile`, block-index,
compact-block, Merkle, wire, snapshot metadata, Bloom, compressed amount,
scalar/field/group, DER, EllSwift, wallet, PSBT, scriptpubkeyman, BIP324,
CMessageHeader, and related audits found no additional clean-master production
bug unless their notes say otherwise. Latent ecmult scratch wrapping, 10x26
magnitude normalization, and SHA/HMAC/RFC6979 retention remain
reachability-limited.

A later caller-side pre-clear or potential cherry-pick can mask a follow-up
oracle without fixing the production state transition. Any such follow-up must
amend its own commit message and this durable note, or merge the changes, and
state the exact target, caller, corpus/mutation, assertion/failure,
master-relative severity, and whether the change masks, preserves, or alters
the behavior being tested. A potential fix is not proof. Every claimed
production bug still requires clean-master or minimal-production-mutation
reproduction and the strongest deterministic regression/functional proof
available.

## CMessageHeader deserialization oracle audit (2026-07-21)

Source commit: `fd87dbdc73` (`fuzz: strengthen CMessageHeader deserialization
oracle`), parent `2dcc728d1e`, on source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit started from Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` resolved to that same commit. A target-scoped query
over `src/protocol.h`, `src/protocol.cpp`, `src/net.cpp`, and
`src/test/fuzz/deserialize.cpp` found no post-base l0rinc commits relevant to
this target, so no cherry-pick was applied.

### Contract and caller boundary

`messageheader_deserialize` previously deserialized `CMessageHeader`, called
`IsMessageTypeValid()`, and discarded the object. The new oracle uses the
shared canonical-prefix check and a fixed-header round trip. Successful input
must reproduce the exact serialized prefix, consume one fixed 24-byte header,
and preserve `pchMessageStart`, all 12 `m_msg_type` bytes, `nMessageSize`, and
all four `pchChecksum` bytes through serialize/reparse. The helper also asserts
that the serialized size is `CMessageHeader::HEADER_SIZE`.

The target still invokes `IsMessageTypeValid()` without requiring it to return
true. Non-printable bytes, nonzero bytes after the first NUL, and other invalid
message types are expected rejection inputs, not fuzzer failures. The shared
prefix helper permits trailing bytes, although this corpus contains successful
headers of at most 24 bytes. No production behavior changed.

Bitcoin Core receives the untrusted V1 header in `CNode::ReceiveMsgBytes` at
`src/net.cpp:668`, decoding it around `:743-755`. It then checks the network
magic at `:763-764`, rejects sizes above `MAX_PROTOCOL_MESSAGE_LENGTH` at
`:771-772`, verifies the checksum, and rejects invalid message types at
`:829-840` before dispatch.

The master-relative rating is **Informational/Low oracle hardening**, not a
production vulnerability. A `CMessageHeader` is transport metadata: malformed
peer bytes are rejected or cause a disconnect and cannot by themselves accept
an invalid block, alter consensus or the UTXO set, move funds, or reach a
cryptographic key/signature transition. This is therefore not Critical under
the Bitcoin Core caller model, despite direct peer control of the bytes. No
clean-master memory, race, resource-amplification, or production failure was
found, and this audit makes no DoS claim. A parser issue would warrant a higher
rating only when the actual Core caller can carry it into an invalid-block,
consensus, funds, or equivalent security-impacting transition.

This audit did not involve a cryptographic nonce. Clearing or retaining a
non-cryptographic nonce or version field is not Critical; severity follows the
reachable caller effect, not the field name or cleanup appearance.

### Corpus and pre-oracle baseline

The frozen corpus is
`/tmp/bitcoin-messageheader-20260721-clean/frozen/messageheader_deserialize`,
copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/messageheader_deserialize`: 71 files,
1,594 bytes total, minimum/maximum 1/24 bytes. The sorted filename manifest
SHA-256 is
`4c0b37ee9c9bb8d77d72ec412f4bae5ca5aa84c11e97669a329948b3dd37c722`.
The filename/size manifest SHA-256 is
`5d0c65b081eabcaff29d7e4ef085c30c672e603fbd8234c23699859fcffb43f5`.

Before adding the oracle, the normal replay exited 0 after 72 executions,
reached coverage/features 112/124, peaked at 55 MiB, and produced log
SHA-256
`3df770ae401662a4cb8d4f95733ccad7f02e8632e7fbfcc80e7a5bf038ec4c62`.
The ASan/UBSan replay also exited 0 after 72 executions, reached 118/151,
peaked at 105 MiB, and produced log SHA-256
`57c92f442c0386c5d6b6f127f92025e84a64e761c8eb50a6d32e6fd6bb7352d8`.

Existing `net_tests` and `serialize_tests` exercise constructed headers and
transport behavior, but the old target did not compare corpus bytes with the
decoded object. Those tests and the previous success-only fuzzer therefore did
not establish this raw-input invariant.

### Differential sensitivity proof

A temporary production-code mutation was inserted after `READWRITE` in
`CMessageHeader`: on read only,
`const_cast<CMessageHeader&>(obj).nMessageSize ^= 1`. A temporary
`messageheader_deserialize_control` target retained the old deserialize-and-
discard behavior. Both temporary changes were removed before the source
commit.

Against the exact frozen corpus, the enhanced normal target hit the
`DeserializeAndAssertCanonicalPrefix` `memcmp` assertion after 10 executions.
The wrapper returned 124 because libFuzzer remained in its
symbolizer/termination path. The mutation log SHA-256 is
`225c36853b78c1aca0cd1c9728daba690dd4dac935ba1f04ba28d4bcaab3b08b`.
The matched normal control exited 0 after all 72 executions; its log SHA-256
is `d56dd596d4556787b0ac5031b3fae86047574e577f5315655431a91044262eab`.

Under ASan/UBSan, the enhanced target hit the same assertion after 10
executions; its mutation log SHA-256 is
`c8ae1d19d3cb06e612d0bdcc369384ae2a217e3f0d445aea8b4c3d27e99c1c9b`.
The matched sanitizer control exited 0 after all 72 executions; its log
SHA-256 is `9a75d896008e65450c72512fe369aac7a0c929f6060cb25d196cdddb5d15aa58`.

This proves that the oracle detects a modeled production state corruption that
the old target missed. It is not a clean-master production finding: no source
mutation remains, no production fix is claimed, and no deterministic
production regression test is claimed for this harness-only hardening.

### Restored verification

After removing the mutation and control target, the normal fuzz binary
SHA-256 is
`efedbaceb60f9830dc08e6648c6c78a3ee7d0edf3789f64171edcc5c8acf85f3`.
Its restored replay exited 0 after 72 executions, reached coverage/features
121/133, peaked at 54 MiB, and produced log SHA-256
`fa997f6a2df91d57d2606db76a449639d3e170a5a27164cfe0875f8b44bbce22`.
The restored ASan/UBSan fuzz binary SHA-256 is
`033fcc4399e38f5d726515941419e9d00087b66efa81ad50707fbde1cd6caa69`.
It exited 0 after 72 executions, reached 138/171, peaked at 104 MiB, and
produced log SHA-256
`de2a5e9051c193f5587229801b68635076e6ff3dfdaf478da976e58647ffb4d4`.

Four disjoint ASan/UBSan workers processed 19/19/19/18 inputs. All exited 0
without sanitizer diagnostics and reported peak RSS 104/103/104/103 MiB.
Worker log SHA-256 values are:

    worker0: 054449684525b995c34429abcd9fcae053a27acf461f0f3f226811f7c599de8b
    worker1: 06a524517742cbdffcc48092aee4875ae13be07cdda2f9b762b97833fc3659f2
    worker2: 9ec3b7d453db8da143e5937a37c9fcc620e63c8dd188ff1808d188686b4d34b
    worker3: 018d66f10e25bb2bc1f649c709d71a59339cf2005efc94e44fe621366756e81c

The 71-worker-file union exactly matches the frozen filename manifest:
`4c0b37ee9c9bb8d77d72ec412f4bae5ca5aa84c11e97669a329948b3dd37c722`.
The focused command

    /tmp/bitcoin-descriptor-test-build/bin/test_bitcoin --run_test=skiplist_tests,net_tests,serialize_tests --log_level=test_suite

exited 0 with `*** No errors detected`; its log SHA-256 is
`6b59dd6e954a84262add338b1a947a3b551c6bb06278b335c3f337cbddb1e230`.
`git diff --check` passed after restoration, and no fuzz, sanitizer, mutation,
or test process remained running.

### Findings carried forward and follow-up policy

Existing findings remain rated against clean master and actual Core callers:
generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync handoff are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier addrman, coins-cache, txgraph,
txdownloadman, txrequest, connman, eviction, headers-sync, UTXO snapshot,
mempool persistence, package, handshake, `BufferedFile`, block-index,
compact-block, Merkle, wire, snapshot metadata, Bloom, compressed amount,
scalar/field/group, DER, EllSwift, wallet, PSBT, scriptpubkeyman, BIP324, and
related audits found no additional clean-master production bug unless their
notes say otherwise. Latent ecmult scratch wrapping, 10x26 magnitude
normalization, and SHA/HMAC/RFC6979 retention remain reachability-limited.

A later caller-side pre-clear or potential cherry-pick can mask a follow-up
oracle without fixing the production state transition. Any such follow-up must
amend its own commit message and this durable note, or merge the changes, and
state the exact target, caller, corpus/mutation, assertion/failure,
master-relative severity, and whether the change masks, preserves, or alters
the behavior being tested. A potential fix is not proof. Every claimed
production bug still requires clean-master or minimal-production-mutation
reproduction and the strongest deterministic regression/functional proof
available.

## CBlockLocator deserialization oracle audit (2026-07-21)

Source commit: `2dcc728d1e` (`fuzz: strengthen CBlockLocator deserialization
oracle`), parent `fff7127e04`, with source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit base is Bitcoin Core master
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` resolve to that same commit. The target-scoped query
over `src/primitives/block.h`, `src/test/fuzz/deserialize.cpp`,
`src/net_processing.cpp`, `src/validation.cpp`, and `src/protocol.h` found no
post-base commits on either ref. No l0rinc commit applied and none was
cherry-picked. Existing `block_header` and `chain` fuzzers cover constructed
locators and CDiskBlockIndex state, but not this raw success-only target.

### Contract, caller boundary, and severity

`blocklocator_deserialize` previously deserialized `CBlockLocator` and
discarded it. The new oracle requires exact object-prefix consumption,
canonical equality for the `vHave` hash-vector payload, and a complete
serialize/reparse equality check. It intentionally excludes only the first
four bytes from canonical comparison: CBlockLocator historically accepts a
network or disk version and writes the ignored field as `DUMMY_VERSION=70016`.
Trailing bytes remain allowed because `GETBLOCKS` and `GETHEADERS` append
`hashStop`. The harness does not require a nonempty or sorted locator and does
not duplicate the caller's 101-entry policy: null locators are meaningful and
Core checks `MAX_LOCATOR_SZ` after parsing.

Bitcoin Core deserializes inbound `GETBLOCKS` at
`src/net_processing.cpp:4325-4328` and inbound `GETHEADERS` at
`:4452-4455`. Both callers check `vHave.size()` against `MAX_LOCATOR_SZ=101`
and disconnect oversized peers. Accepted hashes reach
`Chainstate::FindForkInGlobalIndex` at `src/validation.cpp:129-147`, where
they only select a block-inventory or header response point.

The master-relative rating is **Informational/Low oracle hardening**, not
High/Critical. This is untrusted peer input, but a locator is not a block or
transaction and cannot by itself accept an invalid block, alter the UTXO set,
move funds, affect consensus validation, or invoke cryptographic nonce/key
logic. The parser may allocate before the caller-side size check, but this
audit found no clean-master memory or concurrency failure and makes no denial
of service claim. Invalid locator bytes alone are not an invalid-block
vulnerability.

### Corpus and baseline

The frozen corpus is
`/tmp/bitcoin-blocklocator-20260721-clean/frozen/blocklocator_deserialize`,
copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/blocklocator_deserialize`: 42 files,
1,807,442 bytes, minimum/maximum 1/525,799 bytes. Sorted filename manifest:
`efc0019ed210dae97bdd16f15271d46a1d177c6eb3cecfd18633c90432e80708`.
Filename/size manifest:
`4fcb15582c79450d8ecd266f547f2833084e1e600d03df7baa7706fca501d9c8`.

Before the oracle, the normal fuzz binary was
`430357fd8d624af0a14a46bffb2a7ac584d953a00272450b53e049994437e374`.
Its 43-unit replay exited 0, reached coverage/features 99/212, peaked at
59 MiB, and has log SHA-256
`ca1965a56c93d4a02d4a93996472a107b77ff29e9bbd7767ad0a177e26b0536d`.
The ASan/UBSan binary was
`84a953ece18374acf131572874fef74ed0f739b36cd050691d84ca2722d59f11`.
Its replay exited 0, reached 145/428, peaked at 122 MiB, and has log SHA-256
`843a6a72ffdef8860e1fa7b6d9dd4790a484430039b9efce0921ed56f83f2098`.

### Overbroad oracle and differential proof

The first implementation reused the generic canonical-prefix memcmp,
including the version bytes. It failed on corpus input
`140b86c66169f4ac01edd09e21d539ad23b9e641`, 12,006 bytes, SHA-256
`d5fdd4dedb36a741052be8618029401e06f61a298d830e76094f54450dc34275`.
Its first four bytes are `bd 00 cf ff`, a historical version value. The
direct stale-oracle log SHA-256 is
`529e4a3f3b11d7f02327d132fcc944f248de65aa35d9209d1c49628ac57af3b9`.
This was classified as a stale/overbroad oracle, not a production finding:
CBlockLocator intentionally ignores and normalizes this field. The final
oracle compares the consumed vector payload and exact length while preserving
that contract.

For oracle sensitivity, a temporary production mutation inserted the
read-side-only operation
`const_cast<CBlockLocator&>(obj).vHave.front() = uint256{}` immediately after
`READWRITE(obj.vHave)` when the vector was nonempty. A temporary
`blocklocator_deserialize_control` target retained the old deserialize-and-
discard behavior. Both temporary changes were removed before the commit.
On the exact witness, the enhanced normal and ASan/UBSan targets failed at
`deserialize.cpp:198` on the `vHave` payload assertion; their logs are
`aaec00cb2f639b15b333d2600854db21ac6debce0d0ab58ef0f1babae5cc7d2f` and
`19e28e363231d823ea268d0d1943e2070118f90fa897e5851180ab2098147ecb`.
The timeout status was 124 because libFuzzer terminated after the assertion
while its symbolizer wrapper remained active. The matched old control accepted
the identical witness and exited 0 in both modes; control logs are
`f233f0432fe3dff7bd7917b328dd1652bee9db11b4eecab6391e90338ae147b9` and
`76fb4009917ea9b64d3cf990ee8373f9ee0ae106a455c9e11e1d3b6914f6a9aa`.
This proves oracle sensitivity to a modeled production state corruption, not
a clean-master bug. No production fix or deterministic regression test is
claimed.

### Restored verification

Final normal and ASan/UBSan fuzz binaries are
`54f9b371736db2a8d7df9a87b495aa5b6ee43cb4a748a3e9657d09d7bd6f771b` and
`0ee6b273576da613cab2af75e47a703279d52463ef593ea5f8c818db19ea03d2`.
The restored normal replay exited 0 after 43 units, reached 126/265, peaked
at 60 MiB, and has log SHA-256
`1d9c2b9bac1e01fd7f9cd326798a1b7d273aa657f5dda822a10eb466c73f719b`.
The restored ASan/UBSan replay exited 0 after 43 units, reached 182/495,
peaked at 132 MiB, and has log SHA-256
`ae305d4645a30fe826a5bf33a0390311cccea0d12ce1b8aedf7d32e75b2e7f21`.
The historical-version witness replayed cleanly in normal and sanitizer
modes; exact log SHA-256 values are
`1d2e2f555c88dc2d4766f24c35e94a5449841544544145afe267c053f5b4f1f3` and
`8987d0e05402e5cfd767adf2ed14280844b2f8f65399b8aec30331f667dbdc97`.

Four independent ASan/UBSan workers covered disjoint 11/11/10/10-file
shards and executed 12/12/11/11 units. All exited 0 without sanitizer
diagnostics. Worker log SHA-256 values are
`3fdd7ca085f8ef5b1b7d3e920e80f85919f70e4ee018864d1ff51708cb1ad500`,
`78a7e94f0a7e70f937f8e0dec6d57a52f2e73458783db7b2bacc739893603ae3`,
`7ee6f42591123533b1c62a788965ed650a6ecf32ae356bb901d301393b5921f3`, and
`607dddb4e1d31a04761880d82d6bfc2d9935973274b0a5560d2adc04d469b7a7`.
Their sorted union matched the frozen manifest exactly. The focused command
`test_bitcoin --run_test=skiplist_tests,net_tests,serialize_tests
--log_level=test_suite` passed with exit 0 and `*** No errors detected`; its
log SHA-256 is
`bf27f74cebaf324cafa6037cb4f7389d0e8eb5c8ab175df57c9c4322bdb85dc3`.
`git diff --check` passed and no fuzz, sanitizer, mutation, or test process
remains running.

### Existing findings and follow-up policy

The ledger remains rated against current master and actual Bitcoin Core
callers: raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban integrity are Low or nice-to-have. Latent ecmult scratch wrapping,
forced 10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention are
reachability-limited correctness/hygiene concerns. Earlier addrman,
coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
headers-sync, UTXO snapshot, mempool persistence, package, handshake,
BufferedFile, block-index, compact-block, Merkle, wire, snapshotmetadata,
Bloom, compressed-amount, scalar/field/group, DER, EllSwift, wallet, PSBT,
scriptpubkeyman, and related audits found no additional clean-master
production bug unless their notes say otherwise.

No cryptographic nonce is involved here. A nonce or version-like field with no
cryptographic meaning is not a Critical clearing issue merely because it is
not cleared. Every later fix, minor fix, or cherry-pick that can mask a
follow-up result must amend the same commit and notes with the exact target,
caller, corpus or mutation, assertion, failure status, Core severity,
verifiers, and whether it preserves, changes, or masks clean-master behavior.
A potential fix is not proof that master was vulnerable; a production-bug
claim requires clean-master or minimal-production-mutation reproduction plus
the strongest deterministic proof available.

## Bloom filter deserialization oracle audit (2026-07-21)

Source commit: `fff7127e04` (`fuzz: strengthen bloom filter deserialization
oracle`), parent `24ffd8be60`, with source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit base remains `18c05d93016b28a9afd4c716dfe00b6e0accb30b`, identical for
`origin/master` and `remotes/l0rinc/master`. The target-scoped comparison over
the Bloom implementation, `FILTERLOAD` caller, target, stateful Bloom fuzzer,
and Bloom tests found no l0rinc commits after that base. No fork commit was
cherry-picked for this target.

### Target contract and Core boundary

`bloomfilter_deserialize` previously deserialized a `CBloomFilter` and only
checked that its serialization was non-empty. It now uses the shared
`DeserializeAndAssertCanonicalPrefix` helper: the canonical serialization must
equal the bytes consumed from the input prefix, while unrelated trailing bytes
remain allowed. The oracle does not assert `IsWithinSizeConstraints()`, because
Core intentionally deserializes malformed filters before classifying and
punishing them.

Core deserializes inbound `FILTERLOAD` at
`src/net_processing.cpp:5069-5076`, checks size and hash-function limits at
`src/net_processing.cpp:5078-5082`, and only then installs the filter for
transaction relay at `src/net_processing.cpp:5084-5089`. The filter is later
used by `IsRelevantAndUpdate` at lines 6194 and 6251. The master-relative
severity is **Informational/Low parser hardening**, not Critical. A malformed
filter can be rejected or influence relay, privacy, bandwidth, and CPU, but
this parser contract cannot cause invalid block acceptance, fund loss,
consensus failure, memory corruption, or cryptographic compromise. A modeled
post-read bit-vector loss could make a filter appear empty and match all
transactions, which is relay amplification rather than a consensus or key
security failure. No cryptographic nonce is involved; a non-cryptographic
nonce would not need clearing.

### Corpus and baseline

The frozen corpus was copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/bloomfilter_deserialize` to
`/tmp/bitcoin-bloomfilter-20260721-clean/frozen/bloomfilter_deserialize`:
43 files, 191,912 bytes, minimum/maximum 1/65,550 bytes. The sorted filename
manifest SHA-256 is
`fbd7c5d36e8921b558ceed12e48b1388711aad79e69ad3e2b2c0c1e33935d6db`; the
filename/size manifest SHA-256 is
`b8da57e2d8eb214bf20129cd19a6bc67c72a9380b9eef1c91211eee2771a25f4`.

Before the oracle, the normal binary SHA-256 was
`01fe97fdbe3fed0ed8e5967d5907aadb7e882768986179547e4152e38c83b305`. Its
44-unit replay reached coverage/features 103/155, peaked at 59 MB, and has
log SHA-256
`2fc7fae5115833090a7114906f9deea32a73da576df39c74945b3fd191b77ad2`.
The ASan/UBSan binary SHA-256 was
`978ab814ab5c2fbb0024db912dedf123191780b4b607235ed0715fd54d349133`. Its
44-unit replay reached 135/254, peaked at 117 MB, and has log SHA-256
`0dca3597e9bf96f3731ead8de89c53b91bbd3d4589a5bae064463f8f804b2015`.

### Mutation and matched control

The temporary production mutation appended
`if (ser_action.ForRead()) const_cast<CBloomFilter&>(obj).vData.clear()` after
the `CBloomFilter` `READWRITE` in `src/common/bloom.h`. It models successful
filter-byte consumption followed by loss of the decoded bit vector. The
enhanced corpus run reached `deserialize.cpp:133` on the consumed-prefix
assertion. Its mutation log SHA-256 is
`da6b808db9366adc3b4dac32329162086b08949c94865bdbeff893894ad8c976`; the
abort/symbolizer path was stopped after preserving the assertion output.

The exact witness is
`00e1f9f17eec4235675c8dda3fac47de7e8acb7d`, 65,550 bytes, SHA-256
`31d05e2a5b528e3882493f9419ecde076b7e9c201c541423f4f83664be0d0c6b`.
The direct mutated normal log SHA-256 is
`0e450f70530c8a49e4bb686d875a8fbb54abbfb805d162f9350ed014b8213989`; the
direct mutated ASan/UBSan log SHA-256 is
`b3d73b87dd90ad2c80de30b09196dfc9ae6fea4760acfd0b71aa70b814230f1a`.
Both fail at the canonical-prefix assertion. The exact witness is the
strongest reproducible proof, but it demonstrates oracle sensitivity rather
than a clean-master production vulnerability.

A temporary `bloomfilter_deserialize_control` target retained the old
`DeserializeFromFuzzingInput` behavior under the same mutation. The exact
witness exited 0 in normal and ASan/UBSan modes; control log SHA-256 values are
`664cd73bd02915575119d7d0882960a110233a6a6ff538f5aa9e37662868443b` and
`d8342deaa6d3786c975b21dd5af03cea8976fbbbdde1aec0e07a109cf1d46056`.
The mutation and control target were removed before the source commit. No
production behavior changed and no clean-master bug is claimed.

### Restored replay and tests

The final normal fuzz binary SHA-256 is
`430357fd8d624af0a14a46bffb2a7ac584d953a00272450b53e049994437e374`. The
restored replay executed 44 units, reached coverage/features 103/156, peaked
at 60 MB, and has log SHA-256
`eed1abbc97ee54de451e73b1bebf7f0f7c4faf20a5238c9176c5e7491ec9bb04`.
The final ASan/UBSan binary SHA-256 is
`84a953ece18374acf131572874fef74ed0f739b36cd050691d84ca2722d59f11`. Its
restored replay executed 44 units, reached 138/257, peaked at 117 MB, and has
log SHA-256
`1f7a3343ab22cefc79dacd1f9c596864642190691563d543d6c546921022bc34`.

Four disjoint ASan/UBSan workers covered 11/11/11/10 files and executed
12/12/12/11 units. Worker log SHA-256 values are
`1da755a86dd0b79ca6f3670e68cd7a7ec645b7150c3c360e9de3e97df336df9b`,
`edb012385c884e8ee2db060833e865dc8edb16c0611185732cd98bae705ee3ad`,
`9ce9ecaeca6d6fe57f9a6d0bd33c2e65ebebd77759a60104e5f8a797e4a76b26`, and
`428617a63c98aa1590163175aba799c231ec8b28043c13cc95d91127aed8b8ab`.
Their filename union matched the frozen manifest SHA-256
`fbd7c5d36e8921b558ceed12e48b1388711aad79e69ad3e2b2c0c1e33935d6db`.

The focused `bloom_tests,net_processing_tests` command passed all 13 cases
with exit 0; log SHA-256
`8c163d62fd0f3b405391e091cfd97dd26375b85109acef0451440467b60d8cec`.
No production mutation, control target, artifact, or sanitizer marker
remained after restoration.

### Existing findings and follow-up policy

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening.

Earlier BIP324, EllSwift, key, scriptpubkeyman, wallet, PSBT, tx_pool,
block-index, snapshot metadata, scalar/field/group, DER, merkle, compact-block,
and related audits found no additional clean-master production bug. Latent
ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention concerns remain reachability-limited.

Every future production-bug claim requires clean-master reproduction or a
minimal production-code mutation modeling the exact broken condition, plus
the strongest deterministic proof available. If a later l0rinc cherry-pick,
caller-side fix, or minor fix changes a follow-up result, amend that same
commit and this note with the exact target, corpus or mutation,
assertion/failure, Core caller, severity, verifiers, and whether it masks,
preserves, or changes clean-master behavior. `git diff --check` passed, and no
fuzz, sanitizer, mutation, or test process remains running.

## Prefilled transaction deserialization oracle audit (2026-07-21)

Source commit: `24ffd8be60` (`fuzz: strengthen prefilled transaction
deserialization oracle`), parent `d2c982f090`, with source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` were identical. The target-scoped l0rinc history
after that base was empty across `blockencodings`, compact-block networking,
the target, and its focused tests, so no fork commit was cherry-picked and no
fork behavior was available to mask this result.

### Target contract and Core reachability

`prefilled_transaction_deserialize` previously deserialized one
`PrefilledTransaction` and discarded it after a weak non-empty-serialization
check. The target now retains the reader, requires the canonical serialization
to equal the consumed input prefix, allows unrelated trailing bytes, requires
the decoded `CTransactionRef` pointer to be non-null, and round-trips both the
index and transaction through canonical serialization. It intentionally does
not require `!CTransaction::IsNull()`, a valid transaction, an ordered
prefilled sequence, a block-position bound, or consensus validity. Those are
later compact-block validity decisions, not standalone wire-parser facts.

Core accepts inbound `CMPCTBLOCK` only after the normal capability gates and
deserializes `CBlockHeaderAndShortTxIDs` at
`src/net_processing.cpp:4611-4632`. Its prefilled transactions reach
`PartiallyDownloadedBlock::InitData` at
`src/net_processing.cpp:4752-4757` and the optimistic path at
`src/net_processing.cpp:4803-4812`. `InitData` dereferences each transaction
at `src/blockencodings.cpp:74` before checking `CTransaction::IsNull()` and
the index-placement limits. Invalid compact blocks are rejected and can
affect peer misbehavior accounting or trigger fallback downloads; successful
reconstruction still passes through normal block validation before processing.

The master-relative severity is therefore **Informational/Low compact-block
parser hardening**, not Critical. A malformed peer compact block can cause
rejection, availability/CPU/bandwidth cost, or a fallback `GETDATA`, but this
pointer contract does not permit invalid block acceptance, fund loss,
consensus failure, memory corruption, or cryptographic compromise. No
nonce-clearing issue is involved; a nonce without cryptographic meaning does
not require clearing.

### Corpus and pre-change baseline

The frozen corpus was copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/prefilled_transaction_deserialize` to
`/tmp/bitcoin-prefilled-20260721-clean/frozen/prefilled_transaction_deserialize`:
179 files, 17,008,882 bytes, minimum/maximum 1/938,744 bytes. The sorted
filename manifest SHA-256 is
`ad0871fec2d2578779f229c9d20b3b4959ef677facf48a27027f01d93c722cfb`; the
filename/size manifest SHA-256 is
`f0d8aec186227141b9ab6d9ae6830612b6b470b6360199748afd9b608019907a`.

Before this oracle, the normal binary SHA-256 was
`85f4eb89ebb4a5cdc302c030e181fdaf11cf3bfd484c250bbc426809f89305a`. Its
180-unit replay reached coverage/features 328/1309, peaked at 120 MB, and
has log SHA-256
`3b417aa6776341c165a1c0c82d48f4c91dd3f14b78df89c06c42f3cae55027d8`.
The ASan/UBSan binary SHA-256 was
`5b781c9a0d25528b0673ef0b911b93f5d7b73acdc6f4acb781a87b8ef7f9f351`; its
180-unit replay reached 483/2088, peaked at 429 MB, and has log SHA-256
`fbd7dc8c5e18f77f97964d6d7d4d841293614ec6e957130502a65f8a069ec018`.

### Mutation and matched control

The temporary production mutation appended
`if (ser_action.ForRead()) const_cast<PrefilledTransaction&>(obj).tx.reset()`
after the `PrefilledTransaction` `READWRITE` in `src/blockencodings.h`. It
models successful wire consumption followed by loss of the transaction
reference that Core immediately dereferences. The enhanced corpus run reached
the new `deserialize.cpp:149` assertion. Its mutation log SHA-256 is
`d1d8af7aebe583ca5aeb1a803a787b10f3e4018cd8a189ecc3c7a81fd57136ff`; the
abort/symbolizer path was stopped after preserving the assertion output.

The exact witness is
`02555ce760140d8e8a39922b9e241a4bed445aef`, 142,706 bytes, SHA-256
`f7e0386147d0ab4e0bf69d005f73306ca85d76dc83857abd4d949d1ab02e6965`.
The direct mutated normal log SHA-256 is
`a82aebd07192007031e8fd90f8e93ac9452f3e04a41c12c66c694cb814bcda52`; the
direct mutated ASan/UBSan log SHA-256 is
`f970b98c39df8a9a84e67b19029c690fc6ca14e95ec1856b625dbc82ff22975b`.
Both fail at the new pointer assertion. This direct witness is the strongest
reproducible proof, but it proves oracle sensitivity rather than a clean-master
production vulnerability.

A temporary `prefilled_transaction_deserialize_control` target performed only
the old deserialize-and-discard operation under the same mutation. The exact
witness exited 0 in normal and ASan/UBSan modes; control log SHA-256 values are
`3912772c2b18c9e365191af8f6840a95d5cd7e80baadd2ca725ecd9e72a373ec` and
`72fce750e282e2af040fa1a63a773c6dd7ed190bb9fc4dda6e03ce67af53f4ac`.
The mutation and control target were removed before the source commit. No
production behavior changed, and no deterministic production regression test
was needed because clean master did not fail; the existing compact-block
caller-level suite is the strongest applicable proof.

### Restored replay and integration evidence

The final normal fuzz binary SHA-256 is
`01fe97fdbe3fed0ed8e5967d5907aadb7e882768986179547e4152e38c83b305`. The
restored 179-file replay executed 180 units, reached coverage/features
391/1540, peaked at 123 MB, and has log SHA-256
`03b21863888ee9bc2990954b2d7650187f72b9002a363790aeb965df097a0e87`.
The final ASan/UBSan binary SHA-256 is
`978ab814ab5c2fbb0024db912dedf123191780b4b607235ed0715fd54d349133`.
Its restored replay executed 180 units, reached 560/2344, peaked at 449 MB,
and has log SHA-256
`12971efef96d67fb4747bf0c2f573f5be87bfab940a39d310e7d0461b14e17bc`.

Four disjoint ASan/UBSan workers covered 45/45/45/44 files and executed
46/46/46/45 units. Worker log SHA-256 values are
`b9a9d18413e81744433e79bfb489515b07394a1bec6a327d25c337e6394fe101`,
`0704f740563cbe71b8b5b02854235f64b6ec8a174a57e131a059f0648594ee22`,
`68779c267a4ac7aa8750ece326d8c3369c0686d085c37adf46d30579727c91ee`, and
`a62109cc5d2416b342755849f453ebb27e3813f22a84b1a10d70c77cc848f60`.
Their filename union matched the frozen manifest SHA-256
`ad0871fec2d2578779f229c9d20b3b4959ef677facf48a27027f01d93c722cfb`.

The related `block_header_and_short_txids_deserialize` regression corpus had
220 files and 20,132,986 bytes, filename manifest SHA-256
`3d661f940587a5b9c7be61fd594d2132bc2dbdfbf88e9c9ec5c6f4bda8f22e96`, and
filename/size manifest SHA-256
`b39ff61bb7ad8603b035b315048b1d96f5717e404021bf169f0aa6a18a3814ce`.
Normal replay executed 221 units at coverage/features 381/2077 and 119 MB,
log SHA-256
`cb9b23ebada779cff1b7b2551f3a048bfec9acf3ed372fe6398b30622fa8f1dc`.
ASan/UBSan executed 221 units at 582/3300 and 438 MB, log SHA-256
`67352c3ce7feafaa400126ba2141551cc9aeac8ed00d330df11930219ddf0886`.

The existing `cmpctblock` corpus had 1,435 files and 3,705,961 bytes,
filename manifest SHA-256
`0b79ef0e1bd7824dd6a8d4edae2a6225e7422eca0b4ff32edeb2df6eb42e7e00`, and
filename/size manifest SHA-256
`f7b3581ad44bd048cf2c0670b0059b53ec36634049acd1680668b20da35beb0b`.
Normal replay executed 1,436 units at coverage/features 9695/30532 and
103 MB, log SHA-256
`02f413d19b525316495bd2a0303338a80ff2a387c3c310bddbc8e358bb00bbb4`.
ASan/UBSan executed 1,437 units at 20119/67429 and 514 MB, log SHA-256
`1fd3aff19171e684ed9a813b109544e208095448dc56b0c9a9afc3e93c79e92f`.
The focused `blockencodings_tests,net_processing_tests` command passed all 9
cases with exit 0; log SHA-256
`0b7eafd710a4ab38119164698cfde96f0f0592a90cd614f5c7f96fd911d56424`.
No production mutation, control target, artifact, or sanitizer marker remained
after restoration.

### Existing findings and follow-up policy

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local RPC
correctness; feature-conditional private-broadcast failed-send retention and
empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening.

Earlier BIP324, EllSwift, key, scriptpubkeyman, wallet, PSBT, tx_pool,
block-index, snapshot metadata, scalar/field/group, DER, merkle, and related
audits found no additional clean-master production bug. Latent ecmult scratch
wrapping, 10x26 magnitude normalization, and SHA/HMAC/RFC6979 retention
concerns remain reachability-limited.

Every future production-bug claim requires clean-master reproduction or a
minimal production-code mutation modeling the exact broken condition, plus
the strongest deterministic proof available. If a later l0rinc cherry-pick,
caller-side fix, or minor fix changes a follow-up result, amend that same
commit and this note with the exact target, corpus or mutation,
assertion/failure, Core caller, severity, verifiers, and whether it masks,
preserves, or changes clean-master behavior. `git diff --check` passed, and no
fuzz, sanitizer, mutation, or test process remains running.

## GETBLOCKTXN request deserialization oracle audit (2026-07-21)

Source commit: `d2c982f090` (`fuzz: strengthen GETBLOCKTXN request oracle`),
parent `867d2568e5`, with source branch `codex/fuzz-oracles-current` in
`/tmp/bitcoin-secp256k1-audit-current`. The audit base was
`18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
`remotes/l0rinc/master` were identical, and the target-scoped l0rinc history
after that base was empty. No fork commit was cherry-picked for this target.

### Target, contract, and Core boundary

`blocktransactionsrequest_deserialize` previously deserialized a
`BlockTransactionsRequest` and returned. The separate `difference_formatter`
fuzzer already checked that differential decoding produces strictly
increasing indexes, but it did not check exact input consumption in this
peer-message deserializer. The target now uses the shared
`DeserializeAndAssertCanonicalPrefix` helper and a matching strict-index
postcondition. Trailing bytes remain allowed; no block existence, transaction
count, or consensus-validity assumption is added to the parser oracle.

Bitcoin Core deserializes inbound `GETBLOCKTXN` at
`src/net_processing.cpp:4391-4397`. It checks each requested index against the
selected block at `src/net_processing.cpp:2646-2655` before producing a
`BLOCKTXN` response. This request is not sent to consensus validation. A
malformed peer request can be rejected or cause peer misbehavior accounting,
but this audit found no clean-master state corruption, invalid block
acceptance, fund loss, memory safety failure, or cryptographic consequence.
The master-relative rating is therefore **Informational/Low request-parser
hardening**, not Critical. No nonce-clearing issue is involved in this
request contract.

### Frozen corpus and baseline

The frozen corpus is
`/tmp/bitcoin-getblocktxn-20260721-clean/frozen/blocktransactionsrequest_deserialize`,
copied byte-for-byte from
`/mnt/my_storage/qa-assets/fuzz_corpora/blocktransactionsrequest_deserialize`:
64 files, 221,802 bytes, minimum/maximum 1/65,573 bytes. Its sorted filename
manifest SHA-256 is
`6cbf0c88e032c0effb89ce5159c6817a9c6e8a9b26861f3bb674e848b1da0959`; its
filename/size manifest SHA-256 is
`61903ef5c17fc88e6bc1110a91c48afdc0a5186615dfc1dd6fb79566a620b5e3`.

Before the oracle, normal and ASan/UBSan replays each executed 65 units,
reached coverage/features 110/334 and 156/563, and peaked at 56/109 MB.
Their log SHA-256 values are
`b756fbcf7f2ea2e1eeeda732dc77d02d6e959f24a176667989918004c6152552` and
`6604bd6ab6c74f8f7aa5324bf8fbcbd89fb1d9a7862035acf9989569e19f12ee`.
No baseline artifact or sanitizer marker was emitted.

### Differential mutation proof

The temporary production mutation added
`if (ser_action.ForRead()) const_cast<BlockTransactionsRequest&>(obj).indexes.clear()`
after the request `READWRITE` in `src/blockencodings.h`. It models a
read-side loss of decoded request indexes after successful consumption. The
new prefix assertion failed at `src/test/fuzz/deserialize.cpp:133` on exact
witness `02f3df88448d71b0a90e399e3b83a168155a5c60`, 261 bytes, SHA-256
`f1f21a5cb8fd529e513572e83d4bd8f8713986f28b429365fb762d2f341eafe1`.
The direct one-input assertion log SHA-256 is
`73a6127ccf56c5c023deaa3aff64f080ea2ab44bba2ae5436a904a3682bd99bf`; the
corpus attempt log SHA-256 is
`925f1fde412b52db3bff436e8997110621610abed2bcefc87139a8fe51b4cabf`.
The abort/symbolizer path was stopped after preserving the assertion output;
the direct one-input witness is the strongest reproducible proof. The
mutation was removed before the source commit and no production behavior was
changed.

### Restored replay, workers, and regressions

The final restored fuzz binary SHA-256 values are
`85f4eb89ebb4a5cdc302c030e181fdaf11cf3bfd484c250bbc426809f89305a1` for
normal and
`5b781c9a0d25528b0673ef0b911b93f5d7b73acdc6f4acb781a87b8ef7f9f351` for
ASan/UBSan. Restored normal and ASan/UBSan replays each executed 65 units,
reached coverage/features 112/342 and 164/605, and peaked at 55/109 MB.
Their log SHA-256 values are
`b7e26957581dc853649cd3da2c9b7869023e5bf61143b80208e4a3fe2f0d7640` and
`742cae2677b0ebc5d49866a736080f00e052e345a17d9ab434d6dbde9a7d40ce`.
No sanitizer marker or artifact was emitted.

Four disjoint ASan/UBSan workers covered all 64 frozen files exactly once:

    worker0: 16 files, 17 executions, coverage/features 158/462, peak 106 MB, log bca92656fc1f5e0ac3d3817239cece227196370506864f167e26e8ed9000e4d1
    worker1: 16 files, 17 executions, coverage/features 156/448, peak 106 MB, log 5f59bdbfef5ade328192100c3870166d17e6a5c8c9408e712b391ef2e5e12c1d
    worker2: 16 files, 17 executions, coverage/features 148/478, peak 105 MB, log 2601f5be9edb39141dd2ad0e8c581d155ec67c8d83cca39a4e158b31abdf57ca
    worker3: 16 files, 17 executions, coverage/features 154/455, peak 106 MB, log ed1d3b29cdf5efbd825131a95655156f07201e3fff20fcabcb7874d20159ae4f

The worker filename union SHA-256 is
`6cbf0c88e032c0effb89ce5159c6817a9c6e8a9b26861f3bb674e848b1da0959`, exactly
matching the frozen manifest. No worker artifact or sanitizer marker was
emitted.

The existing `difference_formatter` regression corpus executed 57 units in
normal and ASan/UBSan modes, with coverage/features 98/194 and 155/375, peak
RSS 54/103 MB, and log SHA-256 values
`7f78913be42a9a1639293eda65ad024a73e0c28b290bea536b118539a3de2081` and
`cf55ad6c8602c0904b2877c6289cc24a4fd1cd16f4895345e7cd0e4278c4f5a3`.
The focused `blockencodings_tests,net_processing_tests` command passed all
9 cases with exit 0; log SHA-256
`3da34b953f4a402dd44157e722146ec0829c1af452dd62766314414e50532352`.

### Findings carried forward and follow-up policy

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local
RPC correctness; feature-conditional private-broadcast failed-send retention
and empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, snapshot metadata, scalar/field/group,
DER, merkle, and related audits found no additional clean-master production
bug. Reachability-limited ecmult scratch wrapping, 10x26 magnitude
normalization, and SHA/HMAC/RFC6979 retention remain non-findings.

The mutation proof demonstrates that the oracle catches a modeled request
state loss; it is not evidence of a clean-master vulnerability. Every future
production-bug claim still requires clean-master reproduction or a minimal
production-code mutation modeling the exact broken condition, plus the
strongest deterministic proof available. If a later cherry-pick, caller-side
change, or minor fix can mask a follow-up failure, amend that same commit and
this note, or merge the changes, and state whether it masks, preserves, or
changes master behavior. No fuzz, sanitizer, mutation, or test process
remains running.

## Block and compact-block wire deserialization oracle audit (2026-07-21)

Source commit: `867d2568e5` (`fuzz: strengthen block wire deserialization
oracles`), parent `c759644fc4`, with source branch
`codex/fuzz-oracles-current` in `/tmp/bitcoin-secp256k1-audit-current`. The
audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; both
`origin/master` and `remotes/l0rinc/master` pointed there. The target-scoped
l0rinc history after that base, over `src/test/fuzz/deserialize.cpp`,
`src/primitives/block.h`, `src/blockencodings.h`, and
`src/net_processing.cpp`, was empty. No fork commit was cherry-picked and no
later behavior was allowed to mask the master result.

### Target selection and exact contract

The selected targets were `block_deserialize`,
`block_header_and_short_txids_deserialize`, and `blockheader_deserialize`.
Before this commit, the first and third targets only deserialized and
returned, while the compact-block-header target had no consumed-prefix
postcondition. The existing `block` and `cmpctblock` fuzzers exercise
constructed blocks and higher-level state transitions; they did not provide
an exact byte-consumption oracle for these generic deserializer targets.

`DeserializeAndAssertCanonicalPrefix` now accepts serialization wrappers such
as `TX_WITH_WITNESS`. For every accepted object it requires the serialized
object to equal the exact bytes consumed by `SpanReader`, requires the reader
remainder to equal the input remainder, and permits trailing bytes. This is a
parser/state-preservation oracle, not an assumption that arbitrary accepted
input is a valid consensus block. The compact-block nonce is serialized and
feeds the SipHash short-ID selector; this audit does not invent a nonce
clearing invariant or classify nonce retention as a bug.

### Bitcoin Core boundary and severity

The relevant inbound paths are real peer-facing boundaries:

* `BLOCK` is deserialized at `src/net_processing.cpp:4940`, then reaches
  `ProcessBlock` at `src/net_processing.cpp:4974`, which calls
  `ProcessNewBlock`.
* `CMPCTBLOCK` is deserialized at `src/net_processing.cpp:4632`, its header
  is passed through `ProcessNewBlockHeaders`, and its reconstructed block can
  reach `ProcessCompactBlockTxns` and `ProcessBlock`.

Thus invalid peer bytes can reach these parsers, but this audit found no
clean-master state corruption, consensus bypass, memory-safety failure,
fund-loss path, or cryptographic failure. The master-relative result is
**Informational/Low oracle hardening**, not a Critical production finding.
A real production parser corruption at this boundary would need a separate
downstream-impact proof and would be rated from the actual Core consequence;
the temporary mutations below prove oracle sensitivity only and are not bug
claims. Existing validation and compact-block tests remain the authority for
semantic acceptance after parsing.

### Frozen corpora and clean baseline

The frozen `block_deserialize` corpus is
`/tmp/bitcoin-block-wire-20260721-clean/frozen/block_deserialize`, copied
byte-for-byte from `/mnt/my_storage/qa-assets/fuzz_corpora/block_deserialize`:
209 files, 24,696,537 bytes, minimum/maximum 1/1,009,971 bytes. Its sorted
filename manifest SHA-256 is
`dfaf477f47f6ff1fa1f8fec3087b844f24a4df372efb2167525990347e765a32`; its
filename/size manifest SHA-256 is
`1a32e0d5280a154de5629c2d78e3311cc39c543d601f46afc36767ba5818df97`.

The frozen `block_header_and_short_txids_deserialize` corpus is
`/tmp/bitcoin-block-wire-20260721-clean/frozen/block_header_and_short_txids_deserialize`,
copied from `/mnt/my_storage/qa-assets/fuzz_corpora/block_header_and_short_txids_deserialize`:
220 files, 20,132,986 bytes, minimum/maximum 1/938,267 bytes. Its sorted
filename manifest SHA-256 is
`3d661f940587a5b9c7be61fd594d2132bc2dbdfbf88e9c9ec5c6f4bda8f22e96`; its
filename/size manifest SHA-256 is
`b39ff61bb7ad8603b035b315048b1d96f5717e404021bf169f0aa6a18a3814ce`.

The frozen `blockheader_deserialize` corpus is
`/tmp/bitcoin-block-wire-20260721-clean/frozen/blockheader_deserialize`,
copied from `/mnt/my_storage/qa-assets/fuzz_corpora/blockheader_deserialize`:
10 files, 378 bytes, minimum/maximum 1/80 bytes. Its sorted filename
manifest SHA-256 is
`ff2095d52c4efe0677b44d5c48a49c731453c440b5cd804c3d899fb2fc188200`; its
filename/size manifest SHA-256 is
`f50b26883aedf32ded8759aa63afe126b93b21e21402fdcabe936ab552ca1d86`.

Before the oracle, the normal replays executed 210/221/11 units with
coverage/features 340/1957, 381/2076, and 71/79, peak RSS 120/120/54 MB,
and log SHA-256 values
`7e33c027a709f9e7f8b48e29f8cf592e9faa2b7dffc0b460aae08d394332e41f`,
`e0dd7709f632c7709e99d7db047d30b199cceeae8247189b634e38056c6e480b`, and
`da8ab52cc09dfc90a908d37dd3c65edebfb6d7af7fdea78c5555a8a240a179cf`.
The baseline ASan/UBSan replays executed the same units with
coverage/features 513/3039, 579/3296, and 112/126, peak RSS 437/438/104 MB,
and log SHA-256 values
`ed2cbb5b1eba2bb661352a224eeb4e202db6875a87d99e53207a84c7cf8b6fea`,
`8c89f5814e0443210dd89321e2775c78d732320d0332d2af1e372383db1a9d83`, and
`d4a9ce6c4d911912bb472ddb29dbf136f9990f4b183394d9e404caf74d3483c6`.

### Differential mutation proof

The first temporary production mutation added
`if (ser_action.ForRead()) const_cast<CBlock&>(obj).vtx.clear()` after the
`CBlock` `READWRITE` in `src/primitives/block.h`. It models a read-side
transaction-vector loss after successful wire consumption. The exact
`block_deserialize` witness was
`00f78f6736f84229e803b92a957f455eee5e47dd`, 433 bytes, SHA-256
`a1a5659f92ea64a981688894b92b5f2066265d619acf633b67adc03341e2967e`; the
new prefix assertion failed at `src/test/fuzz/deserialize.cpp:133`. The
one-input exact log SHA-256 is
`ce488e5d39755955b5a7acfb4cb2be79061596f8212985002ceb671fb7e1206e`, and
the corpus attempt log SHA-256 is
`1e4dd54dc174bfcc4e8e706c8cfc875f1e3612191018c4772c8bd0148dfe8320`.

The second temporary production mutation added
`if (ser_action.ForRead()) const_cast<CBlockHeader&>(obj).nNonce = 0` after
the `CBlockHeader` `READWRITE`. It models silent header-field clobbering and
was independently sensitive in both header targets. The exact compact-header
witness was
`005f389ce9161e04c1a2e439fe7c93d442724c57`, 116 bytes, SHA-256
`239b02b0f1b94df5c5e06af4b43fb6a2598ef66a250a720d801a921dc79b6969`; its
assertion failed at `deserialize.cpp:134`, with exact log SHA-256
`02b01b2241c434e49b02da5732657ebaaaa648afa6964b1d96a8828a906d92c5` and
corpus attempt log SHA-256
`a404544cab5efe2d55f34befdf51245cc57b126e800d004cc95e5aaa0de60f56`.
The exact standalone-header witness was
`0932c584447d2324ee4a219c62985f317e67d91a`, 80 bytes, SHA-256
`b5d7415c3014d05518705d454d16d95962bced66c49837ed2cfbb123460531b3`; its
exact log SHA-256 is
`2f914de101cb01b4944440ef98a8e55e635109d0bca870411fdf148335681eef`, and
the corpus attempt log SHA-256 is
`0f40c66a50c9605469a8d028bcc9efa5a912c223aab2067c6059483516f0aa8c`.
The abort/symbolizer path was stopped after preserving each assertion log;
the direct one-input witnesses are the strongest reproducible proof. Both
mutations were removed before rebuilding and committing; no temporary
production behavior remains.

### Restored replay and worker evidence

The final restored fuzz binary SHA-256 values are
`59d20f9e39e6823a390c1600ae59c0ec3efd1727b82841d494203b80a1ed08a6` for
normal and
`5c64c527f7545c31bf33e0cecf5bae65979437e4db7f9e34e5135a4025e71b01` for
ASan/UBSan. The restored normal replays executed 210/221/11 units with
coverage/features 340/1957, 381/2077, and 71/79, peak RSS 120/119/55 MB;
log SHA-256 values are
`1ef2313159e58c97cc4f572e84effa7e22d626ee499ee9c04800ed98c1cf1010`,
`d8cd4c6dba69d77060ccc51fbdf71e9f0e3af86a4b160bb995890351fc7fe8d8`, and
`94e78176064cec0b89db0c5a5c1225c472b66c5cc632a676d63fbbae7343ca95`.
The restored ASan/UBSan replays executed the same units with
coverage/features 514/3041, 582/3300, and 115/129, peak RSS 437/439/104 MB,
and no sanitizer or artifact fault; log SHA-256 values are
`fa00fe1f371fd80ed926bf0d53ad130d6651fb549f43a7331cc4dc7ff107f07f`,
`ed1d071d7cf5ec574797f8823603b5038254f6f4531b0651d6360625189876f1`, and
`100cbb14ee920b1adc6b20f415243a55c5178e1f902d530925a1d44a6bfcf6a4`.

Four disjoint ASan/UBSan workers were run for each target. Every worker
completed without sanitizer markers or artifacts:

    block_deserialize worker0: 53 files, 54 executions, coverage/features 511/2806, peak 264 MB, log 5675085c436dd4bf4150c53f9846dee5f49c8542692fc9acb92370938378ba00
    block_deserialize worker1: 52 files, 53 executions, coverage/features 496/2644, peak 295 MB, log 27fc269413f9fbec2358629211ff0d427d0f4154362fab45bca2f1d21b957a50
    block_deserialize worker2: 52 files, 53 executions, coverage/features 492/2760, peak 192 MB, log 8213301d0b4fb7b3edfabae8b2276cb8086cac61b2e1078b3217d89e96f831d5
    block_deserialize worker3: 52 files, 53 executions, coverage/features 497/2717, peak 289 MB, log ffea85ff78e809d0a4e4cb258a83423fe00ff10cef529d5ac2c699a26da24d08
    block_header_and_short_txids_deserialize worker0: 55 files, 56 executions, coverage/features 536/3017, peak 137 MB, log e8225e443a71a8fabe550fe663fad3132b0aba657180e2fdf2e4f95225cef6ab
    block_header_and_short_txids_deserialize worker1: 55 files, 56 executions, coverage/features 565/3006, peak 239 MB, log 704a8a4640ce24a74a7f22be027259e6e31ee62512062256ae605827e72ca331
    block_header_and_short_txids_deserialize worker2: 55 files, 56 executions, coverage/features 567/2940, peak 362 MB, log 587f3f4b7c13e603d4775e4205e1a6ce6c6be885ebe913c7b76e0d6be21c33ac
    block_header_and_short_txids_deserialize worker3: 55 files, 56 executions, coverage/features 541/2969, peak 152 MB, log 42f4a900eca239e19b310d7fb64b07f7049e914fb621eca447a1f73f5f77441e
    blockheader_deserialize worker0: 3 files, 4 executions, coverage/features 115/122, peak 104 MB, log 597a95941d55404033b8c92132497d73ff4396c62339246fb5aa5dd09281d34a
    blockheader_deserialize worker1: 3 files, 4 executions, coverage/features 60/65, peak 103 MB, log 83d5e9520c59ec0d6d222b7545ec22f4de2e395f8e23deeb721d0163487cce87
    blockheader_deserialize worker2: 2 files, 3 executions, coverage/features 115/121, peak 103 MB, log 927804eaba2b1f99561a545ce5a0a6471d89604fb5dfc352f4608d08131cf31d
    blockheader_deserialize worker3: 2 files, 3 executions, coverage/features 58/60, peak 103 MB, log 9e9bbb7b34adc07731c2bb07a4d5dfe51469e22ada0a0fc562929ff2d5aca783

The worker filename unions exactly match the frozen manifests: union SHA-256
`dfaf477f47f6ff1fa1f8fec3087b844f24a4df372efb2167525990347e765a32` for
`block_deserialize`,
`3d661f940587a5b9c7be61fd594d2132bc2dbdfbf88e9c9ec5c6f4bda8f22e96` for
`block_header_and_short_txids_deserialize`, and
`ff2095d52c4efe0677b44d5c48a49c731453c440b5cd804c3d899fb2fc188200` for
`blockheader_deserialize`.

The focused command
`test_bitcoin --run_test=blockencodings_tests,merkle_tests,merkleblock_tests,validation_block_tests,validation_tests,serialize_tests,net_processing_tests --log_level=test_suite`
passed all 41 selected cases with exit 0; log SHA-256 is
`519e230061be9c8804cbce6d50611e55feac768c9a5644398505e252bc8917a8`.
The existing `blocktransactions_deserialize` target was replayed after the
shared helper refactor against a byte-identical 220-file freeze. Its normal
and ASan/UBSan replays each executed 221 units, reached coverage/features
417/2340 and 605/3555, peaked at 154/461 MB, and had log SHA-256 values
`3c20335e61e4d94f0e854bee5b1d0a1465ff5417dd89e53682a604cc4d7d8354` and
`35d46a676719c355c48aaaf8d51225201fbde2cf58a4a4e4a7115f405297ecbf`.

### Findings carried forward and follow-up policy

Existing findings remain rated against clean master and actual Bitcoin Core
callers: generic raw `finalizepsbt` invalid `final_scriptSig` is Low local
RPC correctness; feature-conditional private-broadcast failed-send retention
and empty HEADERS initial-sync availability are Medium; peer activity refresh,
block-storage failure, oversized transport types, and banman invalid-subnet/
unban are Low or hardening. Earlier BIP324, EllSwift, key, scriptpubkeyman,
wallet, PSBT, tx_pool, block-index, snapshot metadata, scalar/field/group,
DER, and related audits found no additional clean-master production bug.
Latent ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention remain reachability-limited. The prior partial
merkle reusable-output bug remains Low because current Core callers allocate
fresh vectors; it is not an invalid-peer-block or Critical finding.

The source mutation proofs above demonstrate that the new oracle catches a
modeled read-side state transition; they do not upgrade an oracle-hardening
result into a production vulnerability. Every future production-bug claim
still requires clean-master reproduction or a minimal production-code
mutation that models the exact broken condition, plus the strongest
deterministic test available. If a later cherry-pick, caller-side pre-clear,
or minor fix changes a follow-up failure, amend that same commit and this
note, or merge the changes, and state whether it masks, preserves, or changes
the master-relative behavior. No fuzz, sanitizer, mutation, or test process
remains running.

## `block_index` database round-trip oracle audit (2026-07-20)

Source commit: `6909f22d74` (`fuzz: strengthen block_index database
round-trip oracle`). Its parent is `4a06bf618d`, and the audit base is Bitcoin
Core `origin/master` `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`remotes/l0rinc/master`. The relevant-path query over
`src/test/fuzz/block_index.cpp`, `src/node/blockstorage.cpp`, `src/chain.h`,
`src/dbwrapper.cpp`, `src/dbwrapper.h`, `src/txdb.cpp`, and `src/txdb.h`
returned no l0rinc commits. No fork commit was cherry-picked, so no later
change masked or altered this clean-master result.

### Core boundary and severity

`FUZZ=block_index` exercises `kernel::BlockTreeDB` file-info, last-file,
reindexing, flag, batch-write, and `LoadBlockIndexGuts` round trips. Bitcoin
Core uses this database while loading the block index during startup and
reindex. It is persisted-database reconstruction, not a direct peer invalid-
block acceptance path.

The old harness forced every generated index to one hash and returned
`blocks.back()` for every loader callback, so it did not verify per-record
identity, predecessor reconstruction, or most serialized metadata. The new
harness uses regtest's easy PoW target, gives each generated header a stable
hash and genesis predecessor, maps each callback hash to a stable object, and
compares every disk-serialized field after load. Production
`LoadBlockIndexGuts` now asserts callback identity before assigning `pprev`.

No clean-master production bug was confirmed. This is Informational/Low oracle
hardening, not High/Critical. Severity follows actual Core startup, reindex,
and block-index callers: a confirmed corrupted-persistence, unsafe block
selection, startup denial of service, or consensus effect would need separate
proof. Invalid transaction or block-like bytes alone do not make a finding
High/Critical. A nonce with no cryptographic meaning is not Critical merely
because it is not cleared.

### Corpus and replay evidence

The final source SHA-256 values are:

    src/test/fuzz/block_index.cpp  c2a1583e8b38614d38491b2ba05551440a11eaec7984960ee3bbbea3f3d441dd
    src/node/blockstorage.cpp      8b4b0b6105f11569d54435a239e2c72502fd5e21be64dc520333e5c72f5dbbbc

The final normal and ASan/UBSan fuzz binary SHA-256 values are
`8d4882c12acb9ba6b3b83a9936d654771507785472f986f85e757231ce87b99a` and
`0dbdb573eadaa380226914d883c43c9162127737f364e6a65e3a1b3fa97dcaa5`.

The frozen corpus is `/tmp/bitcoin-block-index-20260720/frozen`, copied from
the existing `block_index` corpus: 467 files, 454,540 bytes, minimum 1 byte,
maximum 19,926 bytes. The sorted filename manifest SHA-256 is
`914f56dc05de40df08e5994583ab58d949eb672dfba83075b1694fdff67f7a46`; the
filename/size manifest SHA-256 is
`1d197f7a435798b0026f608687dfca9775ec600612838553a0cc9ea3999b95c3`.

The unmodified baseline normal replay ran 468 units, reached coverage 2,009
and 4,735 features, peaked at 62 MB, exited 0 with no artifacts, and has log
SHA-256 `010bbd3f64f05672057fc38861ef893e30f66dd807eb5cc51124b97c88368ef7`.
The baseline ASan/UBSan replay ran 469 units, reached coverage 3,995 and
10,061 features, peaked at 479 MB, exited 0 with no artifacts, and has log
SHA-256 `93b391da8f912d658227890d613afd6c95786158977058aba6f5cae68e7c7855`.

The final normal replay ran 468 units, reached coverage 2,107 and 5,779
features, peaked at 63 MB, exited 0 with no artifacts, and has log SHA-256
`e4cc12cc844ac36541067f03bdce7d3a0c0e86b137b3fb1ba1096df13d4858f0`.
The final ASan/UBSan replay ran 469 units, reached coverage 4,229 and 12,876
features, peaked at 488 MB, exited 0 with no artifacts, and has log SHA-256
`28fee931f9cce963a295f0f01fd7e3361f89d8a067d89519dbc9adf037f9dea8`.

Four independent ASan/UBSan workers replayed disjoint shards of 117, 117,
117, and 116 files. Their sorted union matched the frozen manifest exactly.
They ran 119, 119, 119, and 118 units, all exited 0 with no artifacts, and
peaked at 453, 452, 449, and 457 MB. Worker log SHA-256 values, in order,
were:

    cc892545637b5f19446e9e3fe09a663282002868c564557448c3d13d00e72957
    bf3024c98c9b61b47fb82c227459d3fae656b4222948c7c75558bb8ed368c456
    2826b01b0a642108765e9979595a51e811d82a79e5ad6fa3056b64c28741d154
    8d9fbc57640c5884f65821fe112bbfdb9933d96cf5cdaf76a51c47e4b63b494b

### Differential oracle proof

This is an oracle-sensitivity proof, not a clean-master production finding.
The exact production mutation changed
`pindexNew->nTx = diskindex.nTx` to
`pindexNew->nTx = diskindex.nTx + 1` in `LoadBlockIndexGuts`. It corrupts one
persisted metadata field after deserialization without bypassing the PoW
check.

With `FUZZ=block_index -runs=467 -seed=20260720 -shuffle=0`, the enhanced
target failed after 14 units at `AssertBlockIndexState` on frozen input
`e0df3b4cb360aea89d4b096958035e44e92cdaff` (11 bytes). The witness SHA-256
is `c0b8f05df646c53ae343fd389efe63e0c2d4dd51e55664282205a7e89dd6a4a6` and
the mutation log SHA-256 is
`caad26aa605a22a89dbab1f7b19c54d0155e1ff2850824afcd0f67f117583755`.

The matched control kept the production mutation and removed only the new
`AssertBlockIndexState` call. Replaying that exact witness once exited 0 with
no artifact; control log SHA-256 is
`e6ba7482c3d81eb3cfbbfb4715f74531f92c972ea6bd2120220fcf264ac370ca`.
The old success-only oracle therefore accepts this modeled metadata
regression while the new per-record state oracle rejects it. The mutation was
restored before the source commit. Restored master has no failure, so no
deterministic production regression test is claimed; the exact mutation,
witness, rejection, and matched control are the strongest relevant proof.

### Existing finding ledger and policy

The confirmed generic raw `finalizepsbt` invalid nonempty `final_scriptSig`
behavior remains Low: private RPC correctness/availability, not fund loss,
consensus, memory safety, or cryptographic failure;
`walletprocesspsbt` rechecks through `CWallet::FillPSBT`. Feature-conditional
private-broadcast failed-send retention and empty-HEADERS initial-sync
availability remain Medium. Peer transaction-activity refresh, block-storage
failure, oversized transport types, and banman invalid-subnet/unban integrity
remain Low or hardening findings.

The scalar/field/group, DER, EllSwift, BIP324, key, scriptpubkeyman,
wallet-construction, PSBT, tx_pool, block-index-tree, and other audited
targets found no additional clean-master production bug in their stated
contracts. Latent ecmult scratch wrapping, 10x26 magnitude normalization, and
SHA/HMAC/RFC6979 retention concerns remain reachability-limited, not Critical
Bitcoin Core vulnerabilities.

Any future cherry-pick or fix that changes a follow-up finding must be
recorded in the same commit message and audit notes, stating whether it masks,
preserves, or changes clean-master behavior. Findings are rated against
master and actual Bitcoin Core callers, not an isolated malformed input or a
later accidental patch.

### Verification and test gap

`git diff --check` passed. Normal and ASan/UBSan fuzz builds, baseline and
final full-corpus replays, the four-worker sanitizer replay, exact mutation
rejection, and matched control acceptance all completed. The configured
fuzz-only build did not provide `test_bitcoin`, so no dedicated unit test was
available or claimed. No production behavior changed on master, no
clean-master bug was confirmed, and no fuzz, sanitizer, mutation, or replay
process remained running.

## `tx_pool` mempool state-transition oracle audit (2026-07-20)

Source commit: `4a06bf618d` (`fuzz: strengthen tx_pool state-transition
oracles`). The source parent was `b8a6b4133cfa62640bd057a9eeb2a7cf0bfe0c9e`.
The audit base was Bitcoin Core `origin/master`
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`remotes/l0rinc/master`. The exact path query over
`src/test/fuzz/tx_pool.cpp`, `src/txmempool.cpp`, `src/validation.cpp`,
`src/policy/ephemeral_policy.cpp`, `src/node/txorphanage.cpp`, and
`src/node/txdownloadman_impl.cpp` returned no l0rinc commits. Nothing was
cherry-picked for this target, so no later fork fix masked or changed the
clean-master behavior.

### Core boundary and severity

`FUZZ=tx_pool_standard` drives synthetic mature-coinbase transactions through
`ProcessNewPackage(..., test_accept=true)` and
`AcceptToMemoryPool(..., test_accept=false)`, including RBF, package policy,
fee deltas, TRUC constraints, and callback bookkeeping. `FUZZ=tx_pool` feeds
arbitrary deserialized transactions, including bypass-limits calls, through
the same ATMP boundary. `Finish` also exercises block-template selection,
block removal/re-addition, reorg descendant updates, recursive removal, trim,
and expiry. These are direct Bitcoin Core mempool callers used by peer and RPC
transaction admission and mining/reorg maintenance.

The harness now snapshots entry identity and metadata, fee deltas,
unbroadcast state, total size and fee, sequence, update counter, and load
state. It asserts valid and invalid ATMP transitions, package result optional
fields, callback/state correspondence, and `CTxMemPool::check()` after each
transition and lifecycle operation. `removeUnchecked()` also asserts that
`mapNextTx` agrees with `mapTx` before removal notifications expose the
transition.

No clean-master production bug was confirmed. The result is therefore
Informational/Low oracle hardening, not a High/Critical vulnerability. The
master code passed all controls and the added production assertion did not
change valid behavior. Severity is based on actual Bitcoin Core callers: a
future confirmed issue would need a reachable effect such as incorrect
mempool state, stale or unsafe mining behavior, denial of service, consensus
impact, or memory/concurrency failure. Invalid transaction or block-like bytes
alone do not make a finding High/Critical. A nonce with no cryptographic
meaning is likewise not Critical merely because it is not cleared.

### Source and corpus identity

The final modified source SHA-256 values are:

    src/test/fuzz/tx_pool.cpp  d16322806b861ee519f2eb61a7bdab9e363bd130e05c2918d99efcd38ac9e8f6
    src/txmempool.cpp          83e31f5f337ec700ed9af55fd36b796f3509e72cc545417c0774ac960c483ae8

The normal and ASan/UBSan fuzz binary SHA-256 values are
`0b46940956ef0554a04eabec6ff22be80c390183e0c72a387c908a85a0a838e4` and
`c1c274763d908bb32f392ef455488f511d26d08a6886645b8de8915b0161ab55`.

The frozen corpus copy was
`/tmp/bitcoin-tx-pool-20260720/frozen`: 5,658 files and 41,820,925 bytes,
with minimum size 1 byte and maximum size 840,495 bytes. The sorted filename
manifest SHA-256 is
`fe5326865d680ed6b8c6490092c21e9f679dfe9c52c67699439342d462ffa637`; the
filename/size manifest SHA-256 is
`1ec994bb592175d4ce6e56ea342d31264611068948afb0c6fd53ac60a7cd6931`.
All authoritative runs used this frozen copy or hard-linked disjoint copies;
no corpus growth was fed back into the evidence.

### Full replay evidence

All full replays used seed `20260720`, `-runs=5658`,
`-print_final_stats=1`, and `LLVM_SYMBOLIZER_PATH=/bin/false`. Sanitizer runs
used `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1`.

    target             build       executions  coverage  features  peak RSS  log SHA-256
    tx_pool_standard   normal      5659        6180      25465     104 MB    ceb0da8df396e819f404d2132d7c6a1a8740e1ac07f88c794a5393cbff94e164
    tx_pool             normal      5659        8716      55235     147 MB    dde614ae2c4ea929e33bb14a66eecaa1d1546439b25f8156d19e0bf7681d2364
    tx_pool_standard   ASan/UBSan  5660        12892     54886     359 MB    2f310a0e2367ee409a84e056dd449bf9d8956a2fe0beabdf32aba53f39061751
    tx_pool             ASan/UBSan  5660        19940     120601    564 MB    7c4f9b077a98a28baac82f8e9bcaf48606559c4f8af2e50c2a8b92cb695fac04

Every run exited 0 and produced zero artifacts. Four independent ASan/UBSan
workers also replayed disjoint shards. The sorted union matched the frozen
manifest exactly. Each target's workers executed 1,417, 1,417, 1,416, and
1,416 inputs. Standard peak RSS was 348, 341, 344, and 338 MB; generic peak
RSS was 436, 409, 497, and 546 MB. All workers exited 0 with zero artifacts.
Worker log SHA-256 values, in shard order, were:

    standard  5fa0dcc11e7d3670b2f96c02f4c4270511781cb36ca70edf61d8be1be8934fc2
              e9ccd541c814112e2cae96485efbcf26528013033c30ed8a76cc69d29fef19b1
              a6cee835d7936929d06ce2eb315b10e04ab67eb29645c5525f2a16a15288a5e2
              a13137907360db3fa00fee399bb5a06a215af402626e05ad97f9b9ca266025dd
    generic   fed2c1ae53d633c4bb506fa303439664f5b1665ac5fb984910be04fb5d9f9af9
              cb1246bddd0a7fa6f1f91fba23233aa36e7025274b76407318ba8adb60dd0f0a
              a55e0e9b47fcdbec88957b24078c85e499d4d5283d26f39341d0426e6d7c944d
              952a7db4d96d935e571ac1e87390814e3fd829457ea92828a401f8c8e4bd3d09

### Differential oracle proof

This is a proof that the new oracle matters, not a clean-master production
finding. The exact production mutation deleted `nTransactionsUpdated++` from
`CTxMemPool::addNewTransaction()`. With the enhanced harness and
`FUZZ=tx_pool_standard -runs=5658 -seed=20260720 -shuffle=0`, the mutation
failed at `AssertValidationDelta` after 26 units on frozen input
`dfc4d02ef03ccd38dfd725e4c315470df3a2efa4` (156 bytes). The crash artifact
was byte-identical to the corpus input and has SHA-256
`7f68eca899e13d759f2357d2546b203aeca6ff7a5f1fd884c57dae96a3cc4729`;
the mutation log SHA-256 is
`229ea9e831b300487fb9baafdf9c0812197f7e1d4003d314f30156b1b90fe2b6`.

The matched control kept the production mutation but removed only the new
update-counter assertion. Replaying that exact artifact once exited 0 with
no artifact; the control log SHA-256 is
`78b24e6cd63bdfb642cdbc56c83357989f4a06466e08f880cdf71b930ab04940`.
The existing `tx_pool.check()` and old harness therefore do not observe this
metadata regression, while the new transition oracle does. The mutation was
removed before the source commit. Because restored master has no failure,
there is no deterministic production regression test to claim; the exact
mutation, witness, control, and verifier commands are the strongest relevant
proof.

### Existing finding ledger and policy

The confirmed generic raw `finalizepsbt` invalid nonempty `final_scriptSig`
behavior remains Low: it is a correctness/availability issue in a private RPC
path, not fund loss, consensus, memory safety, or cryptographic failure;
`walletprocesspsbt` rechecks through `CWallet::FillPSBT`. Earlier audits retain
their caller-based classifications: feature-conditional private-broadcast
failed-send retention and empty-HEADERS initial-sync availability are Medium;
peer transaction-activity refresh, block-storage failure, oversized transport
types, and banman invalid-subnet/unban integrity are Low or hardening issues.
The scalar/field/group, DER, EllSwift, BIP324, key, scriptpubkeyman,
wallet-construction, and other stateful audits found no additional clean-master
production bug. Latent ecmult scratch wrapping, 10x26 magnitude normalization,
and SHA/HMAC/RFC6979 retention concerns remain reachability-limited findings,
not Critical Bitcoin Core vulnerabilities.

Any future cherry-pick or fix that changes a follow-up finding must be
recorded in the same commit's message and in this ledger, stating whether it
masks, preserves, or changes clean-master behavior. Findings are always rated
against master and actual Bitcoin Core callers. A potential fix must not be
treated as proof that a severe master bug was discovered if it merely masks a
later oracle; merge or amend the notes and retain the clean-master control.

### Verification and test gap

`git diff --check` passed. Both fuzz-only normal and ASan/UBSan/fuzzer builds
completed. The configured build did not provide `test_bitcoin`, so no
dedicated unit test was available or claimed. No production behavior changed
on master, no clean-master bug was confirmed by this target, and no fuzz,
sanitizer, mutation, or replay process remained running.

## `psbt` finalization and merge-oracle audit (2026-07-20)

Source commit: `b8a6b4133cfa62640bd057a9eeb2a7cf0bfe0c9e`
(`fuzz: harden PSBT finalization and merge oracles`), parent
`8e62062cd2ad064f8d39f215c030f17392489924`. The audit base was latest
`origin/master` `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`;
`remotes/l0rinc/master` matched it. The exact target-scoped comparison was:

    git log origin/master..remotes/l0rinc/master -- \
      src/test/fuzz/psbt.cpp src/psbt.cpp src/test/psbt_tests.cpp \
      src/node/psbt.cpp src/node/psbt.h src/rpc/rawtransaction.cpp

It returned no output. No relevant l0rinc commit was cherry-picked. No later
fix or cherry-pick was used to mask clean-master behavior. Any later change
to PSBT decoding, finalization, signing, extraction, merge, RPC callers, this
corpus, or this witness must amend its commit message with whether it
preserves, changes, or masks the result; merge or amend deliberately if it
changes a follow-up experiment.

### Core boundary and severity

`FUZZ=psbt` exercises PSBT parsing and the transaction/signature contracts
used by Bitcoin Core's generic `finalizepsbt` RPC, wallet PSBT processing,
GUI/external-signer flows, and PSBT combination. The confirmed master bug is
**Low severity correctness/availability**: a caller-supplied PSBT with a
nonempty invalid `final_scriptSig` could make `FinalizePSBT` report complete
and make `FinalizeAndExtractPSBT` emit an invalid transaction. The generic raw
RPC could therefore return `complete=true` for bytes that fail script
verification. `walletprocesspsbt` rechecks with `CWallet::FillPSBT`, so this is
not a wallet fund-loss, consensus, invalid-block, memory-safety, or
cryptographic vulnerability. Invalid block/header bytes do not directly reach
this private PSBT path.

Severity is based on the problem on master and on how Bitcoin Core actually
calls the method. Invalid input acceptance alone is not High/Critical. A
nonce with no cryptographic meaning is not Critical merely because it is not
cleared. The existing ledger is reiterated rather than reclassified: Medium
feature-conditional private-broadcast failed-send retention; Medium empty
`HEADERS` initial-sync availability/IBD risk; Low under current callers for
peer transaction-activity refresh, block-storage failure, oversized transport
types, banman invalid-subnet and unban integrity; and Medium but latent or
reachability-limited ecmult scratch wrapping, 10x26 magnitude-32
normalization, and SHA/HMAC/RFC6979 retention. Earlier container, network,
storage, mempool, RPC, descriptor-cache, DER, EllSwift, BIP324, key,
scriptpubkeyman, and wallet transaction audits found no other clean-master
production bug.

### Changes and regression

The production finalizer now re-computes transaction data after finalization
and verifies every finalized input. This prevents pre-existing invalid final
fields from being treated as a complete signature set. Production merge and
extraction assertions cover PSBT identity/shape and preservation of input and
output fields.

The fuzzer now checks analysis shape and role values, extracted transaction
envelopes, final-input verification, merge identity and failure atomicity,
AddInput/AddOutput count contracts, and the precise conditions under which
`RemoveUnnecessaryTransactions` may discard non-witness UTXOs. The original
round-trip and signature-data checks remain.

The deterministic regression test uses the exact 550-byte PSBT produced by
`FuzzedDataProvider::ConsumeRandomLengthString` from the 554-byte frozen
witness. Four escaped backslash pairs are part of that harness transformation
and are retained in the test's decoded hex.

### Clean-master proof

The frozen witness is
`/tmp/bitcoin-psbt-20260720/frozen/98426b6bccbcee916f6db8a9f983e03155fe13e8`,
554 bytes, SHA-256
`8aa7244730e3bf0773e268ee53ff566b0f21b7a0b57ade5f0f6e7f84c09c2ceb`.
The pre-audit `src/psbt.cpp` hash was
`8f2e9ab16f3ec5635acd1606ffea5bef97e4ec04`, exactly the
`origin/master` blob. With clean-master production and the enhanced harness
(`d45bf2f98e5e4aaf01d2374ffca61bf463b8bfa5`), the clean sanitizer binary
`6dddcb1d47336c7155ef42ce6aa68b1be36195fa2dbc01e032cedb8fe3ff2304` aborted
at `psbt.cpp:208` because master returned success for an unverifiable final
input; log SHA-256
`fa9ca95670e1f6b223bc9ce26e32f426b100018ef0f5cb7fc6bc896c82dd56a0`.

A matched clean-master control retained production hash
`8f2e9ab16f3ec5635acd1606ffea5bef97e4ec04` and removed only the new final
verification assertion from fuzzer source
`66e7e0b0858265c5389c0648f8edf6e37e6c8b2d`. Its sanitizer binary was
`1b5b61b63258515a800179134095b8b687710a0f0925654d4c4b74d92a12ed4b`; the
same witness exited 0, log SHA-256
`32a45cfb763f22ba016923e7118edf47203273bb25b15ac7beb2f9926695d265`.

With the fix restored, source hashes are `src/psbt.cpp`
`2ea7cee260a94c36c511da0876456bc703710672`, fuzzer
`d45bf2f98e5e4aaf01d2374ffca61bf463b8bfa5`, and regression test
`b8465aa22be39b31fe96b663d234f1e9cb69aeb8`. Sanitizer binary
`398dcfecc9b3747c352e33fd353a78ac02b2e435562aa23410f5a544e398c8e8`
replayed the exact witness with exit 0; log SHA-256
`bd2df51a61a39cf837d94cbc6a9c1a9b609721fc44a316dbcdfe2c740d34e81f`.
The pre-fix deterministic test failed because
`!FinalizeAndExtractPSBT` was false; log SHA-256
`49eafea6778d77d252d067c5c1d7b16cb4aee37913d0e1362f89dc32a821d19b`.
The fixed `psbt_tests` suite passed; log SHA-256
`b966cfcad4ccafae7a062230c248bb46c387e27d397f8634f734e120cedec171`.

### Corpus and replay evidence

The frozen source was `/mnt/my_storage/qa-assets/fuzz_corpora/psbt`, copied to
`/tmp/bitcoin-psbt-20260720/frozen`: 6,134 files, 29,241,189 bytes, sizes
4..835,312. Sorted filename and filename+size manifest SHA-256 values are
`1fbec7cde6e93cd4791d7f5b24fa57d9e129efa9329b5dcca214661d5009c211` and
`bc0ce131ed9893b3996f8303f3014395bf7d9a3eee74aa2965cb3a123df14f2c`.

Before enhancement, sanitizer replay recorded 7,137 executions, coverage
18,161, features 91,849, peak RSS 787 MiB, and no artifacts; log SHA-256
`7f39fbc0f19b2d6b158b5efa710f2a0184d7dbf93dc0c28ee964c7752a7693b9`.
The final normal binary
`699c4583bc5c3618a77ccdd6d297d19473203245780b50853a8bf36086e46f55` passed
all 6,134 files; log SHA-256
`edbd54ed839d28794183f8e3905bb7a6bfc7718ee44f0e6006861cc0286fe424`.
The final sanitizer replay recorded 7,137 executions, coverage 18,291,
features 92,016, peak RSS 790 MiB, and no artifacts; log SHA-256
`b8191c66b1eda7d1d530ed3c022bb37b21cacec5cd5a6ca41eb33daa69537fdb`.

Four disjoint sanitizer workers processed 1,534/1,534/1,533/1,533 files and
executed 2,537/2,537/2,536/2,536 units. Peak RSS was 585/490/485/506 MiB;
no artifacts remained. Worker log SHA-256 values are
`0285111ce121a664e0ef668b811ee97033c29449e2781fa518f43f833084fc9c`,
`04285a82c3f08905598bf9bc3744123609b401ed417c8f7ef3a578d080182230`,
`847b32049ae2c31ebbb3ee7d8a3776c3418cf531835430af926df0b4fb735a28`, and
`bcd99c1fe34b39c4ec67553cf8b1dd7c42465c6d7c2197eeaadb280e69ad77e3`.

Normal and sanitizer fuzz targets and `test_bitcoin` were rebuilt from the
final source. The final test binary SHA-256 is
`6b17a0247587ec498d0566af713fe51fbb8bbf6f0c68de07cae552101ef81db2`.
`git diff --check` passed, and no fuzz, sanitizer, mutation, or replay process
remains running.

## `wallet_create_transaction` construction-oracle audit (2026-07-20)

Source commit: `8e62062cd2ad064f8d39f215c030f17392489924` (`fuzz: audit wallet
transaction construction contracts`), parent
`af007794de201874b47826b16605871244185908`. The audit base was latest
`origin/master` `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`;
`remotes/l0rinc/master` was the same commit. The exact relevant comparison
was:

    git log origin/master..remotes/l0rinc/master -- \
      src/wallet/test/fuzz/spend.cpp src/wallet/spend.cpp src/wallet/wallet.cpp \
      src/wallet/wallet.h src/wallet/test/fuzz/coincontrol.cpp \
      src/wallet/test/fuzz/coinselection.cpp

It returned no output, so no l0rinc commit was cherry-picked for this target.
There is no later fix or cherry-pick in this proof that can mask clean-master
behavior. Any later change affecting wallet transaction construction, TXO
refresh, selection, signing, fee/change accounting, this corpus, or this
mutation witness must amend its commit message with whether it preserves,
changes, or masks this result; merge or amend deliberately if it changes a
follow-up experiment.

### Core boundary, severity, and carried findings

`FUZZ=wallet_create_transaction` exercises private wallet transaction
construction reached from Bitcoin Core wallet interfaces and spend RPC paths.
Invalid block or header bytes do not directly invoke this path. The clean
master replay found no production bug, wallet corruption, consensus failure,
memory/concurrency fault, or cryptographic failure. The rating on master is
therefore **Informational/Low: oracle and harness hardening**, not a confirmed
vulnerability. The missing `RefreshTXOsFromTx` was a harness setup defect, not
a production finding. A future result must be rated from a clean-master
reproduction and actual Core effect: wrong ownership, key/address loss,
incorrect signing or fee accounting, wallet corruption, crash, or memory
safety. Invalid-block reachability alone is not High/Critical. A nonce with no
cryptographic meaning is not Critical merely because it is not cleared.

The existing ledger is reiterated here rather than silently reclassified:
Medium feature-conditional private-broadcast failed-send retention; Medium
empty-`HEADERS` initial-sync availability/IBD risk; Low under current callers
for peer transaction-activity refresh, block-storage failure, and oversized
transport types; Medium but latent/reachability-limited ecmult scratch
wrapping, forced 10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979
retention; and Low/nice-to-have banman invalid-subnet and unban integrity.
The earlier container, network, storage, mempool, RPC, descriptor-cache, DER,
EllSwift, BIP324, key, and scriptpubkeyman audits found no additional
clean-master production bug.

### Oracle changes and harness boundary

The original fuzzer discarded `CreateTransaction` results and directly
inserted synthetic confirmed transactions into `mapWallet` without refreshing
`m_txos`; direct inspection showed `AvailableCoins` size zero. The harness now
uses `RefreshTXOsFromTx`, retains the original arbitrary `sign=true` call and
fuzzed recipients/coin control, and adds a deterministic wallet-owned
`MAX_MONEY` funding fixture with a preselected `sign=false` spend. This makes
the accounting-success path reachable without changing production behavior.

Successful results now require consensus structural validity, nonempty inputs
and outputs, known wallet-owned nonduplicate inputs, `MoneyRange` values,
exact input-minus-output fee accounting, recipient script and amount
preservation, and correct change position/ownership or custom change script.
Production `CreateTransactionInternal` also asserts that the final input count
equals selected inputs and that the returned fee equals final transaction
value accounting. These are narrow contracts, not blanket "accepted means
valid" assumptions.

### Corpus and replay evidence

The frozen corpus was copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/wallet_create_transaction` to
`/tmp/bitcoin-wallet-create-transaction-20260720/frozen`: 1,186 files,
2,641,732 bytes, sizes 1..43,345. Sorted filename, filename+size, and
relative-content manifest SHA-256 values are respectively
`690ac5c0f9901547fe9a41f154af20616b0484bfd9df66188d949486d3b75a2f`,
`9ee0b879a2998fe4f8c3b9eaaa92460c291b6d0e2b2b92cf16eec9ccd148467f`, and
`76e5cfadabe3737bc5d5c6ed3d7c29d185145cf67853a90c804723443bbe0dc0`.

The original normal binary
`b6ed226ba6525f5d5a2c193dafb10842ef9719b7da7b2c95a3be880258892817` passed
all files in 44 seconds; log
`19c57dcb6dc91e610bb129293d21b15239d128c171db25ae985c411e5403f55e`.
The original sanitizer binary
`bb87358214a7647b8fd13262b09322c891827f25cf1ec41633c9b011ab7affba` passed
1,188 executions with coverage 10,709, features 55,081, and peak RSS 570 MiB;
log `4ad12c5709f511d4256ebd5a1c437b9b55fa9d8aff56b8ff3e09e2a62536c7cd`.

The final source hashes are `src/wallet/spend.cpp`
`bc2d38990b78cd7ff35e1fb5fded8c33f4dd19fd9bf4726c1f15540f6aeca700` and
`src/wallet/test/fuzz/spend.cpp`
`81f27bf060544cf880909715e8cb5921c2e6137e3a518caee48da2306470cf3b`.
Normal binary
`d15e6bac60a387dd6f4923dd6a4c30e65ae6228c406d9e13cc14593a567c15a0` passed
all files in 57 seconds; log
`3096e226f3b275df628969929a9c5a09995aeb34a938fd6caf754a891ffc160a`.
Sanitizer binary
`cad9a3ec0e6d04ecd04079644b21eaa5581dec367de7a07f62a8b07c50d0dd4f`
passed 1,189 executions, coverage 17,810, features 101,613, and peak RSS
638 MiB with no artifacts; log
`87e8a546c31a981028a2c2453b15537d272a77b754c3ffe1735620b6dd897b03`.
Four disjoint sanitizer workers processed 297/297/296/296 files and executed
300/300/299/299 units; peak RSS was 592/612/585/608 MiB and no artifacts
remained. Worker log hashes, in order, are
`4de389256de0f4496920cae5e5efca60b704fe8cfa1e09af12471d29535e69a6`,
`041ba2b833a1456c47bd0f2cac8599bee4661a41c4ec88ee88210715608dc994`,
`3499945feb0a8c947c1169ae82641cbc4f319c1d10989c01b5ab6de79580eff0`, and
`3a8e754e22b4b7ee9bfca23c930fe97063d240d058c49ca69d097cc1cabbedd4`.
One 12-second libFuzzer slow unit
(`6ecf119950523157289b15ea371959d9f1a69061cdd39f2357a3834d6e07547d`)
was reported; it was not a failure and no generated unit was retained.

### Differential proof and verification

The exact witness is
`/tmp/bitcoin-wallet-create-transaction-20260720/mutation-witness/witness`,
1,082 bytes, SHA-256
`79f446dc5ba329aaff856d88985e4cb645231b201db904fb1c56574353e9f19a`.
The only production mutation changed the returned fee to `current_fee + 1`;
mutated source hash
`bcc3260ae5d43a237424c48f6989b8e7f1b13bb687936787f01b19b67e584282` and
mutated sanitizer binary
`03eb8c3eeb6c532a669fe3ebdf174fb4d9ecfec83d737155b886ea5668a8eede`.
The enhanced harness aborted at `spend.cpp:74` on
`result.fee == input_value - output_value`, exit 134; log
`f0e2b00d333ac11232f69328b80b20950c909bc3055ef8bcb5d6e8065833342c`.
A matched control retained the mutation, TXO refresh, deterministic fixture,
and fixture-success assertion but removed only the two new postcondition
calls. Its harness hash is
`3af63f0334218c6980b1b12be898cb9512ca111fddb40ac00790819ee8806a9f`, binary
hash `bff61b6d058d1ceb368ff03dc62ca58ad4865dd7e7843bde564c1d17553cef98`,
and the same witness exited 0; log
`ad14d324e0b15a5bd6e1d4bd6153058e68c65630da30266892c6a1dfb2af01a2`.
Restored master exited 0 on the witness; log
`43f1e34cc104a62e2876f77615b437affa8ffc10c1f9f46c60c63f7e8e9ed1f4`.
This proves the oracle detects a modeled accounting defect and does not claim
that clean master is broken.

`git diff --check` passed. Focused `spend_tests` (3), `coinselection_tests`
(4), and `wallet_tests` (14) passed with `*** No errors detected`. Formatting
still reports pre-existing file-wide diagnostics in `spend.cpp`; no unrelated
formatting was changed. No production fix or deterministic regression test is
claimed, and no fuzz, sanitizer, mutation, or replay process remains running.

## `scriptpubkeyman` descriptor-wallet state oracle audit (2026-07-20)

Source commit: `af007794de201874b47826b16605871244185908` (`fuzz: audit
descriptor scriptpubkeyman state contracts`). Parent:
`bc36a929305fd697207bd2ee9ed94b4ff0d7ddef`. The audit base was latest
Bitcoin Core `origin/master` `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`;
`remotes/l0rinc/master` matched it.

### Provenance and severity

The exact l0rinc comparison was:

    git log origin/master..remotes/l0rinc/master -- \
      src/wallet/test/fuzz/scriptpubkeyman.cpp src/wallet/scriptpubkeyman.cpp \
      src/wallet/scriptpubkeyman.h src/wallet/wallet.cpp \
      src/wallet/test/fuzz/spend.cpp

It returned no output. No relevant l0rinc commit was cherry-picked. No later
fix or cherry-pick was used to mask the clean-master result. Any later change
to descriptor expansion, ownership, keypool state, this corpus, or this proof
must amend its message with whether it preserves, changes, or masks the
behavior; merge or amend deliberately when a potential fix changes a follow-up
experiment.

`FUZZ=scriptpubkeyman` reaches `DescriptorScriptPubKeyMan` through wallet
descriptor creation and replacement, address generation, and keypool
maintenance. Bitcoin Core's invalid block/header byte boundary does not
directly invoke these private wallet transitions. Clean master reproduced no
production bug, invalid-block consensus issue, wallet corruption,
memory/concurrency fault, or cryptographic failure. This is
Informational/Low oracle hardening, not a confirmed vulnerability. Re-rate a
future finding from a master reproduction and actual Core effect such as wrong
ownership, address/key loss, signing failure, wallet corruption, crash, or
memory safety. A nonce with no cryptographic meaning is not Critical merely
because it is not cleared.

The existing master-relative ledger remains: Medium feature-conditional
private-broadcast failed-send retention; Medium empty-HEADERS initial-sync
availability/IBD risk; Low under current callers for peer transaction-activity
refresh, `ProcessMessage` block-storage failure, and oversized transport
types; Medium but latent/reachability-limited ecmult scratch wrapping, forced
10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention; and
Low/nice-to-have banman invalid-subnet and unban integrity. No additional
clean-master bug was found in the prior container, network, storage, mempool,
RPC, wallet descriptor, DER, EllSwift, BIP324, or key audits. These are not
silently reclassified by this oracle-only change.

### Oracle changes

Production `DescriptorScriptPubKeyMan::GetNewDestination` now asserts that a
successful descriptor-cache expansion returns a non-empty script list. The
harness checks the matching state contracts after initial manager creation,
descriptor replacement, generated destinations, `MarkUnusedAddresses`, and
the final operation sequence:

    GetScriptPubKeys() -> IsMine(script)
    generated/marked destination -> GetScriptForDestination -> IsMine
    generated/marked script -> GetScriptPubKeys().contains(script)

This keeps the assertion at the ownership/keypool state boundary rather than
assuming that an operation returning success is sufficient evidence.

### Corpus and replay evidence

The frozen corpus is
`/tmp/bitcoin-scriptpubkeyman-20260720/frozen`, copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/scriptpubkeyman`: 10025 files,
79091508 bytes, sizes 1..998620. Sorted filename, filename+size, and
relative-content manifest SHA-256 values are
`ebd8e558eebe79ab8a1ada6f642a7ff01f25a84a0a5b615a7c9d240326956259`,
`586c03db27f367102518cdfca04cc1ec5aa16a67eeee324c9951d9dfd466a942`, and
`827749723ef3db8cc5a74e17bfc26aebee24c9fbf56fc2e501bdc11eb3ee971d`.
All authoritative runs used isolated copies.

The baseline sanitizer replay was completed in four isolated chunks: 2510,
2509, 2509, and 2509 executions; coverage 25330, 25229, 25398, and 25417;
peak RSS 887, 857, 856, and 865 MiB; no artifacts. Log hashes:
`c2cade7e754afdc9973a80a2b2e839405cab16361cbb5b434453b85990c8835a`,
`6031b74d530333760d613e093a140391b30aa8e245995873ee05267866440ed1`,
`3511eba1e05527a2cc709b02bd937b677246c917a401d20b6558f50e81d24c0`, and
`0c50258640f4486f4e6e3a1c571dd5f484e201f79f8a45aba010e9b07205741d`.

The enhanced sanitizer replay used the same four-way isolation: 2510, 2509,
2509, and 2509 executions; coverage 25363, 25266, 25432, and 25452; peak RSS
856, 853, 853, and 908 MiB; no artifacts. Log hashes:
`f660fdf4f3fd9e0bd5f9cfd6fa4ba9633435f42b212da97737f9a40152e8e981`,
`fe39316d3075db6969e70c9dd329a0970686de92eeb88c3ee1ff2254d5bb01b1`,
`35dc2ec66d56dbafc5f38d92a0d2e5ab8cdbeb5cfe959aa47639999bb1bbc14c`, and
`0ea0177eab3d2b89b38678205c1ed514d72612a72ba1424232356f1d95fd76fb`.

Final source hashes are `src/wallet/scriptpubkeyman.cpp`
`fd79083bd274a27d7b616d3c513b80229e75127af196025e59c1be875c413292` and
`src/wallet/test/fuzz/scriptpubkeyman.cpp`
`351738b0bc58c78d41176a36a9740c0e07a8d1b568a3e28e119267256f761580`.
The sanitizer and normal fuzz binaries were
`bb87358214a7647b8fd13262b09322c891827f25cf1ec41633c9b011ab7affba` and
`b6ed226ba6525f5d5a2c193dafb10842ef9719b7da7b2c95a3be880258892817`.
The normal binary passed all 10025 files in 51 seconds; log hash
`b5c287dffca9dcf051abad290d881206bf8ead82ba28c46f8512440e0080e2de`.
`scriptpubkeyman_tests`, `ismine_tests`, and `wallet_tests` passed 2, 1, and
14 cases. The test binary hash was
`53053cecc3eb32d4ec2e71a566cf8f1cdc47b28cab5e9e6de334c850a6ec394d`.

### Differential proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation changed
`m_map_script_pub_keys.contains(script)` in `IsMine` to `return false`.
Mutated production source hash:
`b1bb3e6c2661273eddb671af93de5064ee5f210e09db5ed4249e6e9a0047de2c`.
Mutated enhanced sanitizer binary hash:
`6b7b1333c24632ba476fb526550818d9cf4b3a4a238b80dcf5419a870b7f53f8`.

The exact witness was
`/tmp/bitcoin-scriptpubkeyman-20260720/frozen/0002e72fbe868c7c3f2d3ffbcdd06ce25f38968e`,
5962 bytes, SHA-256
`e792a4f2159ae7aad2df5230344c7aa040422bc9ab150a7f8ce05f77e21244d7`.
Enhanced replay exited 134 at `scriptpubkeyman.cpp:96` on
`assert(spk_manager.IsMine(script))`; log hash
`073caf66a8a4d5c984b8dba20df346a704f5663304155ba7823e05d460abc383`.

For the exact parent-harness control, only the new harness assertions were
removed while the production mutation remained. The mutated production hash
was unchanged, the control sanitizer binary was
`bc4498eba652ce788c769a5a35eebd86817d4c96959a54db9afa6ff931a4193f`, and the
witness exited 0 with no artifact; log hash
`2364e0fe6cb7d0d7f7f62f090cbe0fe7ba692782f2a8fc14df0279774689b1d5`.
Restored final source and harness replayed the witness with exit 0; log hash
`0940e02263ea0e4e9baeccf491b024f5ec2f8e2c5724517fc41b764178fe7623`.
Thus the new oracle catches the modeled broken ownership contract that the
old harness accepted, while the clean-master replay proves no production bug
was found.

### Verification and test gap

Built sanitizer and normal fuzz targets, ran the frozen corpus, four isolated
sanitizer workers, normal replay, focused wallet tests, and mutation/control/
restored witness replay. `git diff --check` passed. `clang-format --dry-run
--Werror` reports pre-existing diagnostics at the legacy include and existing
fuzzer lines 223, 227, 328, and 358 plus file-wide diagnostics in
`scriptpubkeyman.cpp`; none of the added assertion lines were reported and no
unrelated formatting changed. No production fix or deterministic regression
test is claimed because clean master has no confirmed bug. No fuzz, sanitizer,
mutation, or replay process remains running.

## `key` BIP32 and uncompressed-key oracle audit (2026-07-21)

Source parent is `16e576419f3b70a2465b76d03ea9d1f5b4ff4b85` (`fuzz: enforce
BIP324 ECDH state contracts`). The audit base was latest `origin/master`
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; `remotes/l0rinc/master` was
the same commit. The target-specific query

    git log origin/master..remotes/l0rinc/master -- src/test/fuzz/key.cpp src/key.cpp src/key.h src/pubkey.cpp src/pubkey.h src/bip324.cpp src/test/fuzz/bip324.cpp

returned no output. No relevant l0rinc commit was cherry-picked, and no later
fix or cherry-pick was used to mask or alter the clean-master result. Any
follow-up touching CKey derivation, serialization, wallet callers, the corpus,
or this proof must amend the relevant source and evidence commit message with
whether it preserves, changes, or masks this behavior; merge or amend
deliberately if a potential fix changes a later experiment.

### Core boundary, finding, and severity

`FUZZ=key` previously built only a compressed `CKey`, derived only child index
0, and did not use the uncompressed key after constructing it. It now checks
both BIP32 branches against a byte-level HMAC/tweak reference, verifies that a
failed tweak leaves the child invalid, and exercises uncompressed public-key
serialization, ECDSA signing/recovery, WIF/DER export, and both `CKey::Load`
validation modes. Production `CKey::GetPubKey` and `CKey::Derive` assert the
documented success and state postconditions.

Bitcoin Core reaches these private-key methods from wallet persistence and
descriptor/HD key management: `src/wallet/walletdb.cpp` loads serialized
private keys, descriptor setup calls `CExtKey::Derive` through
`src/script/descriptor.cpp`, and wallet signing/address paths call
`GetPubKey`, `GetPrivKey`, and `Load`. The fuzzer's arbitrary bytes first have
to form a valid local private key; invalid block or header bytes do not invoke
these methods. Clean master reproduced no production failure, so the result
is **Informational/Low oracle hardening**, not a confirmed wallet bug, remote
block vulnerability, consensus issue, High, or Critical finding. A real
master-reproducing key mismatch, key loss, wallet corruption, crash, or
memory-safety failure would be re-rated from the reachable Core effect. An
uncleared nonce with no cryptographic meaning is not Critical merely because
it is uncleared.

### Reiterated master-relative findings

The existing ledger remains: **Medium**, feature-conditional private-broadcast
failed-send retention; **Medium**, empty-HEADERS initial-sync availability/IBD
risk; **Low under current Core callers**, peer transaction-activity refresh,
ProcessMessage block-storage failure, and oversized transport types; **Medium
but latent/reachability-limited**, ecmult scratch wrapping, forced 10x26
magnitude-32 normalization, and SHA/HMAC/RFC6979 retention; and
**Low/nice-to-have**, banman invalid-subnet and unban integrity. No additional
clean-master production bug was found in the already audited addrman,
coins-cache, coins-view, txgraph, txdownloadman, txrequest, connman, eviction,
handshake, compact-block, headers-sync, UTXO snapshot, mempool persistence,
package evaluation, RPC, descriptor cache, threadpool, cluster-linearization,
policy estimator, pool resource, versionbits, signature-cache, CuckooCache,
FlatFilePos, prevector, bitdeque, VecDeque, DER key import/export, lax DER,
EllSwift, or BIP324 paths. Severity remains tied to actual Core callers and
input origins; an isolated assertion or invalid block alone is not Critical.

### Corpus and replay evidence

The authoritative corpus was copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/key` to
`/tmp/bitcoin-key-20260720/frozen`. It contains 1,085 files and 34,658 bytes
(minimum 1 byte, maximum 32 bytes). The sorted relative filename, filename
plus size, and sorted relative-content manifest SHA-256 values are
`d61e480c6b097cc876feabdd81b99363767147596b1e9e5fe7b1ba7a3f6fa442`,
`5d34579845f3ff8e8f38921734acc31868ba75f199cfe8a338ecaa706d949ee2`, and
`2d8647bc3a53f907fdf876a5f46f7c5767dcfef83e398a13d6279c2788650624`.

The parent sanitizer binary was
`560bcdd32529c81ade56c7aacb2f60e429f4611258a069e287a79a44368445d8`.
Its baseline replay executed 2,088 units, coverage 7,106, features 13,993,
peak RSS 145 MiB, and no artifacts; log SHA-256 was
`7ec84b3089b02b70cd18bb5a040170988bb1186e19a674a4a1e4c5814a64f4ba`.
The final sanitizer binary is
`d16667652065cc1c80ba969ff2d5af4545adb24619875c5b6769cb712d8024e9`.
The enhanced replay executed 2,088 units, coverage 7,280, features 15,194,
peak RSS 154 MiB, and no artifacts; log SHA-256 is
`800f985d465282eb50c09b41a24e752dd7b3a245b73ce3a20552ff1c535ed2ae`.

Four isolated sanitizer workers each executed 2,088 units with no artifacts;
peak RSS was 153-154 MiB. Worker log SHA-256 values, in order, are
`cc72e912da60378f00b40a90226870aa09b507b6cf85e3077c994d5a71fb43a6`,
`6eeaa226f0ec34dc40f26027ac459ffd043812914764586194cd6e6e74670825`,
`e16450e664074e1963b7fc921722b3b3f53102016c088c5b4a0fbd9f6fcb7579`, and
`9b92e007d7072b5a5ec0de4ec7023b1e8a4e0762033e3a929e5225d56aab3248`.
The final normal binary is
`073451378e29c81d3a4c69fc3b8836a641b8ec6cc3a6b3106d45d6ed11388a04`; its
standalone driver passed all 1,085 files, with log SHA-256
`db670f933a288c9c78bf5d04ad1b9b751928cec7a923b70bba1cf7dd5f160e77`.

### Differential proof and verification

This is oracle sensitivity evidence, not a clean-master production finding.
The exact temporary production mutation changed
`if ((nChild >> 31) == 0)` in `CKey::Derive` to
`if ((nChild >> 31) != 0)`. The mutated source hash was
`a31bee3a633f447a84e2f1bfbc5afdd05ff8528062fc819a881c848508229cd3`, and
the mutated sanitizer binary hash was
`8a0a26503865ffe8535e2e116a7fe404209762b9c1986241c49d0279800a1c`.
The exact witness was
`/tmp/bitcoin-key-20260720/frozen/00235b297abea064ae2e346e492ba338f52a048b`,
32 bytes, SHA-256
`92d12a2e5eeada32fa85804fc9e77a89b50625341cca39498d100fa0d67df638`.
The enhanced mutated replay exited 77 at `key.cpp:143`,
`child_key == reference_child`; mutation log SHA-256 was
`0dd09dd4df0e1851616052bdaeb16bda5dcdf9b54332e9fff907e98f68247694`.
The restored implementation passed the identical fixed witness in one unit;
restored log SHA-256 was
`f1a5002de0f6d98c50440610b900bb67047ed287327d5bcf2e1d19f426c69001`.
This proves the added reference oracle detects a plausible hardened/non-
hardened derivation regression accepted by the old target; it does not prove
that clean master is defective. The mutation was restored before final builds.

Verification commands and results:

    cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2
    cmake --build /tmp/bitcoin-secp256k1-audit-current-normal-build --target fuzz -j2
    FUZZ=key /tmp/bitcoin-secp256k1-audit-current-build/bin/fuzz -runs=1085 /tmp/bitcoin-key-20260720/frozen
    FUZZ=key /tmp/bitcoin-secp256k1-audit-current-normal-build/bin/fuzz /tmp/bitcoin-key-20260720/frozen
    cmake --build /tmp/bitcoin-bitdeque-test-build --target test_bitcoin -j2
    /tmp/bitcoin-bitdeque-test-build/bin/test_bitcoin --run_test=key_tests
    /tmp/bitcoin-bitdeque-test-build/bin/test_bitcoin --run_test=bip32_tests

Both focused unit suites passed (`key_tests`: 9 cases; `bip32_tests`: 6
cases). `git diff --check` passed. Clang-format reports only pre-existing
file-wide diagnostics in `src/key.cpp` and its existing include diagnostic;
none of the newly added lines are reported. No deterministic regression test
was added because clean master has no confirmed production bug. All temporary
mutations, workers, and build processes were restored or stopped.

## `pool_resource` allocation-lifecycle oracle audit (2026-07-21)

Source commit: `88e65091f5dbc0ca86b5a361aab59432dc83e005` (`fuzz: assert pool
resource lifecycle contracts`). This audit extends the existing content-fill and
teardown checks into a stateful oracle for cursor, chunk, range, alignment, and
live-byte contracts.

### Core boundary and severity

Bitcoin Core uses `PoolResource` through `PoolAllocator` for the coins-cache
unordered map in `src/coins.h` and for allocator-aware memory accounting in
`src/memusage.h`; it is also covered by tests and benchmarks. It is not a
consensus validator API. Arbitrary invalid block bytes do not directly call
these allocator contracts, and this audit found no clean-master memory
corruption, crash, or caller-reachable state failure. The clean-master rating
for this commit is therefore **Informational/Low**, as oracle hardening rather
than a production vulnerability or fix. A nonce without cryptographic meaning
is not Critical solely because it is not cleared.

The existing master-relative findings were reiterated while evaluating this
target: **Medium**, feature-conditional private-broadcast failed-send
retention; **Medium** availability/IBD risk in the empty-HEADERS initial-sync
handoff; **Low** under current Core callers for peer transaction-activity
refresh, process-message block-storage failure, and oversized transport types;
**Medium but latent/reachability-limited** for ecmult scratch wrapping, forced
10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention; and
**Low/nice-to-have** for banman invalid-subnet and unban integrity. No
additional clean-master bug was found in the already audited addrman,
coins-cache, txgraph, txdownloadman, txrequest, connman, eviction, handshake,
compact-block, headers-sync, UTXO snapshot, mempool persistence,
package-evaluation, RPC, descriptor-cache, threadpool, cluster-linearization,
or policy-estimator paths. Severity follows reachability through actual Core
callers, so a defect that cannot be triggered by Core input is not Critical.

The audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolved to it. The l0rinc
pull-request history was reviewed for a target-specific prerequisite or fix.
No additional relevant commit was cherry-picked for `PoolResource`; no later
fix was silently used to mask or change clean-master behavior. The exact
mutation and control runs below are recorded so a future cherry-pick can be
evaluated for behavioral masking.

### Oracle contracts

The production header now asserts after chunk allocation, pooled allocation,
operator-new fallback, and deallocation that a chunk exists, the carving
cursor remains within the latest chunk, the cached end equals the latest chunk
plus the configured size, and the remaining distance is aligned to
`ELEM_ALIGN_BYTES`.

The harness maintains an ordered set of live `[begin,end)` ranges. It checks
overflow-safe ordering and non-overlap against neighboring ranges, exact range
removal, equality of live ranges and entries, every live allocation's
alignment/non-empty span, periodic aggregate live bytes, deterministic content
preservation, and the existing `PoolResourceTester` full-accounting teardown
check. The ordered set replaced two prototypes that made large corpus inputs
too slow: a full O(n^2) overlap scan and sampled sorting. Final insertion and
removal are O(log n), with periodic linear byte accounting.

### Corpus and clean replay

The frozen corpus is `/tmp/bitcoin-pool-resource-20260721/frozen`, copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/pool_resource`: 721 files and
4,218,720 bytes. Sorted-entry SHA-256:
`a8c42255a322ecd7598fa32b7f03196beb636d31cdc966e576cef849ace6b0b`.
Manifest SHA-256:
`d77f87881c7caa95127e64021d58c0aef1d14126620395aa83537dc480ad577e`.

The parent-harness sanitizer baseline used binary SHA-256
`02355222337f678b830a6491e9404f634a20375d31b076259c86b2b9097a426e` and log
SHA-256 `54626ee8f9ab62455be7f576f941b35eaf1b4c8f7b5b7a8db5c32d7b10c7a43f`.
It exited 0 after 722 runs with coverage 1499, feature count 8320, peak RSS
754 MiB, and no artifacts. The corresponding normal baseline binary was
`e93fd784c733f2d7c2bb67a0dbd8a4f5231608f4450ab0e8f199197eb491e5e8`, with log
SHA-256 `bf15229da1e2851ef123b54aa04da078386471fe182c89b86f7061c6364ea3ec`;
its one-file driver passed all 721 inputs.

Final source SHA-256 values are `1855ed015248a33fee86559a3201b8b28de1dac7e5673b4ee68fc3f6f0438b68`
for `src/support/allocators/pool.h` and
`ffca02ee7745538bea86f0052efd07c4de3a6e78cd1e76a491910688041884e2` for
`src/test/fuzz/poolresource.cpp`. The final sanitizer binary is
`abc742377ac9a8b633b6095a22e93e459b82e064cc431dd29f617b4159caa0b2`.
The 721-file replay exited 0 after 722 runs, coverage 1840, feature count
10789, peak RSS 757 MiB, no new units, and no artifacts; log SHA-256:
`ad59d580baa6b0f0affab0a3535532df614bd4afcf5a1065f0bc1b1467550622`.
The final normal binary is
`70296333d5a764da382926752ad5e0170ec978cbd12a842864ff64d924f2b0fb`; its
one-file driver passed all 721 inputs, with log SHA-256
`a20ddbc896a3cae7d8e4136d8de366647ba07773a01c7e97ace6c7e383e12a46`.

Four independent sanitizer workers each used a private copy of the frozen
corpus with `-merge=0 -runs=1 -timeout=120 -rss_limit_mb=4096`. All exited 0
after 722 runs, coverage 1840, peak RSS 752 MiB, with no artifacts. Every
copy retained sorted-entry SHA
`a8c42255a322ecd7598fa32b7f03196beb636d31cdc966e576cef849ace6b0b`. Worker
log SHA-256 values, in order `fuzz-0` through `fuzz-3`, are:

    b0a545a714beaedfab4cf07e895411d299a85cc926e9d1fd632724f29504a850
    8538322995324ceb322cc93cf887703780e54b6bfbde2ac5e895294099495fda
    e0c44aa910e0f53edd374e0d48d692456d10ace664b9895b6f4c61bd9fddd683
    ab9e9e06e202087ca636686b8117e0ffa03387310763fee7f6a4975be46221a7

### Differential oracle proof

This is an oracle differential proof, not a clean-master production finding.
A temporary minimal production mutation changed `NumAllocatedChunks()` from
`m_allocated_chunks.size()` to `return 0`. Mutated production source SHA-256:
`598c4d48261f08def277b5c348f79712065c760fd8031e7a5a309d8b2fd5d982`.
Mutated sanitizer binary SHA-256:
`f60980f28cf7b8109b3975bd888fa07a8bd1e53a8a4c18fa5e713e0cfc71437f`.

The frozen replay failed on its first execution (wrapper status 77/libFuzzer
deadly signal) at `poolresource.cpp:46` on
`m_test_resource.NumAllocatedChunks() >= 1`. The exact input was the empty
file, Base64 empty, with SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
Mutation log SHA-256:
`1e6f056da56c2f60e499a0065d4c7eca22e59d1ba35723498a897f45ab60eb86`.

The control kept the production mutation but disabled only the new chunk-count
assertion. Control fuzzer source SHA-256:
`55078301bbd5ae106928c028e8cefd9c984c58d3e269a1803bb3967f7e0255dc`.
Control binary SHA-256:
`4e0c931a3dddca5f96b6493445edbc5bd05a19b0874f9c86dd494f80726f6322`.
The identical input exited 0 with no artifact; control log SHA-256:
`556f0d655fb438e4266a156b893b22a9e7cbbc16d61079c3b23f30515c1d2aac`.
After restoring clean production and harness code, the identical input exited
0 with no artifact in the final sanitizer binary; restored log SHA-256:
`780df4d01fb9444664dd27bd86f02264879d321e17f269a5b7fac1d04adedd53`.
This proves detection of the modeled broken public contract while making no
claim that master contains that bug; it does not justify a higher severity or
a deterministic production regression test.

A separate temporary cursor mutation removed `std::exchange` when carving a
pooled block. The existing `PoolResourceTester` teardown oracle caught it
before the new range oracle produced an isolated failure, so it is not claimed
as a new finding. Mutated source SHA-256:
`7b23fef2b15e898afa57210902b5a482d32433aa7074b92318f69078f56a15f5`.

### Verification gap

Sanitizer and normal targets were built with the two configured fuzz-only
builds using `cmake --build ... --target fuzz -j2`. `git diff --check` and
`clang-format --dry-run --Werror src/test/fuzz/poolresource.cpp` passed. The
production header has pre-existing formatting violations and was not
reformatted unrelatedly. The fuzz-only builds do not provide `test_bitcoin`,
so the dedicated unit suite was unavailable. No clean-master production bug
was found, no deterministic regression test is claimed, all temporary
mutations were restored, and no fuzz/sanitizer/mutation process remains.

## `script_sigcache` cache-population oracle audit (2026-07-21)

Source commit: `101c275213a6e3cf5744c69ff3c174c1b3df50d9` (`fuzz: enforce
signature cache population contracts`). The target now checks the signature
cache immediately after ECDSA and Schnorr verification and repeats the exact
operation to exercise the cache hit path.

### Core boundary and severity

Bitcoin Core reaches `CachingTransactionSignatureChecker` through `CScriptCheck`
in `src/validation.cpp`. Normal `CheckInputScripts` paths set `cacheSigStore`
to true when successful checks are eligible for reuse. The cache must contain
only successful ECDSA/Schnorr verifications; a hit returns true without
repeating cryptographic verification.

No clean-master production failure was reproduced. The master-relative rating
of this commit is therefore **Informational/Low** oracle hardening, not a
production vulnerability, fix, or regression test. The modeled mutation has
different impact: if a production implementation cached a failed signature on
a Core `cacheSigStore=true` path, a later identical invalid script could be
accepted from a cache hit, potentially affecting invalid transaction/block
validation. That hypothetical caller impact would be **High/Critical**, but
it is not a finding against unmodified master and is not inferred from an
invalid fuzzer input alone. A nonce without cryptographic meaning is not
Critical merely because it is not cleared.

The audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolved to it. The l0rinc
pull-request history was reviewed for a target-specific prerequisite or fix.
No additional relevant commit was cherry-picked for this target. Any later
fix, minor fix, or cherry-pick must document whether it preserves, changes, or
masks the proof input and repeat the clean-master replay.

The existing master-relative ledger remains: **Medium**, feature-conditional
private-broadcast failed-send retention; **Medium** availability/IBD risk in
the empty-HEADERS initial-sync handoff; **Low** under current Core callers for
peer transaction-activity refresh, process-message block-storage failure, and
oversized transport types; **Medium but latent/reachability-limited** for
ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for banman invalid-subnet
and unban integrity. No additional clean-master bug was found in the audited
addrman, coins-cache, coins-view, txgraph, txdownloadman, txrequest, connman,
eviction, handshake, compact-block, headers-sync, UTXO snapshot, mempool
persistence, package evaluation, RPC, descriptor cache, threadpool,
cluster-linearization, policy estimator, pool resource, or versionbits paths.
Severity is based on actual Bitcoin Core callers and input origins.

### Oracle contracts and corpus

The fuzzer saves the selected sighash, computes the exact ECDSA/Schnorr cache
key, and asserts `signature_cache.Get(entry, false) == (store && result)`.
It then repeats the same checker operation and requires the result to remain
unchanged. This does not assume random signatures are valid; it checks the
actual cryptographic result and the production store contract.

The frozen corpus is `/tmp/bitcoin-script-sigcache-20260721/frozen`, copied
from `/mnt/my_storage/qa-assets/fuzz_corpora/script_sigcache`: 590 files and
17,567,030 bytes. Sorted relative filename SHA-256:
`4ea2b472881e7feedf67e87b6469ebdd7ab733a27fb682f22c30571a3e8e9b69`.
Sorted relative filename-plus-size SHA-256:
`8c432258a019929b9e33c0c90faa8e6f5efe6b31dcf710905beec550adb6c06e`.
Sorted relative-content manifest SHA-256:
`c2815a630241b183c03b8f1777896d9fd7a44ab6e76e624bd81b9f83cc09dc4d`.

The parent sanitizer binary was
`ba3776955751574844773782906604fdbf72ee279780af965cd0ac0769274256`, with
baseline log SHA-256
`727dd3e5408ba9582004b08d9ecce22146e003f75a562fed370e4eca9d71c208`.
The 590-file replay exited 0 after 591 executions, coverage 5258, feature
count 13416, peak RSS 441 MiB, and no artifacts. The parent normal binary was
`ddf52a676467c831c37c1635c42448e4cb6cf2efa7f27b1f5a1179943e669229`, whose
one-file driver passed all 590 inputs; log SHA-256:
`bd770249345cecfd05dde6861c80e5891a99c1d79d163bfc7709456a219c8a35`.

Final fuzzer source SHA-256 is
`2152f92427727231bbe6e964c50d08e95a5d8adef079d1ae5fc90234ee18ebaa`.
Production `src/script/sigcache.cpp` is unchanged at
`62da43b9482d4b5339ba5418d4ceb9b53f821b281866c3beb4980989d5d2c474`.
The final sanitizer binary is
`ff99646c923730f57e8c4503a21f73ce981cc192c4d102294e619cd7147614ba`.
The clean replay exited 0 after 591 executions, coverage 5269, feature count
14397, peak RSS 441 MiB, no new units, and no artifacts; log SHA-256:
`88491b49d7df60a810a3f9b57c02558ab08a5647590483d43ff6a37ba7c37657`.
The final normal binary is
`89ff25b6aa1aeef4798af464134e42a765b4a7c4899f188e1f9cecc3442d8f84`; its
one-file driver passed all 590 inputs, with log SHA-256
`3cabf1511bf9f1185daf398f8876b01246ba149f5d8cadeb4bd2e4a42d095842`.

Four isolated sanitizer workers each replayed the same 590-file corpus with
`-merge=0 -runs=1 -timeout=120 -rss_limit_mb=4096`. All exited 0 after 591
executions, coverage 5269, peak RSS 441-457 MiB, and no artifacts. Worker log
SHA-256 values, in order `fuzz-0` through `fuzz-3`, are:

    36035d61e31ac12a0583c7d584473ec967191af9b4b7cff92234e2e7d8130519
    dee345b64c00191ad831a6a47a5da14175026994c5c593da9556b862dbf4cd93
    17aca1bf68a33a4530582d181b6921b0ac5ffdac0b920366f284adf10786344d
    f4c005574d56d8ab95d0887b2bb7455aec3581fbb9711cb7505309f3f8418735

### Differential proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation added `if (store) m_signature_cache.Set(entry)`
to the failed ECDSA verification branch. Mutated production source SHA-256:
`a750a577e263adf970a422026700ab46d281b5a05846c99e891d6ffbff9faef8`.
Mutated enhanced sanitizer binary SHA-256:
`5c9078d07a7432cb4df3b9dc0de8318e1dfc0376d4e29a443037446325fa8a55`.

The exact proof input is
`/tmp/bitcoin-script-sigcache-20260721/frozen/08133b12ba09b43020ea434ed5f6e288b34f0314`:
95 bytes, SHA-256
`3f2656641695fc0fc23a3cccc294155d4109bce6bbeba09c30ee6d230b8e45c4`, hex
`270500fefeebd1000042e4815c04306302110000003f0000ebd10000fe42190000fd070201015c5e2103000b047a00faffb2c368939e7fc03d8e7e7100a8432b1872a1e39f4126a4cd06b81eab32ac17fecc9c9c9c9c9c519c9c9c9c9c9c9c9c`,
Base64
`JwUA/v7r0QAAQuSBXAQwYwIRAAAAPwAA69EAAP5CGQAA/QcCAQFcXiEDAAsEegD6/7LDaJOef8A9jn5xAKhDKxhyoeOfQSakzQa4HqsyrBf+zJycnJycUZycnJycnJw=`.

The enhanced mutated fixed-input replay with `-handle_abrt=0` exited 134 at
`src/test/fuzz/script_sigcache.cpp:63`, assertion
`signature_cache.Get(entry, false) == (store && result)`, after one execution
with no artifact. Diagnostic log SHA-256:
`1d4ce625516eaef92911fba39c35ad12b597948290ab4ee981f54a8b59810433`.
The same production mutation with the old single-call harness used control
fuzzer source SHA-256
`cb5e9dfb3373c16e270840308705c9e0ee8a39aaff2e28d48a01b2e1dfc58e96` and
binary SHA-256
`c880729bf5bbf51c5d2f490ca78fd20fdde6fb752be3c840f7535df5eadcc228`.
The identical input exited 0 with no artifact; control log SHA-256:
`1718e8aa72e6d5186f3bcea5a87a04203483c5ffa3a9d085d2c59a28c4b49ff1`.
After restoring production and harness source, the identical input exited 0
in clean binary `ff99646c923730f57e8c4503a21f73ce981cc192c4d102294e619cd7147614ba`
with no artifact; restored log SHA-256:
`0bb47bb3b505941985077017435012535be17f36c6831d3c174f8cb69a8f3895`.
This proves that the new oracle detects the modeled failed-cache population
that the old harness accepts, while clean master does not reproduce it. No
production bug or deterministic regression test is claimed.

### Verification gap

Sanitizer and normal targets were built with the configured fuzz-only builds
using `cmake --build ... --target fuzz -j2`. `git diff --check` and
`clang-format --dry-run --Werror src/test/fuzz/script_sigcache.cpp` passed.
The fuzz-only builds do not provide `test_bitcoin`, so the dedicated unit suite
was unavailable. The temporary production mutation and old-harness control were
restored, and no fuzz, sanitizer, mutation, or build process remains running.

## `cuckoocache` lookup and lifecycle oracle audit (2026-07-21)

Source commit: `84de6de0a9` (`fuzz: assert CuckooCache lookup and setup
contracts`). The production cache now asserts its setup and generation metadata
contracts at setup, insert, and lookup boundaries. The fuzzer uses stable
per-cache hashing, a nonzero `uint32_t` element domain, setup return-value
checks, an ever-inserted model, and a fixed `UINT32_MAX` no-false-positive
probe.

### Core boundary and severity

Bitcoin Core uses this cache for `SignatureCache` ECDSA/Schnorr result reuse
and for `ValidationCache::m_script_execution_cache`. `CheckInputScripts`
queries the latter before executing transaction input scripts and returns true
on a hit. A real production false positive reachable there could bypass script
execution and accept an invalid transaction or block; that modeled impact is
High/Critical according to the exact reachability and effect. It is not a
finding in this commit: clean master reproduced no production failure, memory
error, or caller-reachable state violation. The master-relative rating of this
commit is **Informational/Low** oracle hardening, with no production fix or
deterministic regression test claimed. An invalid fuzzer input alone is not
evidence of a consensus bug, and a nonce without cryptographic meaning is not
Critical merely because it is not cleared.

The audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolved to it, and the source
branch was rebased onto latest master before this sequence. The l0rinc
pull-request history was reviewed for a target-specific prerequisite or fix;
no additional relevant commit was cherry-picked for CuckooCache. A later
cherry-pick or fix that changes cache lookup, setup, hash reachability, or a
proof input must amend its commit message with whether it preserves, changes,
or masks this clean-master evidence and repeat the replay.

The reiterated master-relative ledger is: **Medium**, feature-conditional
private-broadcast failed-send retention; **Medium**, empty-HEADERS
initial-sync availability/IBD risk; **Low** under current Core callers for
peer transaction-activity refresh, process-message block-storage failure, and
oversized transport types; **Medium but latent/reachability-limited** for
ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for banman invalid-subnet
and unban integrity. No additional clean-master bug was found in the audited
addrman, coins-cache, coins-view, txgraph, txdownloadman, txrequest, connman,
eviction, handshake, compact-block, headers-sync, UTXO snapshot, mempool
persistence, package evaluation, RPC, descriptor cache, threadpool,
cluster-linearization, policy estimator, pool resource, versionbits, or
signature-cache paths. Severity is based on actual Bitcoin Core callers and
input origins.

### Oracle contracts and corpus

The production assertions verify one-shot setup, table and epoch metadata
sizes, valid `epoch_size`, nonzero `depth_limit`, and a bounded heuristic
counter. The harness records every value passed to `insert` and requires every
successful `contains` result to be in that model, including a `UINT32_MAX`
sentinel immediately after setup. It also checks documented `setup()` and
`setup_bytes()` return values. The old fuzzer hash consumed new provider bytes
on every hash call; the replacement consumes one seed and derives stable
hashes from the element and selector, restoring the cache hash precondition.

The frozen corpus is `/tmp/bitcoin-cuckoocache-20260721/frozen`, copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/cuckoocache`: 158 files and 4,517,046
bytes. Sorted relative filename SHA-256:
`4dc4b8753cdd874fd2222df3ba92a0f9e556521001dc89acf014f751a48759c3`.
Sorted filename-plus-size SHA-256:
`cf957e572abfd46e4fdfad6435b89107f96f6ed1f9cfe2c1622dc35d046d518c`.
Sorted relative-content manifest SHA-256:
`44b409bdaa546bcc5b71f318e6a38054fa336e0652f3d9b387757a9db1966a2c`.

The parent sanitizer binary was
`ff99646c923730f57e8c4503a21f73ce981cc192c4d102294e619cd7147614ba`.
The baseline replay exited 0 after 159 runs, coverage 326, feature count
1440, peak RSS 188 MiB, and no artifacts; log SHA-256:
`ec7316dffadb3bbdbaf684b6980f0081f817b392b445845500eb59766b669130`.
The parent normal binary was
`89ff25b6aa1aeef4798af464134e42a765b4a7c4899f188e1f9cecc3442d8f84`; its
one-file driver passed all 158 inputs, log SHA-256:
`c66309c81ff985b71637f61bb5c3f2c64f0d017e9adb49e2301fe48430d889d6`.

Final production source SHA-256:
`1ecdb687c2423ce94a97cca7ba12c42e0213587f1af1525ae1973a1c040cb39c`.
Final fuzzer source SHA-256:
`b8e484aa60bf78d603ded27136025ec353f367475acc0702e58057e9c189f052`.
The final sanitizer binary was
`8c34a81e7de98da046013b561c8f595ad8abfff2ce0c57e0af2bd6082c399f99`.
The full replay exited 0 after 159 runs, coverage 287, feature count 839,
peak RSS 398 MiB, and no artifacts; log SHA-256:
`9776ed994eb103beb734db346e3a77ac0f053fa1142d56e7265b81e9ad3418e7`.
A restored clean full replay had the same coverage/features, peak RSS 399 MiB,
and log SHA-256
`b8b14d5a78926f6021724592f4d3eb2b187a3aac1bd8d1ab702b73daba2a22cb`.
The final normal binary was
`0db6ad3950ba11a80ba18240da5e128ae6a67c7807d0f458b3cd2f9529166a47`; its
one-file driver passed all 158 inputs with log SHA-256
`c66309c81ff985b71637f61bb5c3f2c64f0d017e9adb49e2301fe48430d889d6`.

Four isolated sanitizer workers ran `-merge=0 -runs=1 -timeout=120
-rss_limit_mb=4096` against private copies. All exited 0 after 159 runs,
coverage 287, feature count 839, peak RSS 398-399 MiB, and no artifacts. The
worker log SHA-256 values, in order `fuzz-0` through `fuzz-3`, are:

    adf32d304d14e203d4162cf253400889ee95fae8032f8d6288df3091fd93806b
    e102d51b63be82d00f9ab17d094d272a9203d264414c11fcd6b8da1a4c6ac3c5
    bac1d841f2413d7ddbc91aa60ada2090ffbfaf9994c4fbf2b6aabcf8194a2d11
    c7d8ddb0ef69613c839a73ee469dc04a12671f2c5a5188041e3b8da20e80ec7c

### Differential proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation made `contains()` return true immediately
after its invariant check, modeling a false-positive lookup. Mutated
production source SHA-256:
`e95aa63a5dfae059ec905205255ff2487305a0ada728efafc05d406cb595b753`.
Mutated enhanced sanitizer binary SHA-256:
`4a42d236e0b9fae6e5cb4a21d48cadbd1b3fb93adb52f7fbb19083670cd41009`.

The exact proof input was the empty file at
`/tmp/bitcoin-cuckoocache-20260721/empty`, Base64 empty, SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
The enhanced mutation replay with `-handle_abrt=0` exited 134 at
`src/test/fuzz/cuckoocache.cpp:51` on the no-false-positive assertion, after
one execution and with no artifact; mutation log SHA-256:
`f26605f9309d16e9aad5dd181a93c7f254374286a3e061a98ee5731e849dc50a`.

The old-harness control retained the production mutation but restored only the
original fuzzer. Its source SHA-256 was
`f1b83cd1a114f43ceb8cf1353c09dae89284ea182036034be67d609cfe001a95`, binary
SHA-256 was
`731125fce3282f67d074b0724a80afff67480eda20003978de6e6297c97b5f0e`, and the
identical input exited 0 with no artifact; control log SHA-256:
`ac116e100f5ecd911ee96c932561550ac83e3818e17df0c5402fa378599382d4`.
After restoring both source files, the identical input exited 0 with no
artifact in sanitizer binary `8c34a81e7de98da046013b561c8f595ad8abfff2ce0c57e0af2bd6082c399f99`;
restored log SHA-256:
`0d1df4550b94cc54747fac2be6c58c48a445a73228e0d989325eeaf62a0a9434`.
The old harness therefore accepts the modeled false positive, while the new
oracle detects it and clean master does not reproduce it. No production bug or
deterministic regression test is claimed.

### Verification gap

Sanitizer and normal targets were built with the configured fuzz-only builds
using `cmake --build ... --target fuzz -j2`; the fuzz-only builds do not
provide `test_bitcoin`, so the dedicated CuckooCache unit suite was unavailable.
`git diff --check` passed. `clang-format --dry-run --Werror
src/test/fuzz/cuckoocache.cpp` reports only the pre-existing include-order
violation at line 5 (`#include <cuckoocache.h>`); unrelated formatting was not
changed. All temporary mutations and controls were restored, and no fuzz,
sanitizer, mutation, or build process remains running.

## `FlatFilePos` sentinel and serialization oracle audit (2026-07-21)

Source commit `f55a2f1318abc4d5f236b6eca2066a8ee62f517c` (`fuzz: assert
FlatFilePos sentinel and serialization contracts`) strengthens the flatfile
target with the contracts that distinguish an unavailable disk position from
a usable one. The production value constructor and `IsNull()` assert the
public file-number domain `nFile >= -1`; the harness checks the `-1` sentinel,
the exact `ToString()` form, and round-trips every non-null deserialized
position through the production serializer.

### Core boundary and severity

Bitcoin Core uses `FlatFilePos` in block storage and `FlatFileSeq`, in
`BlockFilterIndex`'s persisted filter position, and through `CDiskTxPos` in
`TxIndex` and `TxoSpenderIndex`. `BlockManager::OpenBlockFile` reaches
`FlatFileSeq::Open`, which relies on `IsNull()` to reject an unavailable disk
position. A real predicate regression could redirect a null position into a
numbered file path or make missing block data appear usable; the modeled
impact is Medium/High depending on the reachable caller and state effect.

Clean master reproduced no production failure, corrupt file, or
caller-reachable state violation. The master-relative rating of this commit
is therefore Informational/Low oracle hardening, not a production bug, fix,
or deterministic regression test. Arbitrary invalid block bytes do not
directly construct this internal file-position state, so they are not a
Critical finding here. Current Core persistence paths serialize only usable
nonnegative positions. The serializer's non-null-only behavior for the `-1`
in-memory sentinel was considered and deliberately excluded from the
round-trip oracle. A nonce without cryptographic meaning is not Critical
merely because it is not cleared.

The audit base is `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolve to it, and the source
branch was rebased onto latest master before this audit sequence. The l0rinc
pull-request history was reviewed for a target-specific prerequisite or fix;
no additional relevant commit was cherry-picked for FlatFilePos. No later fix
or cherry-pick was used to mask or alter clean-master behavior. Any later
change to the null sentinel, serialization, file-open reachability, or proof
input must amend its commit message and this ledger with whether it preserves,
changes, or masks the result, and must repeat the clean-master replay.

The reiterated master-relative ledger is: **Medium**, feature-conditional
private-broadcast failed-send retention; **Medium**, empty-HEADERS
initial-sync availability/IBD risk; **Low** under current Core callers for
peer transaction-activity refresh, process-message block-storage failure,
and oversized transport types; **Medium but latent/reachability-limited** for
ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for banman invalid-subnet
and unban integrity. No additional clean-master bug was found in the already
audited addrman, coins-cache, coins-view, txgraph, txdownloadman, txrequest,
connman, eviction, handshake, compact-block, headers-sync, UTXO snapshot,
mempool persistence, package evaluation, RPC, descriptor cache, threadpool,
cluster-linearization, policy estimator, pool resource, versionbits,
signature-cache, or CuckooCache paths. Severity is based on actual Bitcoin
Core callers and input origins; invalid fuzzer state or invalid block bytes
alone is not Critical.

### Oracle contracts and corpus

The production assertions reject file numbers below the null sentinel before
path construction or file access. The harness always checks default and
explicit `FlatFilePos{-1, 0}` null values, equality of the sentinel state,
the exact `FlatFilePos(nFile=..., nPos=...)` representation, and non-null
serialization round trips. Null positions are intentionally excluded from
the serializer round trip because `VARINT_MODE(... NONNEGATIVE_SIGNED)` does
not preserve the negative in-memory sentinel and Core persists only usable
positions; treating that mismatch as an unconditional acceptance contract
would be an overbroad fuzzer oracle.

The frozen corpus was copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/flatfile` to
`/tmp/bitcoin-flatfile-20260721/frozen`: 36 files and 1,974 bytes. Sorted
relative filename SHA-256 is
`c647726c76d69bdc36e5eecf8f1754238d4e588b8a770b1f512e339f475ebfae`; sorted
filename-plus-size SHA-256 is
`0d1c13c95361eee809ef66ece6ba9d9bbfca75c67fed3e4b0f65aedf4751907c`; the
sorted relative-content manifest SHA-256 is
`829e18fe66f1f7d4ffd8ca37106ea591b662862f2bafd7041f7d1af20a3e80`.

The parent sanitizer binary was
`ccc0e9a8576dce3c84f4fc3d286f1e34cd6cb06d1fe7685a34863d0706dd3f7f`; its
execution-only replay exited 0 after 37 runs, coverage 216, feature count
414, peak RSS 97 MiB, and no artifacts; log SHA-256:
`354110cfaeb0ec19068b1a4334971c739d1af46bb2b6faf644d60f172b0cad83`.
The final clean source hashes are `src/flatfile.h`
`5f6827fbf73e76b7d1a8b2352744fb67283df7066d0cfc592dc4612268e85dd5` and
`src/test/fuzz/flatfile.cpp`
`e522b9991bbe6bb750d091d735ca483274afc264cd0bb2088c5ea1caec6c81b0`.
The final sanitizer binary is
`4dd2a5a5d368fcb5a15db93e339bcdbdb8267904de077a34e480427df8fa47f4`; the
clean frozen replay exited 0 after 37 runs, coverage 368, feature count 984,
peak RSS 97 MiB, and no artifacts; log SHA-256:
`4d74f7c8eaa84684f6ce449c9ca235950f233e76cae0e4a496a86579be5e48f6`.
The final normal binary is
`97c39bd846e1ffad8bbf4ecd24f485642bf64b2f3d4c1e7c5891f6c7eca81c59` and
passed all 36 inputs with log SHA-256
`85058f33d70ffa9e11134c8541113bd02da93ac8572a18dc1964fb1435044853`.

Four isolated sanitizer workers each exited 0 after 37 runs with coverage
368, feature count 984, peak RSS 97-98 MiB, and no artifacts. Worker log
SHA-256 values, in order `fuzz-0` through `fuzz-3`, are:

    075737c533f3c01708902a69647757cdc2fa0606dd3ce364db3322e8825bd73e
    c7d808bb7ce8e2453d82e0c3ea969db930c5e7c70b2a67cb506fc0da4e819136
    4fbc0195c86c9a782e6fbb544ef9c19d3abb42fbfe208d970db37b671baeaacb
    fb7b253edd4bbf0cbe73ec847ae6f33aa7e8537b3ff11d8ba74b4e51aadec6c9

### Differential proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation changed `FlatFilePos::IsNull()` from
`return nFile == -1` to `return false`, modeling a null-predicate regression.
The mutated production source SHA-256 is
`eeb57515861a0c69723ed0c7f3091c7084535628ef1a9567ee61d93cd073ea8c`; the
mutated enhanced sanitizer binary SHA-256 is
`bdd58aab49dcb0c27f91c8fc4875d9b905dce0e91ff0045d25efc83c3fe7bd2e`.

The exact witness was the empty file at
`/tmp/bitcoin-flatfile-20260721/empty`, Base64 empty, SHA-256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
The enhanced mutation replay with `-runs=1 -handle_abrt=0` exited 134 at
`src/test/fuzz/flatfile.cpp:21`, on
`pos.IsNull() == (pos.nFile == -1)`, after one execution and with no artifact;
mutation log SHA-256:
`cca8528f41c9c28366c99e989bfe541b5a1fab36c727099b5f2f796b33a21c24`.

The old-harness control retained the production mutation but restored only
the parent fuzzer. Its source SHA-256 was
`4714c2d89843f55fa5271951f80e64e8ea1475d9bedeeee5aa713118ff843db5`, its
sanitizer binary SHA-256 was
`12d4215dbbcffaa28062a048b56c65d48d753ad2cdda470aeb5551179766d671`, and
the identical input exited 0 with no artifact; control log SHA-256:
`54b2301531b3d0b495e9bfec613bd379d7bfda5b20dc8b126bcc106219ad0594`.
After restoring both sources, the identical empty input exited 0 with no
artifact in final sanitizer binary
`4dd2a5a5d368fcb5a15db93e339bcdbdb8267904de077a34e480427df8fa47f4`;
restored log SHA-256:
`7223447cd4f011b6647fed763d5c83eb4ee100d5d78a628f9fa15637e62dbef9`.
The new oracle therefore detects the modeled predicate regression that the
old harness accepts, while clean master does not reproduce it. No production
bug or deterministic regression test is claimed.

### Verification gap

Sanitizer and normal targets were built with the configured fuzz-only builds
using `cmake --build ... --target fuzz -j2`; the normal verifier ran one input
per corpus file because its non-sanitizer driver does not accept libFuzzer
corpus options. `git diff --check` passed. `clang-format --dry-run --Werror
src/test/fuzz/flatfile.cpp` reports only the pre-existing include-order
violation at line 5 (`#include <flatfile.h>`); unrelated formatting was not
changed. The fuzz-only builds do not provide `test_bitcoin`, so the dedicated
FlatFilePos unit suite was unavailable. All temporary mutations and controls
were restored, and no fuzz, sanitizer, mutation, or build process remains
running.

## `prevector` representation and transition oracle audit (2026-07-21)

Source commit `b3abd1e1408e02961dfcf37d622fe646810dbb70` (`fuzz: assert
prevector representation and transition contracts`) strengthens the generic
prevector target with direct/indirect representation assertions, capacity and
pointer contracts, per-transition checks, alternate-buffer checks, and a
second model at Bitcoin Core's actual `CScriptBase` boundary.

### Core boundary and severity

`src/script/script.h:400` defines `CScriptBase` as
`prevector<36, uint8_t>`, and `CScript` is the serialized script type used in
transaction inputs and outputs. `src/serialize.h:863-891` serializes and
deserializes prevector values, including `resize_uninitialized()` and writable
byte spans. Core also uses prevector for compressed scripts, onion-address
bytes, network selection, and dynamic-memory accounting through
`src/memusage.h:106-110` and `allocated_memory()`.

Invalid transaction or block bytes can reach these deserialization paths, but
invalid block bytes alone do not prove an internal prevector invariant failure.
Clean master reproduced no production failure, data corruption, memory error,
consensus divergence, or caller-reachable state violation. The master-relative
classification is therefore **Informational/Low oracle hardening**, not a
production vulnerability, fix, or deterministic regression test. The modeled
`allocated_memory() -> 0` mutation only undercounts indirect storage in
`DynamicUsage`, so its current caller impact is Low diagnostic/resource
accounting risk. A data-preservation or representation bug could be High or
Critical if it changed script validation, hashes, or memory safety, but no such
master bug was found. Invalid fuzzer state or an invalid block alone is not
Critical, and a nonce without cryptographic meaning is not Critical merely
because it is not cleared.

The audit base is `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolved to it, and the source
branch was rebased onto latest master before this sequence. The l0rinc history
query for `src/prevector.h`, `src/test/fuzz/prevector.cpp`, and
`src/script/script.h` was empty, so no target-specific l0rinc commit was
cherry-picked. No later fix or cherry-pick masked clean-master behavior. Any
later change to prevector representation, CScript serialization,
memory-accounting behavior, or the proof input must amend its commit message
and this ledger with whether it preserves, changes, or masks the result, and
repeat the clean-master replay, exact witness, and mutated-production control.

The reiterated master-relative ledger remains: **Medium**,
feature-conditional private-broadcast failed-send retention; **Medium**,
empty-HEADERS initial-sync availability/IBD risk; **Low** under current Core
callers for peer transaction-activity refresh, process-message block-storage
failure, and oversized transport types; **Medium but latent/reachability-
limited** for ecmult scratch wrapping, forced 10x26 magnitude-32
normalization, and SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for
banman invalid-subnet and unban integrity. No additional clean-master bug was
found in the audited addrman, coins-cache, coins-view, txgraph, txdownloadman,
txrequest, connman, eviction, handshake, compact-block, headers-sync, UTXO
snapshot, mempool persistence, package evaluation, RPC, descriptor cache,
threadpool, cluster-linearization, policy estimator, pool resource,
versionbits, signature-cache, CuckooCache, or FlatFilePos paths. Severity is
based on actual Bitcoin Core callers and input origins.

### Oracle contracts and corpus

`src/prevector.h` now checks the direct/indirect encoding at capacity changes,
pointer access, capacity queries, move/copy transitions, swap, and destruction:
direct storage has `_size <= N`; indirect storage has a non-null allocation,
`capacity > N`, and logical size no greater than capacity. The harness checks
size/empty agreement, capacity bounds, exact indirect `allocated_memory()`,
`data()`/`begin()` and `end()`/`data()+size` pointer contracts, and front/back
agreement after every operation. Final checks cover both primary and
alternate buffers, element addresses, iterator arithmetic, forward/reverse
traversal, and serialization equivalence. `shrink_to_fit()` requires
`capacity == max(N, size)`, preserving inline capacity when `size <= N`.

The target runs `prevector<8, int>` to preserve the existing transition corpus
and `prevector<CScriptBase::STATIC_SIZE, uint8_t>` to exercise Core's actual
script representation. The old target performed one final content and
serialization check only for `prevector<8, int>`.

The frozen corpus was copied from
`/mnt/my_storage/qa-assets/fuzz_corpora/prevector` to
`/tmp/bitcoin-prevector-20260721/frozen`: 241 files and 898,976 bytes. Sorted
relative filename SHA-256 is
`d8125a24460e0d6db636c824d13ddff13e176a7780359da33f6f093412e71ee2`;
filename-plus-size SHA-256 is
`69fcb8093f5eba4bf2ac77370eb3ba6c5ea47025eb6d9190b3c0dbaa2478d68f`; and
the relative-content manifest SHA-256 is
`0a6c439bcb96bcbdbef05f7e6f81f850924ac0ac1881566d29cc63cd74853090`.

The parent sanitizer binary was
`4dd2a5a5d368fcb5a15db93e339bcdbdb8267904de077a34e480427df8fa47f4`.
Its old-harness baseline exited 0 after 242 executions, coverage 885,
features 5,751, peak RSS 572 MiB, and no artifacts; log SHA-256:
`033d6055ff3980d9f80b57e1dd7cca81eaf0ebf40cc0331f73b907bdfb958ad5`.
The parent normal binary was
`97c39bd846e1ffad8bbf4ecd24f485642bf64b2f3d4c1e7c5891f6c7eca81c59`; its
one-file driver passed all 241 inputs; log SHA-256:
`02a7b3a86d2f4d6014c8456af5348162d286905fd433116b7443dcf0825d081b`.

Final source SHA-256 values are `src/prevector.h`
`76e704f34dda4fa02dd4d7cf2769d717b867b6a720c85934892d011269f584a4` and
`src/test/fuzz/prevector.cpp`
`190925e9761ce5ad50df06cf97e7847eca15d1650e44b86ca4c285a3b6d41d78`.
The final sanitizer binary is
`cd43bf93ad20199331178a1d2cf3d7e00dad7c29717e15b0491ac95d9a49ab8c`. The
restored full replay exited 0 after 242 executions, coverage 1,546, features
10,490, peak RSS 646 MiB, and no artifacts; log SHA-256:
`b8cb6dd139ae79721b4361cc664b2ab415fe17f83355a13690f64cc1eac72391`.
The final normal binary is
`f6289dbea21d2cae5777465bf42e004b897d2c4cc346e7c67c90f122debbbf99`; its
restored one-file replay passed all 241 inputs with no artifacts; log SHA-256:
`02a7b3a86d2f4d6014c8456af5348162d286905fd433116b7443dcf0825d081b`.

Four isolated sanitizer workers each exited 0 after 242 executions, with
coverage 1,546, features 10,490, peak RSS 646-647 MiB, and no artifacts. The
parent worker log SHA-256 is
`52938de7701409d0fba40f4a9bec30a38cc3c75b7c9452e7253aaff61b35dfe1`.
Worker logs, in order `fuzz-0` through `fuzz-3`, are:

    ebd826135c9c9969fa13844c6026a26ee8521172c6d394b6db8f7c1e35ef75f9
    781cd9d4f4b4e69c4ff7b33ec08311c8d4d068a0319252b77952f7f300d1da43
    b6460ab49c75b90053e3dfcaf5382c68c57feff76cffc3316f192cf579ff48cd
    517fa026c546aa16af3f581f56c0a902fd6021cdf4d967850b1ab480b5da59e9

### Differential proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation changed the indirect branch of
`prevector::allocated_memory()` to return `0`, modeling lost indirect memory
accounting. Mutated production source SHA-256 is
`896dd02436ff9ced86e1ff751e6ddda36046497ec2dbbe4c9ba4ad2524e49902`; the
mutated enhanced sanitizer binary SHA-256 is
`a5e82c72463ec1913186ca90f23ce8dc8d2278f5ef1528e143342376ff815d25`.

The exact witness is
`/tmp/bitcoin-prevector-20260721/frozen/04092d36e2ce88b4563db8b37d3ee5a498d5bcba`:
two bytes, hex `ffc0`, Base64 `/8=`, SHA-256
`36cf7346f0d0c76f16dd999870c6db6c7dd76557592e00b519bcefb73f0a80f6`.
The enhanced mutation replay exited 134 at
`src/test/fuzz/prevector.cpp:38` on the `allocated_memory()` postcondition,
after one execution and with no artifact; log SHA-256:
`4905ae1cf96e268c8d34e21f6ab1110a49e6d188d57287f732cff827b5899a4b`.

The exact parent fuzzer control retained the same production mutation. Its
fuzzer source SHA-256 was
`44a6b68e157ef472276d30b41b7a32ea43e5bffa2246dbb195929b77905244dc`, its
sanitizer binary SHA-256 was
`1a6d1570461a7f6bc56dac16a06877a9fa301feab43c7edb81265d937bba46ac`, and
the identical input exited 0 with no artifact; control log SHA-256:
`ba70882405c81b9797659ce61f0605e57565c6427a43ef5c87cdd190dd7e5dfa`.
After restoring both sources, the identical witness exited 0 with no artifact
in final sanitizer binary
`cd43bf93ad20199331178a1d2cf3d7e00dad7c29717e15b0491ac95d9a49ab8c`;
restored witness log SHA-256:
`dad8cac928e1be94cd6aec17f14c6e859700da844d507547e6717d4bffcfa33d`.
The full restored corpus also passed. This proves that the new oracle detects
the modeled accounting regression the parent harness accepts, while clean
master does not reproduce it. No production bug or deterministic regression
test is claimed.

An earlier draft asserted `shrink_to_fit()` capacity equal to size
unconditionally. Clean replay exposed that as an overbroad oracle for inline
storage, where capacity remains `N` when size is smaller; the assertion was
corrected to `capacity == max(N, size)`. The failed partial log SHA-256 was
`87e7d27d88ec16d0c7dede5d0ac296e1a0d80772e179597073dd27d2d42fb896`.
This was a harness correction, not a production finding.

### Verification gap

Sanitizer and normal targets were built with the configured fuzz-only builds
using `cmake --build ... --target fuzz -j2`; restored sanitizer build log
SHA-256 is `257a8940b778892489e78408bc7b2bf3867dce34df572cfcd62629ecdaaf3cd0`
and the final normal incremental build log SHA-256 is
`30edc0a59ade1119f97578482f343f9e8d73a385c2e0fb78f59ba6b61712c302`.
`git diff --check` passed. `clang-format --dry-run --Werror
src/test/fuzz/prevector.cpp` reports only the pre-existing include-order
violation at line 5 (`#include <prevector.h>`); formatter log SHA-256:
`48d42f59cccf9728e2704d8f0e01f4acb8a4754f53d72b01b7ce5a6ef79b1c34`.
The configured fuzz-only builds do not provide `test_bitcoin`, so the
dedicated prevector unit suite was unavailable. The normal replay used one
input per process because its driver does not accept libFuzzer corpus options.
All temporary mutations and controls were restored, and no fuzz, sanitizer,
mutation, or build process remains running.

## `bitdeque` moved-from and transition oracle audit (2026-07-21)

Source commit `26cba88343d701e31c2447d6c4d0b6d5f9dddd05` (`util: preserve
bitdeque moved-from invariants`) fixes a clean-master moved-from state bug,
adds production representation assertions, adds a deterministic regression,
and turns the existing target into a transition oracle.

### Finding, Core boundary, and severity

The defaulted move constructor and move assignment copied
`m_pad_begin`/`m_pad_end` while the backing `std::deque<word_type>` moved out.
A moved-from object could report an underflowed `size()` and crash when
reused. A 129-bit deque is enough to make the stale padding observable.

Bitcoin Core stores the default `bitdeque<>` in
`src/headerssync.h:235` as `HeadersSyncState::m_header_commitments`.
`src/headerssync.cpp:206` pushes one-bit commitments during PRESYNC,
`:266-274` reads and pops them during REDOWNLOAD, and `:54` clears and shrinks
the deque at finalization. `HeadersSyncState` is owned through a `unique_ptr`
in `src/net_processing.cpp:401,2839`; the current Core caller does not move
and reuse this member. Invalid peer headers can exercise push/size/front/pop,
but invalid block or header bytes do not trigger the moved-from path.

Clean master reproduces the generic library bug, so this is a confirmed
production correctness issue. Based on current Bitcoin Core reachability, the
master-relative severity is **Low/nice-to-have API hardening**, not High or
Critical network, consensus, or invalid-block impact. A future caller that
moves and reuses the member would require a new caller-specific crash/DoS
rating. An invalid fuzzer state or invalid block alone is not Critical, and a
nonce without cryptographic meaning is not Critical merely because it is not
cleared.

The reiterated master-relative ledger remains: **Medium**,
feature-conditional private-broadcast failed-send retention; **Medium**,
empty-HEADERS initial-sync availability/IBD risk; **Low** under current Core
callers for peer transaction-activity refresh, process-message block-storage
failure, and oversized transport types; **Medium but latent/reachability-
limited** for ecmult scratch wrapping, forced 10x26 magnitude-32
normalization, and SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for
banman invalid-subnet and unban integrity. No additional clean-master bug was
found in the already audited addrman, coins-cache, coins-view, txgraph,
txdownloadman, txrequest, connman, eviction, handshake, compact-block,
headers-sync, UTXO snapshot, mempool persistence, package evaluation, RPC,
descriptor cache, threadpool, cluster-linearization, policy estimator, pool
resource, versionbits, signature-cache, CuckooCache, FlatFilePos, or
prevector paths. Severity is based on actual Bitcoin Core callers and input
origins, not on an assertion failure alone.

The audit base was `18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; both
`origin/master` and `remotes/l0rinc/master` resolved to it before this
sequence, and the source branch was rebased onto latest master. The l0rinc
history query for `src/util/bitdeque.h`, `src/test/fuzz/bitdeque.cpp`,
`src/headerssync.cpp`, and `src/headerssync.h` returned no commits, so no
relevant fork commit was cherry-picked. No later fix or cherry-pick masked the
clean-master result. Any later change to bitdeque move semantics, padding,
iterators, HeadersSyncState commitment handling, or the proof input must amend
the relevant source and evidence commit messages with whether it preserves,
changes, or masks this result, then repeat the clean-master witness, parent
control, deterministic test, and corpus replay.

### Contracts and regression test

`src/util/bitdeque.h:134-145` adds `assert_valid()`: empty backing storage
requires zero padding, each padding counter stays within one word, and a
single backing word cannot have padding sum greater than `BITS_PER_WORD`.
Multiple backing words may legitimately have both ends padded while retaining
live bits, so the sum bound is intentionally limited to the one-word case.
The assertion is checked around front/back extension and erasure, insertion,
assignment, iterators, size/empty/max_size, shrink, clear, swap, and moves.

`src/util/bitdeque.h:263-284` exchanges both source padding counters to zero
in the move constructor and move assignment, validates both objects, and
preserves self-move assignment. `src/test/fuzz/bitdeque.cpp:38-56` checks
size, empty, max_size, iterator distances, front/back, and sampled logical
positions after initialization and every operation while retaining a full
final `std::deque<bool>` comparison. Lines `58-85` exercise move construction,
move assignment, source reuse, and self-move at a 129-bit boundary.
`src/test/util_tests.cpp:124-149` adds `bitdeque_move_state` as a deterministic
regression.

### Corpus and clean baseline

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/bitdeque` to
`/tmp/bitcoin-bitdeque-20260721/frozen`: 931 files and 2,009,361 bytes.
Manifest SHA-256 values are:

- Sorted filenames:
  `2d1e9c686aedac760d5a0d5d5abad6b55a8165794fc039ddcddcabab18ed6f0d`
- Filename plus size:
  `7feedf165b993e18434223b9f200d952c25f84f8476b8c6a3c1fb4c00fc5aca5`
- Relative contents:
  `2e903f3661092ee0ab2cf8a25ed3addd724307a74cfc4a4b121e07f26c46991d`

The parent bitdeque header and fuzzer SHA-256 values were
`99f8a473ee4437c88d74dec2bcc3eab282266201233f793f012d1975f21d0bb8` and
`091e8295c4bbc123ce41ce57be97ab4eac1b78c35001b5d9f5d835f61b09b8cd`.
Parent sanitizer binary
`cd43bf93ad20199331178a1d2cf3d7e00dad7c29717e15b0491ac95d9a49ab8c`
passed the old harness after 932 executions with coverage 1850, features
14300, peak RSS 478 MiB, and no artifacts; log SHA-256:
`b01bf9aeb4c084acf09435faa4bf6ff086c73fc476d4b550841b024ff27c9141`.
The parent normal binary
`f6289dbea21d2cae5777465bf42e004b897d2c4cc346e7c67c90f122debbbf99`
passed all 931 one-file replays; log SHA-256:
`19a7d35e2d3ece54e8bd4e46793d0eca9c80b9ee82a0794cd371362674eaeb68`.

### Differential proof

The exact witness is one byte, hex `01`, SHA-256
`4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a`.
It selects the moved-from contract probe without changing the existing
operation provider.

A direct clean-master probe used source SHA-256
`8e6075c9cd5e29190f055865c5b34a732e415ef2ccff1e80c3ae82e2f473bd8d` and
binary SHA-256
`d8b0ada30f8b73d185aff347d48555a09417a97f7591414276120e2b889ad0c2`.
Reusing the moved-from source exited 139 with a segmentation fault; log
SHA-256:
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.

The enhanced harness with clean-master production used fuzzer source SHA-256
`f3a3ac4121426ed4df193bf99fc977e9683fe8c51153fdff0d7f1542335a8e37` and
sanitizer binary SHA-256
`bce227a2ea78ceeb55567b55b8c4fd72d148f05f3f44fec0fe977d9de091c19b`.
The same `01` input exited 134 at
`src/test/fuzz/bitdeque.cpp:65` on `source.size() == 0`; log SHA-256:
`2f4e8ba77ea5378eceac7ba67ef978d38467286ca34ae5a2fed4584617b2b628`.
The unchanged parent fuzzer with clean-master production used sanitizer
binary SHA-256
`57c5a43464d5e4755dff718f77d1c443c0b8c32912dda0ee326e5a516f196173`.
The identical input exited 0 with no artifact; log SHA-256:
`747fcc41f5d32edfc5375f996f0cecd1be2b616e80fcbb2dfc45c75e139db16f`.
This proves a real clean-master production bug, rather than a harness-only
finding or a hypothetical mutation.

### Final replay and limits

Final source SHA-256 values are `src/util/bitdeque.h`
`e7b5e86644878ad06f8ce218c998f0c46da3233cc9627627b2daf54b2cc7bcbb`,
`src/test/fuzz/bitdeque.cpp`
`f3a3ac4121426ed4df193bf99fc977e9683fe8c51153fdff0d7f1542335a8e37`, and
`src/test/util_tests.cpp`
`6daa4e7ec2db1ac8f1df7f4eac70f26b1c6fb123f78322b0d1e55c9c02cd030e`.

The final sanitizer binary
`353176b7419abd8e7ef14af8a798cfa7b1efd3f7d3dec96934a5c78aabced765`
passed the frozen corpus after 932 executions with coverage 1899, features
14579, peak RSS 478 MiB, and no artifacts; log SHA-256:
`be962759c38e254618c0781c557d60a27d857a4543af6df600960209bac85351`.
The final normal binary
`39ea68ad9e944281b4cf9db56ba6531017b58ed5200ea0d278a4ffacb2bb382a`
passed all 931 one-file inputs; log SHA-256:
`b614acfa8cc02474e006ac4bbb735838a86568d46a3670daac9dea3ffcdff602`.
The exact witness passed in the final sanitizer binary with no artifact; log
SHA-256:
`4273ca032f1f7698e677fd3a12638da298834e7d640ae1dc98d1d58f0c813a7f`.

Four isolated sanitizer workers each passed 932 executions with coverage
1899, features 14579, peak RSS 478 MiB, and no artifacts. Parent worker log
SHA-256:
`4c06d485a3a0b7d2bb1e4eed81bff141e642004f21a9462ce3258e5b4a3d48a3`.
Worker logs in order `fuzz-0` through `fuzz-3`:

    7fce60b3f248224e70d1580f4dbd0620dbdc10f93ee5b4af71be7cdd15c84170
    cbee19ecd376aef0fe1fb3c0cfe52e46a7672973ad34936ef36ca064f4a9c751
    cce625428feb519fe0cec3b4ad9411e9a7ea744c2298e038f16edd23f98beae4
    123c97b2b46f2c61e3f703d2f6de74ccdea0cf0d55106575380ba21e0c426817

The dedicated unit test passed with no errors. Its final `test_bitcoin`
binary SHA-256 is
`c5197d4066f9be402af0fcf3f187d3cf4e61cb27d766ca0274014869a4fa07df` and
the final unit log SHA-256 is
`250baa69a4213c4e159bb31da85588c5352c319dbb5666982b66259c7b812b0c`.
The test build used `BUILD_FOR_FUZZING=OFF`, `BUILD_TESTS=ON`,
`BUILD_GUI=OFF`, `BUILD_BENCH=OFF`, and `WITH_ZMQ=OFF`.

An earlier draft asserted `m_pad_begin + m_pad_end <= BITS_PER_WORD` for
every nonempty backing deque. Clean replay showed that this is overbroad for
multiple backing words. The first failing corpus input was
`fd26003a81c8a64ec7a652b079037877d0b11841`, 939 bytes, SHA-256
`02cbc506ab9d80abef453ad606e5f38007049f8ca2c7e73740b68443057d4564`.
The broad-assertion sanitizer binary was
`43a9f20396bd93d3116f7436e1249f156d4a10756291f5280adb3755b73d1f24`;
partial sanitizer and normal logs were
`9b078d8c5d88f42528f64e63a7bf6754a51799dd86948acc6fc4e3710ab66515` and
`acd924618603e90f2392e489a3aebb511450d0bd78d3cdb90770ee278bc57773d`.
This was an oracle correction, not a production finding; the one-word-only
condition was then replayed across the complete corpus.

Final sanitizer, normal, and test build log SHA-256 values are
`12ff8e6b4d73121a65d63d8705021f37cb1a43f85d51e6a2a788192103eb954a`,
`f49fadae7efc059340cfce19fb3a5fac631f8409cc613ebe0cee1bdd44ae1964`, and
`768be2ad166a6d68a50daa62963ea86c897e5817665b4fab6bb6de04f0c8e6f5`.
`git diff --check` passed. The configured fuzz-only builds do not provide
`test_bitcoin`; the separate non-fuzz build supplied the deterministic test.
`clang-format --dry-run --Werror` still reports pre-existing file-wide
violations in these legacy files; no unrelated formatting was changed. All
temporary mutations, parent controls, probes, workers, and builds were
restored or stopped, and no fuzz process remains running.

## `VecDeque` representation and move oracle audit (2026-07-21)

Source commit `111d79d1239b61c525c7932983ee5390ae87e66b` (`fuzz: enforce
VecDeque representation and move contracts`) strengthens `FUZZ=vecdeque` and
adds matching production representation checks. The untouched master
implementation and its corpus replay showed no clean-master `VecDeque`
production bug; this commit claims no production vulnerability and makes no
production behavior fix.

### Core boundary and severity

`VecDeque` backs the two FIFO queues in `src/cluster_linearize.h:774-781`.
The current Core path reaches them through
`GenericClusterImpl::Relinearize` at `src/txgraph.cpp:2157-2182`, from
internal mempool and block-building cluster work. The fuzzer consumes encoded
internal states, not peer messages or block bytes.

Rate this result **Informational/Low oracle hardening**. A future corruption
of a cluster queue could affect mempool linearization or availability, but an
invalid block cannot directly trigger this container contract and there is no
consensus, memory-safety, cryptographic, High, or Critical finding on master.
Re-rate only if a Core caller establishes a triggerable impact. A nonce with
no cryptographic meaning is not Critical merely because it is not cleared.

The audit base was
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`; `origin/master` and
`remotes/l0rinc/master` matched it before the sequence. The l0rinc query for
`src/util/vecdeque.h`, `src/test/fuzz/vecdeque.cpp`,
`src/cluster_linearize.h`, `src/txgraph.cpp`, `src/txgraph.h`, and related
mempool paths returned no relevant commits, so nothing was cherry-picked. No
later fix or cherry-pick was allowed to mask this clean-master result. Any
later potential fix or cherry-pick touching `VecDeque`, cluster queues, or the
witness must amend the relevant commit message with whether it preserves,
changes, or masks this result, then repeat the parent control, mutation
witness, unit test, and corpus replays.

### Reiterated findings

The master-relative ledger remains: **Medium**, feature-conditional
private-broadcast failed-send retention; **Medium**, empty-HEADERS
initial-sync availability/IBD risk; **Low** under current Core callers for
peer transaction-activity refresh, ProcessMessage block-storage failure, and
oversized transport types; **Medium but latent/reachability-limited** for
ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
SHA/HMAC/RFC6979 retention; and **Low/nice-to-have** for banman invalid-subnet
and unban integrity. No additional clean-master production bug was found in
the already audited addrman, coins-cache, coins-view, txgraph, txdownloadman,
txrequest, connman, eviction, handshake, compact-block, headers-sync, UTXO
snapshot, mempool persistence, package evaluation, RPC, descriptor cache,
threadpool, cluster-linearization, policy estimator, pool resource,
versionbits, signature-cache, CuckooCache, FlatFilePos, prevector, or
bitdeque paths. Severity is tied to actual Bitcoin Core callers and input
origins, not an isolated assertion failure.

### Contracts and tests

`src/util/vecdeque.h` adds `AssertValid()` for `size <= capacity`,
null-buffer/zero-capacity pairing, and valid ring offsets. It is checked at
allocation, resize, destruction, copy/swap/move, insertion/removal,
reservation, access, comparison, and transition boundaries. These are
representation contracts, not assumptions about user input.

`src/test/fuzz/vecdeque.cpp` reuses moved-from sources after move construction
and move assignment, checks self-move preservation, and compares the model
after every completed operation. The moved-from check requires only a valid,
reusable object; it intentionally does not require unspecified contents or
capacity. The existing uint and `TrackedObj` models remain in use.

`src/test/util_tests.cpp` adds `vecdeque_move_state`, covering wrapped
contents, move construction, source reuse, move assignment, source clearing
before reuse, and self-move. This is a deterministic API-contract test, not a
claimed production regression for clean master.

### Corpus and baseline

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/vecdeque` to
`/tmp/bitcoin-vecdeque-20260721/frozen`: 437 files and 1,052,870 bytes.
Manifest SHA-256 values are:

- Sorted filenames:
  `13269615ee55e8c0ed73bd16e48fb919ba364d6823f70bf0b2865e2d7c8a1ea2`
- Filename plus size:
  `8cb9cf2baac07e72c24f21eaebecb439e4d0145b978fa2b6d8e942dfd551eb29`
- Relative contents:
  `e2f1aa427d5abf0896a786a2df7b11778490208336eed47414086f0fe0bb13f1`

Parent source SHA-256 values were `vecdeque.h`
`afbb4908912203e75ff5fd098d431ad7c053022e346a2f2ed03640e57e8921e5` and
`test/fuzz/vecdeque.cpp`
`90d749a1606908cf8f689b0a16ccb70286a7f658e635b18d76e00812a266bac3`.
The parent sanitizer binary was
`353176b7419abd8e7ef14af8a798cfa7b1efd3f7d3dec96934a5c78aabced765`.
Its untouched replay processed 438 executions, coverage 6761, features
49715, peak RSS 568 MiB, and no artifacts; log SHA-256:
`3df29a982c31c0280b48301b3863a5abd944eb7015c68281675050199d6846d9`.

### Differential oracle proof

A temporary production mutation removed the `pop_front()` wraparound:
`if (m_offset == m_capacity) m_offset = 0;`.
The mutated header SHA-256 was
`a2e6d8dd1720f17ac97088b9bfbb67ef05cb32dcb791fce07691195671f4b283`, and
the mutated sanitizer binary SHA-256 was
`3ba927745237f24dcce1ebd52942f97ac4e2393446b24515f39c496ca0c3d048`.

The deterministic witness is 11 bytes,
`15 11 00 00 00 00 00 00 00 00 00`, SHA-256
`79525d2e72cba3e17652e3a1a73f893a2690e022a8e996530a9372bafe674a71`.
Because `FuzzedDataProvider` consumes from the end, it creates a default
buffer, emplaces at the front, and then pops the front. The mutated build
aborts at `AssertValid()` on `m_offset < m_capacity`; log SHA-256:
`21afe658b1de791c44cc65169d946af5dfb4c0a573eac11c1160406b4b44ed38`.
This proves immediate transition detection for a broken ring-buffer
mutation. It is oracle proof only: the mutation is not present on master and
no clean-master production bug is claimed. The temporary mutation was
restored before the final build and replay.

### Final verification

Final source SHA-256 values are `vecdeque.h`
`8cfb6824fb0b956f99aba1b368a2f2fe8f7efa24040aec5615892b10b270182e`,
`test/fuzz/vecdeque.cpp`
`22760a0ecb40684d17475fa08da715972a3aa6278417ab9d399f4c76d538dccb`, and
`test/util_tests.cpp`
`034f77628c78e7414d9e230bd6d0432201e8e017a0be07e80b500e34be60ec41`.

The final sanitizer binary
`8e5f162d5ae376514ab976cc077e6b3cb1bef6a099171572c36c2ad53d699e18`
passed 438 executions with coverage 6958, features 51231, peak RSS 541 MiB,
and no artifacts; log SHA-256:
`f80a5f3c3c63738e5bee1de4fd35b6e5101f62cfaf3d5f22ff9a6f0a2e021571`.
The final normal binary
`d3ee3606016e668ce277760170c0a1d6e52f179f7946a21c8ede5bda438e469c`
passed all 437 one-file inputs; log SHA-256:
`d3842e9ef3d8e18986369c7425fa2a8abf8ed9825984c1028e8a5742af9441ab`.
The restored final build passed the exact witness with no artifact; log
SHA-256:
`ca1ae45ede07fcb83546498fd642c34ccb0b307fb84e6edcf7f1b08ecc78ee86`.

Four sanitizer workers each passed 438 executions with no artifacts. Combined
worker log SHA-256:
`f1f98b20a628aa4532b260248ef6008ec495b6510f5f0421753c8b2eedd2bb24`.
Worker logs in `fuzz-0` through `fuzz-3` order:

    087033b8877704e755191bb1a1c4ae8d5e32a0b34314deb8c485466aa917f138
    19e0787d213615795f4f4994a3c76d424c0c05e6a338c017d81c2c0a650a3dd1
    a86a00b87bcd6c842411a0a8326850557c84c321acadc4cfe67550cd721ad7d9
    dc81277e4f52eaf35532749e2e3ced658eb0302517a47aeb2d26565dc5113bbc

The dedicated unit test passed with no errors. Its final `test_bitcoin`
binary SHA-256 is
`cc1b61929fc5b562610ab1fb1857b11f13eb66f8a6a326ad155a9cd550e78634` and
the unit log SHA-256 is
`d859092cbc2ecda458764f0e60be9efb2fc575342d4c55d9206586183d6cfeec`.
Build log SHA-256 values are sanitizer
`d82740ff612a08b70493218c11e64c15d9d88670868bbca6ce7f4b1b71c22c5c`,
normal `8b32d6df3d69e05c380aa674f4f93a66234b83add43a653320d29418e05a4a9a`,
and test `db6ab7768bdf5a441fbcdb488b4c72f8f58fc0111b79248cc5daace21dc236d7`.
`git diff --check` passed. `clang-format --dry-run --Werror` reports
pre-existing file-wide violations in these legacy files; no unrelated
formatting was changed. All temporary mutations, probes, workers, and build
processes were restored or stopped, and no fuzz job remains running.

## secp256k1 DER key import/export oracle audit (2026-07-21)

Source commit `e39ccb415f80d805c1a8f01494f06de04f501e2d` (`fuzz: enforce
secp256k1 DER key import contracts`) strengthens
`FUZZ=secp256k1_ec_seckey_import_export_der` with explicit failure,
round-trip, output-length, and empty-input contracts. It also makes the
production DER importer establish those contracts at its boundary.

### Finding and Bitcoin Core severity

Untouched master computes `seckey + seckeylen` before inspecting the length.
An empty serialized `CPrivKey` can reach this API from wallet loading, where
`CPrivKey::data()` may be null and the length is zero. The clean-master
enhanced fuzzer probe and a direct pointer-overflow UBSan probe did not report
a runtime failure for this case. This audit therefore claims no confirmed
clean-master production crash or vulnerability. The change makes the
zero-length precondition explicit and keeps corrupt wallet data on a
deterministic rejection path.

Bitcoin Core reaches the parser from `src/wallet/walletdb.cpp:304-340`,
through `CKey::Load` at `src/key.cpp:278-284`. The input is local wallet
database state, not a peer message, block, or header. Rate this result
**Low/nice-to-have defensive wallet-corruption hardening**, not High or
Critical. An invalid block cannot trigger this path. The deterministic
empty-key test proves the intended local failure behavior, but does not turn
the source-level concern into a master-relative runtime bug. A nonce without
cryptographic meaning is not Critical merely because it is not cleared.

### Audit provenance and cherry-pick context

- Source parent: `111d79d1239b61c525c7932983ee5390ae87e66b`
  (`fuzz: enforce VecDeque representation and move contracts`).
- Audit base: `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
  `remotes/l0rinc/master` matched this commit before the audit sequence.
- The l0rinc query over `src/key.cpp`, `src/key.h`, `src/pubkey.cpp`,
  `src/pubkey.h`, `src/test/fuzz/secp256k1_ec_seckey_import_export_der.cpp`,
  and `src/test/fuzz/secp256k1_ecdsa_signature_parse_der_lax.cpp` returned no
  output. No relevant fork commit was cherry-picked.
- No later fix or cherry-pick was allowed to mask the clean-master control.
  A follow-up touching this parser, wallet-load boundary, or proof input must
  amend the relevant source and evidence notes with whether it preserves,
  changes, or masks this result, then repeat the clean-master control,
  deterministic test, and corpus replays.

### Reiterated findings

The master-relative ledger remains:

- **Medium**, feature-conditional private-broadcast failed-send retention.
- **Medium**, empty-HEADERS initial-sync availability/IBD risk.
- **Low under current Core callers**, peer transaction-activity refresh,
  ProcessMessage block-storage failure, and oversized transport types.
- **Medium but latent/reachability-limited**, ecmult scratch wrapping, forced
  10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention.
- **Low/nice-to-have**, banman invalid-subnet and unban integrity.
- No additional clean-master production bug was found in the already audited
  addrman, coins-cache, coins-view, txgraph, txdownloadman, txrequest,
  connman, eviction, handshake, compact-block, headers-sync, UTXO snapshot,
  mempool persistence, package evaluation, RPC, descriptor cache, threadpool,
  cluster-linearization, policy estimator, pool resource, versionbits,
  signature-cache, CuckooCache, FlatFilePos, prevector, bitdeque, or VecDeque
  paths. Severity is tied to actual Bitcoin Core callers and input origins,
  not an assertion failure alone.

### Contracts and tests

`src/key.cpp` now rejects `seckeylen == 0` before forming the end pointer.
Every parser failure uses one helper that clears all 32 output bytes and
checks that postcondition with `Assume`, preserving the existing invalid-key
clearing behavior while making it uniform and auditable.

The fuzzer initializes the import output with `0xa5` and requires failed
imports to clear every byte. Successful exports must report the exact
compressed or uncompressed DER size, and the imported key must equal the
source key. Failed exports must report zero output length. An explicit
`(nullptr, 0)` probe covers the empty `CPrivKey::data()` boundary.

`src/test/key_tests.cpp` adds `key_load_empty_private_key`, verifying that an
empty `CPrivKey` is rejected and leaves the `CKey` invalid.

### Corpus and controls

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/secp256k1_ec_seckey_import_export_der`
to `/tmp/bitcoin-secpder-20260721/import-frozen`: 74 files and 21,810 bytes.
Manifest SHA-256 values are:

- Sorted filenames:
  `e5367dc0bcd88ff500d42a4eb4293e1d25c4035f7b924c2cb70b959db65ee8c2`
- Filename plus size:
  `62eb1464fd419d02d1e501d1064fefd9fadfcd6d6bf24845bc255c1f881be839`
- Relative contents:
  `0a72fb43af6e4aa80df1e96c74aacad5190331fb632d1be8310c5e79906e5168`

Parent source SHA-256 values were `src/key.cpp`
`e6ebcbe39d63f880268256d53d75eecc420379e5f3fa0209b0afba5125269af1`,
the import/export fuzzer
`2fab62aad055ccfc60587a6059690c51ae161e348308a58db9c46a16725d8ce1`,
and the DER-lax fuzzer
`e8a6469cb00e7f0326d6eae834a096ec64141b35c92ec6e5f982e9cc5d888219`.

The old harness replay used sanitizer binary
`8e5f162d5ae376514ab976cc077e6b3cb1bef6a099171572c36c2ad53d699e18` and
processed 75 executions, coverage 2070, features 2295, peak RSS 100 MiB,
and no artifacts. Log SHA-256:
`59eb9842165009f5884ec53a239ffbebc2caf8c849011b2b8a81e5fa1ff37d57`.

The exact clean-master empty witness is one byte `00`, SHA-256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.
With clean-master production and the enhanced empty-input harness it exited
0 with no sanitizer report. Log SHA-256:
`351db6ce881317be38a8330b3bdf6180eb46dce5df8d78a279ac58b66f242364`.
That negative control is why this is documented as defensive hardening, not a
confirmed production vulnerability.

An intermediate `fail` helper draft omitted its second `memset`. A generated
312-byte input exposed that harness/refactor mistake, not production code.
Artifact SHA-256:
`478c0db68848dbae0e5316e6404ac087bac19fdfcdb05481a8d10e37e08518e4`;
failing log SHA-256:
`b14b0b1c4f02a6e0764f5c96712ab95461f3bff487da15d54f89f84612413976`.
The helper was corrected and the exact artifact passed on the final sanitizer
binary; replay log SHA-256:
`40ae8618eaa301c49980cc007717646c54f4d72c56e4998a86a87fc97d757d8`.
This correction prevents the intermediate false positive from being
reiterated as a production finding.

### Final verification

Final source SHA-256 values are `src/key.cpp`
`178be2285765a344f4f0d683110a120ca910dc1f00f59bf3caa8b23a57d8252e`,
the import/export fuzzer
`af162d790db21d75c071fd3554342d3705dec67b8dfd9d8ce59e9ee7a307c01f`,
and `src/test/key_tests.cpp`
`e546959fa7af7aa35efe76413ce1c02f8e9c7037c21fc653b005895f169f0171`.

The final sanitizer binary
`80975e16f194232d16ae75376e4a58d580b691d688d69fbc7ec7a84a565d1ecd`
passed the full frozen replay: 75 executions, coverage 2106, features 2333,
peak RSS 100 MiB, and no artifacts. Log SHA-256:
`78d7db09ddd5b041784c6ee563c9442520582766cbc71433b4ad19dfc749c285`.
The exact empty witness passed with no artifact; log SHA-256:
`d5cda413a02667f22f7e96852bb716b8b1875a0396d7c5d26a8f3609f16656e8`.

Four isolated sanitizer workers each processed 75 executions with no
artifacts, coverage 2106, and features 2333. Combined log SHA-256:
`17fa20d1717b1ce5e4745460546fe4d2c57669d222d528b93fd77c0b25ab3cdd`.
Worker log SHA-256 values, `fuzz-0` through `fuzz-3`, are:

    dac444a326581c941bd7fb1e5f9426e850f810c10371c618ab0ed443cbd7934c
    08b8446d70549bb2fc05e5110edc04fcb8b3f9d289f0e2746bfb19b0ea24671
    0c08c70202f3fc5bc40256df53ffc382b3feb672b0ec7d222847659032b23604
    715e1b08a6de9f45b24030f82c58f78cb96dec2d8eb97d0244a8c83abe4f7018

The normal fuzz binary
`bc28657d80d805c731ba2d91b6bd4a5fe2d10c816e419a47277a88e85a07b512`
passed all 74 corpus inputs with the one-file driver. Replay log SHA-256:
`957bb94467fe34ea4cd8ba5df754dcf50d9c95294cd55a89bfafd324c3579429`.
The normal build log SHA-256 is
`825f5df3c71af6fd9fb1e38a2012e8cbb3db9135eaeedeeff4060b631b6c2a21`.

The focused `key_tests/key_load_empty_private_key` test passed with no
errors. The final `test_bitcoin` binary SHA-256 is
`249c46413356dac1ae27fb87b3ed0fdf4a3f8ea301439d7c2f0efc64f1fa13a5`;
unit log SHA-256:
`1a69a2c1805afda53db7943f9e659cbdaad2ccedff559e37187c24d7c6b2961e`;
test build log SHA-256:
`29847f7e5e97f79f2ed73c89bb4d2844786ced2cb7d8acf777fff46596decfdb`.

`git diff --check` passed. `clang-format --dry-run --Werror` reports the
pre-existing file-wide violations in these legacy files; no unrelated
formatting was changed. Formatter log SHA-256:
`9084d1a70eb4589a423f9b9e2772869c757615e5361918196e4bdf46b7b81090`.
All controls, probes, workers, artifacts, and build processes were stopped or
restored; no fuzz job remains running.

## secp256k1 lax DER signature parser oracle audit (2026-07-21)

Source commit `c953da7841661c6fffaab6f256bb1e73beb38fb1` (`fuzz: enforce lax
DER signature parser contracts`) turns
`FUZZ=secp256k1_ecdsa_signature_parse_der_lax` into a state/output oracle for
Bitcoin Core's intentional pre-BIP66 signature compatibility parser.

### Finding and Bitcoin Core severity

The untouched master parser and its frozen corpus replay found no clean-master
production bug. This commit makes no parser behavior change; the two
production assertions only verify that libsecp256k1 can construct the
documented canonical invalid signature used on parse failure and overflow.

This target is consensus-adjacent. `CPubKey::Verify` and
`CPubKey::CheckLowS` call the parser from `src/pubkey.cpp:284-294` and
`:426-430`, while script evaluation reaches verification through
`src/script/interpreter.cpp:349`, `:1177`, and `:1702`. Signature bytes can
originate in block data. BIP66 strict-encoding checks precede current
consensus verification, while historical pre-BIP66 blocks still require this
lax parser. A confirmed memory, consensus, or invalid-block-triggered parser
failure would therefore require a High/Critical review. None was reproduced
on master here, so rate this result **Informational/Low oracle hardening** with
no claimed production vulnerability or severity-raising fix. A nonce without
cryptographic meaning is not Critical merely because it is not cleared.

The lax acceptance of negative integers, excessive padding, long length
descriptors, ignored sequence lengths, and trailing garbage is intentional
historical compatibility. The oracle does not assert strict DER rejection for
those cases.

### Audit provenance and cherry-pick context

- Source parent: `e39ccb415f80d805c1a8f01494f06de04f501e2d`
  (`fuzz: enforce secp256k1 DER key import contracts`).
- Audit base: `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
  `remotes/l0rinc/master` matched this commit before the audit sequence.
- The l0rinc query covered `src/pubkey.cpp`, `src/pubkey.h`, `src/key.cpp`,
  and both secp256k1 DER fuzz targets. It returned no output, so no relevant
  fork commit was cherry-picked.
- No later fix or cherry-pick was allowed to mask clean-master behavior. Any
  follow-up touching this parser, its Core callers, or a proof input must
  amend the relevant source/evidence notes with whether it preserves, changes,
  or masks this result, then repeat the parent control, deterministic test,
  mutation witness, and corpus replays.

### Reiterated findings

The master-relative ledger remains:

- **Medium**, feature-conditional private-broadcast failed-send retention.
- **Medium**, empty-HEADERS initial-sync availability/IBD risk.
- **Low under current Core callers**, peer transaction-activity refresh,
  ProcessMessage block-storage failure, and oversized transport types.
- **Medium but latent/reachability-limited**, ecmult scratch wrapping, forced
  10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention.
- **Low/nice-to-have**, banman invalid-subnet and unban integrity.
- No additional clean-master production bug was found in the already audited
  addrman, coins-cache, coins-view, txgraph, txdownloadman, txrequest,
  connman, eviction, handshake, compact-block, headers-sync, UTXO snapshot,
  mempool persistence, package evaluation, RPC, descriptor cache, threadpool,
  cluster-linearization, policy estimator, pool resource, versionbits,
  signature-cache, CuckooCache, FlatFilePos, prevector, bitdeque, VecDeque,
  or DER key import/export paths. Severity is tied to actual Bitcoin Core
  callers and input origins, not an assertion failure alone.

### Contracts and tests

`src/pubkey.cpp` now checks the return value of the compact-parser call used
to initialize the output to a correctly parsed but invalid signature, both at
entry and in the overflow reset. These are internal library-state contracts,
not assertions about peer-provided DER.

The fuzzer seeds the output with a valid compact signature before parsing. A
return value of zero must overwrite it with the canonical all-zero invalid
signature. For every strict DER input accepted by libsecp256k1, the lax parser
must accept it and produce identical compact R/S values. Nonzero lax outputs
are serialized back to strict DER and reparsed to verify stable round trips.
`SigHasLowR` is checked for repeatability and against the compact R high-bit
contract. Historical lax cases remain allowed to return an initialized but
invalid signature.

The harness explicitly exercises `(nullptr, 0)`, which the Bitcoin copy
safely rejects before reading input. `src/test/key_tests.cpp` adds
`ecdsa_signature_parse_der_lax_contracts`, covering empty/truncated output
reset and a deterministic strict/lax signature round trip.

### Corpus and baseline

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/secp256k1_ecdsa_signature_parse_der_lax`
to `/tmp/bitcoin-secpder-lax-20260721/frozen`: 86 files and 422,212 bytes.
Manifest SHA-256 values are:

- Sorted filenames:
  `5229123a7d108d0622c4a2cbd3d245bb25c9e580b84f7e774fedf95a4a6eeba4`
- Filename plus size:
  `b64d18c6447a1b89bdb4dc71a435d7f1402b0516d3b950060476b0caf0dc826c`
- Relative contents:
  `a4f2ef21802aac51073469f1c345d8b7de0a0cde6e226340b70e6cb8a453f7d4`

Parent source SHA-256 values were `src/pubkey.cpp`
`0c86716f3626f591e643bd327fe0e48f6cebba8da3aba91ec6587256d725f1c0`,
the lax DER fuzzer
`e8a6469cb00e7f0326d6eae834a096ec64141b35c92ec6e5f982e9cc5d888219`,
and `src/test/key_tests.cpp`
`e546959fa7af7aa35efe76413ce1c02f8e9c7037c21fc653b005895f169f0171`.

The old harness replay used sanitizer binary
`80975e16f194232d16ae75376e4a58d580b691d688d69fbc7ec7a84a565d1ecd` and
processed 173 executions, coverage 2674, features 2925, peak RSS 106 MiB,
and no artifacts. Log SHA-256:
`64b53c74a9d33c771ae5ace8d167cdd81358419c0c6c8154329ec897918c1e85`.

### Differential mutation proof

The exact witness is the frozen one-byte input `bd` at
`frozen/9034aaf45143996a2b14465c352ab0c6fa26b221`, SHA-256
`68325720aabd7c82f30f554b313d0570c95accbb7dc4b5aae11204c08ffe732b`.

A minimal production mutation removed the entire entry initialization call
`Assert(secp256k1_ecdsa_signature_parse_compact(..., tmpsig) == 1)`, modeling
lost invalid-signature initialization. The mutated `src/pubkey.cpp` SHA-256
was `a4e7ed2362ac8e922671183d396872e724c42d842a332ef3b79b183ea534a6e3`.
The mutated normal fuzz binary SHA-256 was
`afdf1b250fa044c84adc17283cfb8744fd30d5cb848fde9deee0b07d61c28a83`.
The witness immediately reported `Assertion is_zero(compact_lax) failed` at
the new failure-output oracle; the normal wrapper returned an input-processing
failure. Mutation log SHA-256:
`478b06ab22b254cf19e45196909fade58dacb1cce62b3f5cd5dcdf1dd7f2dcab`.
Mutation build log SHA-256:
`6ca970154ac5be77b4caac6af52f1bada0954f74ec6c754233370cdf2fee78e5`.

This proves the assertion catches a broken production state transition. It is
mutation/oracle proof only: the mutation is not present on master and no
hypothetical master vulnerability is claimed. The mutation was restored
before all final builds and replays.

### Final verification

Final source SHA-256 values are `src/pubkey.cpp`
`18791c14b0bd988808a4cb245895443173e1bfbbcd549c01efb0219e3cd5c175`,
the lax DER fuzzer
`2f0261b50aabbc3e3669d91a9cf491333b7f2936f1d715bff827d771104f105b`,
and `src/test/key_tests.cpp`
`ae55f08b1ac34641678f92b462cf189f7bb30dd421283c638c1381367dc516ba`.

The final sanitizer binary
`7ce8759c6bec658e46f48b0388e666e8a7dc0a9ec64724c29a7ed10387a68371`
passed the restored frozen replay: 173 executions, coverage 2747, features
3025, peak RSS 107 MiB, and no artifacts. Build log SHA-256:
`f0ef578891c4125ed8ff1e730503e0e4d2914b9fe2b5609d963566c4ecd9fd92`.
Replay log SHA-256:
`5c6c8c9a47ae116640091ae5416075eef6bc51bf9d22186913c2f3d1b888d0e1`.
The exact `bd` witness passed after restoration; log SHA-256:
`9ffd8c866f382dc0d0af3f2a95f9389abe6281787fcf443e9396b54f23872913`.

Four isolated sanitizer workers each processed 173 executions with no
artifacts, coverage 2747, features 3025, and peak RSS 106-107 MiB. Combined
log SHA-256:
`5d0dfc63a5b8d805d8a2ff86c138f42d4cb985cfcf97dfb336f2a4297a84beab`.
Worker logs, `fuzz-0` through `fuzz-3`, have SHA-256 values:

    d9a17086008e11b5f6f9235940e555dc088dad0e4ffe05ea42bf4ce5a5dbf4b8
    d04d8cf7a008d9bad882844263338757983e0fb1f7eddb1dc1c1a933e6df242e
    22ff036c12f741e1dd4d6356dccfc676a8226c301e91dd87f9d8effe5f372b51
    e748b0a1d3d408b087901b49ec7f4da22f86c185a68e9cb22ae038326185ee0a

The final normal fuzz binary
`5acfb8c2281c4672d480f8ac64579cbad56db18c5d2b51036ccff65f63216b0e`
passed all 86 corpus inputs with the restored one-file driver. Replay log
SHA-256:
`aec1257f04fecb51ba470b5b512d7e6f325ff765b1187442385d0a04de62cda5`.
Build log SHA-256:
`573456345e57c513ca8d60db4bb6c29567546a30de5f83003bd49e690e1e5011`.

The focused `key_tests/ecdsa_signature_parse_der_lax_contracts` test passed
with no errors. Final `test_bitcoin` binary SHA-256:
`0224706549a9d216a7e5b74da2678804b1495c059f0485d9b1eeeed55f2de5b6`;
unit log SHA-256:
`9bbff1778eff0410751cd209b1e3723a6381886bcddb8bf35194b55bdb417210`;
test build log SHA-256:
`5cd1623ea35b90ffb45734758c73aeee39fd0b285b9a925113dbfe660f4348d4`.

`git diff --check` passed. `clang-format --dry-run --Werror` reports only
pre-existing file-wide violations in the legacy files; no unrelated
formatting was changed. Formatter log SHA-256:
`bbbe150e2f222737908c77075317581799db78d4561c737564a9dd134b2c2919`.
All mutation builds, probes, workers, artifacts, and test processes were
stopped or restored; no fuzz job remains running.

## EllSwift decode and serialization oracle audit (2026-07-21)

Source commit `4ecc40ea7e` (`fuzz: enforce EllSwift decode and serialization
contracts`) turns `FUZZ=ellswift_roundtrip` into a state, representation, and
output oracle for Bitcoin Core's ElligatorSwift key exchange path.

### Finding and Bitcoin Core severity

The untouched master target and its frozen corpus replay found no clean-master
production bug. The commit does not change EllSwift encoding, decoding,
serialization, or BIP324 behavior. It asserts documented libsecp256k1 return
contracts that were previously ignored.

The relevant Core boundary is network-reachable. `V2Transport::ProcessReceivedKeyBytes`
receives exactly 64 bytes from a peer at `src/net.cpp:1128-1158`, with the
receive limit enforced at `src/net.cpp:1284-1303`. Those bytes flow through
`BIP324Cipher::Initialize`, `CKey::ComputeBIP324ECDHSecret`, and EllSwift XDH.
The EllSwift API documents that every 64-byte value decodes successfully, so
arbitrary peer bytes are not invalid-block data and cannot make the new decode
assertion fail merely by being malformed. A separately proven memory-safety,
resource, or remotely triggered transport failure would require High/Critical
review; none was reproduced on master. This result is therefore
**Informational/Low oracle hardening**, with no claimed production
vulnerability or severity-raising fix. A nonce without cryptographic meaning
is not Critical merely because it is not cleared.

`CKey::EllSwiftCreate` is a local private-key path; the received-key path is
BIP324 transport rather than block or consensus validation. Severity is based
on those actual Core callers and input origins, not an assertion failure in an
artificial harness state. The upstream API also permits EllSwift encodings to
change across library versions, so the oracle checks only same-version
encode/decode behavior and copy representation.

### Audit provenance and cherry-pick context

- Source parent: `c953da7841661c6fffaab6f256bb1e73beb38fb1`
  (`fuzz: enforce lax DER signature parser contracts`).
- Audit base: `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
  `remotes/l0rinc/master` matched this commit before the audit sequence.
- The exact query was:
  `git log origin/master..remotes/l0rinc/master -- src/test/fuzz/key.cpp src/key.cpp src/key.h src/pubkey.cpp src/pubkey.h src/bip324.cpp src/test/fuzz/bip324.cpp`.
  It returned no output. No relevant l0rinc commit was cherry-picked.
- No later fix or cherry-pick was allowed to mask clean-master behavior. A
  follow-up that changes EllSwift, BIP324 callers, corpus inputs, or proof
  conditions must amend its source/evidence notes with whether it preserves,
  changes, or masks this result; merge or amend deliberately if a potential
  fix changes a follow-up experiment.

### Reiterated findings

The master-relative ledger remains:

- **Medium**, feature-conditional private-broadcast failed-send retention.
- **Medium**, empty-HEADERS initial-sync availability/IBD risk.
- **Low under current Core callers**, peer transaction-activity refresh,
  ProcessMessage block-storage failure, and oversized transport types.
- **Medium but latent/reachability-limited**, ecmult scratch wrapping, forced
  10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention.
- **Low/nice-to-have**, banman invalid-subnet and unban integrity.
- No additional clean-master production bug was found in the audited addrman,
  coins-cache, coins-view, txgraph, txdownloadman, txrequest, connman,
  eviction, handshake, compact-block, headers-sync, UTXO snapshot, mempool
  persistence, package evaluation, RPC, descriptor cache, threadpool,
  cluster-linearization, policy estimator, pool resource, versionbits,
  signature-cache, CuckooCache, FlatFilePos, prevector, bitdeque, VecDeque,
  DER key import/export, or lax DER parser paths. Severity remains tied to
  actual Bitcoin Core callers and input origins.

### Contracts and tests

`EllSwiftPubKey::Decode` now asserts that
`secp256k1_ellswift_decode` and compressed
`secp256k1_ec_pubkey_serialize` return their documented value of one before
using the resulting objects. This is an internal state contract, not a stricter
acceptance rule for peer bytes.

The fuzzer checks that a fixed-size copy preserves the 64-byte representation,
decode reproduces the generated `CPubKey`, the decoded key is fully valid, and
a signature from the source key verifies under the decoded key. It also feeds
an arbitrary fixed-size 64-byte value into `EllSwiftPubKey` to model the
BIP324 peer boundary and checks that the documented always-successful decode
produces a fully valid `CPubKey`. The deterministic `key_ellswift` test adds
the matching copy check. No check assumes cross-version EllSwift byte
stability.

### Corpus and baseline

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/ellswift_roundtrip` to
`/tmp/bitcoin-ellswift-20260721/frozen`: 1,755 files and 117,632 bytes, with
minimum size 1 byte and maximum size 332 bytes.

Manifest SHA-256 values:

- Sorted filenames:
  `e2ec6ced51ce83cba625760aa3294795cf9442e8d4b41c22d28558c0f92ce122`
- Filename plus size:
  `66d36a72b8c25301be041c338c8df899850ccf2bd224337c4d98dcb939484fda`
- Relative contents:
  `8b5a8765adf33ad286f0749f2aa4447c09caa84b8603f1fee8ff7ea27fff3c9c`

Parent source SHA-256 values were `src/pubkey.cpp`
`18791c14b0bd988808a4cb245895443173e1bfbbcd549c01efb0219e3cd5c175`,
`src/test/fuzz/key.cpp`
`2b9765130f931503c747fb405c91f530da239dc0713e759e837c2649472290a3`, and
`src/test/key_tests.cpp`
`ae55f08b1ac34641678f92b462cf189f7bb30dd421283c638c1381367dc516ba`.

The pre-change target replay used sanitizer binary
`7ce8759c6bec658e46f48b0388e666e8a7dc0a9ec64724c29a7ed10387a68371` and
processed 1,756 executions with coverage 6,018, features 18,090, peak RSS
131 MiB, and no artifacts. Baseline log SHA-256:
`51af29afef5fd4c56dfc6b40b4724396dd74464ea7e955b02a8b23355d71742d`.

### Differential mutation proof

The exact witness was
`frozen/4fcbabea316231101f676f4ea5f592b8155e29b4`, 332 bytes, SHA-256
`50bb19e7ddeab918db423d5309843b6c197e25ae1643cc374fc5314b399b6eb5`.

After the final harness change, the minimal production mutation inserted
`vch_bytes[0] ^= 1` immediately after compressed serialization in
`EllSwiftPubKey::Decode`. The mutated `src/pubkey.cpp` SHA-256 was
`d567562bbfb6ca46ceefa1ee72d82439b082787fbca2a3279cfdb43d7781c1bc`; the
mutated normal binary SHA-256 was
`d86c272e8436aca508307e5f200573e04ecc1531bcb6e11084cac6bf17325998`.
The witness immediately failed at
`assert(decoded_pubkey == key.GetPubKey())` with process exit 1. Mutation
build-log SHA-256:
`ebcaccc7e69c0c5949e5f96b8b24fa228de60cb4016ab7799786a07e3f1a2b3a`;
mutation-log SHA-256:
`51e9d263962976494812618b665b836e9a4d44a3d0685f606aff7c7020231f50`.

This proves the equality oracle catches a broken production state transition.
It is mutation/oracle proof only: the byte flip is not present on master and
no hypothetical master vulnerability is claimed. The source was restored
before every final build and replay.

### Final verification

Final source SHA-256 values are `src/pubkey.cpp`
`8526e65cf02e5264cf08507e5dcae24885cad17635c5f92f1332dda3debe4567`,
`src/test/fuzz/key.cpp`
`eb362fa171c65cca69b395b6363128c91ce61b5df1f2daf0c2bbf937692fc479`, and
`src/test/key_tests.cpp`
`9cdc37a860dfdbcbc68031a323cdedd028168bab3ac91f2d0fb5794472873d40`.

The final sanitizer binary
`5f6e19b4e4549cc9e5e6272b87415d97f85d35970d108f4cc5169723b15f8383` passed
the frozen corpus replay: 1,756 executions, coverage 6,046, features 16,619,
peak RSS 131 MiB, and no artifacts. Build-log SHA-256:
`cd597905779446b5360b6093a4c2a1e3f0890828001e36c29b02499fa7eea35f`;
replay-log SHA-256:
`a7a49297610ed76147f74123513c5c047c9f972381da984267caca81fea38840`.

Four isolated sanitizer workers each processed 1,756 executions with
coverage 6,046, features 16,619, peak RSS 130-131 MiB, and no artifacts.
Combined log SHA-256:
`2c38f014e0c7bd877a6c2f0e2a90ec98565809bfdf7ff418f3643245f8b941a0`.
Worker log SHA-256 values, `fuzz-0` through `fuzz-3`, are:

    9d8eb5170aca5d8caa34be05dd1ffe0304e494fae5f107e91fb151da0bca4463
    0dbc3269e060edd50a7978fc363465c8233b003cddf19e20c026019b6fce6be5
    2391b2c97afe2147a92fc653de3d619b6ed267960b61ba64d7c0576858140ca1
    263d64a1b342065abf3fdaf7754d57b2042dbaca78c229eb1d5fef86fccbf9a7

The final normal fuzz binary
`bb583a6c0cc9e59125079c79d1af128e465d0f0273ad2aae8d16e7290174a679`
passed all 1,755 corpus inputs. Build-log SHA-256:
`f06016992b773f4185cef898723ff790c36db8eebdb9bb32c45c2f5eb31edf49`;
replay-log SHA-256:
`0cf014a7369a309ec9ed7407e2c96110a851827193700475e5b160118ae28722`.
The exact witness passed after restoration in both normal and sanitizer
builds; witness-log SHA-256 values are
`636a6757789287f228549062563f91727d7462a8d548e2d12ac61306cc80a7f0` and
`491040dab4c67a0316350832da5d8861044081b5ae01320b8db0f81b2be0e732`.

`key_tests/key_ellswift` passed with no errors. Final `test_bitcoin` binary
SHA-256:
`83e4a4a24eee18974d99dae1c8ece93303bd44edc2eab04c24ddaa2d6413e2d7`;
build-log SHA-256:
`832829a028cff77fbe5159ea5b42b77f47605fabdbdd7c265305264e4634df24`;
unit-log SHA-256:
`4123506487c14c9dc0cf56380bf2f82de948bb819d753d430297d8d596fdeba9`.

`git diff --check` passed. `clang-format-22 --dry-run --Werror` reported only
pre-existing file-wide legacy diagnostics; none named the modified EllSwift
lines. Formatter-log SHA-256:
`0895f653ecb2cafde76183384b5bb15f956fe3be930a5fc90a433d5437fd8208`.
All mutation processes, probes, workers, artifacts, and tests were stopped or
restored; no fuzz job remains running.

## BIP324 ECDH state oracle audit (2026-07-21)

Source commit `16e576419f` (`fuzz: enforce BIP324 ECDH state contracts`)
strengthens `FUZZ=bip324_ecdh` with an arbitrary-peer-input and deterministic
XDH-output oracle, and makes the CKey BIP324 ECDH success contract fatal when
violated.

### Finding and Bitcoin Core severity

The clean-master target and frozen corpus replay found no production bug.
Valid Bitcoin Core behavior is unchanged. The production changes only fail
immediately on an impossible local-key/XDH failure and zero-initialize the
fixed-size output before libsecp256k1 fills it.

The actual Core boundary is network-reachable: a peer supplies 64 bytes to
`V2Transport::ProcessReceivedKeyBytes` at `src/net.cpp:1128-1158`, the receive
limit is `src/net.cpp:1284-1303`, and the bytes flow through
`BIP324Cipher::Initialize` to `CKey::ComputeBIP324ECDHSecret`. Invalid block
bytes do not invoke this path. XDH returns zero for an overflowing or zero
local scalar, or for a hash callback returning zero. Core reaches this method
with a checked valid scalar and the built-in BIP324 hash callback always
returns one; arbitrary 64-byte EllSwift peer values are valid inputs. A
separately proven remotely triggered crash, memory-safety issue, or transport
DoS would require High/Critical review, but none was reproduced on master.
This result is therefore **Informational/Low oracle hardening**, with no
claimed clean-master vulnerability or severity-raising fix. A nonce without
cryptographic meaning is not Critical merely because it is not cleared.

Severity follows the real Core caller and input origin. This is a BIP324
transport contract, not block or consensus validation; an assertion failure in
the harness alone is not a Critical finding.

### Audit provenance and cherry-pick context

- Source parent: `4ecc40ea7eee7c63fde8b1fecb57e9fef32f745a`
  (`fuzz: enforce EllSwift decode and serialization contracts`).
- Audit base: `18c05d93016b28a9afd4c716dfe00b6e0accb30b`; `origin/master` and
  `remotes/l0rinc/master` matched this commit before the audit sequence.
- The l0rinc query over `src/test/fuzz/key.cpp`, `src/key.cpp`, `src/key.h`,
  `src/pubkey.cpp`, `src/pubkey.h`, `src/bip324.cpp`, and
  `src/test/fuzz/bip324.cpp` returned no output. No relevant fork commit was
  cherry-picked, and no later fix was allowed to mask clean-master behavior.
- Follow-ups changing this target, its Core callers, corpus inputs, or proof
  conditions must amend their source/evidence notes with whether they
  preserve, change, or mask this result. Merge or amend deliberately if a
  potential fix changes a later experiment.

### Reiterated findings

The master-relative ledger remains:

- **Medium**, feature-conditional private-broadcast failed-send retention.
- **Medium**, empty-HEADERS initial-sync availability/IBD risk.
- **Low under current Core callers**, peer transaction-activity refresh,
  ProcessMessage block-storage failure, and oversized transport types.
- **Medium but latent/reachability-limited**, ecmult scratch wrapping, forced
  10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979 retention.
- **Low/nice-to-have**, banman invalid-subnet and unban integrity.
- No additional clean-master production bug was found in the audited addrman,
  coins-cache, coins-view, txgraph, txdownloadman, txrequest, connman,
  eviction, handshake, compact-block, headers-sync, UTXO snapshot, mempool
  persistence, package evaluation, RPC, descriptor cache, threadpool,
  cluster-linearization, policy estimator, pool resource, versionbits,
  signature-cache, CuckooCache, FlatFilePos, prevector, bitdeque, VecDeque,
  DER key import/export, lax DER parser, or EllSwift paths. Severity remains
  tied to actual Bitcoin Core callers and input origins.

### Contracts and tests

`CKey::ComputeBIP324ECDHSecret` now uses `Assert(keydata)`, value-initializes
its fixed 32-byte `ECDHSecret`, and uses `Assert(success)` for the documented
always-successful BIP324 XDH call. This prevents an impossible failure from
silently returning an uninitialized secret.

`bip324_ecdh` feeds an arbitrary fixed-size 64-byte peer EllSwift value into
the same CKey/XDH path used by the transport and checks that repeated calls
with identical inputs produce identical secrets. Its existing two-party
oracle continues to check role mapping, shared-secret agreement, wrong-party
separation, and distinct-encoding separation. The deterministic
`key_ellswift` test adds an all-zero peer EllSwift input and repeated-secret
check. The zero encoding is a valid arbitrary peer value, not invalid-block
data.

### Corpus and baseline

The corpus was frozen from
`/mnt/my_storage/qa-assets/fuzz_corpora/bip324_ecdh` to
`/tmp/bitcoin-bip324-ecdh-20260721/frozen`: 1,533 files and 2,250,288 bytes,
with minimum size 1 byte and maximum size 2,097,151 bytes.

Manifest SHA-256 values:

- Sorted filenames:
  `08457a9cd0e75e805737e47a085ac822a186d64423fc69652ee7dcd9a0fc70d2`
- Filename plus size:
  `294c31e88390d22ca368751d6793f22df7c4fae36acc714c609651c35e400258`
- Relative contents:
  `f999202469812c5908ecdd02f6786c57fe98a5a902cce46bf72905bac13455c4`

Parent source SHA-256 values were `src/key.cpp`
`178be2285765a344f4f0d683110a120ca910dc1f00f59bf3caa8b23a57d8252e`,
`src/test/fuzz/key.cpp`
`eb362fa171c65cca69b395b6363128c91ce61b5df1f2daf0c2bbf937692fc479`, and
`src/test/key_tests.cpp`
`9cdc37a860dfdbcbc68031a323cdedd028168bab3ac91f2d0fb5794472873d40`.

The pre-change sanitizer binary was
`5f6e19b4e4549cc9e5e6272b87415d97f85d35970d108f4cc5169723b15f8383`.
It processed 1,534 executions with coverage 5,127, features 13,867, peak RSS
130 MiB, and no artifacts. Baseline log SHA-256:
`68e6875d27f696ba471dc6bb0cf156b59862a3e27bd7fa6ba645815b2eeb8bc0`.
The pre-change normal binary was
`bb583a6c0cc9e59125079c79d1af128e465d0f0273ad2aae8d16e7290174a679` and its
one-file driver passed all 1,533 inputs. Replay log SHA-256:
`d1f2883f71237fd338a287d2e8434b174a81a13ed5f9e215b5280c1270a0f988`.

### Differential mutation proof

The target-specific witness is
`frozen/00169efed183b750af496e0d48183197bed7fce8`, 34 bytes, SHA-256
`91f766c92b02b5e32dbf8e5b653ba3f099cd8a006eaeb099451153cde619febd`.

A minimal production mutation swapped the `party` mapping from
`initiating ? 0 : 1` to `initiating ? 1 : 0` in
`CKey::ComputeBIP324ECDHSecret`. The mutated `src/key.cpp` SHA-256 was
`c5ba27fc9705623deded2f3babc7662c2649a7b0f6626d7e3c4e1ed8e1182f90`, and the
mutated normal binary SHA-256 was
`07fbea472eb6d3ca610e6153acf80310b0ef0b52a56cba9614e0ac9db7bd59cf`.
The witness immediately failed at `ecdh_secret_1 == ecdh_secret_2` with exit
1. Mutation build-log SHA-256:
`e51a733af88c8523764980f779cd756c2ca3eda1d42b000af1abcb3ba903de74`;
failure-log SHA-256:
`02f64b9a22986ef60257a8b556a4f26316d425b28e8b60ebdfb82a71b2f9f779`.

The largest corpus input,
`frozen/552fb36372683570a82e04fab5280146524ec93d`, is 2,097,151 bytes,
SHA-256
`227cd4bd4d1e4a1f10f50b8e75d56ad431f4c1925dfb75d5f64af03229df87e3`, and
was a non-triggering control under the same mutation. The scan continued
until the 34-byte witness failed, avoiding a claim based on a corpus input
that did not exercise two distinct valid parties.

This is oracle/mutation proof only: the mutation is not present on master, no
hypothetical master vulnerability is claimed, and the source was restored
before final builds and replays.

### Final verification

Final source SHA-256 values are `src/key.cpp`
`1b9ab9ac50d8e2e096374c84a4613a98f5482708b8d93b3a485bfe966ec66011`,
`src/test/fuzz/key.cpp`
`6cbbc14221239a0cb33385625de4b6de02ff0d17404109cc6832927038bc4902`, and
`src/test/key_tests.cpp`
`1ebe1acdadb4c9e693742a40baa8dbcfdcc433e1b7fa127c0ab236f927f8bbd6`.

The final sanitizer binary
`560bcdd32529c81ade56c7aacb2f60e429f4611258a069e287a79a44368445d8` passed
the frozen replay: 1,534 executions, coverage 5,143, features 10,919, peak
RSS 130 MiB, and no artifacts. Build-log SHA-256:
`9a96a9c2fbf920d68eca8a9cde01e9d024ec5895a24b9a5177f2f843a2dba8fb`;
replay-log SHA-256:
`7a6752f2bb8c51afd23ebce49be42c6d5afcd6da3625cb8dd8431743fbfa71e0`.

Four isolated sanitizer workers each processed 1,534 executions with
coverage 5,143, features 10,919, peak RSS 130 MiB, and no artifacts. Combined
log SHA-256:
`00fc4d4ad6c2c1dfefb582f2278fa29aede85070b920dec9106ba84f0871b43b`.
Worker log SHA-256 values, `fuzz-0` through `fuzz-3`, are:

    544c162aa5da9ffc5ccdc69c6c18d800d4aa62b872124be50cb64ca612ab5aca
    321635f2023353e28af2b3414ad1ac1582d79004f524c034d9d71968b4dfc7ce
    a7ccf9130b711c19edad12816c1b59e2a3819228ef087b7286658b61b35927e5
    de3c7836a20f0823556a91e1bcb2bc4ec7496eff1b3f5d6929c7f86122e55b3f

The final normal fuzz binary
`d0cbf5b79db46b35e3af684f8946c01bf2cc1cf475be33cf7a0c272541d3efaa`
passed all 1,533 corpus inputs. Build-log SHA-256:
`b1dbe504412b211d44eea0713dc5040857b0973c1a300604d0f4791660bfe461`;
replay-log SHA-256:
`361b166be8a02a1a8fdce7f4a4c7dfc9cf6f40618a1c86f7254ac6428f1a6a24`.
The target-specific witness passed after restoration in both modes. Witness
log SHA-256 values are
`93c3632332df5e22040d9c156cf9411f0ba7fe72f150f610c9e2cbab0c1873b5` and
`912706ba772835dbf8c23e3fb3558e12083c1b82770391409bc4e6002eaf898b`.

`key_tests/key_ellswift` passed with no errors, including the zero
EllSwift/XDH determinism check. Final `test_bitcoin` binary SHA-256:
`103a326358154fbaa7deac6a2ca33561c08d5dafe9034796f75778b5eb58afb9`;
test build-log SHA-256:
`3fd07ce6e634f6edebb607567873dfcf28f78b0c3adf8c86e53de7204a553159`;
unit-log SHA-256:
`eca6438bd6fd0808d5992e545c2f889a0c1ea11f4800e6ad9c27f7470355dad6`.

`git diff --check` passed. `clang-format-22 --dry-run --Werror` reported only
pre-existing file-wide legacy diagnostics; none named the modified BIP324
contract lines. Formatter-log SHA-256:
`8e176b190c220715de6f615a2546048c9736ee181ccc685b4037486a02ea02ce`.
All mutation processes, probes, workers, artifacts, and tests were stopped or
restored; no fuzz job remains running.

## Policy estimator stateful oracle

Source commit `d7ca28616e` (`fuzz: model policy estimator tracking contracts`)
hardens `FUZZ=policy_estimator` with a production-side `FlushUnconfirmed()` map
postcondition and a model of tracked transaction IDs, best-height gates,
block-removal behavior, `removeTx()` return values, flush clearing, fee-query
domains, horizon ordering, `FeeCalculation.best_height`, and read-only
serialization. Serialization snapshots are sampled every 64 transitions. The
production variable `m_has_no_mempool_parents` is mirrored with the explicit
model name `tx_has_no_mempool_parents`; an earlier inverted harness model was
caught by the final cleanup probe and was corrected, not reported as a
production failure.

### Audit boundary and severity

The audited parent is `8c8beb430ab55a67a19dd2362c7fa43fbda9df3`. Both
`origin/master` and `remotes/l0rinc/master` resolve to
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`. No additional l0rinc commit was
relevant to this target, so no target-specific cherry-pick was made and no
later fix masks clean-master behavior.

Bitcoin Core uses this object for node-local fee estimation through mempool and
block bookkeeping, RPC, and wallet policy surfaces. It is not a consensus
validator, and arbitrary invalid block bytes do not directly invoke these fee
query contracts. The clean-master classification is therefore Informational to
Low oracle hardening, not High or Critical. No clean-master production bug was
found and no deterministic regression test is claimed. A nonce without
cryptographic meaning is not Critical merely because it is not cleared.

The reiterated master-relative ledger remains: Medium and feature-conditional
for private-broadcast failed-send retention; Medium availability/IBD risk for
the empty-HEADERS initial-sync handoff; Low for peer transaction-activity
refresh, process-message block-storage failure, and oversized transport types
under current Core callers; Medium but latent/reachability-limited for ecmult
scratch wrapping, forced 10x26 magnitude-32 normalization, and SHA/HMAC/RFC6979
retention; and Low/nice-to-have for banman invalid-subnet and unban integrity.
No additional clean-master issue was found in the previously audited addrman,
coins-cache, txgraph, txdownloadman, txrequest, connman, eviction, handshake,
compact-block, headers-sync, UTXO snapshot, mempool persistence,
package-evaluation, RPC, descriptor-cache, threadpool, or cluster-linearization
paths.

### Corpus and clean replay

The frozen corpus is `/tmp/bitcoin-policy-estimator-20260721/frozen`, copied
from `/mnt/my_storage/qa-assets/fuzz_corpora/policy_estimator`: 800 files,
39,548,144 bytes. Its sorted-entry SHA-256 is
`e94be70b6fb472e634b5bafffdab99ec50aa283634b71c04cb4e7cb61a4f05c1`; its
manifest SHA-256 is
`50f54ad8e737e57611347deb99b046d6d4b3c18126a68648fbae785ddf2322b2`.

The parent-harness sanitizer baseline binary SHA-256 was
`3db619b33bdc0a164bdbc3f6641ffedf71dbc64885e43a0efae0ce24fd68bd93`; its log
SHA-256 was `eb5c9a56c570b7feb682708010099b11829ba6bb940f7897d8e5d1c1be08d92f`.
It exited 0 after 801 executions with coverage 2059, feature count 10963, and
peak RSS 655 MiB. The normal baseline binary was
`4b006cb6d23ec5fae503d74b1756741b4705efcfe04816bc8e914105fe32f79c`, with log
SHA-256 `480be4ddb3631334ecb043aa4b498b976ca664935146ee20591b112aa77a292e`;
its standalone driver passed all 800 inputs.

Final source SHA-256 values are
`9f824745f72edeb90215c3ecd2dd19e7288a178f0e3df8e96d464858d751ddbb`
(`src/policy/fees/block_policy_estimator.cpp`) and
`3544573ba0fc702a8953fa78b1fd632fd3ad876a2b6c4c02fbba28af6e450c0a`
(`src/test/fuzz/policy_estimator.cpp`). The final sanitizer binary is
`02355222337f678b830a6491e9404f634a20375d31b076259c86b2b9097a426e`.
The `-merge=0 -runs=1 -timeout=120 -rss_limit_mb=4096` replay exited 0 after
801 executions, coverage 2045, feature count 8626, peak RSS 646 MiB, with no
new units or artifacts. Log SHA-256:
`e1a0552d8bcbf29f5aa952443a77ead980672da7aad0696ea37c8d7d48135545`.

The final normal binary is
`e93fd784c733f2d7c2bb67a0dbd8a4f5231608f4450ab0e8f199197eb491e5e8`; its
one-file-at-a-time standalone replay passed all 800 inputs with no artifacts.
Log SHA-256:
`7c2d53bf5e94f25a9d87be1606840d8e229e8dceef8d63afbf98712d9f5d30d8`.

Four sanitizer jobs used `-jobs=4 -workers=4 -merge=0 -runs=1 -timeout=120
-rss_limit_mb=4096 -print_final_stats=1` with four isolated 800-file copies.
Each job loaded all four supplied copies, 3,200 files, and executed 3,201
units. All exited 0, peaked at 660-667 MiB, retained the corpus entry SHA, and
produced no artifacts. Parent log SHA-256:
`b1f071bcd5902939a27c98f1fd7f3fcd1370bd215eb40d830a28c8e7100f1844`.

### Differential proof

This is an oracle proof, not a clean-master production finding. A temporary
minimal production mutation changed
`HighestTargetTracked(SHORT_HALFLIFE)` from
`shortStats->GetMaxConfirms()` to `return 0`. Mutated production source
SHA-256: `370fdaf9031618fb9e8e000510c3a45d530c8d2fda8f03e4dd7c41dbb9bf7c0f`.
The mutated sanitizer binary SHA-256 was
`b0a42a8c7c4e85b4beb3f0fe84ef29a89cc077e2de53fd6e79e6591365f1149d`.

The exact six-byte input was
`32 f7 ff 7f 7f 49` (Base64 `Mvf/f39J`), SHA-256
`2e6f736176f0acefe7684d965d35448fa29d70ba65a22010625f08f0a51d244d`.
The mutated fixed-input replay exited 77 after one execution at
`policy_estimator.cpp:154` on
`short_target > 0 && short_target <= medium_target && medium_target <= long_target`.
Mutation log SHA-256:
`6cc5276d93dfbd68a9d2064884e406ad40512854eb03c2959a7ccdca70481de6`.

The control kept the production mutation but disabled only that new assertion:
control fuzzer source SHA-256
`86973b2d5117273d87c66ae083466dc1e5176694deac5dd01ccd87d5c393d65b`, binary
SHA-256 `852141ce0e73e47a5eb600ee89abc7ee6b2ae19f6ad9e26d17f9611b78e664e1`,
and log SHA-256
`dea8b97b765a7eeae6594a2559d5c452d7ffca900b253f872fb22f188ecde45f`.
The identical input exited 0 with no artifact. After restoring clean master,
the identical input exited 0 in binary
`02355222337f678b830a6491e9404f634a20375d31b076259c86b2b9097a426e`, with no
artifact; restored log SHA-256:
`171b98b9d72b098ba2e7d687304034e69abb3a28c2606792d480ecdfb27b1313`.
All temporary mutations were removed before the source commit.

### Verification gap

Sanitizer and normal fuzz targets were built with the two configured `cmake
--build ... --target fuzz -j2` commands. `git diff --check` passed and the
fuzzer file passed clang-format dry-run; the production file retains unrelated
legacy clang-format violations. The configured fuzz-only builds have no
`test_bitcoin` target, so the dedicated unit suite was unavailable. No fuzz,
sanitizer, or mutation process remains running.

## `cluster_linearize` permutation and transition oracle audit (2026-07-21)

Source commit: `8c8beb430a` (`fuzz: enforce cluster linearization state
contracts`). Its parent is `d490ea2da497d73bcfb9dc89e5c0e92a3dacb434`. The
audit base is Bitcoin Core `origin/master`
`18c05d93016b28a9afd4c716dfe00b6e0accb30b4`, identical to
`remotes/l0rinc/master`. The l0rinc pull-request paths were checked for this
target and no relevant commit was cherry-picked. No later fix was allowed to
mask clean-master behavior; any future cherry-pick or potential fix must say
whether it preserves, changes, or masks this result and amend its notes when
needed.

### Core boundary and severity

`FUZZ=clusterlin_depgraph_sim`, `clusterlin_depgraph_serialization`,
`clusterlin_linearize`, and `clusterlin_sfl` exercise the dependency-graph,
spanning-forest, and linearization state machines. `Linearize()` and
`PostLinearize()` are reached through `GenericClusterImpl::Relinearize()` in
`src/txgraph.cpp`; Bitcoin Core uses that graph through `CTxMemPool` for
transaction admission, dependency and fee handling, removal, staging, and
`DoWork`, and through block building. The fuzzers consume internally encoded
dependency graphs. They do not pass an invalid peer block directly to this
contract.

The production-side oracle now requires `LoadLinearization()` to receive a
complete, in-range, duplicate-free permutation of the active transactions.
`Linearize()` and `PostLinearize()` assert that they preserve the transaction
count. The SFL harness checks state after every transition and requires its
cost not to decrease. Existing post-linearization checks still validate the
topological permutation and diagram.

Severity on clean master is **Informational/Low**, as oracle hardening rather
than a production vulnerability. A malformed internal cached order could
cause mempool linearization inconsistency or availability/resource behavior,
but the audited boundary is not directly peer-controlled and this is not a
consensus, memory-safety, cryptographic, or High/Critical finding. No clean
master production bug, production fix, or deterministic regression test is
claimed.

### Existing findings reiterated

The current master-relative ledger remains:

- Private-broadcast failed-send retention: Medium and feature-conditional.
- Empty HEADERS initial-sync handoff: Medium availability/IBD impact.
- Peer transaction-activity refresh and `ProcessMessage` local block-storage
  failure: Low with current Bitcoin Core callers.
- Oversized transport types: Low with current callers.
- Ecmult scratch wrapping, forced 10x26 magnitude-32 normalization, and
  SHA/HMAC/RFC6979 retention: Medium latent/reachability-limited correctness
  or hygiene findings.
- Banman invalid-subnet/unban integrity: Low/nice-to-have.
- Addrman, coins-cache, txgraph, txdownloadman, txrequest, connman, eviction,
  handshake, compact-block, headers-sync, UTXO snapshot,
  mempool-persistence, package-evaluation, RPC, and descriptor-cache audits
  found no additional clean-master production bug.

A nonce with no cryptographic meaning is not Critical merely because it is
not cleared. Severity is based on Bitcoin Core reachability and impact.

### Corpus identity and replay evidence

Frozen copies came from `/mnt/my_storage/qa-assets/fuzz_corpora` and were
isolated under `/tmp/bitcoin-clusterlin-20260721`. Their sorted-entry and
manifest SHA-256 values are:

    target                         files  bytes    entries SHA-256                                      manifest SHA-256
    clusterlin_depgraph_sim       219    296837   fbe2ed63c9dfaadeeb1bb4a76a2f9e9827d8134fdb0ca758dfa9d87940792875  9d108627e31b49d648c9fcf5d34ecbc7ba97cdafcf41420b4bbb6f92dca19698
    clusterlin_depgraph_serialization 189 15459   5b51f7f49fbeaaa9d2e6b522f1471d217a778c8a283461502bc9b3e037b2b8f5 87ee0df50f4521854801c9c0676a376c084c7e3262610b01510a77e02d0ec553
    clusterlin_linearize           440    48277    14342098ce523d8d9422afe5cfa5d49b656f132c9e3da5916775de15358a800f d5e80a0a021bc27d9bc988ba72e24d33796a81068bff16cc66a7534ad1a971b2
    clusterlin_sfl                 611    72558    2831c98083ae6800385db8a31e111ccdfb118b98b5342ad71d1395d2f0f39057 7948126db9926b5844399d16fd34361d8d9fea637bbc91b6c1504d4c5699177d

The final sanitizer binary SHA-256 is
`3db619b33bdc0a164bdbc3f6641ffedf71dbc64885e43a0efae0ce24fd68bd93`.
Using `-merge=0 -runs=1 -timeout=60 -rss_limit_mb=4096
-print_final_stats=1`, the frozen replays exited 0, produced no artifacts,
and recorded these log SHA-256 values:

    target                         runs  coverage  features  peak RSS  log SHA-256
    clusterlin_depgraph_sim       220   681       3574      107 MiB   7ac02e6cae5794f32f8a326d2127b79b444479be442f087569f8f0db1e2233fa
    clusterlin_depgraph_serialization 190 585      2544      104 MiB   8a84b9cd4636daa18e5c1dbcadb6152dd5f60cb65c5767f932b347c07cae59c6
    clusterlin_linearize           441   1612      9718      124 MiB   d671de7f5f5b5e3f165acf6f1c821e34dc557e1c942d73f78ee79e0e58a37d0e
    clusterlin_sfl                 612   1900      11838     303 MiB   b9b473be2c67786470d4553cad501c0d5ab0cf4c65d91bd4f4746660a0d62884

Four independent sanitizer workers used `-jobs=4 -workers=4`, isolated
corpus/artifact paths, and the same `-merge=0 -runs=1 -timeout=60
-rss_limit_mb=4096 -print_final_stats=1` settings. Every worker exited 0;
all 219/189/440/611-file copies stayed unchanged and no artifacts were
created. Parent log SHA-256 values were:

    sim         b64c35f402745abb487afaecb048f4f9bf81ecaff0d5273dd4fb02e114dfbba1
    serialization 2c4e911f182fe6d9cbe4a62e1b04ce215255966f1d5a45cb876d8c023ef7fadc
    linearize   43cee475b8452d91e0f1fecd9de0ab811ba3b09981ccf9a90561d3788dc5ced1
    sfl         60e088f896d994757c52ba243657d967ad5ff312686ae940e88ab2be340de5fd

The final normal standalone fuzz binary SHA-256 is
`4b006cb6d23ec5fae503d74b1756741b4705efcfe04816bc8e914105fe32f79c`.
Using the correct input-path interface, it passed all frozen files with no
artifacts. The counts and log SHA-256 values were: sim 219/
`ed5ebc3a93ef7b8b776d8de56a372aefeece03cecbba51735636602b0722a828`,
serialization 189/
`69cd117eed635d81e90bbcd738a01b4d9b210b529fc2bd0132958f413f31d6d5`,
linearize 440/
`c1116c1a74de2bc430244484ef032c550c42025835af96790ec3d82b9f489878`, and
SFL 611/
`42845c861bdc99326748df18f7903a59e34c78e5253e2ec24f843aa5648dad80`.

### Differential oracle proof

This is an oracle differential proof, not a clean-master production finding.
A temporary production mutation changed
`forest.LoadLinearization(old_linearization)` to
`forest.LoadLinearization(old_linearization.first(old_linearization.size() - 1))`.
The mutated header SHA-256 was
`e1ee3f989cbf11d9ce2ff4c6f26715cfd520c5991e8863bdaa68822ab615ec73`; the
mutated sanitizer binary SHA-256 was
`146a3df5a39e802cb3f9a46bba4684b94a9edf59eca6b47a03b7565c5dccd879`.

The exact 7-byte input was
`/tmp/bitcoin-clusterlin-20260721/clusterlin_linearize/mutated-artifacts/crash-1b810b5862c7a750c7f5f1bc5618b885505a832f`,
SHA-256
`d6bf0a550c19df04867b1db6f56cf798fa526d78aaf30d8724bc1c8a7693fe38`, bytes
`ff 1f 2c 2c 1f 2c 2c`, Base64 `/x8sLB8sLA==`. The mutated run aborted at
the new `LoadLinearization` size assertion on `cluster_linearize.h:1219`,
from `Linearize:1827` and `cluster_linearize.cpp:1049`, after 12 executed
units. Its log SHA-256 is
`5f28284fd29c01c771cea8ee225f2557b9adb4e52513054332b9a1123a0797f3`.

With the `LoadLinearization` guard removed but the temporary mutation kept,
the same artifact exited 0 with no artifact; the control log SHA-256 is
`90ab048efbe209be97267d3ba793ba6905bb223e8dbde0593b342defbb7bf4df`.
With restored final production source, it also exited 0; the fixed-input
replay log SHA-256 is
`d7e9fd8ce6060518880023a4a75cf5a79b628d3230bb8974829577a5369b3c82`.
This proves the strengthened oracle catches malformed internal caller state
that the old harness accepted. It does not prove a peer-triggerable Bitcoin
Core vulnerability. The final source changed an equivalent `seen[tx_idx]`
check to bounded iteration only to avoid an unrelated GCC warning; the
contract and proof are unchanged.

### Verification and test gap

`git diff --check` passed. `clang-format --dry-run --Werror` still reports
pre-existing violations elsewhere in these files; unrelated formatting was
not changed. The sanitizer and normal fuzz targets were built with:

    cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2
    cmake --build /tmp/bitcoin-secp256k1-audit-current-normal-build --target fuzz -j2

These configurations have no `test_bitcoin` target, so the dedicated unit
suite was unavailable. No production behavior changed, no deterministic
regression test is claimed, and no fuzz, sanitizer, mutation, or replay
process remains running.

## `process_messages` block-index and mempool postcondition audit (2026-07-20)

Source commit: `54c1ca5f5ae3cdedb0eaf206114b6fcb44a8fe8e` (`fuzz: assert Core
state after process_messages`). Its parent is
`515620e10c51afdf26b63cc5f033527aca4acdfd`; the audit base is Bitcoin Core
master `18c05d93016b28a9afd4c716dfe00b6e0accb30b`, identical to
`l0rinc/master`. No l0rinc pull-request commit was relevant to this target.

### Core boundary and severity

`FUZZ=process_messages` drives Bitcoin Core's
`PeerManagerImpl::ProcessMessages()` and `ProcessMessage()` path with
arbitrary peer message types and payloads. That path can reach transaction
acceptance, block-header/block processing, peer state, the block index, and
the mempool. The pre-existing harness caught `std::ios_base::failure` and
only checked final block-index size plus the TX eviction timestamp.

The new harness synchronizes validation callbacks and, after every processing
attempt including the exception path, calls the production
`ChainstateManager::CheckBlockIndex()` and `CTxMemPool::check()` contracts
under `cs_main`. The test setup fixes `check_block_index=1` and mempool
`check_ratio=1`, making these otherwise optional production checks
deterministic. They validate block-index ancestry, map identity, best-header
and candidate structure, plus mempool transaction-graph and `mapNextTx`
consistency.

Severity on master is Informational/Low oracle hardening. Clean master
reproduced no production bug, invalid-block acceptance, consensus failure,
memory/concurrency fault, or cryptographic issue. The input boundary is
untrusted peer data, but this audit found no master behavior that warrants a
High/Critical rating. No production fix or deterministic regression test is
claimed.

### Source and corpus identity

The original harness source SHA-256 was
`b95f6c3be50be6ddff4452bfb82f461193d0012947cc1a2595e2bf5197b0026a`; the
enhanced source SHA-256 is
`e28a67b9ba73456aed24e1390a3642e9015ccc3b8b12737c55c264fab70d9f37`.
Restored production `src/net_processing.cpp` is
`afc14cf644760b60670fa82fb088b03ffa792d421a52c5f6c73b2e67672cf419`.

The frozen corpus is
`/tmp/bitcoin-process-messages-20260720/frozen`: 3,783 files,
49,066,585 bytes, minimum 1 byte, maximum 820,232 bytes. The per-file
`sha256sum` manifest SHA-256 is
`a11698520037b489126246ca6508764ad595e58cb3e685b41da7fe1d3202ee0b`;
the sorted filename-list SHA-256 is
`84f2493ebd0c2f49010acf0f7e0b0e4b38cc197768701b35301e586fd15b81f7`.
All authoritative runs used isolated copies so libFuzzer corpus growth could
not alter the frozen evidence.

### Replay evidence

The original-harness sanitizer baseline used binary SHA-256
`77091629a8c1b35c010dd56ab05578fcdaaeffb09246e9977952478e7ac3417e`, exited
0 after 5,213 executions, and has log SHA-256
`7ede3264a1fcb3957b9c10ca3f3e36413bed6e48fb008b32faab906ece15ce61`.
The enhanced sanitizer binary SHA-256 is
`bbd23e03a665870c4a98e6ef804deed4ed173d3dcdab810614168ccec4626ce7`; the
3,783-file replay exited 0 after 5,219 executions, added no units, produced
no artifacts, and has log SHA-256
`bc78c13771fa7e851261d5c092c12dc132aa830855619fa9d5c2b26990681866`.

The normal standalone fuzz binary SHA-256 is
`29b7fabb8842843a7bb5f6d1a9cf3993637c21b7fe3b3b1780dc4b9a7818fe97`; it
passed all 3,783 files in 13 seconds with no artifacts. Its log SHA-256 is
`341ff147b90a3ee8bace4d60bb444e09a08547289fb67e255bf714045823569e`.
The standalone driver was invoked with only the corpus directory because it
does not parse libFuzzer flags.

Four independent sanitizer workers used
`-max_total_time=60 -timeout=60 -rss_limit_mb=0 -use_value_profile=1
-print_final_stats=1`, isolated corpus copies, and isolated artifact paths.
All exited 0, left 3,783-file copies unchanged, produced no artifacts, and
peaked at 495 MiB. Executions and worker log SHA-256 values were:

    fuzz-0: 5204, c21c0774cf23048201343e6758ea92e45cc4e9989042352fddc492efd4f76969
    fuzz-1: 5207, df4c111b37d9af12625931506c9a2949ecc83c8f107e9615ff6c7916183dffdd
    fuzz-2: 5212, 2eab71258cb6b69539360292198eca0c8472e9a1e86fb1ef83b71f95f96ec4c8
    fuzz-3: 5218, 5187b3d705e23bea09262433d4e141f4a5b4130ab888bba300e4404b04988fd8

### Differential oracle proof

This is an oracle differential proof, not a clean-master production
finding. A temporary production mutation inserted
`LOCK(cs_main); ++m_chainman.ActiveChain().Tip()->nHeight;` immediately after
`ProcessMessage()` in `PeerManagerImpl::ProcessMessages()`. The mutated
production source SHA-256 was
`bb308f8f563dee3c53f670fe0619a2624ee7386cb5237cad812f3c2fd9a0454b`.

The exact frozen input was
`/tmp/bitcoin-process-messages-20260720/frozen/b235c5716d2d54f2bd87c9514a3a3be1a259ffcc`:
81 bytes, SHA-256
`409001a02be0723a0237924654e62ddf0f4c4e1a8088793dfc220a3fbca0fdc5`.
With the enhanced harness, mutated binary SHA-256
`4fae9d441ed958fa54970b2c54848ee079218b42aeeb855021219166e122c7de`
exited 134 after 188 executions at `validation.cpp:5195` in
`CheckBlockIndex()`. The diagnostic log SHA-256 is
`959f802229e8468e6b8d2c20e0bcad9dcbf14da998a28185f2991a2ca8875058`.
The saved crash artifact has the same input SHA-256.

With the original harness and identical production mutation, binary SHA-256
`c3ad9af46eb4ce4b0f6d4cff0b891d6a4a225aa00d53d21ccc3f6f0196c2c988`
exited 0 after one fixed execution with no artifact; control log SHA-256 is
`3b9f6aa0ed63dc2adc1c323ae6c63d82783aee0b11e5aece1ad7e31c8a8d1ee7`.
This proves the old size/timestamp oracle can accept unchanged-size index
metadata corruption while the new production contract detects it. The
mutation was removed before the source commit. With restored production, the
exact input exited 0 in the final sanitizer binary; log SHA-256 is
`a39e859b8569b9c68682da602beae9af37c6b326f5638065754d4330f1fd8180`.

### Verification and test gap

`git diff --check` and `clang-format --dry-run --Werror` passed. Sanitizer
and normal targets were built with:

    cmake --build /tmp/bitcoin-secp256k1-audit-current-build --target fuzz -j2
    cmake --build /tmp/bitcoin-secp256k1-audit-current-normal-build --target fuzz -j2

The configured fuzz-only build has no `test_bitcoin` target, so the dedicated
unit suite was unavailable. No production behavior changed, no production
bug is asserted, and no deterministic regression test was required. No fuzz,
sanitizer, or mutation process remains running.
