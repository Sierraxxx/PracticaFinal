// ════════════════════════════════════════════════════════════════════════════
//  src/algorithms/parallel_sample_sort.cpp
//
//  Parallel SampleSort using std::thread.
//  Reference: Sanders & Winkel (ESA 2004), IPS4o (Axtmann et al. 2022).
//
//  COMPLEXITY
//    Best  : Θ(n log p)        balanced buckets
//    Avg   : Θ(n log p)
//    Worst : Θ(n log n)        bad splitters
//    Space : Θ(n + p²)         per-thread bucket clones
//
//  ALGORITHM
//    1. Random sample of (p × oversample) keys. Sort sample.
//    2. Pick (p − 1) splitters at evenly spaced positions in the sample.
//    3. Each thread scans its chunk, classifying every key into one of
//       p buckets via binary search on the splitters. Buckets are
//       per-thread to avoid contention.
//    4. Concatenate per-thread buckets into global buckets in parallel
//       (one thread per global bucket).
//    5. Each thread sorts its assigned global bucket.
//    6. Copy back into v in order. Buckets are disjoint key-ranges, so
//       the concatenation is correct.
// ════════════════════════════════════════════════════════════════════════════

#include "algorithms.h"
#include "parallel_utils.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

namespace {
constexpr int OVERSAMPLE_K = 10;
}

void parallel_sample_sort(KeyVec& v, int /*U_unused*/) {
    const size_t n = v.size();
    if (n <= 1) return;

    const unsigned p = pu::hw_threads();
    if (p <= 1) {
        std::sort(v.begin(), v.end());
        return;
    }

    // ── Step 1: Sample ─────────────────────────────────────────────────────
    const size_t ss = std::min<size_t>(static_cast<size_t>(p) * OVERSAMPLE_K, n);
    std::vector<Key> sample(ss);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> pick(0, n - 1);
    for (auto& x : sample) x = v[pick(rng)];
    std::sort(sample.begin(), sample.end());

    // ── Step 2: Splitters ──────────────────────────────────────────────────
    std::vector<Key> splitters(p - 1);
    for (unsigned i = 0; i < p - 1; ++i)
        splitters[i] = sample[(static_cast<size_t>(i) + 1) * OVERSAMPLE_K];

    // ── Step 3: Classify into per-thread buckets ───────────────────────────
    // local[t][b] = elements that thread t classified into global bucket b.
    std::vector<std::vector<std::vector<Key>>> local(
        p, std::vector<std::vector<Key>>(p));

    pu::parallel_chunks(n, p, [&](size_t lo, size_t hi, unsigned t) {
        // Pre-reserve to amortize push_back cost
        for (unsigned b = 0; b < p; ++b)
            local[t][b].reserve((hi - lo) / static_cast<size_t>(p) + 16);

        for (size_t i = lo; i < hi; ++i) {
            const auto it = std::lower_bound(splitters.begin(),
                                             splitters.end(), v[i]);
            const auto b  = static_cast<unsigned>(it - splitters.begin());
            local[t][b].push_back(v[i]);
        }
    });

    // ── Step 4 + 5: Merge per-thread buckets into global buckets, then sort
    std::vector<std::vector<Key>> buckets(p);

    // One thread per global bucket
    pu::parallel_for(p, p, [&](size_t b_idx) {
        const unsigned b = static_cast<unsigned>(b_idx);
        size_t total = 0;
        for (unsigned t = 0; t < p; ++t) total += local[t][b].size();
        buckets[b].reserve(total);
        for (unsigned t = 0; t < p; ++t)
            buckets[b].insert(buckets[b].end(),
                              local[t][b].begin(),
                              local[t][b].end());
        std::sort(buckets[b].begin(), buckets[b].end());
    });

    // ── Step 6: Concatenate sorted buckets back into v ─────────────────────
    size_t pos = 0;
    for (unsigned b = 0; b < p; ++b) {
        std::copy(buckets[b].begin(), buckets[b].end(),
                  v.begin() + static_cast<std::ptrdiff_t>(pos));
        pos += buckets[b].size();
    }
}
