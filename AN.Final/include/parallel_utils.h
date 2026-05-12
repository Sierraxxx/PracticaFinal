#pragma once
// include/parallel_utils.h
//
// Lightweight parallelism primitives built on std::thread.
// No OpenMP, no TBB, no external dependencies.
//
// DESIGN DECISION
// ---------------
// Uses direct std::thread spawn per call — simple, correct, no deadlock risk.
// Thread creation overhead (~50µs each) is negligible compared to the sorting
// work (milliseconds to seconds per case). A thread pool would save ~5ms over
// a full benchmark run — not worth the complexity and deadlock risk.
//
// PUBLIC API
// ----------
//   pu::hw_threads()              → number of hardware threads (min 1)
//   pu::parallel_for(n, f)        → run f(i) for i in [0,n) in parallel
//   pu::parallel_for(n, p, f)     → same, capped at p threads
//   pu::parallel_chunks(n, f)     → run f(lo,hi,tid) per chunk
//   pu::parallel_chunks(n, p, f)  → same, capped at p threads

#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

namespace pu {

// ── Hardware concurrency ──────────────────────────────────────────────────────
inline unsigned hw_threads() noexcept {
    const unsigned p = std::thread::hardware_concurrency();
    return (p == 0) ? 1u : p;
}

// ── parallel_for ──────────────────────────────────────────────────────────────
// Splits [0, n) into p chunks and runs f(i) for every i.
// Falls back to sequential when p==1 or n is too small.
template <typename F>
void parallel_for(size_t n, unsigned p, F&& f) {
    if (n == 0) return;

    // Sequential fallback — no thread overhead for tiny inputs
    if (p <= 1 || n < static_cast<size_t>(p) * 2) {
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
        workers.emplace_back([lo, hi, &f]() noexcept {
            for (size_t i = lo; i < hi; ++i) f(i);
        });
    }

    for (auto& w : workers) w.join();
}

template <typename F>
void parallel_for(size_t n, F&& f) {
    parallel_for(n, hw_threads(), std::forward<F>(f));
}

// ── parallel_chunks ───────────────────────────────────────────────────────────
// Splits [0, n) into p chunks and runs f(lo, hi, tid) for each chunk.
// Useful when the body wants to maintain per-thread state.
template <typename F>
void parallel_chunks(size_t n, unsigned p, F&& f) {
    if (n == 0) return;

    // Sequential fallback
    if (p <= 1) {
        f(size_t{0}, n, unsigned{0});
        return;
    }

    const size_t chunk = (n + p - 1) / p;
    std::vector<std::thread> workers;
    workers.reserve(p);

    for (unsigned t = 0; t < p; ++t) {
        const size_t lo = static_cast<size_t>(t) * chunk;
        const size_t hi = std::min(n, lo + chunk);
        if (lo >= hi) break;
        workers.emplace_back([lo, hi, t, &f]() noexcept {
            f(lo, hi, t);
        });
    }

    for (auto& w : workers) w.join();
}

template <typename F>
void parallel_chunks(size_t n, F&& f) {
    parallel_chunks(n, hw_threads(), std::forward<F>(f));
}

} // namespace pu
