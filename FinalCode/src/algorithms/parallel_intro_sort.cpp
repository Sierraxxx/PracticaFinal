// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/parallel_intro_sort.cpp
//
//  Parallel IntroSort: chunk-parallel std::sort + bottom-up merge tree.
//  Reference: GCC -D_GLIBCXX_PARALLEL, Microsoft PPL, Intel TBB.
//
//  std::sort is itself an introsort (QuickSort + HeapSort + InsertionSort)
//  so each chunk benefits from the hybrid in-place behavior, and the
//  global O(n log n) bound is preserved by the merge phase.
//
//  COMPLEXITY
//    Best  : Θ(n log n)
//    Avg   : Θ(n log n)
//    Worst : Θ(n log n)        — HeapSort fallback inside std::sort
//    Span  : Θ(log² n)
//    Space : Θ(log n)          — recursion stack only (no merge buffer)
// ════════════════════════════════════════════════════════════════════════════

#include "algorithms.h"
#include "parallel_utils.h"

#include <algorithm>
#include <cstddef>
#include <vector>

void parallel_intro_sort(KeyVec& v, int /*U_unused*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    const unsigned p = pu::hw_threads();
    const size_t   CHUNK =
        std::max<size_t>(16384, n / static_cast<size_t>(p * 2));

    // ── Phase 1: chunk-parallel std::sort ──────────────────────────────────
    {
        const size_t nchunks = (n + CHUNK - 1) / CHUNK;
        pu::parallel_for(nchunks, p, [&](size_t k) {
            const size_t lo = k * CHUNK;
            const size_t hi = std::min(lo + CHUNK, n);
            std::sort(v.begin() + static_cast<std::ptrdiff_t>(lo),
                      v.begin() + static_cast<std::ptrdiff_t>(hi));
        });
    }

    // ── Phase 2: bottom-up merge tree (parallel pairs at each level) ───────
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
