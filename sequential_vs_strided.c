/**
 * Section 2.2: Sequential vs strided memory access.
 *
 * We write to an array with different strides. The key metric is
 * effective throughput: how many useful bytes per second do we get?
 *
 * The hardware always fetches 64-byte cache lines from RAM.
 * Sequential access uses all 64 bytes. Strided access wastes most of them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N (64 * 1024 * 1024)  // 64M floats = 256 MB
#define WARMUP_ITERS 3
#define BENCH_ITERS 10

static double time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

__attribute__((noinline)) static void write_s1(float *a, int n) {
    for (int i = 0; i < n; i += 1) a[i] = (float)i;
}
__attribute__((noinline)) static void write_s4(float *a, int n) {
    for (int i = 0; i < n; i += 4) a[i] = (float)i;
}
__attribute__((noinline)) static void write_s16(float *a, int n) {
    for (int i = 0; i < n; i += 16) a[i] = (float)i;
}
__attribute__((noinline)) static void write_s64(float *a, int n) {
    for (int i = 0; i < n; i += 64) a[i] = (float)i;
}

typedef struct {
    const char *name;
    void (*func)(float *, int);
    int stride;
} Benchmark;

int main(void) {
    float *a = (float *)malloc((size_t)N * sizeof(float));
    if (!a) { fprintf(stderr, "malloc failed\n"); return 1; }

    size_t total_bytes = (size_t)N * sizeof(float);
    printf("Array: %d floats (%.0f MB, exceeds L2 cache)\n\n", N,
           total_bytes / (1024.0 * 1024.0));

    printf("%-14s %8s %10s %14s %16s %10s\n",
           "Pattern", "Stride", "Time (ms)", "Writes", "Effective BW", "vs seq.");
    printf("%-14s %8s %10s %14s %16s %10s\n",
           "-------", "------", "---------", "------", "------------", "------");

    Benchmark bms[] = {
        {"Sequential", write_s1,  1},
        {"Stride 4",   write_s4,  4},
        {"Stride 16",  write_s16, 16},
        {"Stride 64",  write_s64, 64},
    };
    int num = sizeof(bms) / sizeof(bms[0]);
    double seq_throughput = 0;

    for (int b = 0; b < num; b++) {
        memset(a, 0, total_bytes);

        for (int i = 0; i < WARMUP_ITERS; i++)
            bms[b].func(a, N);

        double total = 0;
        for (int i = 0; i < BENCH_ITERS; i++) {
            double t0 = time_seconds();
            bms[b].func(a, N);
            double t1 = time_seconds();
            total += (t1 - t0);
        }

        double avg = total / BENCH_ITERS;
        int stride = bms[b].stride;
        int writes = N / stride;

        // Effective bandwidth: only count bytes we actually used
        double useful_bytes = (double)writes * sizeof(float);
        double eff_bw = useful_bytes / avg / 1e9;

        if (b == 0) seq_throughput = eff_bw;

        printf("%-14s %8d %7.1f ms %14d %12.1f GB/s %8.0f%%\n",
               bms[b].name, stride, avg * 1000, writes,
               eff_bw, (eff_bw / seq_throughput) * 100.0);
    }

    printf("\nAll patterns traverse the same 256 MB memory region.\n");
    printf("Stride 16 does 16x fewer writes, but the hardware still loads full\n");
    printf("cache lines (64 bytes). Most fetched bytes are never used.\n");

    free(a);
    return 0;
}
