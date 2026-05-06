# DialSort Dashboard

Interactive HTML dashboard for the DialSort benchmark. **No server required.**

## Open it

Simply double-click `index.html` and your browser will open it.
Works offline — only Chart.js is loaded from a CDN.

## Features

| View | What it does |
|------|--------------|
| **Overview**       | Summary tiles + best performer chart |
| **Run Benchmark**  | Execute all 5 algorithms in JavaScript with live progress + log |
| **Charts**         | Bar/line charts with metric selector + heatmap |
| **Full Results**   | Sortable, searchable table of all measurements |
| **Big-O Analysis** | Theoretical complexity table + empirical-vs-theoretical chart |
| **Import CSV**     | Drag-and-drop loader for the C++ binary's output |

## Two ways to use it

### A) Run live in the browser
1. Open the dashboard
2. Click **Run Benchmark** in the sidebar
3. Configure N, U, runs, distributions
4. Click the green **Run Benchmark** button

JavaScript runs single-threaded so absolute times will be slower than the
C++ binary, but **relative comparisons remain valid** for demonstration.

### B) Import C++ benchmark CSV (recommended for the report)
1. Run the C++ binary:
   ```bash
   ./dialsort_bench --full
   ```
2. Open the dashboard
3. Go to **Import CSV** in the sidebar
4. Drag `results/benchmark_results.csv` onto the drop zone

All views populate with the real (multi-threaded, optimized) measurements.

## Tech notes

- Single HTML file, ~86 KB
- Pure HTML/CSS/JS — no build step, no framework
- Only external dependency: Chart.js 4.4 from CDN
- Works in any modern browser (Chrome, Firefox, Safari, Edge)
