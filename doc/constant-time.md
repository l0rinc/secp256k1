# Constant-Time Execution

The library's secret-key operations are written to avoid data-dependent control flow and memory access.
These source properties do not by themselves guarantee that every compiler and processor combination executes in constant time.
Constant-time assurance applies to the final binary for a specific compiler, target, CPU feature baseline, optimization, and linker configuration.

## Compiler-generated scalar branches

The 8x32 scalar backend converts unsigned overflow comparisons into integer carry values.
Its multiplication macros contain operations equivalent to:

```c
c0 += tl;
th += (c0 < tl);
c1 += th;
c2 += (c1 < th);
```

The comparisons are intended to produce the values zero or one.
Some compiler backends instead implement them with conditional branches whose direction depends on secret-derived scalar limbs.

A source-faithful `-O2` compiler matrix produced the following representative branch counts.
"Scalar paths" combines the reviewed scalar operations, while "key generation" covers the initial fixed-base scalar-add path.

| Target configuration | Scalar paths | Key generation |
| --- | ---: | ---: |
| x86-64 Clang | 0 | 0 |
| AArch64 | 0 | 0 |
| Thumb-1, Cortex-M0 | 41 | 10 |
| Thumb-2, Cortex-M3 | 0 | 0 |
| RV32 base ISA | 3 | 1 |
| RV32 with Zicond | 0 | 0 |
| Generic PPC32 | 46 | 13 |
| PPC32 pwr8 | 0 | 0 |
| Generic PPC64 big-endian | 12 | 2 |
| SystemZ default, 4x64 | 12 | 3 |
| SystemZ z196 | 0 | 0 |
| SystemZ z13 vector | 6 | 2 |
| SystemZ z13 without vector lowering | 0 | 0 |
| SystemZ default, 8x32 | 35 | 10 |

The paired results show that this is a compiler and instruction-set property rather than an inherent loop branch.
Thumb-2 conditional execution, RISC-V Zicond, PowerPC `isel`, and selected SystemZ conditional-load configurations avoid the tested branches, while nearby baselines without the required lowering do not.
Selected i386, i486, and Pentium configurations also generated secret-dependent carry branches; selected i686 configurations did not.

These observations are compiler-version and optimization dependent.
They do not imply that every binary for a listed architecture is affected or that every generated branch is measurable on hardware.
The compiler identity, flags, preprocessed source, object, and linked binary must accompany any claim about a deployment tuple.

Valgrind and MemorySanitizer constant-time tests remain useful for the binary and host on which they run.
They cannot establish the generated control flow of an unrelated cross target.
Optimized compiler IR is also insufficient because a target backend can introduce branches later.

## Generic PPC32 trace and secret recovery

The strongest analyzed instance is a Clang 17 `-O2` generic-PPC32 scalar program.
Its 512-bit multiplication core contains 124 conditional branch sites, and scalar reduction adds another 104 sites.
A valid scalar multiplication executes 226 conditional branches in this model.
With one multiplier fixed, 185 of the 228 sites changed outcome over sampled secret inputs.

A restricted interpreter executed the exact generated multiplication program for 1,000 random valid scalar pairs.
Every result matched exact `(a * b) mod n`, and all 124 multiplication-core branch outcomes matched an independent carry model.
For 5,000 valid secrets with one public multiplier fixed, the interpreted path executed between 1,771 and 1,801 instructions, with 31 distinct counts and approximately 4.21 bits of empirical instruction-count entropy.

These numbers are specific to that compiler generation and CPU baseline.
LLVM 20 made the tested multiplication core branchless but retained 28 conditional sites in reduction and final order handling on classic PPC32 targets.
The tested pwr8 path used integer select instructions and contained no conditional branches in the same scalar path.

### MuSig key aggregation coefficients

MuSig partial signing parity-adjusts the secret key and multiplies it by the participant's public KeyAgg coefficient.
A malicious co-signer can vary its valid public key to produce many protocol-valid coefficients while the victim key remains fixed.

The analyzed campaign independently recomputed 64 two-party BIP327 key lists, victim coefficients, and aggregate-key parities with separate elliptic-curve code and an OpenSSL implementation.
The scalar products use the same secret-first, public-coefficient-second operand order as the generated PPC32 model.

The recovery oracle does not reveal a complete branch trace.
For each selected short unrolled multiplication slice, it returns only the number of taken carry branches in that slice.
Candidates for each 32-bit secret limb are tested against observations collected before later unknown limbs enter the relevant partial-product schedule.
Across three independent test keys, each limb became unique after 7 through 24 protocol observations, depending on the key and limb.
All three 256-bit keys were recovered exactly.

### Multiplicative secret-key tweaks

`secp256k1_ec_seckey_tweak_mul` exposes the same scalar operand order more directly: the secret key is the first operand and the caller-provided valid tweak is the second.
Using 64 valid nonzero tweak factors and the same short-slice oracle, the analyzed campaign recovered all eight limbs of one test secret exactly.

This scenario requires the application to restore the same original secret before every query because the API mutates its input buffer.
An ordinary one-way tweak chain does not provide the demonstrated repeated-query surface.

### Evidence boundary

The short-slice oracle is substantially stronger than whole-call timing or a total branch count.
No complete target-linked library was executed on affected PowerPC hardware.
No timing, performance-counter, branch-predictor, cache, power, electromagnetic, or remote channel has demonstrated access to the selected slice counts.
The recoveries establish the information present in this exact generated branch trace under the stated oracle; they do not establish a deployable attack.

An earlier ECDSA recovery model placed the secret key in the first scalar multiplication operand and public `r` in the second.
The implementation computes `public r * secret key` instead.
The scalar product is algebraically commutative, but the generated carry trace depends on operand order.
The earlier complete ECDSA recovery is therefore retracted; no corrected complete ECDSA recovery has been validated.

## Operand-dependent multiplication latency

A fixed instruction sequence can still take secret-dependent time when the processor's integer multiplier finishes early for small operands.
This affects constant-time reasoning even when the compiler emits no secret-dependent branches.

### Fixed-step scalar inversion

The 32-bit safegcd scalar inverse executes a fixed 20 rounds of 30 divsteps.
Its update routines perform approximately 1,800 multiplications with secret-derived operands during one inverse.
Software models using documented early-termination rules produced different aggregate costs for different inputs:

| Multiplier model | Modeled aggregate range | Distinct totals in 20,000 samples |
| --- | ---: | ---: |
| ARM7TDMI | 4110 through 4662 | 440 |
| MIPS 4Kc | 2178 through 2637 | 226 |

For 64 verified deterministic ECDSA signatures, the modeled inverse component produced 57 distinct ARM7TDMI totals and 45 distinct MIPS 4Kc totals.
Repeating a key and message repeats the RFC6979 nonce and therefore the modeled value, creating an averaging target.
No physical timing distribution, nonce recovery, or signing-key recovery has been demonstrated from this model.

### GLV decomposition

Constant-time arbitrary-point multiplication splits a secret scalar into two approximately 128-bit GLV components.
The decomposition uses scalar products whose operands depend on the secret.

On the 8x32 scalar backend, a positive small split component has zero high limbs while its negative representation lies near the group order and has high limbs close to `0xffffffff`.
Under the same early-multiplier models, aggregate modeled multiplication cost classified the sign of the second component with 100% accuracy in the ARM7TDMI model and 99.9992% accuracy in the MIPS 4Kc model over 500,000 valid scalars.

That sign is one predicate of a transformed scalar, not a literal private-key bit.
No method has converted it into complete key recovery.
The model is relevant to callers such as ECDH and ElligatorSwift XDH only when the selected scalar backend, compiler operand placement, processor multiplier, and measurement channel match its assumptions.

Supported constant-time configurations must use integer multiplication with data-independent timing or an audited fixed-latency implementation for secret-derived operands.
Fixed loop counts and branchless disassembly are insufficient on processors without that property.

## Build and deployment assurance

Constant-time behavior is a property of a complete build tuple and its execution environment.
A supported tuple must identify:

* The compiler identity and version.
* The target architecture and CPU feature baseline.
* The selected field and scalar backends.
* Optimization, LTO, assembly, and linker settings.
* Any required data-independent-timing processor mode.

The corresponding assurance record should retain the preprocessed secret-bearing translation units, object files, final linked binary, and annotated disassembly.
Review must trace each conditional predicate to secret, public, or fixed-loop state.
Counting branch mnemonics alone produces false positives from public loops and deterministic backedges.

Compiler upgrades, optimization changes, and CPU-baseline changes require renewed review.
CI can make that boundary visible by building pinned target configurations and rejecting unreviewed changes to secret-bearing final control flow or calls to variable-time runtime helpers.
The generated-code check should complement existing Valgrind and MemorySanitizer tests rather than replace them.

Target-side execution is required to check instruction behavior that disassembly cannot establish.
For affected classic PPC32 and other historical targets, the next evidence step is to build the complete library and a representative application, confirm the sites in the linked image, and collect timing, retired-branch, branch-miss, power, or electromagnetic measurements with fixed public multipliers and selected secret witnesses.
Any key-recovery claim should include stable trace alignment, negative controls, noise classification, and held-out recovery.

There is no verified portable-C rewrite that removes every observed branch across the affected targets.
Rewriting direct comparisons removed some branches but left multiplication carry branches and other scalar operations on several configurations.
Where an older target must remain supported, use an audited target-specific primitive that provides data-independent control flow and instruction timing.
Do not enable an instruction such as PowerPC `isel` unless the configured processor implements it.

Until a tuple satisfies these requirements, claims should be limited to the tested source properties and host configuration rather than generalized to every supported compiler and processor.

Related open discussions include [issue #784](https://github.com/bitcoin-core/secp256k1/issues/784) and [issue #1138](https://github.com/bitcoin-core/secp256k1/issues/1138).
