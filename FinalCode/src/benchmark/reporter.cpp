// src/benchmark/reporter.cpp
#include "benchmark.h"
#include "parallel_utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

// ── ANSI colors ───────────────────────────────────────────────────────────────
#ifndef _WIN32
  #define C_RESET   "\033[0m"
  #define C_BOLD    "\033[1m"
  #define C_DIM     "\033[2m"
  #define C_CYAN    "\033[36m"
  #define C_GREEN   "\033[32m"
  #define C_YELLOW  "\033[33m"
  #define C_RED     "\033[31m"
  #define C_BLUE    "\033[34m"
  #define C_MAGENTA "\033[35m"
  #define C_WHITE   "\033[37m"
#else
  #define C_RESET  "" 
  #define C_BOLD   ""
  #define C_DIM    ""
  #define C_CYAN   ""
  #define C_GREEN  ""
  #define C_YELLOW ""
  #define C_RED    ""
  #define C_BLUE   ""
  #define C_MAGENTA ""
  #define C_WHITE  ""
#endif

static constexpr int W = 112;

static void hline(char c = '-') {
    std::cout << '+' << std::string(W - 2, c) << "+\n";
}

// ── Banner ────────────────────────────────────────────────────────────────────
void print_banner(const BenchConfig& cfg, int threads) {
    hline('=');
    std::cout << "| " << C_BOLD << C_CYAN
              << "  SORTING BENCHMARK v0.7.0"
              << C_RESET
              << std::string(W - 30, ' ') << "|\n";
    hline('=');

    std::cout << "| " << C_BOLD << "  N sizes : " << C_RESET;
    for (size_t s : cfg.sizes) std::cout << s << "  ";
    std::cout << "\n";

    std::cout << "| " << C_BOLD << "  U sizes : " << C_RESET;
    for (int u : cfg.universes) std::cout << u << "  ";
    std::cout << "\n";

    std::cout << "| " << C_BOLD
              << "  Runs    : " << cfg.runs
              << "   Warmup: "  << cfg.warmup
              << "   Threads: " << threads
              << C_RESET << "\n";
    hline('=');
    std::cout << "\n";
}

// ── Big-O table ───────────────────────────────────────────────────────────────
void print_complexity_table(const std::vector<AlgoInfo>& algos) {
    std::cout << C_BOLD << C_CYAN
              << "\n  BIG-O COMPLEXITY REFERENCE\n" << C_RESET;
    hline('=');
    std::cout << "| "
              << std::left
              << std::setw(28) << "Algorithm"
              << std::setw(18) << "Best"
              << std::setw(18) << "Average"
              << std::setw(18) << "Worst"
              << std::setw(20) << "Space"
              << " |\n";
    hline('-');
    for (const auto& a : algos) {
        bool dial = (a.name.find("DialSort") != std::string::npos);
        std::cout << "| " << (dial ? C_GREEN : "")
                  << std::left << std::setw(28) << a.name
                  << std::setw(18) << a.bigo_best
                  << std::setw(18) << a.bigo_avg
                  << std::setw(18) << a.bigo_worst
                  << std::setw(20) << a.bigo_space
                  << (dial ? C_RESET : "") << " |\n";
    }
    hline('=');
    std::cout << "\n";
}

// ── Section header ────────────────────────────────────────────────────────────
void print_section(const std::string& title) {
    std::cout << "\n" << C_BOLD << C_YELLOW
              << "  >>> " << title << C_RESET << "\n";
    hline('=');
    std::cout << "| " << C_BOLD
              << std::left  << std::setw(26) << "Algorithm"
              << std::right
              << std::setw(12) << "N"
              << std::setw(8)  << "U"
              << std::setw(12) << "Median ms"
              << std::setw(11) << "Mean ms"
              << std::setw(10) << "StdDev"
              << std::setw(10) << "Min ms"
              << std::setw(11) << "M keys/s"
              << std::setw(10) << "Mem KB"
              << std::setw(6)  << "OK"
              << C_RESET << " |\n";
    hline('-');
}

// ── Row ───────────────────────────────────────────────────────────────────────
void print_case_row(const BenchCase& bc) {
    if (bc.stats.skipped) {
        std::cout << "| " << C_YELLOW
                  << std::left << std::setw(26) << bc.algo
                  << std::right << std::setw(W - 30)
                  << "-- skipped --"
                  << C_RESET << " |\n";
        return;
    }
    bool dial = (bc.algo.find("DialSort") != std::string::npos);
    std::string ok_str = bc.stats.correct
        ? (std::string(C_GREEN) + " OK" + C_RESET)
        : (std::string(C_RED)   + "ERR" + C_RESET);

    std::cout << "| " << (dial ? C_GREEN : "")
              << std::left  << std::setw(26) << bc.algo
              << std::right << std::fixed    << std::setprecision(2)
              << std::setw(12) << static_cast<double>(bc.n)
              << std::setw(8)  << bc.U
              << std::setw(12) << bc.stats.median_ms
              << std::setw(11) << bc.stats.mean_ms
              << std::setw(10) << bc.stats.stddev_ms
              << std::setw(10) << bc.stats.min_ms
              << std::setw(11) << bc.stats.throughput
              << std::setw(10) << bc.stats.mean_mem_kb
              << (dial ? C_RESET : "")
              << "  " << ok_str << " |\n";
}

void print_section_end() {
    hline('=');
}

// ═════════════════════════════════════════════════════════════════════════════
//  CUADRO COMPARATIVO FINAL
//  El más importante de toda la salida — muestra speedup, ranking y veredicto
// ═════════════════════════════════════════════════════════════════════════════
void print_winner_summary(const std::vector<BenchCase>& all) {

    // ── 1. Recolectar resultados válidos ──────────────────────────────────
    std::vector<const BenchCase*> valid;
    for (const auto& bc : all)
        if (!bc.stats.skipped && bc.stats.correct)
            valid.push_back(&bc);

    if (valid.empty()) {
        std::cout << C_RED << "  No hay resultados validos.\n" << C_RESET;
        return;
    }

    // ── 2. Agrupar por (dist, N, U) y calcular estadísticas cruzadas ─────
    // Para cada escenario: ranking de todos los algoritmos
    struct ScenarioResult {
        std::string dist;
        size_t      n;
        int         u;
        struct AlgoPerf {
            std::string algo;
            double      median_ms;
            double      throughput;
            size_t      mem_kb;
        };
        std::vector<AlgoPerf> perfs;
    };

    std::map<std::string, ScenarioResult> scenarios;
    for (const auto* bc : valid) {
        std::string key = bc->dist + "_N" + std::to_string(bc->n)
                        + "_U" + std::to_string(bc->U);
        auto& sc = scenarios[key];
        sc.dist = bc->dist;
        sc.n    = bc->n;
        sc.u    = bc->U;
        sc.perfs.push_back({bc->algo, bc->stats.median_ms,
                            bc->stats.throughput, bc->stats.mean_mem_kb});
    }

    // Ordenar perfs por mediana dentro de cada escenario
    for (auto& [k, sc] : scenarios)
        std::sort(sc.perfs.begin(), sc.perfs.end(),
                  [](const auto& a, const auto& b){
                      return a.median_ms < b.median_ms; });

    // ── 3. Cuadro comparativo detallado por escenario ─────────────────────
    std::cout << "\n";
    std::string title = "  COMPARATIVE ANALYSIS — DETAILED RESULTS";
    std::cout << C_BOLD << C_CYAN << title << C_RESET << "\n";
    hline('=');

    for (const auto& [key, sc] : scenarios) {
        // Encabezado del escenario
        std::cout << "\n| " << C_BOLD << C_YELLOW
                  << "Scenario: " << sc.dist
                  << "   N = " << sc.n
                  << "   U = " << sc.u
                  << C_RESET << "\n";
        hline('-');

        // Cabecera columnas
        std::cout << "| "
                  << C_BOLD
                  << std::left  << std::setw(5)  << "Rank"
                  << std::setw(28) << "Algorithm"
                  << std::right
                  << std::setw(12) << "Median ms"
                  << std::setw(12) << "M keys/s"
                  << std::setw(10) << "Mem KB"
                  << std::setw(12) << "Speedup"
                  << std::setw(12) << "vs slowest"
                  << C_RESET << " |\n";
        hline('-');

        const double slowest_ms  = sc.perfs.back().median_ms;
        const double fastest_ms  = sc.perfs.front().median_ms;

        int rank = 1;
        for (const auto& p : sc.perfs) {
            // is_dial accessor removed - color handled by rank position
            const bool is_first  = (rank == 1);
            const bool is_last   = (rank == static_cast<int>(sc.perfs.size()));

            // Speedup vs el primero (para ver cuánto más lento es)
            const double speedup_vs_best   = p.median_ms / fastest_ms;
            // Speedup del ganador vs este (cuánto gana DialSort sobre él)
            const double speedup_vs_slow   = slowest_ms  / p.median_ms;

            // Color según posición
            std::string col = "";
            if (is_first) col = C_GREEN;
            else if (is_last) col = C_RED;
            else col = C_RESET;

            std::cout << "| " << col
                      << std::left  << std::setw(5) << ("#" + std::to_string(rank))
                      << std::setw(28) << p.algo
                      << std::right << std::fixed << std::setprecision(2)
                      << std::setw(12) << p.median_ms
                      << std::setw(12) << p.throughput
                      << std::setw(10) << p.mem_kb;

            // Speedup vs el ganador
            if (is_first) {
                std::cout << std::setw(12) << "WINNER"
                          << std::setw(12) << "---";
            } else {
                std::cout << std::setw(10) << speedup_vs_best << "x slower"
                          << std::setw(10) << speedup_vs_slow << "x faster";
            }

            std::cout << C_RESET << " |\n";
            ++rank;
        }

        // Conclusión del escenario
        const auto& winner = sc.perfs.front();
        const auto& second = sc.perfs[std::min((size_t)1, sc.perfs.size()-1)];
        const bool  dial_wins = (winner.algo.find("DialSort") != std::string::npos);
        const double gap = second.median_ms / winner.median_ms;

        hline('-');
        std::cout << "| " << C_BOLD
                  << (dial_wins ? C_GREEN : C_YELLOW)
                  << "  Winner: " << winner.algo
                  << "  |  "
                  << std::fixed << std::setprecision(1)
                  << gap << "x faster than #2 ("
                  << second.algo << ")"
                  << "  |  "
                  << winner.throughput << " M keys/s"
                  << C_RESET << "\n";
        hline('=');
    }

    // ── 4. Tabla resumen global (todos los escenarios en una vista) ────────
    std::cout << "\n" << C_BOLD << C_CYAN
              << "  SUMMARY TABLE — All scenarios at a glance\n" << C_RESET;
    hline('=');
    std::cout << "| " << C_BOLD
              << std::left  << std::setw(26) << "Algorithm"
              << std::right
              << std::setw(8)  << "Wins"
              << std::setw(14) << "Best ms"
              << std::setw(14) << "Worst ms"
              << std::setw(14) << "Avg M/s"
              << std::setw(12) << "Avg Mem KB"
              << std::setw(16) << "Max Speedup"
              << C_RESET << " |\n";
    hline('-');

    // Agregar stats por algoritmo
    struct AlgoStats {
        int    wins       = 0;
        double best_ms    = 1e9;
        double worst_ms   = 0;
        double sum_tput   = 0;
        double sum_mem    = 0;
        double max_speedup = 0;
        int    count      = 0;
    };
    std::map<std::string, AlgoStats> astats;

    for (const auto& [key, sc] : scenarios) {
        // fastest_ms removed - using sc.perfs.back/front directly
        astats[sc.perfs.front().algo].wins++;

        for (const auto& p : sc.perfs) {
            auto& as = astats[p.algo];
            as.best_ms    = std::min(as.best_ms, p.median_ms);
            as.worst_ms   = std::max(as.worst_ms, p.median_ms);
            as.sum_tput  += p.throughput;
            as.sum_mem   += static_cast<double>(p.mem_kb);
            as.max_speedup = std::max(as.max_speedup,
                                      sc.perfs.back().median_ms / p.median_ms);
            ++as.count;
        }
    }

    // Ordenar por victorias desc
    std::vector<std::pair<std::string, AlgoStats>> sorted_stats(
        astats.begin(), astats.end());
    std::sort(sorted_stats.begin(), sorted_stats.end(),
              [](const auto& a, const auto& b){
                  return a.second.wins > b.second.wins; });

    int global_rank = 1;
    for (const auto& [algo, as] : sorted_stats) {
        const bool dial = (algo.find("DialSort") != std::string::npos);
        const bool top  = (global_rank == 1);

        std::cout << "| " << (top ? C_GREEN : (dial ? C_GREEN : ""))
                  << std::left  << std::setw(26) << algo
                  << std::right << std::fixed    << std::setprecision(2)
                  << std::setw(8)  << as.wins
                  << std::setw(14) << as.best_ms
                  << std::setw(14) << as.worst_ms
                  << std::setw(14) << (as.count ? as.sum_tput / as.count : 0)
                  << std::setw(12) << (as.count ? as.sum_mem  / as.count : 0)
                  << std::setw(14) << as.max_speedup << "x"
                  << (top || dial ? C_RESET : "") << " |\n";
        ++global_rank;
    }
    hline('=');

    // ── 5. Veredicto final ─────────────────────────────────────────────────
    if (!sorted_stats.empty()) {
        const auto& [best_algo, best_as] = sorted_stats.front();
        const bool dial_won = (best_algo.find("DialSort") != std::string::npos);

        std::cout << "\n";
        hline('=');
        std::cout << "| " << C_BOLD
                  << (dial_won ? C_GREEN : C_YELLOW)
                  << "  VERDICT: "
                  << best_algo
                  << " won " << best_as.wins << " of "
                  << scenarios.size() << " scenarios"
                  << C_RESET << "\n";

        if (dial_won) {
            std::cout << "| " << C_GREEN
                      << "  Reason : Non-comparative O(n+U) self-indexing principle."
                      << " No order comparisons performed."
                      << C_RESET << "\n";
            std::cout << "| " << C_DIM
                      << "  Caveat : DialSort requires integer keys and bounded U."
                      << " For U > 10M it dispatches to LSD-Radix automatically."
                      << C_RESET << "\n";
        }
        hline('=');
    }

    std::cout << "\n";
}

// ── CSV export ────────────────────────────────────────────────────────────────
void export_csv(const std::vector<BenchCase>& all,
                const std::string& path) {
    std::ofstream f(path);
    if (!f) { std::cerr << "[!] Cannot write CSV: " << path << "\n"; return; }

    f << "algorithm,short_name,distribution,n,U,"
         "median_ms,mean_ms,stddev_ms,min_ms,max_ms,"
         "throughput_Mkeys_s,mem_kb,"
         "bigo_best,bigo_avg,bigo_worst,bigo_space,"
         "correct\n";

    for (const auto& bc : all) {
        if (bc.stats.skipped) continue;
        f << '"' << bc.algo       << '"' << ','
          << '"' << bc.short_name << '"' << ','
          << '"' << bc.dist       << '"' << ','
          << bc.n << ',' << bc.U  << ','
          << std::fixed << std::setprecision(4)
          << bc.stats.median_ms  << ',' << bc.stats.mean_ms   << ','
          << bc.stats.stddev_ms  << ',' << bc.stats.min_ms    << ','
          << bc.stats.max_ms     << ',' << bc.stats.throughput << ','
          << bc.stats.mean_mem_kb << ','
          << '"' << bc.bigo_best  << '"' << ','
          << '"' << bc.bigo_avg   << '"' << ','
          << '"' << bc.bigo_worst << '"' << ','
          << '"' << bc.bigo_space << '"' << ','
          << (bc.stats.correct ? "true" : "false") << '\n';
    }
    std::cout << "[+] CSV exportado: " << path << "\n";
}
