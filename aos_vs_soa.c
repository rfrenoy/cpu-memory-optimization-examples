/**
 * Section 2.3: Array of Structures (AoS) vs Structure of Arrays (SoA).
 *
 * Simulates a particle position update to show how data layout
 * affects cache line utilization and throughput.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N (4 * 1024 * 1024)  // 4M particles
#define WARMUP_ITERS 3
#define BENCH_ITERS 10

static double time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Array of Structures
typedef struct {
    float x, y, z;       // position
    float vx, vy, vz;    // velocity
    float mass;
    float charge;         // 32 bytes total
} Particle;

// Structure of Arrays
typedef struct {
    float *x, *y, *z;
    float *vx, *vy, *vz;
    float *mass;
    float *charge;
} Particles;

static void update_aos(Particle *particles, int n, float dt) {
    for (int i = 0; i < n; i++) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].z += particles[i].vz * dt;
    }
}

static void update_soa(Particles *p, int n, float dt) {
    for (int i = 0; i < n; i++) {
        p->x[i] += p->vx[i] * dt;
    }
    for (int i = 0; i < n; i++) {
        p->y[i] += p->vy[i] * dt;
    }
    for (int i = 0; i < n; i++) {
        p->z[i] += p->vz[i] * dt;
    }
}

int main(void) {
    float dt = 0.016f;

    // ---- AoS setup ----
    Particle *aos = (Particle *)malloc((size_t)N * sizeof(Particle));
    if (!aos) { fprintf(stderr, "malloc failed\n"); return 1; }

    srand(42);
    for (int i = 0; i < N; i++) {
        aos[i].x  = (float)rand() / RAND_MAX;
        aos[i].y  = (float)rand() / RAND_MAX;
        aos[i].z  = (float)rand() / RAND_MAX;
        aos[i].vx = (float)rand() / RAND_MAX;
        aos[i].vy = (float)rand() / RAND_MAX;
        aos[i].vz = (float)rand() / RAND_MAX;
        aos[i].mass   = 1.0f;
        aos[i].charge = 0.5f;
    }

    // ---- SoA setup ----
    Particles soa;
    soa.x  = (float *)malloc((size_t)N * sizeof(float));
    soa.y  = (float *)malloc((size_t)N * sizeof(float));
    soa.z  = (float *)malloc((size_t)N * sizeof(float));
    soa.vx = (float *)malloc((size_t)N * sizeof(float));
    soa.vy = (float *)malloc((size_t)N * sizeof(float));
    soa.vz = (float *)malloc((size_t)N * sizeof(float));
    soa.mass   = (float *)malloc((size_t)N * sizeof(float));
    soa.charge = (float *)malloc((size_t)N * sizeof(float));

    for (int i = 0; i < N; i++) {
        soa.x[i]  = aos[i].x;
        soa.y[i]  = aos[i].y;
        soa.z[i]  = aos[i].z;
        soa.vx[i] = aos[i].vx;
        soa.vy[i] = aos[i].vy;
        soa.vz[i] = aos[i].vz;
        soa.mass[i]   = aos[i].mass;
        soa.charge[i] = aos[i].charge;
    }

    printf("Particles: %d (AoS: %.0f MB, SoA: %.0f MB)\n\n",
           N,
           (double)N * sizeof(Particle) / (1024 * 1024),
           (double)N * 8 * sizeof(float) / (1024 * 1024));

    printf("%-20s %12s %12s\n", "Layout", "Time (ms)", "Speedup");
    printf("%-20s %12s %12s\n", "------", "---------", "-------");

    // ---- Benchmark AoS ----
    for (int i = 0; i < WARMUP_ITERS; i++) update_aos(aos, N, dt);

    double total_aos = 0;
    for (int i = 0; i < BENCH_ITERS; i++) {
        double start = time_seconds();
        update_aos(aos, N, dt);
        double end = time_seconds();
        total_aos += (end - start);
    }
    double avg_aos = total_aos / BENCH_ITERS;

    // ---- Benchmark SoA ----
    for (int i = 0; i < WARMUP_ITERS; i++) update_soa(&soa, N, dt);

    double total_soa = 0;
    for (int i = 0; i < BENCH_ITERS; i++) {
        double start = time_seconds();
        update_soa(&soa, N, dt);
        double end = time_seconds();
        total_soa += (end - start);
    }
    double avg_soa = total_soa / BENCH_ITERS;

    printf("%-20s %9.1f ms %11s\n", "AoS", avg_aos * 1000, "1.0x");
    printf("%-20s %9.1f ms %10.1fx\n", "SoA", avg_soa * 1000, avg_aos / avg_soa);

    printf("\nWhy? AoS cache line has 2 particles (32 bytes each), loading mass/charge\n");
    printf("we don't need. SoA cache line has 16 consecutive x (or vx) values, all used.\n");

    // Cleanup
    free(aos);
    free(soa.x); free(soa.y); free(soa.z);
    free(soa.vx); free(soa.vy); free(soa.vz);
    free(soa.mass); free(soa.charge);
    return 0;
}
