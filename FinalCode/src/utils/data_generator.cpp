// src/utils/data_generator.cpp
#include "data_generator.h"

#include <climits>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

KeyVec generate(size_t N, Distribution dist, int U, unsigned seed) {
    std::mt19937 rng(seed);
    KeyVec v(N);

    bool full = (U <= 0);
    int  hi   = full ? INT_MAX : U - 1;

    switch (dist) {
        case Distribution::UNIFORM: {
            std::uniform_int_distribution<int> d(0, hi);
            for (auto& x : v) x = d(rng);
            break;
        }
        case Distribution::SKEWED: {
            int top = std::max(1, static_cast<int>(static_cast<double>(hi + 1) * 0.05));
            std::uniform_int_distribution<int> hot(0, top - 1);
            std::uniform_int_distribution<int> cold(0, hi);
            std::bernoulli_distribution        is_hot(0.80);
            for (auto& x : v) x = is_hot(rng) ? hot(rng) : cold(rng);
            break;
        }
        case Distribution::SORTED:
            for (size_t i = 0; i < N; ++i)
                v[i] = static_cast<int>(i % static_cast<size_t>(hi + 1));
            break;
        case Distribution::REVERSE:
            for (size_t i = 0; i < N; ++i)
                v[i] = static_cast<int>((N - 1 - i) % static_cast<size_t>(hi + 1));
            break;
        case Distribution::NEARLY_SORTED:
            for (size_t i = 0; i < N; ++i)
                v[i] = static_cast<int>(i % static_cast<size_t>(hi + 1));
            {
                std::uniform_int_distribution<size_t> d(0, N - 1);
                for (size_t i = 0; i < N / 100; ++i)
                    std::swap(v[d(rng)], v[d(rng)]);
            }
            break;
    }
    return v;
}

// ── Sanitize filename: replace any character that's invalid on POSIX/Windows ──
static std::string sanitize_filename(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        // Replace path separators and invalid chars
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?'
         || c == '"' || c == '<' || c == '>' || c == '|') {
            out += '_';
        } else {
            out += c;
        }
    }
    return out;
}

void write_dataset(const KeyVec& v, const std::string& path) {
    fs::path p(path);
    // Sanitize filename portion (keep the parent directory intact)
    fs::path sanitized = p.parent_path() / sanitize_filename(p.filename().string());

    // Auto-create parent directory if it doesn't exist
    if (sanitized.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(sanitized.parent_path(), ec);
        // Don't throw on EEXIST — just continue
    }

    std::ofstream f(sanitized);
    if (!f) throw std::runtime_error("Cannot write: " + sanitized.string());
    for (Key k : v) f << k << '\n';
}

KeyVec read_dataset(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot read: " + path);
    KeyVec v;
    Key k;
    while (f >> k) v.push_back(k);
    return v;
}
