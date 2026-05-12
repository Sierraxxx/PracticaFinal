#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  include/dialsort.h
//
//  DialSort — Non-comparative integer sorting via the self-indexing principle.
//
//  ARCHITECTURAL OVERVIEW
//  ──────────────────────
//  DialSort is a UNIFIED algorithm presented through a single entry point.
//  Internally it dispatches to the appropriate strategy based on the
//  detected universe size U = max - min + 1:
//
//        ┌─────────────────────────────────────────────────────────┐
//        │                  dialsort(v)                            │
//        │                       │                                 │
//        │                       ▼                                 │
//        │             [scan: detect mn, mx, U]                    │
//        │                       │                                 │
//        │              ┌────────┴────────┐                        │
//        │              ▼                 ▼                        │
//        │        U ≤ U_MAX           U > U_MAX                   │
//        │              │                 │                        │
//        │              ▼                 ▼                        │
//        │      ┌──────────────┐   ┌──────────────┐                │
//        │      │ CRN strategy │   │ LSD strategy │                │
//        │      │ (histogram)  │   │ (4 passes)   │                │
//        │      └──────────────┘   └──────────────┘                │
//        └─────────────────────────────────────────────────────────┘
//
//  CORE PRINCIPLE — Self-Indexing
//  ───────────────────────────────
//  For any integer key k ∈ [mn, mx], the value (k - mn) IS the address
//  of its frequency in H[]. No comparison (<, >, ≤) is performed at any
//  point. Sorting reduces to:
//
//      Phase 1 (ingestion):    H[k - mn]++       — auto-indexation
//      Phase 2 (projection):   ∀y, emit (y + mn) exactly H[y] times
//
//  This is FUNDAMENTALLY different from comparison sorts (n log n lower
//  bound) and even from classic CountingSort (which still requires a
//  prefix sum).
//
//  PUBLIC API
//  ──────────
//      DialSort::sort(vec)            // automatic strategy selection
//      DialSort::sort(vec, opts)      // with custom options
//      DialSort::sort_with_report(vec, &report)  // detailed metrics
// ════════════════════════════════════════════════════════════════════════════

#include "types.h"
#include <cstdint>
#include <string>

namespace DialSort {

// ── Maximum universe size for the CRN (histogram) strategy ──────────────────
// Beyond this threshold (~40 MB of histogram memory), the LSD radix variant
// is provably faster due to cache thrashing on the projection phase.
inline constexpr uint64_t MAX_U_HISTOGRAM = 10'000'000ULL;

// ── Strategy actually executed ──────────────────────────────────────────────
enum class Strategy : uint8_t {
    Auto      = 0,  ///< let DialSort decide based on detected U
    CRN       = 1,  ///< force histogram + parallel ingestion
    LSDRadix  = 2,  ///< force 4-pass LSD radix
    Sequential= 3,  ///< force single-thread CRN (debug / tiny inputs)
};

// ── Configuration for advanced callers ──────────────────────────────────────
struct Options {
    Strategy strategy   = Strategy::Auto;
    int      threads    = 0;     ///< 0 = hardware_concurrency()
    bool     verbose    = false; ///< log strategy choice to stderr
};

// ── Detailed execution report (for benchmarking / visualization) ────────────
struct Report {
    Strategy    strategy_used  = Strategy::Auto;
    std::string strategy_name;            ///< human-readable
    int         min_value      = 0;
    int         max_value      = 0;
    uint64_t    universe_size  = 0;       ///< U = max - min + 1
    uint64_t    aux_memory_bytes = 0;     ///< exact auxiliary memory used
    int         threads_used   = 1;
    double      ingestion_ms   = 0.0;
    double      merge_ms       = 0.0;
    double      projection_ms  = 0.0;
    double      total_ms       = 0.0;
    bool        success        = false;   ///< false if input invalid
    std::string error_message;            ///< populated on failure
};

// ════════════════════════════════════════════════════════════════════════════
//  PUBLIC API — three overloads of increasing detail
// ════════════════════════════════════════════════════════════════════════════

/// Sort `v` in place using automatic strategy selection.
/// Throws nothing. Returns false only if input contains invalid data
/// (e.g. NaN-like sentinels) — extremely rare for int.
bool sort(KeyVec& v) noexcept;

/// Sort with caller-controlled options (force a strategy, set thread count).
bool sort(KeyVec& v, const Options& opts) noexcept;

/// Sort and populate a detailed Report with metrics for each phase.
/// Useful for benchmarking and the visualizer.
bool sort_with_report(KeyVec& v, Report& report) noexcept;
bool sort_with_report(KeyVec& v, const Options& opts, Report& report) noexcept;

// ── Utility: human-readable strategy name ───────────────────────────────────
const char* strategy_name(Strategy s) noexcept;

} // namespace DialSort
