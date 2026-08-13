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
