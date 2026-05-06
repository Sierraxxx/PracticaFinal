// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/dialsort.cpp
//
//  DialSort — non-comparative integer sorting via the self-indexing principle.
//
//  This translation unit contains:
//    • Public dispatcher  (DialSort::sort / sort_with_report)
//    • CRN strategy       — small/medium U, parallel histograms
//    • LSD Radix strategy — full int32 range
//    • Sequential strategy — debug / tiny inputs
//
//  All paths produce identical output; the dispatcher chooses based on
//  the detected universe size U = max - min + 1.
//
//  Built on std::thread via the pu:: utilities — no OpenMP, no TBB.
// ════════════════════════════════════════════════════════════════════════════

#include "dialsort.h"
#include "algorithms.h"
#include "parallel_utils.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <iostream>
#include <vector>

namespace DialSort {

using Clock = std::chrono::high_resolution_clock;
using ms_t  = std::chrono::duration<double, std::milli>;

// ════════════════════════════════════════════════════════════════════════════
//  Helper 1 — single-pass extent of the input
// ════════════════════════════════════════════════════════════════════════════
struct Extent {
    int      mn       = 0;
    int      mx       = 0;
    uint64_t U        = 0;
    bool     overflow = false;
};

static Extent compute_extent(const KeyVec& v) noexcept {
    Extent e{};
    if (v.empty()) return e;

    e.mn = v.front();
    e.mx = v.front();
    for (size_t i = 1; i < v.size(); ++i) {
        const int x = v[i];
        if (x < e.mn) e.mn = x;
        if (x > e.mx) e.mx = x;
    }
    e.U = static_cast<uint64_t>(
              static_cast<int64_t>(e.mx) - static_cast<int64_t>(e.mn)
          ) + 1ULL;
    e.overflow = (e.U > MAX_U_HISTOGRAM);
    return e;
}

// ════════════════════════════════════════════════════════════════════════════
//  Helper 2 — strategy auto-selection
// ════════════════════════════════════════════════════════════════════════════
static Strategy auto_select_strategy(size_t n, const Extent& e) noexcept {
    if (n < 1024)        return Strategy::Sequential;
    if (e.overflow)      return Strategy::LSDRadix;
    return Strategy::CRN;
}

// ════════════════════════════════════════════════════════════════════════════
//  Strategy 1 — Sequential (single-thread reference)
// ════════════════════════════════════════════════════════════════════════════
static bool run_sequential(KeyVec& v, const Extent& e, Report& r) noexcept {
    const size_t n = v.size();
    const size_t U = static_cast<size_t>(e.U);

    r.aux_memory_bytes = U * sizeof(int);
    r.threads_used     = 1;

    auto t0 = Clock::now();

    // Phase 1 — Ingestion: H[k − mn]++ (self-indexing, zero comparisons)
    std::vector<int> H(U, 0);
    for (size_t i = 0; i < n; ++i)
        H[static_cast<size_t>(v[i] - e.mn)]++;

    auto t1 = Clock::now();

    // Phase 2 — Geometric projection
    size_t out = 0;
    for (size_t y = 0; y < U; ++y) {
        const int key = static_cast<int>(y) + e.mn;
        for (int c = H[y]; c > 0; --c)
            v[out++] = key;
    }

    auto t2 = Clock::now();

    r.ingestion_ms  = ms_t(t1 - t0).count();
    r.merge_ms      = 0.0;
    r.projection_ms = ms_t(t2 - t1).count();
    r.total_ms      = ms_t(t2 - t0).count();
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  Strategy 2 — CRN (Conflict Resolution Network, parallel ingestion)
//
//  Each thread ingests its chunk into a private histogram H_t[U].
//  No shared writes during ingestion → zero contention.
//  Merge: H[y] = Σ H_t[y]  (additive reduction — no order comparisons).
//
//  This software simulation of the CRN preserves the paper's invariant:
//  conflicts at concurrent indices are resolved by *addition*, never by
//  ordering decisions.
// ════════════════════════════════════════════════════════════════════════════
static bool run_crn(KeyVec& v, const Extent& e,
                    int threads_requested, Report& r) noexcept {
    const size_t n = v.size();
    const size_t U = static_cast<size_t>(e.U);

    const unsigned p = (threads_requested > 0)
        ? static_cast<unsigned>(threads_requested)
        : pu::hw_threads();

    r.aux_memory_bytes = (p + 1) * U * sizeof(int);
    r.threads_used     = static_cast<int>(p);

    const auto t0 = Clock::now();

    // ── Phase 1: parallel ingestion into private histograms ────────────────
    std::vector<std::vector<int>> locals(p, std::vector<int>(U, 0));
    pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) noexcept {
        auto& Ht = locals[t];
        for (size_t i = lo; i < hi; ++i)
            Ht[static_cast<size_t>(v[i] - e.mn)]++;
    });

    const auto t1 = Clock::now();

    // ── Phase 2: additive merge — the CRN itself, in software ──────────────
    std::vector<int> H(U, 0);
    for (unsigned t = 0; t < p; ++t)
        for (size_t y = 0; y < U; ++y)
            H[y] += locals[t][y];

    const auto t2 = Clock::now();

    // ── Phase 3: geometric projection ──────────────────────────────────────
    size_t out = 0;
    for (size_t y = 0; y < U; ++y) {
        const int key = static_cast<int>(y) + e.mn;
        for (int c = H[y]; c > 0; --c)
            v[out++] = key;
    }

    const auto t3 = Clock::now();

    r.ingestion_ms  = ms_t(t1 - t0).count();
    r.merge_ms      = ms_t(t2 - t1).count();
    r.projection_ms = ms_t(t3 - t2).count();
    r.total_ms      = ms_t(t3 - t0).count();
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  Strategy 3 — LSD Radix (full int32 range, sequential per pass)
// ════════════════════════════════════════════════════════════════════════════
static bool run_lsd_radix(KeyVec& v, Report& r) noexcept {
    const size_t n = v.size();
    if (n <= 1) { r.total_ms = 0.0; return true; }

    constexpr int B = 256;
    r.aux_memory_bytes = n * sizeof(int) + B * sizeof(int);
    r.threads_used     = 1;

    const auto t0 = Clock::now();

    std::vector<int> buf(n);
    int* src = v.data();
    int* dst = buf.data();

    int cnt[B];

    for (int pass = 0; pass < 4; ++pass) {
        const int shift = pass * 8;
        std::fill(cnt, cnt + B, 0);

        for (size_t i = 0; i < n; ++i) {
            const unsigned key = static_cast<unsigned>(src[i]) ^ 0x80000000u;
            cnt[(key >> shift) & 0xFF]++;
        }

        int sum = 0;
        for (int i = 0; i < B; ++i) {
            const int c = cnt[i]; cnt[i] = sum; sum += c;
        }

        for (size_t i = 0; i < n; ++i) {
            const int      val = src[i];
            const unsigned key = static_cast<unsigned>(val) ^ 0x80000000u;
            dst[cnt[(key >> shift) & 0xFF]++] = val;
        }
        std::swap(src, dst);
    }

    if (src != v.data())
        std::move(buf.begin(), buf.end(), v.begin());

    const auto t1 = Clock::now();

    r.ingestion_ms  = ms_t(t1 - t0).count();
    r.merge_ms      = 0.0;
    r.projection_ms = 0.0;
    r.total_ms      = ms_t(t1 - t0).count();
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  Public API
// ════════════════════════════════════════════════════════════════════════════

const char* strategy_name(Strategy s) noexcept {
    switch (s) {
        case Strategy::Auto:       return "Auto";
        case Strategy::CRN:        return "CRN-Parallel";
        case Strategy::LSDRadix:   return "LSD-Radix";
        case Strategy::Sequential: return "Sequential";
    }
    return "Unknown";
}

static bool sort_core(KeyVec& v, const Options& opts, Report& r) noexcept {
    r = Report{};

    if (v.size() <= 1) {
        r.success       = true;
        r.strategy_used = Strategy::Sequential;
        r.strategy_name = strategy_name(Strategy::Sequential);
        return true;
    }

    Extent e        = compute_extent(v);
    r.min_value     = e.mn;
    r.max_value     = e.mx;
    r.universe_size = e.U;

    Strategy strat = (opts.strategy == Strategy::Auto)
                     ? auto_select_strategy(v.size(), e)
                     : opts.strategy;

    if ((strat == Strategy::CRN || strat == Strategy::Sequential) && e.overflow) {
        if (opts.verbose)
            std::cerr << "[DialSort] U=" << e.U
                      << " exceeds MAX_U_HISTOGRAM=" << MAX_U_HISTOGRAM
                      << " — falling back to LSD-Radix.\n";
        strat = Strategy::LSDRadix;
    }

    r.strategy_used = strat;
    r.strategy_name = strategy_name(strat);

    if (opts.verbose)
        std::cerr << "[DialSort] strategy=" << r.strategy_name
                  << " n=" << v.size() << " U=" << e.U << "\n";

    bool ok = false;
    switch (strat) {
        case Strategy::Sequential: ok = run_sequential(v, e, r); break;
        case Strategy::CRN:        ok = run_crn(v, e, opts.threads, r); break;
        case Strategy::LSDRadix:   ok = run_lsd_radix(v, r); break;
        case Strategy::Auto:
            r.error_message = "Internal error: Auto strategy not resolved";
            return false;
    }
    r.success = ok;
    return ok;
}

bool sort(KeyVec& v) noexcept {
    Options opts; Report r;
    return sort_core(v, opts, r);
}

bool sort(KeyVec& v, const Options& opts) noexcept {
    Report r;
    return sort_core(v, opts, r);
}

bool sort_with_report(KeyVec& v, Report& r) noexcept {
    Options opts;
    return sort_core(v, opts, r);
}

bool sort_with_report(KeyVec& v, const Options& opts, Report& r) noexcept {
    return sort_core(v, opts, r);
}

} // namespace DialSort

// ────────────────────────────────────────────────────────────────────────────
//  C-style adapter for the benchmark registry.
// ────────────────────────────────────────────────────────────────────────────
void dialsort_unified(KeyVec& v, int /*U_hint_unused*/) {
    DialSort::sort(v);
}
