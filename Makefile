CC = cc
CFLAGS_COMMON = -Wall -Wextra

CFLAGS_SEQ = $(CFLAGS_COMMON) -O2
CFLAGS_AOS = $(CFLAGS_COMMON) -O2
CFLAGS_SIMD = $(CFLAGS_COMMON) -O2

all: sequential_vs_strided aos_vs_soa scalar_vs_simd

sequential_vs_strided: sequential_vs_strided.c
	$(CC) $(CFLAGS_SEQ) -o $@ $<

aos_vs_soa: aos_vs_soa.c
	$(CC) $(CFLAGS_AOS) -o $@ $<

scalar_vs_simd: scalar_vs_simd.c
	$(CC) $(CFLAGS_SIMD) -o $@ $<

run: all
	@echo "=== Sequential vs Strided Access ==="
	@./sequential_vs_strided
	@echo ""
	@echo "=== AoS vs SoA Layout ==="
	@./aos_vs_soa
	@echo ""
	@echo "=== Scalar vs SIMD ==="
	@./scalar_vs_simd

clean:
	rm -f sequential_vs_strided aos_vs_soa scalar_vs_simd

.PHONY: all run clean
