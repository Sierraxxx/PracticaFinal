// src/main_gen.cpp
// Genera todos los datasets requeridos por el paper:
// N ∈ {100K, 500K, 1M, 5M, 10M} × U ∈ {256, 1024} × 5 distribuciones
//
// Uso:
//   ./gen_datasets                  (genera todos)
//   ./gen_datasets --n 1000000      (solo N=1M)

#include "data_generator.h"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::vector<size_t> sizes = {100'000, 500'000, 1'000'000,
                                  5'000'000, 10'000'000};
    std::vector<int>    universes = {256, 1024};

    // Filtrar por --n si se provee
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--n" && i+1 < argc) {
            sizes = {(size_t)std::stoll(argv[++i])};
        }
    }

    const std::vector<Distribution> dists = {
        Distribution::UNIFORM,
        Distribution::SKEWED,
        Distribution::SORTED,
        Distribution::REVERSE,
        Distribution::NEARLY_SORTED,
    };

    fs::create_directories("datasets");

    size_t total = sizes.size() * universes.size() * dists.size();
    size_t done  = 0;

    std::cout << "\n[>] Generating " << total << " datasets...\n\n";

    for (size_t N : sizes) {
        for (int U : universes) {
            for (Distribution d : dists) {
                std::string name = std::string(dist_name(d))
                    + "_N" + std::to_string(N)
                    + "_U" + std::to_string(U)
                    + ".csv";
                std::string path = "datasets/" + name;

                std::cout << "  [" << std::setw(3) << ++done << "/" << total << "] "
                          << std::left << std::setw(50) << name << "... " << std::flush;

                KeyVec data = generate(N, d, U);
                write_dataset(data, path);

                std::cout << N << " keys written\n";
            }
        }
    }

    std::cout << "\n[OK] All datasets written to datasets/\n\n";
    return 0;
}
