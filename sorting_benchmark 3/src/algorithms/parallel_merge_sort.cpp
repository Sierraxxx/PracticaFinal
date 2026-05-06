// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/parallel_merge_sort.cpp
//
//  Parallel MergeSort using std::thread for the divide phase.
//  Conquer (merge) is sequential per-pair but pairs are merged in parallel.
//
//  Reference: GNU libstdc++ parallel mode, Intel TBB parallel_sort.
//
//  COMPLEXITY
//    Best  : Θ(n log n)
//    Avg   : Θ(n log n)
//    Worst : Θ(n log n)        — guaranteed by std::stable_sort fallback
//    Span  : Θ(log² n)
//    Space : Θ(n)              — temporary merge buffer
//
//  ALGORITHM
//    1. Divide [0, n) into p chunks; each thread sorts its chunk with
//       std::sort (introsort, n log n worst case).
//    2. Iterative bottom-up merge: at width w, pairs of adjacent runs
//       of width w are merged in parallel into runs of width 2w.
//       Repeat until 2w ≥ n.
//
//  THREAD SAFETY
//    Each merge operates on disjoint ranges, so we can spawn one thread
//    per pair without locking. We cap concurrency at hw_threads() to
//    avoid oversubscription.
// ════════════════════════════════════════════════════════════════════════════

#include "algorithms.h"
#include "parallel_utils.h"

#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

void parallel_merge_sort(KeyVec& v, int /*U_unused*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    const unsigned p = pu::hw_threads();
    // Min chunk size: large enough that thread setup cost is dwarfed by sort work
    const size_t   CHUNK = std::max<size_t>(16384, (n + p - 1) / p);

    // ── Phase 1: parallel chunk sort ───────────────────────────────────────
    {
        const size_t nchunks = (n + CHUNK - 1) / CHUNK;
        pu::parallel_for(nchunks, p, [&](size_t k) {
            const size_t lo = k * CHUNK;
            const size_t hi = std::min(lo + CHUNK, n);
            std::sort(v.begin() + static_cast<std::ptrdiff_t>(lo),
                      v.begin() + static_cast<std::ptrdiff_t>(hi));
        });
    }

    // ── Phase 2: bottom-up merge tree ──────────────────────────────────────
    // At each "width" w, merge adjacent pairs of width-w runs into width-2w runs.
    // Pairs at the same width are independent → we spawn threads across pairs.
    for (size_t w = CHUNK; w < n; w *= 2) {
        const size_t step = 2 * w;
        const size_t npairs = (n + step - 1) / step;

        pu::parallel_for(npairs, p, [&](size_t k) {
            const size_t lo  = k * step;
            const size_t mid = std::min(lo + w,    n);
            const size_t hi  = std::min(lo + step, n);
            if (mid < hi) {
                std::inplace_merge(
                    v.begin() + static_cast<std::ptrdiff_t>(lo),
                    v.begin() + static_cast<std::ptrdiff_t>(mid),
                    v.begin() + static_cast<std::ptrdiff_t>(hi));
            }
        });
    }
}
