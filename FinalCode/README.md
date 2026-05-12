# Sorting Benchmark v0.7.0

> Comparative analysis of **DialSort** vs 4 state-of-the-art parallel sorting
> algorithms. Datasets from 100K to 10M records.
>
> **Pure C++17** — no OpenMP, no TBB, no external dependencies.
> Only the standard library + `std::thread`.

---

## Team Members

Team information is kept in `AUTHORS.private` (excluded from version control).

This file is in `.gitignore` and will never be uploaded to GitHub.
---

## The 5 Algorithms Compared

| # | Algorithm | Paradigm | Best | Average | Worst | Space |
|---|-----------|----------|------|---------|-------|-------|
| 1 | **DialSort** | Non-comparative, self-indexing | O(n + U) | O(n/p + U) | O(n log n)¹ | O(p·U) or O(n) |
| 2 | Parallel RadixSort | LSD digit-by-digit | O(d·n) | O(d·n) | O(d·n) | O(n + p·B) |
| 3 | Parallel MergeSort | Divide & conquer | O(n log n) | O(n log n) | O(n log n) | O(n) |
| 4 | Parallel SampleSort | Splitter-based partition | O(n log p) | O(n log p) | O(n log n) | O(n + p²) |
| 5 | Parallel IntroSort | QS + HS + IS hybrid | O(n log n) | O(n log n) | O(n log n) | O(log n) |

¹ DialSort dispatches to LSD-Radix when U > 10M, so worst-case is bounded by O(d·n) ≈ O(n).

### DialSort's Self-Indexing Principle

For any integer `k ∈ [min, max]`, the value `(k − min)` IS the address of
its frequency in `H[]`. The sorted position of `k` is known **without any
order comparison** (`<`, `>`, `≤`).

DialSort has a single public entry point `dialsort_unified(v, U)` that
internally dispatches to one of three strategies:

| Strategy | When | Complexity |
|----------|------|-----------|
| **CRN** (parallel histogram) | `U ≤ 10M` and `n ≥ 1024` | O(n/p + U) |
| **LSD-Radix** | `U > 10M` (full int32) | O(4n) |
| **Sequential** | `n < 1024` | O(n + U) |

---

## Repository Structure

```
dialsort_benchmark/
├── CMakeLists.txt              ← only depends on Threads (pthreads)
├── include/
│   ├── types.h                 ← shared types
│   ├── algorithms.h            ← public API of the 5 algorithms
│   ├── dialsort.h              ← namespace DialSort {} rich API
│   ├── benchmark.h             ← runner / stats / reporter / exporters
│   ├── simulator.h             ← interactive ASCII visualizer
│   ├── data_generator.h
│   ├── memory.h                ← RSS + analytical estimator
│   └── parallel_utils.h        ← std::thread helpers (no OpenMP)
├── src/
│   ├── main_bench.cpp          ← benchmark entry point
│   ├── main_sim.cpp            ← simulator entry point
│   ├── main_gen.cpp            ← dataset generator entry point
│   ├── algorithms/
│   │   ├── dialsort.cpp                ← unified DialSort (3 strategies)
│   │   ├── parallel_radix_sort.cpp     ← std::thread implementation
│   │   ├── parallel_merge_sort.cpp     ← std::thread implementation
│   │   ├── parallel_sample_sort.cpp    ← std::thread implementation
│   │   ├── parallel_intro_sort.cpp     ← std::thread implementation
│   │   └── registry.cpp                ← Big-O metadata
│   ├── benchmark/
│   │   ├── runner.cpp
│   │   ├── stats.cpp
│   │   ├── reporter.cpp                ← terminal ASCII tables
│   │   └── html_exporter.cpp           ← interactive HTML report
│   ├── simulator/
│   │   └── visualizer.cpp              ← step-by-step ASCII traces
│   └── utils/
│       ├── data_generator.cpp
│       └── memory.cpp
├── datasets/                   ← auto-generated CSV
└── results/                    ← CSV + HTML output
```

---

## Build Requirements

| Tool | Version | Notes |
|------|---------|-------|
| C++ compiler | GCC 9+ / Clang 10+ / AppleClang / MSVC 2019+ | C++17 required |
| CMake | 3.20+ | |
| pthreads | (built-in) | POSIX systems include it; MSVC has its own threading |

**No external libraries needed.** No Homebrew formula, no apt-get.
Just a standard C++17 compiler.

---

## Interactive Dashboard

Open `dashboard/index.html` in any browser (double-click) for an interactive
visualization of results. Features:

- **Run live**: execute all 5 algorithms in the browser via JS
- **Import CSV**: load results from the C++ binary
- **Interactive charts**: bar, scalability line, heatmap
- **Searchable results table** with filters
- **Big-O analysis** with empirical vs theoretical comparison

See `dashboard/README.md` for details.


## Build & Run

### From CLion

1. **File → Open** → select the `dialsort_benchmark/` folder
2. CLion auto-detects `CMakeLists.txt`
3. Set profile to **Release** (top-right toolbar)
4. **Build → Build All** (⌘F9 / Ctrl+F9)
5. The three executables appear in `cmake-build-release/`:
   - `dialsort_bench`
   - `dialsort_sim`
   - `gen_datasets`

### From the terminal

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Run the benchmark

```bash
# Quick test — N up to 1M, 3 runs
./build/dialsort_bench --quick

# Full report — N up to 10M, 7 runs
./build/dialsort_bench --full

# Custom case
./build/dialsort_bench --n 5000000 --u 1024 --runs 5
```

**Outputs:**
- `results/benchmark_results.csv` — raw data
- `results/report.html` — **interactive report** (Chart.js graphs, filters)

### Run the visualizer

```bash
./build/dialsort_sim                  # interactive menu
./build/dialsort_sim --demo --n 30   # one-shot demo of all 5 algorithms
```

---

## Big-O Complexity Defended

The benchmark prints a complexity table at startup. During the oral defense
you will be asked to justify each entry. Key points:

- **DialSort O(n + U)**: linear in n (single ingestion pass) plus a linear
  scan over the U-sized histogram during projection. No comparisons.
- **DialSort O(n/p + U)**: parallel ingestion divides the n cost across p
  threads; merge is Θ(p·U) but for typical p ≈ 8 and U ≪ n, it's negligible.
- **Parallel RadixSort O(d·n)**: d = 4 passes for int32; each pass is two
  linear scans (histogram + scatter). Independent of distribution.
- **Parallel MergeSort O(n log n)**: log n levels × n total work per level.
- **Parallel SampleSort O(n log p)**: each key does one binary search over
  the p−1 splitters → log p comparisons; chunks then sort independently.
- **Parallel IntroSort O(n log n)**: std::sort is introsort (QS+HS+IS); the
  HeapSort fallback guarantees the worst case.

---

## Reference

```bibtex
@techreport{narvaez2026dialsort,
  title  = {DialSort: Non-Comparative Integer Sorting via the Self-Indexing Principle},
  author = {Narvaez, Alexander},
  month  = {March}, year = {2026}
}
```

---

## Interactive Dashboard

The repo ships with an HTML dashboard at `dashboard/index.html`.
**Just double-click it** — no server needed.

Features:
- Run the 5 algorithms live in the browser (JavaScript port)
- Drag & drop the C++ binary's CSV to visualize results
- Interactive charts, heatmap, and full results table
- Big-O analysis with empirical vs theoretical curve

See `dashboard/README.md` for details.
