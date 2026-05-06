// src/benchmark/html_exporter.cpp
// Genera un reporte HTML interactivo con Chart.js y tablas dinámicas.

#include "benchmark.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

// ─── Helpers ──────────────────────────────────────────────────────────────────

static std::string js_str_array(const std::vector<std::string>& v) {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        ss << '"' << v[i] << '"';
        if (i + 1 < v.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

// Paleta de colores — uno por algoritmo
static const char* COLORS[] = {
    "#4CAF50","#2196F3","#FF9800","#9C27B0","#F44336","#00BCD4"
};

void export_html(const std::vector<BenchCase>& all,
                 const std::vector<AlgoInfo>&  algos,
                 const BenchConfig&            /*cfg*/,
                 const std::string&            path) {

    // ── Recolectar metadatos ──────────────────────────────────────────────
    std::set<std::string>  dists_set;
    std::set<size_t>       ns_set;
    std::set<int>          us_set;
    std::vector<std::string> algo_names;

    for (const auto& bc : all) {
        if (bc.stats.skipped) continue;
        dists_set.insert(bc.dist);
        ns_set.insert(bc.n);
        us_set.insert(bc.U);
    }
    for (const auto& a : algos) algo_names.push_back(a.name);

    std::vector<std::string> dists(dists_set.begin(), dists_set.end());
    std::vector<size_t>      ns(ns_set.begin(), ns_set.end());

    std::ofstream f(path);
    if (!f) { std::cerr << "[!] Cannot write HTML: " << path << "\n"; return; }

    // ── Inicio HTML ───────────────────────────────────────────────────────
    f << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>DialSort Benchmark Report — 2026</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
  :root {
    --bg: #0f1117; --card: #1a1d27; --border: #2a2d3e;
    --text: #e2e8f0; --muted: #94a3b8; --accent: #4CAF50;
    --dial: #4CAF50;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { background: var(--bg); color: var(--text); font-family: 'Segoe UI', system-ui, sans-serif; padding: 24px; }
  h1 { font-size: 2rem; color: var(--accent); margin-bottom: 4px; }
  h2 { font-size: 1.2rem; color: var(--text); margin: 32px 0 12px; border-left: 4px solid var(--accent); padding-left: 12px; }
  h3 { font-size: 1rem; color: var(--muted); margin-bottom: 8px; }
  .subtitle { color: var(--muted); margin-bottom: 32px; }
  .grid2 { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
  .grid3 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 16px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 20px; }
  .chart-wrap { position: relative; height: 320px; }
  table { width: 100%; border-collapse: collapse; font-size: 0.85rem; }
  th { background: #252836; color: var(--muted); text-align: left; padding: 10px 12px; border-bottom: 1px solid var(--border); }
  td { padding: 9px 12px; border-bottom: 1px solid var(--border); }
  tr:hover td { background: rgba(255,255,255,0.03); }
  .dial { color: var(--dial); font-weight: 600; }
  .best { background: rgba(76,175,80,0.15); color: #4CAF50; font-weight: 700; }
  .badge { display: inline-block; padding: 2px 8px; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; }
  .badge-ok  { background: rgba(76,175,80,0.2);  color: #4CAF50; }
  .badge-err { background: rgba(244,67,54,0.2);  color: #F44336; }
  .bigo-table td, .bigo-table th { font-family: monospace; }
  .stat-grid { display: grid; grid-template-columns: repeat(3,1fr); gap: 10px; margin-bottom: 24px; }
  .stat-card { background: var(--card); border: 1px solid var(--border); border-radius: 10px; padding: 14px 18px; }
  .stat-val { font-size: 1.6rem; font-weight: 700; color: var(--accent); }
  .stat-lbl { font-size: 0.8rem; color: var(--muted); margin-top: 2px; }
  select, button { background: var(--card); color: var(--text); border: 1px solid var(--border);
                   border-radius: 8px; padding: 8px 14px; font-size: 0.9rem; cursor: pointer; }
  select:focus, button:hover { border-color: var(--accent); outline: none; }
  .controls { display: flex; gap: 12px; flex-wrap: wrap; margin-bottom: 16px; align-items: center; }
  .controls label { color: var(--muted); font-size: 0.85rem; }
  footer { margin-top: 48px; color: var(--muted); font-size: 0.8rem; border-top: 1px solid var(--border); padding-top: 16px; }
</style>
</head>
<body>
<h1>&#9870; DialSort Benchmark Report</h1>
<p class="subtitle">2026 &mdash; Comparing DialSort with 4 Parallel Sorting Algorithms</p>
)";

    // ── Tarjetas de estadísticas globales ─────────────────────────────────
    // Encontrar el ganador global
    const BenchCase* global_winner = nullptr;
    for (const auto& bc : all)
        if (!bc.stats.skipped && bc.stats.correct)
            if (!global_winner || bc.stats.throughput > global_winner->stats.throughput)
                global_winner = &bc;

    size_t total_cases = 0;
    for (const auto& bc : all) if (!bc.stats.skipped) total_cases++;

    f << "<div class='stat-grid'>\n"
      << "<div class='stat-card'><div class='stat-val'>" << algos.size()
      << "</div><div class='stat-lbl'>Algorithms Compared</div></div>\n"
      << "<div class='stat-card'><div class='stat-val'>" << total_cases
      << "</div><div class='stat-lbl'>Total Benchmark Cases</div></div>\n";

    if (global_winner)
        f << "<div class='stat-card'><div class='stat-val dial'>"
          << global_winner->algo
          << "</div><div class='stat-lbl'>Global Throughput Winner ("
          << std::fixed << std::setprecision(1) << global_winner->stats.throughput
          << " M keys/s)</div></div>\n";

    f << "</div>\n";

    // ── Big O Complexity Table ────────────────────────────────────────────
    f << "<h2>&#128202; Complexity Analysis (Big O)</h2>\n"
      << "<div class='card'><table class='bigo-table'>\n"
      << "<tr><th>Algorithm</th><th>Best Case</th><th>Average Case</th>"
         "<th>Worst Case</th><th>Space</th></tr>\n";

    for (const auto& a : algos) {
        bool is_dial = (a.name.find("DialSort") != std::string::npos);
        std::string cls = is_dial ? " class='dial'" : "";
        f << "<tr" << cls << ">"
          << "<td" << cls << ">" << a.name << "</td>"
          << "<td>" << a.bigo_best  << "</td>"
          << "<td>" << a.bigo_avg   << "</td>"
          << "<td>" << a.bigo_worst << "</td>"
          << "<td>" << a.bigo_space << "</td>"
          << "</tr>\n";
    }
    f << "</table></div>\n";

    // ── Controles interactivos ────────────────────────────────────────────
    f << "<h2>&#128200; Interactive Charts</h2>\n"
      << "<div class='controls'>\n"
      << "  <label>Distribution:</label>\n"
      << "  <select id='sel-dist' onchange='updateCharts()'>\n";
    for (const auto& d : dists)
        f << "    <option value='" << d << "'>" << d << "</option>\n";
    f << "  </select>\n"
      << "  <label>Universe U:</label>\n"
      << "  <select id='sel-u' onchange='updateCharts()'>\n";
    for (int u : us_set)
        f << "    <option value='" << u << "'>" << u << "</option>\n";
    f << "  </select>\n"
      << "</div>\n";

    f << "<div class='grid2'>\n"
      << "<div class='card'><h3>Throughput (M keys/s) — higher is better</h3>"
         "<div class='chart-wrap'><canvas id='chart-tput'></canvas></div></div>\n"
      << "<div class='card'><h3>Median Time (ms) — lower is better</h3>"
         "<div class='chart-wrap'><canvas id='chart-time'></canvas></div></div>\n"
      << "</div>\n";

    // ── Gráfica de escalabilidad ──────────────────────────────────────────
    f << "<h2>&#128640; Scalability (N vs Throughput)</h2>\n"
      << "<div class='controls'>\n"
      << "  <label>Distribution:</label>\n"
      << "  <select id='sel-dist2' onchange='updateScale()'>\n";
    for (const auto& d : dists)
        f << "    <option value='" << d << "'>" << d << "</option>\n";
    f << "  </select>\n"
      << "  <label>Universe U:</label>\n"
      << "  <select id='sel-u2' onchange='updateScale()'>\n";
    for (int u : us_set)
        f << "    <option value='" << u << "'>" << u << "</option>\n";
    f << "  </select>\n</div>\n"
      << "<div class='card'><div class='chart-wrap' style='height:380px'>"
         "<canvas id='chart-scale'></canvas></div></div>\n";

    // ── Tabla completa de resultados ──────────────────────────────────────
    f << "<h2>&#128203; Full Results Table</h2>\n"
      << "<div class='controls'>\n"
      << "  <label>Filter distribution:</label>\n"
      << "  <select id='tbl-dist' onchange='filterTable()'>\n"
      << "    <option value='all'>All</option>\n";
    for (const auto& d : dists)
        f << "    <option value='" << d << "'>" << d << "</option>\n";
    f << "  </select>\n</div>\n"
      << "<div class='card' style='overflow-x:auto'><table id='results-table'>\n"
      << "<tr><th>Algorithm</th><th>Distribution</th><th>N</th><th>U</th>"
         "<th>Median ms</th><th>Mean ms</th><th>StdDev</th><th>Min ms</th>"
         "<th>M keys/s</th><th>Mem KB</th><th>Status</th></tr>\n";

    for (const auto& bc : all) {
        if (bc.stats.skipped) continue;
        bool is_dial = (bc.algo.find("DialSort") != std::string::npos);
        std::string row_cls = is_dial ? " class='dial'" : "";

        f << "<tr data-dist='" << bc.dist << "'>\n"
          << "<td" << row_cls << ">" << bc.algo << "</td>\n"
          << "<td>" << bc.dist << "</td>\n"
          << "<td>" << bc.n << "</td>\n"
          << "<td>" << bc.U << "</td>\n"
          << std::fixed << std::setprecision(3)
          << "<td>" << bc.stats.median_ms  << "</td>\n"
          << "<td>" << bc.stats.mean_ms    << "</td>\n"
          << "<td>" << bc.stats.stddev_ms  << "</td>\n"
          << "<td>" << bc.stats.min_ms     << "</td>\n"
          << "<td>" << bc.stats.throughput << "</td>\n"
          << "<td>" << bc.stats.mean_mem_kb << "</td>\n"
          << "<td><span class='badge "
          << (bc.stats.correct ? "badge-ok'>OK" : "badge-err'>ERR")
          << "</span></td>\n"
          << "</tr>\n";
    }
    f << "</table></div>\n";

    // ── JavaScript con los datos embebidos ────────────────────────────────
    f << "<script>\n"
      << "const RAW = [\n";

    for (const auto& bc : all) {
        if (bc.stats.skipped) continue;
        f << "  {algo:\"" << bc.algo << "\","
          << "dist:\""    << bc.dist << "\","
          << "n:"         << bc.n    << ","
          << "U:"         << bc.U    << ","
          << "med:"       << std::fixed << std::setprecision(3) << bc.stats.median_ms << ","
          << "tput:"      << bc.stats.throughput << ","
          << "mem:"       << bc.stats.mean_mem_kb << "},\n";
    }

    f << "];\n";

    // Paleta
    f << "const ALGOS = " << js_str_array(algo_names) << ";\n";
    f << "const COLORS = ['" ;
    for (size_t i = 0; i < algo_names.size(); ++i) {
        f << COLORS[i % 6];
        if (i + 1 < algo_names.size()) f << "','";
    }
    f << "'];\n";

    f << R"(
// ── Chart state ───────────────────────────────────────────────────────────
let chartTput = null, chartTime = null, chartScale = null;

function filtered(dist, U) {
    return RAW.filter(r => r.dist === dist && r.U === Number(U));
}

function byAlgo(data, algo) {
    return data.filter(r => r.algo === algo);
}

function makeBarDatasets(data, key) {
    const ns = [...new Set(data.map(r => r.n))].sort((a,b)=>a-b);
    return ALGOS.map((algo, i) => ({
        label: algo,
        backgroundColor: COLORS[i % COLORS.length] + 'cc',
        borderColor:     COLORS[i % COLORS.length],
        borderWidth: 1,
        data: ns.map(n => {
            const r = data.find(d => d.algo === algo && d.n === n);
            return r ? r[key] : null;
        })
    }));
}

function makeLabels(data) {
    return [...new Set(data.map(r => r.n))].sort((a,b)=>a-b)
           .map(n => n >= 1e6 ? (n/1e6)+'M' : (n/1e3)+'K');
}

function initChart(id, type, labels, datasets, yLabel) {
    const ctx = document.getElementById(id).getContext('2d');
    return new Chart(ctx, {
        type, data: { labels, datasets },
        options: {
            responsive: true, maintainAspectRatio: false,
            plugins: { legend: { labels: { color: '#e2e8f0', font: { size: 11 } } } },
            scales: {
                x: { ticks: { color: '#94a3b8' }, grid: { color: '#2a2d3e' } },
                y: { ticks: { color: '#94a3b8' }, grid: { color: '#2a2d3e' },
                     title: { display: true, text: yLabel, color: '#94a3b8' } }
            }
        }
    });
}

function updateCharts() {
    const dist = document.getElementById('sel-dist').value;
    const U    = document.getElementById('sel-u').value;
    const data = filtered(dist, U);
    const lbl  = makeLabels(data);

    // Throughput chart
    if (chartTput) chartTput.destroy();
    chartTput = initChart('chart-tput', 'bar', lbl,
        makeBarDatasets(data, 'tput'), 'M keys/s');

    // Time chart
    if (chartTime) chartTime.destroy();
    chartTime = initChart('chart-time', 'bar', lbl,
        makeBarDatasets(data, 'med'), 'Median ms');
}

function updateScale() {
    const dist = document.getElementById('sel-dist2').value;
    const U    = document.getElementById('sel-u2').value;
    const data = filtered(dist, U);
    const ns   = [...new Set(data.map(r => r.n))].sort((a,b)=>a-b);
    const lbl  = ns.map(n => n >= 1e6 ? (n/1e6)+'M' : (n/1e3)+'K');

    const datasets = ALGOS.map((algo, i) => ({
        label: algo,
        borderColor:     COLORS[i % COLORS.length],
        backgroundColor: COLORS[i % COLORS.length] + '33',
        tension: 0.3, fill: false, pointRadius: 5,
        data: ns.map(n => {
            const r = data.find(d => d.algo === algo && d.n === n);
            return r ? r.tput : null;
        })
    }));

    if (chartScale) chartScale.destroy();
    chartScale = initChart('chart-scale', 'line', lbl, datasets, 'M keys/s');
}

function filterTable() {
    const dist = document.getElementById('tbl-dist').value;
    document.querySelectorAll('#results-table tr[data-dist]').forEach(row => {
        row.style.display = (dist === 'all' || row.dataset.dist === dist) ? '' : 'none';
    });
}

// Resaltar mejor en cada columna numérica
function highlightBest() {
    const tbl = document.getElementById('results-table');
    const cols = [4, 8]; // median, throughput
    const asc  = [true, false]; // menor=mejor para tiempo, mayor=mejor para tput
    cols.forEach((col, ci) => {
        const cells = [...tbl.querySelectorAll(`tr[data-dist] td:nth-child(${col+1})`)];
        const vals  = cells.map(c => parseFloat(c.textContent));
        const best  = asc[ci] ? Math.min(...vals) : Math.max(...vals);
        cells.forEach((c, i) => { if (vals[i] === best) c.classList.add('best'); });
    });
}

// Init
window.onload = () => { updateCharts(); updateScale(); highlightBest(); };
</script>
)";

    f << "<footer>Generated by DialSort Benchmark v2.0 &mdash; 2026</footer>\n"
      << "</body></html>\n";

    std::cout << "[+] HTML report exported: " << path << "\n";
}
