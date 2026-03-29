/**
 * Section 3.1: Scalar vs SIMD vector add.
 *
 * Compares a true scalar loop (auto-vectorization disabled) with
 * explicit SIMD using NEON (ARM) or AVX2 (x86).
 *
 * On ARM (Apple Silicon): NEON processes 4 floats per instruction.
 * On x86: AVX2 processes 8 floats, AVX-512 processes 16.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_SIMD 1
#define SIMD_WIDTH 4
#define SIMD_NAME "NEON (4 floats/op)"
#elif defined(__AVX2__)
#include <immintrin.h>
#define HAS_SIMD 1
#define SIMD_WIDTH 8
#define SIMD_NAME "AVX2 (8 floats/op)"
#else
#define HAS_SIMD 0
#define SIMD_WIDTH 1
#define SIMD_NAME "none"
#endif

#define N (64 * 1024 * 1024)  // 64M floats = 256 MB per array
#define WARMUP_ITERS 3
#define BENCH_ITERS 10

static double time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// True scalar: disable auto-vectorization with pragma
__attribute__((noinline))
static void vector_add_scalar(const float * restrict a,
                              const float * restrict b,
                              float * restrict c, int n) {
    #pragma clang loop vectorize(disable) interleave(disable)
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

// Let the compiler auto-vectorize freely
__attribute__((noinline))
static void vector_add_auto(const float * restrict a,
                            const float * restrict b,
                            float * restrict c, int n) {
    #pragma clang loop vectorize(enable) interleave(enable)
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

// Explicit SIMD intrinsics
__attribute__((noinline))
static void vector_add_simd(const float * restrict a,
                            const float * restrict b,
                            float * restrict c, int n) {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    int i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vc = vaddq_f32(va, vb);
        vst1q_f32(&c[i], vc);
    }
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
#elif defined(__AVX2__)
    int i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&c[i], vc);
    }
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
#else
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
#endif
}

typedef struct {
    const char *name;
    void (*func)(const float *, const float *, float *, int);
} Benchmark;

int main(void) {
    float *a = (float *)malloc((size_t)N * sizeof(float));
    float *b = (float *)malloc((size_t)N * sizeof(float));
    float *c = (float *)malloc((size_t)N * sizeof(float));
    if (!a || !b || !c) { fprintf(stderr, "malloc failed\n"); return 1; }

    srand(42);
    for (int i = 0; i < N; i++) {
        a[i] = (float)rand() / RAND_MAX;
        b[i] = (float)rand() / RAND_MAX;
    }

    size_t total_bytes = 3UL * N * sizeof(float);  // 2 reads + 1 write
    printf("Array: %d floats (%.0f MB per array, %.0f MB total traffic)\n",
           N,
           (double)N * sizeof(float) / (1024 * 1024),
           (double)total_bytes / (1024 * 1024));
    printf("SIMD available: %s\n\n", SIMD_NAME);

    printf("%-28s %10s %10s %10s\n", "Method", "Time (ms)", "BW (GB/s)", "Speedup");
    printf("%-28s %10s %10s %10s\n", "------", "---------", "---------", "-------");

    Benchmark bms[] = {
        {"Scalar (vectorize off)",   vector_add_scalar},
        {"Auto-vectorized",          vector_add_auto},
        {"Explicit SIMD intrinsics", vector_add_simd},
    };
    int num = sizeof(bms) / sizeof(bms[0]);
    double baseline = 0;

    for (int bi = 0; bi < num; bi++) {
        for (int i = 0; i < WARMUP_ITERS; i++)
            bms[bi].func(a, b, c, N);

        double total = 0;
        for (int i = 0; i < BENCH_ITERS; i++) {
            double t0 = time_seconds();
            bms[bi].func(a, b, c, N);
            double t1 = time_seconds();
            total += (t1 - t0);
        }

        double avg = total / BENCH_ITERS;
        double bw = (double)total_bytes / avg / 1e9;
        if (bi == 0) baseline = avg;

        printf("%-28s %7.1f ms %7.1f %8.1fx\n",
               bms[bi].name, avg * 1000, bw, baseline / avg);
    }

    // Verify
    vector_add_simd(a, b, c, 4);
    float expected = a[0] + b[0];
    printf("\nVerification: c[0] = %.6f (expected %.6f) %s\n",
           c[0], expected, (c[0] == expected) ? "OK" : "MISMATCH");

    free(a); free(b); free(c);
    return 0;
}
