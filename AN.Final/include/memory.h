#pragma once
// include/memory.h
// Medición precisa de memoria para benchmarks.
//
// Estrategia dual:
//   1. RSS (Resident Set Size) del proceso — memoria física real
//   2. Conteo manual de allocaciones del algorithmo — memoria auxiliar exacta
//
// La medición #2 es más precisa para comparar algorithmos porque excluye
// el ruido del runtime y otros sistemas del proceso.

#include <atomic>
#include <cstddef>

// ── RSS del proceso (Linux/macOS) ────────────────────────────────────────────
size_t get_rss_bytes();
size_t get_peak_rss_bytes();

// ── Tracker de allocaciones por hilo ────────────────────────────────────────
// Permite medir exactamente cuánta memoria auxiliar usa un algorithmo
// independiente de la que usa el resto del proceso.
struct AllocationTracker {
    std::atomic<size_t> current{0};
    std::atomic<size_t> peak{0};

    void record_alloc(size_t bytes);
    void record_free (size_t bytes);
    void reset();
    size_t peak_bytes() const { return peak.load(); }
};

// ── Probe combinado: usa ambas estrategias ──────────────────────────────────
struct MemoryProbe {
    size_t rss_before  = 0;
    size_t rss_peak    = 0;
    size_t aux_estimate = 0;  // estimación heurística por algorithmo + N + U

    MemoryProbe();
    void   sample_now();
    size_t delta_kb() const;
};

// Heurísticas de memoria auxiliar por algorithmo (conocida del análisis Big O)
size_t estimate_aux_memory(const char* algo_short_name,
                            size_t n, int U, int threads);
