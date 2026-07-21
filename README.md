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
