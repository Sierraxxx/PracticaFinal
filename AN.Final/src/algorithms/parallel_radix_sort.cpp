// src/algorithms/parallel_radix_sort.cpp
//
// Parallel RadixSort LSD base-256, 4 passes (full int32 range).
// Built on std::thread via pu:: — no OpenMP, no external dependencies.
//
// Complexity:
//   Best  : Θ(d·n)      d = 4 (passes for int32)
//   Avg   : Θ(d·n)      distribution-independent
//   Worst : Θ(d·n)      deterministic
//   Space : Θ(n + p·B)  two buffers + per-thread histograms (B=256)

#include "algorithms.h"
#include "parallel_utils.h"
#include <algorithm>
#include <vector>

void parallel_radix_sort(KeyVec& v, int /*U*/) {
    const size_t   n = v.size();
    if (n <= 1) return;

    constexpr int    B       = 256;
    constexpr size_t B_SIZE  = static_cast<size_t>(B);
    const unsigned   p       = pu::hw_threads();
    const size_t     p_size  = static_cast<size_t>(p);

    // Bias signed → unsigned: maps INT_MIN→0 … INT_MAX→UINT_MAX
    std::vector<unsigned> src(n), dst(n);
    pu::parallel_for(n, p, [&](size_t i) {
        src[i] = static_cast<unsigned>(v[i]) ^ 0x80000000u;
    });

    // Per-thread private histograms
    std::vector<std::vector<int>> locals(p_size, std::vector<int>(B_SIZE, 0));

    for (int pass = 0; pass < 4; ++pass) {
        const int shift = pass * 8;

        // Reset
        for (auto& h : locals) std::fill(h.begin(), h.end(), 0);

        // Step 1: each thread builds its private histogram
        pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) {
            auto& H = locals[static_cast<size_t>(t)];
            for (size_t i = lo; i < hi; ++i)
                H[static_cast<size_t>((src[i] >> shift) & 0xFF)]++;
        });

        // Step 2: merge histograms + compute per-thread offsets
        std::vector<int> totals(B_SIZE, 0);
        for (size_t b = 0; b < B_SIZE; ++b) {
            int running = 0;
            for (size_t t = 0; t < p_size; ++t) {
                int c             = locals[t][b];
                locals[t][b]      = totals[b] + running;
                running          += c;
            }
            totals[b] = running;
        }

        // Step 3: exclusive prefix sum over bucket totals
        int running = 0;
        for (size_t b = 0; b < B_SIZE; ++b) {
            const int t = totals[b];
            totals[b]   = running;
            running    += t;
        }

        // Step 4: add bucket base offsets to per-thread cursors
        for (size_t b = 0; b < B_SIZE; ++b)
            for (size_t t = 0; t < p_size; ++t)
                locals[t][b] += totals[b];

        // Step 5: parallel scatter
        pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) {
            auto& cursor = locals[static_cast<size_t>(t)];
            for (size_t i = lo; i < hi; ++i) {
                const size_t b = static_cast<size_t>((src[i] >> shift) & 0xFF);
                dst[static_cast<size_t>(cursor[b]++)] = src[i];
            }
        });

        std::swap(src, dst);
    }

    // Copy back with unsigned→signed de-bias
    pu::parallel_for(n, p, [&](size_t i) {
        v[i] = static_cast<int>(src[i] ^ 0x80000000u);
    });
}
