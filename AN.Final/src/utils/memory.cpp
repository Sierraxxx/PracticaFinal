// src/utils/memory.cpp
#include "memory.h"
#include <cstdio>
#include <fstream>
#include <string>

#if defined(__linux__)
size_t get_rss_bytes() {
    std::ifstream f("/proc/self/status");
    std::string   line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            size_t kb = 0;
            std::sscanf(line.c_str(), "VmRSS: %zu kB", &kb);
            return kb * 1024u;
        }
    }
    return 0;
}
size_t get_peak_rss_bytes() {
    std::ifstream f("/proc/self/status");
    std::string   line;
    while (std::getline(f, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            size_t kb = 0;
            std::sscanf(line.c_str(), "VmHWM: %zu kB", &kb);
            return kb * 1024u;
        }
    }
    return 0;
}

#elif defined(__APPLE__)
#include <mach/mach.h>
size_t get_rss_bytes() {
    mach_task_basic_info   info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<size_t>(info.resident_size);
    return 0;
}
size_t get_peak_rss_bytes() {
    mach_task_basic_info   info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<size_t>(info.resident_size_max);
    return 0;
}
#else
size_t get_rss_bytes()      { return 0; }
size_t get_peak_rss_bytes() { return 0; }
#endif

MemoryProbe::MemoryProbe()
    : rss_before(get_rss_bytes()), rss_peak(rss_before) {}

void MemoryProbe::sample_now() {
    const size_t now = get_rss_bytes();
    if (now > rss_peak) rss_peak = now;
}

size_t MemoryProbe::delta_kb() const {
    if (aux_estimate > 0) return aux_estimate / 1024u;
    const size_t delta = (rss_peak > rss_before) ? (rss_peak - rss_before) : 0u;
    return delta / 1024u;
}

size_t estimate_aux_memory(const char* algo_short_name,
                            size_t n, int U, int threads) {
    using SV = std::string_view;
    const SV a(algo_short_name);
    const size_t p = static_cast<size_t>(threads > 0 ? threads : 1);

    if (a == "DialSort") {
        const size_t U_eff = (U > 0) ? static_cast<size_t>(U) : 256u;
        if (U_eff <= 10'000'000u)
            return (p + 1u) * U_eff * sizeof(int);
        return n * sizeof(int) + 256u * sizeof(int);
    }
    if (a == "P-Radix")
        return 2u * n * sizeof(unsigned) + 256u * sizeof(int);
    if (a == "P-Merge")
        return n * sizeof(int) / 2u;
    if (a == "P-Sample")
        return n * sizeof(int) + p * p * 64u * sizeof(int);
    if (a == "P-Intro")
        return p * 64u * sizeof(int);
    return 0u;
}
