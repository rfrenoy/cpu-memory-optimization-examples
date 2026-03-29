# CPU Memory Optimization Examples

Companion code for the blog post [RAM vs VRAM: A Complete Guide to How CPUs and GPUs Use Memory](https://rfrenoy.github.io/blog/why-llm-inference-slower-on-cpu/).

These benchmarks demonstrate three key CPU memory optimization concepts with measurable performance differences.

## Benchmarks

### 1. Sequential vs Strided Access (Section 2.2)

Shows how access patterns affect effective memory bandwidth. The hardware always fetches 64-byte cache lines from RAM. Sequential access uses all 64 bytes; strided access wastes most of them.

### 2. AoS vs SoA Layout (Section 2.3)

Compares Array-of-Structures vs Structure-of-Arrays for a particle position update. AoS loads unused fields (mass, charge) into cache lines. SoA keeps each field contiguous, so every byte fetched is useful.

### 3. Scalar vs SIMD (Section 3.1)

Compares scalar, auto-vectorized, and explicit SIMD (NEON on ARM, AVX2 on x86) vector addition. Shows how SIMD increases throughput by processing multiple elements per instruction.

## Build and Run

```bash
make run
```

Requires a C compiler (GCC or Clang). Tested on Apple Silicon (ARM/NEON) and x86-64 (AVX2).

## Sample Output (Apple M2)

```
=== Sequential vs Strided Access ===
Array: 67108864 floats (256 MB, exceeds L2 cache)

Pattern          Stride  Time (ms)         Writes     Effective BW    vs seq.
-------          ------  ---------         ------     ------------     ------
Sequential            1     4.4 ms       67108864         60.5 GB/s      100%
Stride 4              4     7.4 ms       16777216          9.0 GB/s       15%
Stride 16            16     7.1 ms        4194304          2.4 GB/s        4%
Stride 64            64     5.1 ms        1048576          0.8 GB/s        1%

=== AoS vs SoA Layout ===
Particles: 4194304 (AoS: 128 MB, SoA: 128 MB)

Layout                  Time (ms)      Speedup
------                  ---------      -------
AoS                        3.7 ms        1.0x
SoA                        2.2 ms        1.7x

=== Scalar vs SIMD ===
Array: 67108864 floats (256 MB per array, 768 MB total traffic)
SIMD available: NEON (4 floats/op)

Method                        Time (ms)  BW (GB/s)    Speedup
------                        ---------  ---------    -------
Scalar (vectorize off)          21.8 ms    37.0      1.0x
Auto-vectorized                 12.3 ms    65.6      1.8x
Explicit SIMD intrinsics        12.3 ms    65.5      1.8x
```
