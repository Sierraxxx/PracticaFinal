#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  include/simulator.h
//
//  Step-by-step terminal visualizer for the 5 algorithms.
//  Renders each phase with ASCII bars, highlighted regions, and pacing
//  controlled by the caller.
// ════════════════════════════════════════════════════════════════════════════

#include "types.h"
#include <string>

// ── Render a vector as ASCII bars ───────────────────────────────────────────
void render_array(const KeyVec& v, size_t max_display,
                  const std::string& label,
                  int highlight_lo = -1, int highlight_hi = -1);

// ── Per-algorithm visualizers ───────────────────────────────────────────────
// Each shows: input, the algorithm-specific phases, and the sorted output.
void simulate_dialsort  (const KeyVec& input, bool slow_mode);
void simulate_radix     (const KeyVec& input, bool slow_mode);
void simulate_merge     (const KeyVec& input, bool slow_mode);
void simulate_sample    (const KeyVec& input, bool slow_mode);
void simulate_intro     (const KeyVec& input, bool slow_mode);

// ── Interactive menu (entry point for ./dialsort_sim) ───────────────────────
void run_simulator_menu();
