// src/benchmark/stats.cpp
#include "benchmark.h"
#include <algorithm>
#include <cmath>
#include <numeric>

Stats compute_stats(std::vector<RunResult>& runs) {
    // Filtrar corridas no correctas
    std::vector<double> times;
    bool all_correct = true;
    size_t total_mem = 0;

    for (auto& r : runs) {
        times.push_back(r.time_ms);
        total_mem += r.mem_bytes;
        if (!r.correct) all_correct = false;
    }

    std::sort(times.begin(), times.end());

    double sum = 0, sq = 0;
    for (double t : times) { sum += t; sq += t * t; }
    double mean = sum / static_cast<double>(times.size());
    double var  = sq  / static_cast<double>(times.size()) - mean * mean;
    double med  = times[times.size() / 2];

    Stats s;
    s.mean_ms    = mean;
    s.median_ms  = med;
    s.stddev_ms  = std::sqrt(std::max(var, 0.0));
    s.min_ms     = times.front();
    s.max_ms     = times.back();
    s.throughput = (med > 0.0)
                   ? static_cast<double>(runs[0].correct ? runs.size() : 1)
                   : 0.0;
    // throughput = n / median_seconds / 1e6  (M keys/s)
    // Se recalcula en run_case con el N real
    s.mean_mem_kb = (total_mem / runs.size()) / 1024;
    s.correct     = all_correct;
    return s;
}

bool is_sorted_ok(const KeyVec& v) {
    for (size_t i = 1; i < v.size(); ++i)
        if (v[i] < v[i - 1]) return false;
    return true;
}
