// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/parallel_radix_sort.cpp
//
//  Parallel LSD RadixSort, base 256, 4 passes (covers full int32 range).
//  Built on std::thread — no OpenMP, no external dependencies.
//
//  Reference: NVIDIA CUB, AMD rocPRIM, Intel oneTBB radix_sort.
//
//  COMPLEXITY
//    Best  : Θ(d · n)   d = 4 (passes for int32)
//    Avg   : Θ(d · n)   distribution-independent
//    Worst : Θ(d · n)   deterministic
//    Space : Θ(n + p·B) two buffers + per-thread histograms (B = 256)
//
//  ALGORITHM
//    Per pass (LSD: pass 0 = lowest byte):
//      1. Each thread builds a private 256-entry histogram for its chunk.
//      2. Sequential merge of histograms (256 × p entries — trivial).
//      3. Exclusive prefix sum to get bucket starts.
//      4. Each thread scatters its chunk into the destination buffer
//         using a thread-local cursor that starts at the global bucket
//         offset corresponding to that thread's contribution.
//
//  Step 4 is the subtle part: to avoid write conflicts, every thread must
//  know "the position in the destination at which my elements going to
//  bucket b begin". We compute this by reusing each thread's histogram
//  AS the cursor: after the merge we rewrite locals[t][b] so it holds
//  the absolute starting offset for thread t into bucket b.
// ════════════════════════════════════════════════════════════════════════════

#include "algorithms.h"
#include "parallel_utils.h"

#include <algorithm>
#include <cstdint>
#include <vector>

void parallel_radix_sort(KeyVec& v, int /*U_unused*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    constexpr int B = 256;
    const unsigned p = pu::hw_threads();

    // Bias signed → unsigned so that unsigned ascending order
    // == signed ascending order (handles negatives correctly).
    std::vector<unsigned> src(n), dst(n);
    pu::parallel_for(n, p, [&](size_t i) {
        src[i] = static_cast<unsigned>(v[i]) ^ 0x80000000u;
    });

    // Per-thread private histograms reused across passes.
    std::vector<std::vector<int>> locals(p, std::vector<int>(B, 0));

    for (int pass = 0; pass < 4; ++pass) {
        const int shift = pass * 8;

        // Reset thread-local histograms
        for (auto& h : locals) std::fill(h.begin(), h.end(), 0);

        // Step 1: Each thread builds its private histogram
        pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) {
            auto& H = locals[t];
            for (size_t i = lo; i < hi; ++i)
                H[(src[i] >> shift) & 0xFF]++;
        });

        // Step 2 + 3: Merge into global counts and convert each
        // locals[t][b] into the absolute starting offset for thread t,
        // bucket b.
        std::vector<int> totals(B, 0);
        for (int b = 0; b < B; ++b) {
            int running = 0;
            for (unsigned t = 0; t < p; ++t) {
                int c = locals[t][b];
                locals[t][b] = totals[b] + running;  // not yet shifted
                running += c;
            }
            totals[b] = running;  // total elements in bucket b
        }

        // Convert totals to exclusive prefix sums (bucket starts in dst).
        int running = 0;
        for (int b = 0; b < B; ++b) {
            const int t = totals[b];
            totals[b] = running;
            running += t;
        }
        // Add the bucket-base offsets to each thread's cursor:
        // locals[t][b] becomes the global position where thread t starts
        // writing into bucket b.
        for (int b = 0; b < B; ++b)
            for (unsigned t = 0; t < p; ++t)
                locals[t][b] += totals[b];

        // Step 4: Each thread scatters its chunk using its private cursor.
        pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) {
            auto& cursor = locals[t];
            for (size_t i = lo; i < hi; ++i) {
                const int b = (src[i] >> shift) & 0xFF;
                dst[cursor[b]++] = src[i];
            }
        });

        std::swap(src, dst);
    }

    // 4 passes is even → result is in src (which aliases the original
    // src vector, not v). Copy back to v.
    pu::parallel_for(n, p, [&](size_t i) {
        v[i] = static_cast<int>(src[i] ^ 0x80000000u);
    });
}
