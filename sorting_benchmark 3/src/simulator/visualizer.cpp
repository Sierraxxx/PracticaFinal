// ════════════════════════════════════════════════════════════════════════════
//  src/simulator/visualizer.cpp
//
//  Interactive ASCII visualizer for the 5 algorithms.
//  Each visualizer prints:
//    1. Header with algorithm name and complexity
//    2. Input snapshot
//    3. Phase-by-phase trace with ASCII bars
//    4. Final sorted output and a brief summary
//
//  This module is purely diagnostic — the production algorithms in
//  src/algorithms/ are NOT modified to instrument them.
//  The simulator re-implements the phases with the same logic so it can
//  pause and print intermediate state.
// ════════════════════════════════════════════════════════════════════════════

#include "simulator.h"
#include "data_generator.h"
#include "dialsort.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// ── ANSI colors (no-op on Windows) ──────────────────────────────────────────
#ifndef _WIN32
  #define C_RESET    "\033[0m"
  #define C_BOLD     "\033[1m"
  #define C_DIM      "\033[2m"
  #define C_GREEN    "\033[32m"
  #define C_YELLOW   "\033[33m"
  #define C_CYAN     "\033[36m"
  #define C_BLUE     "\033[34m"
  #define C_MAGENTA  "\033[35m"
  #define C_RED      "\033[31m"
#else
  #define C_RESET "" 
  #define C_BOLD ""
  #define C_DIM ""
  #define C_GREEN "" 
  #define C_YELLOW ""
  #define C_CYAN ""
  #define C_BLUE ""
  #define C_MAGENTA ""
  #define C_RED ""
#endif

// ── Pacing for slow mode ────────────────────────────────────────────────────
static void pace(bool slow) {
    if (slow) std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

// ── Section headers ─────────────────────────────────────────────────────────
static void hr(char c = '=', int n = 78) {
    std::cout << std::string(n, c) << "\n";
}

static void sim_header(const std::string& name,
                       const std::string& complexity,
                       const KeyVec& input) {
    std::cout << "\n";
    hr('=');
    std::cout << C_BOLD << C_MAGENTA
              << "  ALGORITHM: " << name << C_RESET << "\n";
    std::cout << C_DIM << "  Complexity: " << complexity << C_RESET << "\n";
    std::cout << "  N = " << input.size() << "  |  Preview: ";
    for (size_t i = 0; i < std::min((size_t)15, input.size()); ++i)
        std::cout << input[i] << " ";
    if (input.size() > 15) std::cout << "...";
    std::cout << "\n";
    hr('=');
    std::cout << "\n";
}

static void sim_footer(const KeyVec& v) {
    std::cout << "\n" << C_GREEN << C_BOLD << "  RESULT: ";
    for (size_t i = 0; i < std::min((size_t)15, v.size()); ++i)
        std::cout << v[i] << " ";
    if (v.size() > 15) std::cout << "...";
    std::cout << C_RESET << "\n\n";
}

// ════════════════════════════════════════════════════════════════════════════
//  ASCII bar renderer (public)
// ════════════════════════════════════════════════════════════════════════════
void render_array(const KeyVec& v, size_t max_display,
                  const std::string& label, int hi_lo, int hi_hi) {
    if (v.empty()) return;
    size_t step = std::max((size_t)1, v.size() / max_display);
    int    maxv = *std::max_element(v.begin(), v.end());
    if (maxv <= 0) maxv = 1;

    std::cout << C_BOLD << "  " << label << C_RESET << "\n  ";
    for (size_t i = 0; i < v.size(); i += step) {
        const int  height = std::max(1, v[i] * 20 / maxv);
        const bool hi     = (hi_lo >= 0 && (int)i >= hi_lo && (int)i <= hi_hi);
        std::string bar(height, hi ? '|' : ':');
        std::cout << (hi ? C_GREEN : C_CYAN) << bar << " " << C_RESET;
    }
    std::cout << "\n";
}

// ════════════════════════════════════════════════════════════════════════════
//  SIMULATE DIALSORT (unified — shows strategy auto-selection in action)
// ════════════════════════════════════════════════════════════════════════════
void simulate_dialsort(const KeyVec& input, bool slow) {
    sim_header("DialSort (unified, auto-strategy)",
               "O(n + U) ingestion + O(U + n) projection",
               input);
    KeyVec v = input;
    const size_t n = v.size();

    // ── Detect extent (just like the real dispatcher) ──────────────────────
    int mn = v.front(), mx = v.front();
    for (int x : v) { if (x < mn) mn = x; if (x > mx) mx = x; }
    const int U = mx - mn + 1;

    std::cout << C_BOLD << C_CYAN
              << "  Step 1: Detect universe extent\n" << C_RESET
              << "    min = " << mn << "  max = " << mx
              << "  →  U = max - min + 1 = " << U << "\n\n";
    pace(slow);

    // ── Strategy selection ─────────────────────────────────────────────────
    std::cout << C_BOLD << C_CYAN
              << "  Step 2: Strategy selection\n" << C_RESET;
    if (n < 1024) {
        std::cout << "    n=" << n << " < 1024  →  using SEQUENTIAL\n";
    } else if ((uint64_t)U > DialSort::MAX_U_HISTOGRAM) {
        std::cout << "    U=" << U << " > " << DialSort::MAX_U_HISTOGRAM
                  << "  →  using LSD-RADIX\n";
    } else {
        std::cout << "    U=" << U << " ≤ " << DialSort::MAX_U_HISTOGRAM
                  << "  →  using CRN-PARALLEL\n";
    }
    std::cout << "\n";
    pace(slow);

    // ── Phase 1: Ingestion (self-indexation) ───────────────────────────────
    std::cout << C_BOLD << C_CYAN
              << "  Phase 1: Ingestion — H[k - mn]++ (self-indexing)\n"
              << C_RESET
              << "    Each key becomes its own index in the histogram H.\n"
              << "    No order comparisons performed.\n\n";

    std::vector<int> H((size_t)U, 0);
    const size_t snap = std::max((size_t)1, n / 8);

    for (size_t i = 0; i < n; ++i) {
        H[(size_t)(v[i] - mn)]++;

        if ((i + 1) % snap == 0 || i == n - 1) {
            std::cout << "    after " << std::setw(6) << (i+1)
                      << " keys (" << std::setw(3) << ((i+1)*100/n) << "%):  H = [";

            const size_t show = std::min((size_t)U, (size_t)32);
            for (size_t y = 0; y < show; ++y) {
                std::cout << (H[y] > 0 ? C_GREEN : C_DIM)
                          << std::setw(3) << H[y] << C_RESET;
            }
            if ((size_t)U > show) std::cout << " …";
            std::cout << "]\n";
            pace(slow);
        }
    }

    // ── Phase 2: Projection ────────────────────────────────────────────────
    std::cout << "\n" << C_BOLD << C_CYAN
              << "  Phase 2: Geometric projection — emit (y + mn) exactly H[y] times\n"
              << C_RESET
              << "    Linear scan over [0, U), O(U + n) total work.\n\n";

    size_t out = 0;
    for (int y = 0; y < U; ++y) {
        for (int c = H[(size_t)y]; c > 0; --c)
            v[out++] = y + mn;

        if (y % std::max(1, U / 10) == 0) {
            std::cout << "    y = " << std::setw(5) << y
                      << "    emitted = " << std::setw(7) << out
                      << "    (" << std::setw(3) << (out*100/n) << "%)\n";
            pace(slow);
        }
    }

    sim_footer(v);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIMULATE PARALLEL RADIXSORT
// ════════════════════════════════════════════════════════════════════════════
void simulate_radix(const KeyVec& input, bool slow) {
    sim_header("Parallel RadixSort (LSD base-256)",
               "O(d·n) — d=4 passes for int32",
               input);
    KeyVec v = input;
    const size_t n = v.size();

    std::vector<unsigned> src(n), dst(n);
    for (size_t i = 0; i < n; ++i)
        src[i] = (unsigned)v[i] ^ 0x80000000u;

    std::cout << C_BOLD << C_CYAN
              << "  Step 1: Bias signed → unsigned (XOR 0x80000000)\n"
              << C_RESET
              << "    Maps INT_MIN→0, …, INT_MAX→UINT_MAX so unsigned order\n"
              << "    coincides with signed order. Required for negative ints.\n\n";
    pace(slow);

    for (int pass = 0; pass < 4; ++pass) {
        const int sh = pass * 8;
        std::cout << C_YELLOW << "  Pass " << (pass + 1) << "/4 — "
                  << "bits [" << sh << "..." << (sh + 7) << "]" << C_RESET << "\n";

        std::vector<int> cnt(256, 0);
        for (size_t i = 0; i < n; ++i) cnt[(src[i] >> sh) & 0xFF]++;

        std::cout << "    Histogram (first 16 buckets): ";
        for (int b = 0; b < 16; ++b) std::cout << "[" << b << "]=" << cnt[b] << " ";
        std::cout << "…\n";

        std::vector<int> pre(256, 0);
        for (int b = 1; b < 256; ++b) pre[b] = pre[b-1] + cnt[b-1];
        std::cout << "    Prefix sum computed → scattering …\n";
        pace(slow);

        for (size_t i = 0; i < n; ++i) {
            const int b = (src[i] >> sh) & 0xFF;
            dst[pre[b]++] = src[i];
        }
        std::swap(src, dst);
        pace(slow);
    }

    for (size_t i = 0; i < n; ++i) v[i] = (int)(src[i] ^ 0x80000000u);
    sim_footer(v);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIMULATE PARALLEL MERGESORT
// ════════════════════════════════════════════════════════════════════════════
void simulate_merge(const KeyVec& input, bool slow) {
    sim_header("Parallel MergeSort (std::thread)",
               "O(n log n) work, O(log² n) span", input);
    KeyVec v = input;
    const size_t n = v.size();
    const size_t chunk = std::max((size_t)1, n / 8);

    std::cout << C_BOLD << C_CYAN
              << "  Step 1: Divide into " << ((n+chunk-1)/chunk)
              << " chunks of ~" << chunk << " elements\n" << C_RESET;
    pace(slow);

    std::cout << "\n  Step 2: Sort each chunk independently (parallel std::sort)\n";
    for (size_t i = 0; i < n; i += chunk) {
        const size_t e = std::min(i + chunk, n);
        std::sort(v.begin() + (ptrdiff_t)i, v.begin() + (ptrdiff_t)e);
        std::cout << "    chunk [" << std::setw(3) << i
                  << ", " << std::setw(3) << e << ") sorted\n";
        pace(slow);
    }

    std::cout << "\n  Step 3: Bottom-up merge tree\n";
    for (size_t w = chunk; w < n; w *= 2) {
        std::cout << "    width = " << w << "  →  merging "
                  << ((n + 2*w - 1) / (2*w)) << " pairs\n";
        for (size_t lo = 0; lo < n; lo += 2*w) {
            const size_t mid = std::min(lo + w, n);
            const size_t hi  = std::min(lo + 2*w, n);
            if (mid < hi)
                std::inplace_merge(v.begin() + (ptrdiff_t)lo,
                                   v.begin() + (ptrdiff_t)mid,
                                   v.begin() + (ptrdiff_t)hi);
        }
        pace(slow);
    }

    sim_footer(v);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIMULATE PARALLEL SAMPLESORT
// ════════════════════════════════════════════════════════════════════════════
void simulate_sample(const KeyVec& input, bool slow) {
    sim_header("Parallel SampleSort (splitter-based partitioning)",
               "O(n log p) — p threads", input);
    KeyVec v = input;
    const size_t n = v.size();
    const int    p = 4;

    std::cout << C_BOLD << C_CYAN
              << "  Step 1: Random sampling — " << (p * 10) << " keys → "
              << (p - 1) << " splitters\n" << C_RESET;

    std::vector<int> sample;
    sample.reserve((size_t)p * 10);
    for (size_t i = 0; i < (size_t)p * 10 && i < n; ++i)
        sample.push_back(v[i * n / std::max((size_t)1, sample.capacity())]);
    std::sort(sample.begin(), sample.end());

    std::vector<int> splitters(p - 1);
    for (int i = 0; i < p - 1; ++i)
        splitters[i] = sample[std::min(sample.size() - 1, (size_t)(i + 1) * 10)];

    std::cout << "    Splitters: ";
    for (int s : splitters) std::cout << s << " ";
    std::cout << "\n\n";
    pace(slow);

    std::cout << C_BOLD << C_CYAN
              << "  Step 2: Bucket assignment via binary search on splitters\n"
              << C_RESET;
    std::vector<std::vector<int>> buckets(p);
    for (int k : v) {
        const int b = (int)(std::lower_bound(splitters.begin(),
                                             splitters.end(), k)
                            - splitters.begin());
        buckets[b].push_back(k);
    }
    for (int b = 0; b < p; ++b)
        std::cout << "    bucket " << b << ": " << buckets[b].size() << " keys\n";
    pace(slow);

    std::cout << "\n" << C_BOLD << C_CYAN
              << "  Step 3: Sort each bucket in parallel\n" << C_RESET;
    for (int b = 0; b < p; ++b) {
        std::sort(buckets[b].begin(), buckets[b].end());
        std::cout << "    bucket " << b << " sorted\n";
        pace(slow);
    }

    size_t pos = 0;
    for (auto& b : buckets) for (int k : b) v[pos++] = k;
    sim_footer(v);
}

// ════════════════════════════════════════════════════════════════════════════
//  SIMULATE PARALLEL INTROSORT
// ════════════════════════════════════════════════════════════════════════════
void simulate_intro(const KeyVec& input, bool slow) {
    sim_header("Parallel IntroSort (chunk-parallel std::sort + merge)",
               "O(n log n) — QS + HS + IS hybrid", input);
    KeyVec v = input;
    const size_t n = v.size();
    const int    p = 4;
    const size_t chunk = std::max((size_t)1, n / (size_t)p);

    std::cout << C_BOLD << C_CYAN
              << "  IntroSort = QuickSort + HeapSort + InsertionSort\n"
              << "  std::sort uses this hybrid (since C++11).\n\n"
              << "  Step 1: " << p << " parallel chunks, std::sort on each\n"
              << C_RESET;

    for (int t = 0; t < p; ++t) {
        const size_t lo = (size_t)t * chunk;
        const size_t hi = std::min(lo + chunk, n);
        std::sort(v.begin() + (ptrdiff_t)lo, v.begin() + (ptrdiff_t)hi);
        std::cout << "    thread " << t << ": [" << lo << ", " << hi << ") sorted\n";
        pace(slow);
    }

    std::cout << "\n  Step 2: Bottom-up merge tree (same as MergeSort)\n";
    for (size_t w = chunk; w < n; w *= 2) {
        std::cout << "    width = " << w << "\n";
        for (size_t lo = 0; lo < n; lo += 2*w) {
            const size_t mid = std::min(lo + w, n);
            const size_t hi  = std::min(lo + 2*w, n);
            if (mid < hi)
                std::inplace_merge(v.begin() + (ptrdiff_t)lo,
                                   v.begin() + (ptrdiff_t)mid,
                                   v.begin() + (ptrdiff_t)hi);
        }
        pace(slow);
    }

    sim_footer(v);
}

// ════════════════════════════════════════════════════════════════════════════
//  INTERACTIVE MENU
// ════════════════════════════════════════════════════════════════════════════
void run_simulator_menu() {
    while (true) {
        hr('=');
        std::cout << C_BOLD << C_MAGENTA
                  << "  DIALSORT BENCHMARK — ALGORITHM VISUALIZER\n" << C_RESET;
        hr('=');
        std::cout
            << "  Pick an algorithm to visualize step by step:\n\n"
            << "    1) " << C_GREEN << "DialSort (unified, auto-strategy)" << C_RESET << "\n"
            << "    2) Parallel RadixSort\n"
            << "    3) Parallel MergeSort\n"
            << "    4) Parallel SampleSort\n"
            << "    5) Parallel IntroSort\n"
            << "    6) Run ALL on the same input (side-by-side demo)\n"
            << "    0) Exit\n\n"
            << "  Choice: ";

        int choice;
        if (!(std::cin >> choice)) break;
        if (choice == 0) break;
        if (choice < 1 || choice > 6) {
            std::cout << C_RED << "  Invalid choice.\n" << C_RESET;
            continue;
        }

        size_t N    = 40;
        int    U    = 20;
        int    slow = 0;
        std::cout << "  N (elements, default 40):    "; std::cin >> N;
        std::cout << "  U (universe, default 20):    "; std::cin >> U;
        std::cout << "  Slow mode? (0=fast, 1=slow): "; std::cin >> slow;

        KeyVec data = generate(N, Distribution::UNIFORM, U);
        const bool s = (slow != 0);

        switch (choice) {
            case 1: simulate_dialsort(data, s); break;
            case 2: simulate_radix(data, s);    break;
            case 3: simulate_merge(data, s);    break;
            case 4: simulate_sample(data, s);   break;
            case 5: simulate_intro(data, s);    break;
            case 6:
                std::cout << "\n" << C_BOLD
                          << "  Running all 5 algorithms on the same input...\n"
                          << C_RESET;
                simulate_dialsort(data, s);
                simulate_radix(data, s);
                simulate_merge(data, s);
                simulate_sample(data, s);
                simulate_intro(data, s);
                break;
        }
    }
    std::cout << "\nGoodbye.\n";
}
