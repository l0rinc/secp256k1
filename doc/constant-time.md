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
