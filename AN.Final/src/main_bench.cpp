// src/main_bench.cpp
//
// Sorting Benchmark v0.7.0
//
// Uso:
//   ./dialsort_bench                   (default: quick, ~2 min)
//   ./dialsort_bench --demo            (sustentacion en vivo, ~30 seg)
//   ./dialsort_bench --quick           (desarrollo, ~2-3 min)
//   ./dialsort_bench --full            (reporte final, ~30-60 min)
//   ./dialsort_bench --n 1000000 --u 1024 --runs 3
//   ./dialsort_bench --help

#include "types.h"
#include "algorithms.h"
#include "benchmark.h"
#include "data_generator.h"
#include "parallel_utils.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─── Parsear argumentos ───────────────────────────────────────────────────────
BenchConfig parse_args(int argc, char* argv[]) {
    BenchConfig cfg;

    // Default sensato: quick (no el full que tarda 1 hora)
    cfg.sizes     = {100'000, 500'000, 1'000'000};
    cfg.universes = {256, 1024};
    cfg.runs      = 3;
    cfg.warmup    = 1;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        // ── --demo: modo sustentación en vivo (~30 segundos) ──────────────
        // Un solo N, un solo U, solo 2 distribuciones, 3 runs.
        // Produce resultados representativos sin esperar minutos.
        if (arg == "--demo") {
            cfg.sizes     = {1'000'000};
            cfg.universes = {1024};
            cfg.runs      = 3;
            cfg.warmup    = 1;
            // Solo distribuciones más interesantes para demostrar
            cfg.demo_mode = true;
        }
        // ── --quick: desarrollo y pruebas (~2-3 minutos) ──────────────────
        else if (arg == "--quick") {
            cfg.sizes     = {100'000, 500'000, 1'000'000};
            cfg.universes = {256, 1024};
            cfg.runs      = 3;
            cfg.warmup    = 1;
        }
        // ── --full: reporte final completo (~30-60 minutos) ───────────────
        else if (arg == "--full") {
            cfg.sizes     = {100'000, 500'000, 1'000'000,
                             2'000'000, 5'000'000, 10'000'000};
            cfg.universes = {256, 1024, 65536};
            cfg.runs      = 7;
            cfg.warmup    = 2;
        }
        else if (arg == "--n" && i + 1 < argc) {
            cfg.sizes = {(size_t)std::stoll(argv[++i])};
        }
        else if (arg == "--u" && i + 1 < argc) {
            cfg.universes = {std::stoi(argv[++i])};
        }
        else if (arg == "--runs" && i + 1 < argc) {
            cfg.runs = std::stoi(argv[++i]);
        }
        else if (arg == "--no-html") {
            cfg.html_out = "";
        }
        else if (arg == "--out" && i + 1 < argc) {
            std::string dir = argv[++i];
            cfg.csv_out  = dir + "/benchmark_results.csv";
            cfg.html_out = dir + "/report.html";
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout
                << "\nSorting Benchmark v0.7.0\n\n"
                << "Modos de uso:\n"
                << "  (sin flags)      Quick mode: N hasta 1M, 3 runs       ~2 min\n"
                << "  --demo           Sustentacion: 1M keys, 30 segundos\n"
                << "  --quick          Desarrollo: N hasta 1M, 2 universos   ~2 min\n"
                << "  --full           Reporte final: N hasta 10M, 7 runs    ~60 min\n\n"
                << "Opciones individuales:\n"
                << "  --n N            Fijar un N especifico\n"
                << "  --u U            Fijar un universo especifico\n"
                << "  --runs R         Numero de runs por caso\n"
                << "  --no-html        No generar reporte HTML\n"
                << "  --out DIR        Directorio de salida\n\n"
                << "Ejemplos:\n"
                << "  ./dialsort_bench --demo\n"
                << "  ./dialsort_bench --n 5000000 --u 1024 --runs 5\n\n";
            std::exit(0);
        }
    }
    return cfg;
}

int main(int argc, char* argv[]) {
    BenchConfig cfg = parse_args(argc, argv);

    fs::create_directories("results");
    fs::create_directories("datasets");

    const int threads = static_cast<int>(pu::hw_threads());
    auto algos = get_all_algorithms();

    print_banner(cfg, threads);
    print_complexity_table(algos);

    // Distribuciones: en modo demo solo las 2 más representativas
    const std::vector<Distribution> dists_full = {
        Distribution::UNIFORM,
        Distribution::SKEWED,
        Distribution::SORTED,
        Distribution::REVERSE,
        Distribution::NEARLY_SORTED,
    };
    const std::vector<Distribution> dists_demo = {
        Distribution::UNIFORM,
        Distribution::SKEWED,
    };

    const auto& dists = cfg.demo_mode ? dists_demo : dists_full;

    std::vector<BenchCase> all_results;
    const size_t total_cases = algos.size()
                             * cfg.sizes.size()
                             * cfg.universes.size()
                             * dists.size();
    size_t done = 0;

    if (cfg.demo_mode) {
        std::cout << "\n  [DEMO MODE] N=1M · U=1024 · 2 distribuciones · ~30 segundos\n\n";
    }

    for (Distribution dist : dists) {
        const std::string dname = dist_name(dist);
        print_section("Distribution: " + dname);

        for (size_t N : cfg.sizes) {
            for (int U : cfg.universes) {

                std::cout << "\n  [Generating N=" << N
                          << "  U=" << U << "  " << dname << "]...\n";
                KeyVec base = generate(N, dist, U);

                // Guardar dataset en CSV solo si N <= 100K
                if (N <= 100'000) {
                    const std::string dpath = "datasets/"
                        + dname + "_N" + std::to_string(N)
                        + "_U" + std::to_string(U) + ".csv";
                    write_dataset(base, dpath);
                }

                for (const auto& algo : algos) {
                    std::cout << "    "
                              << std::left << std::setw(30) << algo.name
                              << "... " << std::flush;

                    BenchCase bc = run_case(algo, base, dist, N, U, cfg);
                    all_results.push_back(bc);
                    ++done;

                    if (bc.stats.skipped) {
                        std::cout << "SKIPPED\n";
                    } else {
                        std::cout << std::fixed << std::setprecision(1)
                                  << bc.stats.median_ms << " ms   "
                                  << bc.stats.throughput << " M/s   "
                                  << "[" << done << "/" << total_cases << "]\n";
                    }
                    print_case_row(bc);
                }
            }
        }
        print_section_end();
    }

    print_winner_summary(all_results);
    export_csv(all_results, cfg.csv_out);

    if (!cfg.html_out.empty())
        export_html(all_results, algos, cfg, cfg.html_out);

    std::cout << "\n[OK] Benchmark completado.\n"
              << "     CSV  -> " << cfg.csv_out  << "\n";
    if (!cfg.html_out.empty())
        std::cout << "     HTML -> " << cfg.html_out << "\n";
    std::cout << "\n";

    return 0;
}
