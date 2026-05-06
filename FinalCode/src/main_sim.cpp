// ════════════════════════════════════════════════════════════════════════════
//  src/main_sim.cpp
//  Entry point for the interactive ASCII visualizer.
//
//  Modes:
//    ./dialsort_sim                    — interactive menu
//    ./dialsort_sim --demo             — run all 5 algorithms once
//    ./dialsort_sim --demo --n 30      — same with custom N
// ════════════════════════════════════════════════════════════════════════════

#include "simulator.h"
#include "data_generator.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        size_t N = 30;
        int    U = 15;
        for (int i = 2; i < argc; ++i) {
            const std::string a = argv[i];
            if (a == "--n" && i + 1 < argc) N = (size_t)std::stoll(argv[++i]);
            if (a == "--u" && i + 1 < argc) U = std::stoi(argv[++i]);
        }
        const KeyVec data = generate(N, Distribution::UNIFORM, U);
        simulate_dialsort(data, false);
        simulate_radix(data,    false);
        simulate_merge(data,    false);
        simulate_sample(data,   false);
        simulate_intro(data,    false);
        return 0;
    }

    run_simulator_menu();
    return 0;
}
