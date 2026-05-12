#pragma once
// include/benchmark.h

#include "types.h"
#include <vector>
#include <string>

// ── Stats ─────────────────────────────────────────────────────────────────────
Stats compute_stats(std::vector<RunResult>& runs);
bool  is_sorted_ok(const KeyVec& v);

// ── Runner ────────────────────────────────────────────────────────────────────
BenchCase run_case(const AlgoInfo& algo,
                   const KeyVec&   base,
                   Distribution    dist,
                   size_t          n,
                   int             U,
                   const BenchConfig& cfg);

// ── Reporter (terminal) ───────────────────────────────────────────────────────
void print_banner(const BenchConfig& cfg, int threads);
void print_complexity_table(const std::vector<AlgoInfo>& algos);
void print_section(const std::string& title);
void print_case_row(const BenchCase& bc);
void print_section_end();
void print_winner_summary(const std::vector<BenchCase>& all);

// ── CSV export ────────────────────────────────────────────────────────────────
void export_csv(const std::vector<BenchCase>& all,
                const std::string& path);

// ── HTML export ───────────────────────────────────────────────────────────────
void export_html(const std::vector<BenchCase>& all,
                 const std::vector<AlgoInfo>&  algos,
                 const BenchConfig&            cfg,
                 const std::string&            path);
