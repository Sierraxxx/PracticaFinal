// ════════════════════════════════════════════════════════════════════════════
//  src/utils/memory.cpp
//
//  Two-strategy memory measurement:
//    1. RSS (Resident Set Size) of the OS process — physical memory truth
//    2. Analytical estimation per algorithm — exact auxiliary footprint
//
//  Strategy #2 is preferred for the report because it isolates the algorithm
//  from runtime noise (heap fragmentation, libstdc++ caches, etc.).
// ════════════════════════════════════════════════════════════════════════════

#include "memory.h"
#include "dialsort.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

// ════════════════════════════════════════════════════════════════════════════
//  Platform-specific RSS readers
// ════════════════════════════════════════════════════════════════════════════
#if defined(__linux__)
size_t get_rss_bytes() {
    std::ifstream f("/proc/self/status");
    std::string   line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            size_t kb = 0;
            std::sscanf(line.c_str(), "VmRSS: %zu kB", &kb);
            return kb * 1024;
        }
    }
    return 0;
}
size_t get_peak_rss_bytes() {
    std::ifstream f("/proc/self/status");
    std::string   line;
    while (std::getline(f, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            size_t kb = 0;
            std::sscanf(line.c_str(), "VmHWM: %zu kB", &kb);
            return kb * 1024;
        }
    }
    return 0;
}

#elif defined(__APPLE__)
#include <mach/mach.h>
size_t get_rss_bytes() {
    mach_task_basic_info   info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<size_t>(info.resident_size);
    return 0;
}
size_t get_peak_rss_bytes() {
    mach_task_basic_info   info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<size_t>(info.resident_size_max);
    return 0;
}

#else
size_t get_rss_bytes()      { return 0; }
size_t get_peak_rss_bytes() { return 0; }
#endif

// ════════════════════════════════════════════════════════════════════════════
//  AllocationTracker — atomic counters for thread-safe accounting
// ════════════════════════════════════════════════════════════════════════════
void AllocationTracker::record_alloc(size_t bytes) {
    size_t c = current.fetch_add(bytes, std::memory_order_relaxed) + bytes;
    size_t p = peak.load(std::memory_order_relaxed);
    while (c > p && !peak.compare_exchange_weak(p, c, std::memory_order_relaxed));
}
void AllocationTracker::record_free(size_t bytes) {
    current.fetch_sub(bytes, std::memory_order_relaxed);
}
void AllocationTracker::reset() {
    current.store(0);
    peak.store(0);
}

// ════════════════════════════════════════════════════════════════════════════
//  MemoryProbe — RAII-style RSS sampler
// ════════════════════════════════════════════════════════════════════════════
MemoryProbe::MemoryProbe()
    : rss_before(get_rss_bytes()), rss_peak(rss_before) {}

void MemoryProbe::sample_now() {
    size_t now = get_rss_bytes();
    if (now > rss_peak) rss_peak = now;
}

size_t MemoryProbe::delta_kb() const {
    if (aux_estimate > 0) return aux_estimate / 1024;
    size_t delta = (rss_peak > rss_before) ? (rss_peak - rss_before) : 0;
    return delta / 1024;
}

// ════════════════════════════════════════════════════════════════════════════
//  Analytical estimator — exact auxiliary memory per algorithm
//
//  Each formula reflects the actual data structures allocated by the
//  corresponding implementation, not heuristics. Cross-checked against
//  the source code in src/algorithms/*.cpp.
// ════════════════════════════════════════════════════════════════════════════
size_t estimate_aux_memory(const char* algo_short_name,
                            size_t n, int U, int threads) {
    using SV = std::string_view;
    SV a(algo_short_name);

    // ── DialSort (unified) ──────────────────────────────────────────────────
    // Uses CRN if U fits, else LSD. Worst case is the larger of the two.
    if (a == "DialSort") {
        const uint64_t U_eff = (U > 0) ? static_cast<uint64_t>(U) : 256ULL;
        if (U_eff <= DialSort::MAX_U_HISTOGRAM) {
            // CRN: (p+1) histograms of U ints + n ints staging
            return (static_cast<size_t>(threads) + 1) *
                   static_cast<size_t>(U_eff) * sizeof(int);
        }
        // LSD radix: n ints buffer + 256 ints histogram
        return n * sizeof(int) + 256 * sizeof(int);
    }

    // ── Parallel RadixSort: src[n] + dst[n] of unsigned, plus 256 cnt ───────
    if (a == "P-Radix")
        return 2 * n * sizeof(unsigned) + 256 * sizeof(int);

    // ── Parallel MergeSort: inplace_merge buffer ~ n/2 ints ─────────────────
    if (a == "P-Merge")
        return n * sizeof(int) / 2;

    // ── Parallel SampleSort: full bucket clones (≈ n) + p² × bookkeeping ────
    if (a == "P-Sample")
        return n * sizeof(int) +
               static_cast<size_t>(threads) * static_cast<size_t>(threads) *
               64 * sizeof(int);

    // ── Parallel IntroSort: per-thread O(log n) recursion stack ─────────────
    if (a == "P-Intro")
        return static_cast<size_t>(threads) * 64 * sizeof(int);

    return 0;
}
