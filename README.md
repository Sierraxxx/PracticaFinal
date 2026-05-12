# Sorting Benchmark v0.7.0

> Comparative analysis of **DialSort** vs 4 state-of-the-art parallel sorting
> algorithms. Datasets from 100K to 10M records across 5 distributions.
>
> **Pure C++17** — no OpenMP, no TBB, no external dependencies.
> Only the standard library + `std::thread`.

- Juan Sierra

This file is listed in `.gitignore` and will never be uploaded to GitHub.

---

## The 5 Algorithms

| # | Algorithm | Paradigm | Best | Average | Worst | Space |
|---|-----------|----------|------|---------|-------|-------|
| 1 | **DialSort** | Non-comparative, self-indexing | O(n + U) | O(n/p + U) | O(n log n)¹ | O(p·U) or O(n) |
| 2 | Parallel RadixSort | LSD digit-by-digit | O(d·n) | O(d·n) | O(d·n) | O(n + p·B) |
| 3 | Parallel MergeSort | Divide & conquer (stable) | O(n log n) | O(n log n) | O(n log n) | O(n) |
| 4 | Parallel SampleSort | Splitter-based partition | O(n log p) | O(n log p) | O(n log n) | O(n + p²) |
| 5 | Parallel IntroSort | QS + HS + IS hybrid | O(n log n) | O(n log n) | O(n log n) | O(n) practical |

¹ DialSort auto-dispatches to LSD-Radix (O(4n)) when U > 10M.

### DialSort — Self-Indexing Principle

For any integer `k ∈ [min, max]`, the value `(k − min)` **is** the address
of its frequency in `H[]`. Sorted position is known without any order
comparison. The implementation auto-selects the optimal strategy at runtime:

| Strategy | Condition | Complexity |
|----------|-----------|------------|
| CRN Parallel | U ≤ 10M and n ≥ 1024 | O(n/p + U) |
| LSD-Radix | U > 10M (full int32) | O(4n) |
| Sequential | n < 1024 | O(n + U) |

---

## Benchmark Results

Executed on 12 hardware threads · 3 runs + 1 warmup · Median reported.

### Summary Table — 30 Scenarios (N: 100K–1M · U: 256, 1024 · 5 distributions)

| Algorithm | Wins | Best ms | Worst ms | Avg M/s | Avg Mem KB | Max Speedup |
|-----------|------|---------|----------|---------|------------|-------------|
| **DialSort** | **30 / 30** | **0.54** | 5.12 | **263.10** | **32.53** | **51.11×** |
| Parallel RadixSort | 0 | 2.41 | 20.35 | 51.51 | 4,167.83 | 9.77× |
| Parallel SampleSort | 0 | 2.89 | 38.69 | 29.42 | 2,119.00 | 5.57× |
| Parallel IntroSort | 0 | 4.26 | 85.48 | 17.66 | 3.27 | 3.70× |
| Parallel MergeSort | 0 | 7.66 | 127.63 | 10.43 | 1,042.53 | 1.02× |

**VERDICT: DialSort won 30 of 30 scenarios.**

### Key Results by Distribution (N = 1,000,000, U = 1,024)

| Distribution | DialSort ms | RadixSort ms | MergeSort ms | Speedup vs #2 |
|---|---|---|---|---|
| Uniform | 2.69 | 16.36 | 109.06 | **6.1×** |
| Skewed-80-5 | 2.78 | 16.55 | 127.63 | **6.0×** |
| Sorted | 2.48 | 16.36 | 111.94 | **6.6×** |
| Reverse | 2.52 | 16.10 | 114.59 | **6.4×** |
| Nearly-Sorted | 2.56 | 15.32 | 110.63 | **6.0×** |

### Maximum Speedup: 51.11× (Nearly-Sorted, N=500K, U=1024)

- **DialSort: 1.48 ms** — 336.7 M keys/s
- Parallel MergeSort: 75.90 ms — 6.6 M keys/s

### Memory Efficiency

| Algorithm | Avg Mem KB | vs DialSort |
|---|---|---|
| **DialSort** | **32.53** | baseline |
| Parallel IntroSort | 3.27 | 0.1× |
| Parallel MergeSort | 1,042.53 | 32× more |
| Parallel SampleSort | 2,119.00 | 65× more |
| Parallel RadixSort | 4,167.83 | **128× more** |

> Full results: `docs/technical_report.html` · Interactive: `dashboard/index.html`

---

## Repository Structure

```
sorting_benchmark/
├── CMakeLists.txt              ← only depends on pthreads (no OpenMP)
├── README.md
├── AUTHORS.private             ← local only, never uploaded (see .gitignore)
├── docs/
│   ├── technical_report.html  ← full technical report (open in browser)
│   └── technical_report.md    ← report in Markdown
├── dashboard/
│   ├── index.html              ← interactive dashboard (double-click to open)
│   └── README.md
├── datasets/                  ← 18 pre-generated CSV datasets (N=100K, 500K)
│   ├── Uniform_N100000_U256.csv
│   ├── Skewed-80-5_N100000_U256.csv
│   └── ...
├── include/
│   ├── types.h                 ← shared types
│   ├── algorithms.h            ← public API of the 5 algorithms
│   ├── dialsort.h              ← namespace DialSort, strategy dispatch API
│   ├── benchmark.h             ← runner, stats, reporter, exporters
│   ├── simulator.h             ← step-by-step ASCII visualizer
│   ├── data_generator.h
│   ├── memory.h                ← RSS + analytical memory estimator
│   └── parallel_utils.h        ← std::thread primitives (no OpenMP)
└── src/
    ├── CMakeLists.txt
    ├── main_bench.cpp          ← benchmark entry point
    ├── main_sim.cpp            ← simulator entry point
    ├── main_gen.cpp            ← dataset generator entry point
    ├── algorithms/
    │   ├── dialsort.cpp        ← unified DialSort (CRN + LSD-Radix + Sequential)
    │   ├── parallel_radix_sort.cpp
    │   ├── parallel_merge_sort.cpp
    │   ├── parallel_sample_sort.cpp
    │   ├── parallel_intro_sort.cpp
    │   └── registry.cpp        ← Big-O metadata
    ├── benchmark/
    │   ├── runner.cpp
    │   ├── stats.cpp
    │   ├── reporter.cpp        ← terminal tables + comparative analysis
    │   └── html_exporter.cpp
    ├── simulator/
    │   └── visualizer.cpp
    └── utils/
        ├── data_generator.cpp
        └── memory.cpp
```

---

## Build Requirements

| Tool | Version | Notes |
|------|---------|-------|
| GCC / Clang / AppleClang | GCC 9+ / Clang 10+ | C++17 required |
| CMake | 3.20+ | |
| pthreads | built-in | no install needed |

**No external libraries. No Homebrew. No apt-get.**

---

## Build & Run

### From CLion

1. `File → Open` → select the `sorting_benchmark/` folder
2. CLion detects `CMakeLists.txt` automatically
3. Switch to **Release** profile (top-right toolbar)
4. `Build → Build All` (`⌘F9` / `Ctrl+F9`)

> **Note:** If CLion shows `cmake-build-debug` with `.o` files, right-click it
> and select `Mark Directory as → Excluded`.

### From the terminal

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Usage

### Benchmark modes

```bash
# Oral defense — runs in ~30 seconds, most impactful output
./build/dialsort_bench --demo

# Development — N up to 1M, ~2 minutes
./build/dialsort_bench --quick

# Full report — N up to 10M, ~60 minutes
./build/dialsort_bench --full

# Custom
./build/dialsort_bench --n 5000000 --u 1024 --runs 5

# Help
./build/dialsort_bench --help
```

Output files:
- `results/benchmark_results.csv` — raw data
- `results/report.html` — interactive HTML report

### Visualizer

```bash
./build/dialsort_sim               # interactive menu
./build/dialsort_sim --demo --n 30 # demo of all 5 algorithms
```

### Dataset generator

```bash
./build/gen_datasets               # generates all datasets to datasets/
./build/gen_datasets --n 1000000   # single N
```

---

## Interactive Dashboard

Open `dashboard/index.html` in any browser — **double-click, no server needed.**

| Feature | Description |
|---------|-------------|
| Run live | Execute all 5 algorithms in JavaScript with live progress |
| Import CSV | Drag-and-drop `results/benchmark_results.csv` |
| Charts | Bar, scalability line, heatmap |
| Results table | Searchable and filterable |
| Big-O analysis | Theoretical + empirical comparison |

---

## Technical Report

Open `docs/technical_report.html` in any browser for the full analysis:
- Algorithm descriptions and Big-O complexity
- Results by distribution with interactive charts
- Memory consumption analysis
- Discussion of why DialSort wins and when it does not

---

## Big-O Key Points for the Defense

- **DialSort O(n + U)**: linear ingestion pass + linear projection. Zero comparisons.
- **DialSort O(n/p + U)**: parallel ingestion divides n across p threads.
- **RadixSort O(d·n)**: d=4 fixed passes for int32, distribution-independent.
- **MergeSort O(n log n)**: log n levels × n work per level, stable.
- **SampleSort O(n log p)**: one binary search per key over p−1 splitters.
- **IntroSort O(n log n)**: HeapSort fallback guarantees worst case.

---

## Reference

```bibtex
@techreport{narvaez2026dialsort,
  title       = {DialSort: Non-Comparative Integer Sorting via the Self-Indexing Principle},
  author      = {Narvaez, Alexander},
  institution = {Universidad EAFIT},
  address     = {Envigado, Colombia},
  month       = {March},
  year        = {2026}
}
```
