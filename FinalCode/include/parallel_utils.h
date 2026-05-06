#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  include/parallel_utils.h
//
//  Lightweight thread utilities built on std::thread.
//  No external dependencies (no OpenMP, no TBB, no boost).
//
//  PRIMITIVES
//  ──────────
//    parallel_for(n, f)         — split [0, n) across hardware threads
//    parallel_for(n, p, f)      — split [0, n) across exactly p threads
//    parallel_chunks(n, p, f)   — give each thread (lo, hi, tid)
//
//  Each function blocks until all work is done.
// ════════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

namespace pu {

/// Number of hardware threads, never less than 1.
inline unsigned hw_threads() noexcept {
    unsigned p = std::thread::hardware_concurrency();
    return (p == 0) ? 1u : p;
}

/// Run f(i) for every i in [0, n) split across `p` threads.
/// `f` must be thread-safe (no shared mutable state without sync).
template <typename F>
void parallel_for(size_t n, unsigned p, F&& f) {
    if (n == 0) return;
    if (p <= 1 || n < 2 * p) {
        for (size_t i = 0; i < n; ++i) f(i);
        return;
    }
    const size_t chunk = (n + p - 1) / p;
    std::vector<std::thread> workers;
    workers.reserve(p);
    for (unsigned t = 0; t < p; ++t) {
        const size_t lo = static_cast<size_t>(t) * chunk;
        const size_t hi = std::min(n, lo + chunk);
        if (lo >= hi) break;
        workers.emplace_back([lo, hi, &f]() {
            for (size_t i = lo; i < hi; ++i) f(i);
        });
    }
    for (auto& w : workers) w.join();
}

template <typename F>
void parallel_for(size_t n, F&& f) {
    parallel_for(n, hw_threads(), std::forward<F>(f));
}

/// Run f(lo, hi, tid) on each thread's chunk.
/// Useful when the body wants to amortize per-thread state allocation.
template <typename F>
void parallel_chunks(size_t n, unsigned p, F&& f) {
    if (n == 0) return;
    if (p <= 1) { f(size_t{0}, n, unsigned{0}); return; }

    const size_t chunk = (n + p - 1) / p;
    std::vector<std::thread> workers;
    workers.reserve(p);
    for (unsigned t = 0; t < p; ++t) {
        const size_t lo = static_cast<size_t>(t) * chunk;
        const size_t hi = std::min(n, lo + chunk);
        if (lo >= hi) break;
        workers.emplace_back([lo, hi, t, &f]() { f(lo, hi, t); });
    }
    for (auto& w : workers) w.join();
}

template <typename F>
void parallel_chunks(size_t n, F&& f) {
    parallel_chunks(n, hw_threads(), std::forward<F>(f));
}

} // namespace pu
