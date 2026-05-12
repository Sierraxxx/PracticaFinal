// src/algorithms/parallel_intro_sort.cpp
//
// Parallel IntroSort — chunk-parallel std::sort + bottom-up merge tree.
// std::sort implements IntroSort: QuickSort (fast avg) + HeapSort (worst-case
// guarantee) + InsertionSort (small arrays). This is what production compilers
// ship. The merge phase uses inplace_merge which requests O(n) buffer when
// available, falling back to O(1) extra space at a log-factor cost.
//
// Complexity:
//   Best  : Θ(n log n)
//   Avg   : Θ(n log n)
//   Worst : Θ(n log n)   — HeapSort fallback inside std::sort
//   Space : Θ(n) practical — inplace_merge requests O(n) buffer when it can
//           Θ(log n) theoretical — if OS denies the buffer allocation

#include "algorithms.h"
#include "parallel_utils.h"
#include <algorithm>

void parallel_intro_sort(KeyVec& v, int /*U*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    const unsigned p = pu::hw_threads();
    // Smaller chunks than MergeSort: IntroSort handles cache effects better
    // with tighter chunks due to QuickSort's locality behavior.
    const size_t CHUNK = std::max<size_t>(16384,
                                          n / static_cast<size_t>(p * 2));

    // Phase 1: each thread runs std::sort (= IntroSort) on its chunk
    {
        const size_t nchunks = (n + CHUNK - 1) / CHUNK;
        pu::parallel_for(nchunks, p, [&](size_t k) {
            const size_t lo = k * CHUNK;
            const size_t hi = std::min(lo + CHUNK, n);
            // std::sort = IntroSort: unstable, in-place, O(n log n) worst case
            std::sort(
                v.begin() + static_cast<std::ptrdiff_t>(lo),
                v.begin() + static_cast<std::ptrdiff_t>(hi));
        });
    }

    // Phase 2: bottom-up merge (same as MergeSort — merge is merge)
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
