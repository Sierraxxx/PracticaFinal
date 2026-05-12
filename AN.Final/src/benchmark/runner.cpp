// ════════════════════════════════════════════════════════════════════════════
//  src/benchmark/runner.cpp
//  Executes a single (algorithm × dataset) pair under controlled conditions.
// ════════════════════════════════════════════════════════════════════════════

#include "benchmark.h"
#include "memory.h"
#include "parallel_utils.h"

#include <algorithm>

BenchCase run_case(const AlgoInfo&    algo,
                   const KeyVec&      base,
                   Distribution       dist,
                   size_t             n,
                   int                U,
                   const BenchConfig& cfg) {
    BenchCase bc;
    bc.algo       = algo.name;
    bc.short_name = algo.short_name;
    bc.dist       = dist_name(dist);
    bc.n          = n;
    bc.U          = U;
    bc.bigo_best  = algo.bigo_best;
    bc.bigo_avg   = algo.bigo_avg;
    bc.bigo_worst = algo.bigo_worst;
    bc.bigo_space = algo.bigo_space;

    // Skip algorithms that need a bounded U when running with full int32.
    const bool full_int = (U <= 0);
    if (algo.needs_bounded_U && full_int) {
        bc.stats.skipped = true;
        return bc;
    }

    const int total = cfg.warmup + cfg.runs;
    std::vector<RunResult> results;
    results.reserve(static_cast<size_t>(cfg.runs));

    // Analytical estimate of auxiliary memory (precise, free of OS noise).
    const int threads = static_cast<int>(pu::hw_threads());
    const size_t aux_bytes = estimate_aux_memory(
        algo.short_name.c_str(), n, U > 0 ? U : 256, threads);

    for (int r = 0; r < total; ++r) {
        // Clone the array so every run starts from the same input.
        KeyVec data = base;

        MemoryProbe probe;

        const auto t0 = Clock::now();
        algo.fn(data, U);
        const double elapsed = ms_since(t0);

        probe.sample_now();

        if (r >= cfg.warmup) {
            RunResult rr;
            rr.time_ms   = elapsed;
            rr.mem_bytes = std::max(probe.delta_kb() * 1024, aux_bytes);
            rr.correct   = is_sorted_ok(data);
            results.push_back(rr);
        }
    }

    if (results.empty()) {
        bc.stats.skipped = true;
        return bc;
    }

    bc.stats = compute_stats(results);
    bc.stats.throughput = (bc.stats.median_ms > 0.0)
        ? static_cast<double>(n) / (bc.stats.median_ms / 1000.0) / 1e6
        : 0.0;

    return bc;
}
