#pragma once
// include/types.h
// Tipos fundamentales compartidos por todo el proyecto.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ── Tipo de clave ─────────────────────────────────────────────────────────────
using Key    = int32_t;
using KeyVec = std::vector<Key>;

// ── Reloj de alta resolución ──────────────────────────────────────────────────
using Clock   = std::chrono::high_resolution_clock;
using Seconds = std::chrono::duration<double>;

inline double ms_since(Clock::time_point t0) {
    return std::chrono::duration_cast<Seconds>(Clock::now() - t0).count() * 1000.0;
}

// ── Distribuciones de datos ───────────────────────────────────────────────────
enum class Distribution : uint8_t {
    UNIFORM,        // uniforme aleatorio en [0, U-1]
    SKEWED,         // 80% de claves en el 5% inferior del universo
    SORTED,         // ascendente (mejor caso para comparativos)
    REVERSE,        // descendente (peor caso para comparativos)
    NEARLY_SORTED,  // 1% de intercambios aleatorios
};

inline const char* dist_name(Distribution d) {
    switch (d) {
        case Distribution::UNIFORM:       return "Uniform";
        case Distribution::SKEWED:        return "Skewed-80-5";
        case Distribution::SORTED:        return "Sorted";
        case Distribution::REVERSE:       return "Reverse";
        case Distribution::NEARLY_SORTED: return "Nearly-Sorted";
    }
    return "?";
}

// ── Firma de función de ordenamiento ─────────────────────────────────────────
using SortFn = std::function<void(KeyVec&, int /*U*/)>;

// ── Descriptor de algoritmo ───────────────────────────────────────────────────
struct AlgoInfo {
    std::string name;
    std::string short_name;     // para cabeceras de tabla
    SortFn      fn;
    bool        needs_bounded_U; // true → solo funciona cuando U es pequeño
    // Big O strings
    const char* bigo_best;
    const char* bigo_avg;
    const char* bigo_worst;
    const char* bigo_space;
};

// ── Resultado de una corrida individual ──────────────────────────────────────
struct RunResult {
    double time_ms   = 0.0;
    size_t mem_bytes = 0;
    bool   correct   = false;
};

// ── Estadísticas agregadas ────────────────────────────────────────────────────
struct Stats {
    double mean_ms     = 0.0;
    double median_ms   = 0.0;
    double stddev_ms   = 0.0;
    double min_ms      = 0.0;
    double max_ms      = 0.0;
    double throughput  = 0.0;   // M keys / segundo
    size_t mean_mem_kb = 0;     // KB promedio de memoria auxiliar
    bool   correct     = false;
    bool   skipped     = false;
};

// ── Resultado completo de un caso de benchmark ────────────────────────────────
struct BenchCase {
    std::string  algo;
    std::string  short_name;
    std::string  dist;
    size_t       n      = 0;
    int          U      = 0;
    Stats        stats;
    // Big O para reporte
    std::string  bigo_best, bigo_avg, bigo_worst, bigo_space;
};

// ── Configuración global del benchmark ───────────────────────────────────────
struct BenchConfig {
    std::vector<size_t> sizes     = {100'000, 500'000, 1'000'000};
    std::vector<int>    universes = {256, 1024};
    int   runs          = 3;
    int   warmup        = 1;
    bool  measure_memory = true;
    bool  demo_mode     = false;   // true -> --demo (sustentacion en vivo)
    std::string csv_out  = "results/benchmark_results.csv";
    std::string html_out = "results/report.html";
};
