#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  include/algorithms.h
//
//  Public declarations of the 5 sorting algorithms compared in the benchmark.
//
//  ALGORITHMS
//  ──────────
//  Public declarations of the 5 sorting algorithms compared in the benchmark.
//    • Parallel RadixSort     — comparative #1
//    • Parallel MergeSort     — comparative #2
//    • Parallel SampleSort    — comparative #3
//    • Parallel IntroSort     — comparative #4
//
//  The DialSort entry is unified — it uses strategy dispatch internally.
//  See include/dialsort.h for the rich namespace API; this header only
//  exposes the C-style adapter used by the benchmark registry.
// ════════════════════════════════════════════════════════════════════════════

#include "types.h"
#include <vector>

// ─── DialSort (unified entry point) ─────────────────────────────────────────
/// Adapter for the benchmark engine. Internally dispatches to CRN, LSDRadix
/// or Sequential based on the detected universe size.
/// The `U` parameter is ignored — DialSort detects U from the data.
void dialsort_unified(KeyVec& v, int U);

// ─── 4 Comparative parallel algorithms ──────────────────────────────────────
/// Parallel LSD radix sort, base 256, std::thread parallel histograms.
/// Reference: NVIDIA CUB, AMD rocPRIM, Intel oneTBB.
void parallel_radix_sort(KeyVec& v, int U);

/// Parallel merge sort with std::thread (recursive divide and conquer).
/// Reference: GNU libstdc++ parallel mode, Intel TBB parallel_sort.
void parallel_merge_sort(KeyVec& v, int U);

/// Parallel sample sort: random sampling → splitters → bucket → local sort.
/// Reference: Sanders & Winkel ESA 2004; IPS4o (Axtmann et al. 2022).
void parallel_sample_sort(KeyVec& v, int U);

/// Parallel introsort: chunk-parallel std::sort + iterative bottom-up merge.
/// Reference: GCC -D_GLIBCXX_PARALLEL, Microsoft PPL, Intel TBB.
void parallel_intro_sort(KeyVec& v, int U);

// ─── Global registry ────────────────────────────────────────────────────────
/// Returns the 5 algorithms with their Big-O metadata, in display order.
std::vector<AlgoInfo> get_all_algorithms();
