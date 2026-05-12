// src/benchmark/stats.cpp
//
// NOTE on throughput: this function intentionally sets throughput = 0.
// The correct value requires knowing N (number of elements), which is
// only available in run_case() inside runner.cpp. Setting it to 0 here
// makes the responsibility explicit — runner.cpp always overwrites it.
// There is no dual-responsibility bug: stats.cpp owns time statistics,
// runner.cpp owns throughput.

#include "benchmark.h"
#include <algorithm>
#include <cmath>
#include <numeric>

Stats compute_stats(std::vector<RunResult>& runs) {
    if (runs.empty()) return {};

    std::vector<double> times;
    times.reserve(runs.size());
    bool   all_correct = true;
    size_t total_mem   = 0;

    for (const auto& r : runs) {
        times.push_back(r.time_ms);
        total_mem += r.mem_bytes;
        if (!r.correct) all_correct = false;
    }

    std::sort(times.begin(), times.end());

    double sum = 0, sq = 0;
    for (double t : times) { sum += t; sq += t * t; }
    const double mean = sum / static_cast<double>(times.size());
    const double var  = sq  / static_cast<double>(times.size()) - mean * mean;
    const double med  = times[times.size() / 2];

    Stats s;
    s.mean_ms     = mean;
    s.median_ms   = med;
    s.stddev_ms   = std::sqrt(std::max(var, 0.0));
    s.min_ms      = times.front();
    s.max_ms      = times.back();
    s.throughput  = 0.0;   // ← set by runner.cpp using the real N
    s.mean_mem_kb = (runs.size() > 0) ? (total_mem / runs.size()) / 1024 : 0;
    s.correct     = all_correct;
    return s;
}

bool is_sorted_ok(const KeyVec& v) {
    for (size_t i = 1; i < v.size(); ++i)
        if (v[i] < v[i - 1]) return false;
    return true;
}
