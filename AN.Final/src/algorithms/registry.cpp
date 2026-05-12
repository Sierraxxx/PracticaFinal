// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/registry.cpp
//
//  Single source of truth for the 5 algorithms compared in the benchmark.
//  Each entry includes: display name, short name (for table headers), the
//  C-style sort function, the bounded-U flag, and the Big-O complexity
//  in the four standard cases (best, average, worst, space).
//
//  The order here determines display order in tables and chart legends.
// ════════════════════════════════════════════════════════════════════════════

#include "algorithms.h"

std::vector<AlgoInfo> get_all_algorithms() {
    return {
        // ── 1. DialSort (paper algorithm — single unified entry) ──────────
        {
            "DialSort",
            "DialSort",
            dialsort_unified,
            /*needs_bounded_U=*/ false,  // dispatcher handles all ranges
            "O(n + U)",                  // best  — small U, single pass
            "O(n/p + U)",                // avg   — CRN parallel ingestion
            "O(n log n)",                // worst — falls back to LSD which is O(4n) ≈ O(n) anyway, kept conservative
            "O(p·U) or O(n)"             // CRN: p·U; LSD: n
        },
        // ── 2. Parallel RadixSort ─────────────────────────────────────────
        {
            "Parallel RadixSort",
            "P-Radix",
            parallel_radix_sort,
            false,
            "O(d·n)",                    // d = number of digits = 4 for int32
            "O(d·n)",
            "O(d·n)",
            "O(n + p·B)"                 // B = base = 256
        },
        // ── 3. Parallel MergeSort ─────────────────────────────────────────
        {
            "Parallel MergeSort",
            "P-Merge",
            parallel_merge_sort,
            false,
            "O(n log n)",                // identical for all cases (stable)
            "O(n log n)",
            "O(n log n)",
            "O(n)"                       // stable_sort + inplace_merge buffers
        },
        // ── 4. Parallel SampleSort ────────────────────────────────────────
        {
            "Parallel SampleSort",
            "P-Sample",
            parallel_sample_sort,
            false,
            "O(n log p)",                // p = number of threads (splitters)
            "O(n log p)",
            "O(n log n)",                // degenerate: bad splitters
            "O(n + p²)"                  // local buckets per thread
        },
        // ── 5. Parallel IntroSort ─────────────────────────────────────────
        {
            "Parallel IntroSort",
            "P-Intro",
            parallel_intro_sort,
            false,
            "O(n log n)",                // QS+HS+IS hybrid bounds it
            "O(n log n)",
            "O(n log n)",                // HeapSort fallback in worst case
            "O(n) practical"             // inplace_merge requests O(n) buffer
        },
    };
}
