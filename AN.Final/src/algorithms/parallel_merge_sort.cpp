// src/algorithms/parallel_merge_sort.cpp
//
// Parallel MergeSort — true merge-based sort using std::stable_sort per chunk.
// Uses std::stable_sort (not std::sort) to guarantee stable merge semantics
// and avoid confusion with IntroSort. The merge phase is identical but the
// per-chunk sort is explicitly stable, which is a meaningful algorithmic choice.
//
// Complexity:
//   Best  : Θ(n log n)
//   Avg   : Θ(n log n)
//   Worst : Θ(n log n)   — stable_sort guarantees this
//   Space : Θ(n)          — merge buffer allocated by stable_sort + inplace_merge

#include "algorithms.h"
#include "parallel_utils.h"
#include <algorithm>

void parallel_merge_sort(KeyVec& v, int /*U*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    const unsigned p     = pu::hw_threads();
    // Each chunk must be large enough to justify thread overhead.
    // MergeSort uses exactly n/p chunks (no doubling like IntroSort).
    const size_t   CHUNK = std::max<size_t>(8192, (n + p - 1) / p);

    // Phase 1: stable sort each chunk independently (preserves relative order)
    {
        const size_t nchunks = (n + CHUNK - 1) / CHUNK;
        pu::parallel_for(nchunks, p, [&](size_t k) {
            const size_t lo = k * CHUNK;
            const size_t hi = std::min(lo + CHUNK, n);
            // stable_sort: guaranteed O(n log n), uses O(n) buffer internally
            std::stable_sort(
                v.begin() + static_cast<std::ptrdiff_t>(lo),
                v.begin() + static_cast<std::ptrdiff_t>(hi));
        });
    }

    // Phase 2: bottom-up merge — pairs at the same level are independent
    for (size_t w = CHUNK; w < n; w *= 2) {
        const size_t step   = 2 * w;
        const size_t npairs = (n + step - 1) / step;
        pu::parallel_for(npairs, p, [&](size_t k) {
            const size_t lo  = k * step;
            const size_t mid = std::min(lo + w,    n);
            const size_t hi  = std::min(lo + step, n);
            if (mid < hi)
                std::inplace_merge(
                    v.begin() + static_cast<std::ptrdiff_t>(lo),
                    v.begin() + static_cast<std::ptrdiff_t>(mid),
                    v.begin() + static_cast<std::ptrdiff_t>(hi));
        });
    }
}
