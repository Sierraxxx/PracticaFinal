# src/CMakeLists.txt
# Todos los archivos fuente listados explícitamente por target.
# CLion indexa mejor con listas explícitas que con variables intermedias.

# ── Target 1: benchmark principal ─────────────────────────────────────────────
add_executable(dialsort_bench
    main_bench.cpp

    algorithms/dialsort.cpp
    algorithms/parallel_radix_sort.cpp
    algorithms/parallel_merge_sort.cpp
    algorithms/parallel_sample_sort.cpp
    algorithms/parallel_intro_sort.cpp
    algorithms/registry.cpp

    benchmark/runner.cpp
    benchmark/stats.cpp
    benchmark/reporter.cpp
    benchmark/html_exporter.cpp

    utils/data_generator.cpp
    utils/memory.cpp
)

# ── Target 2: simulador visual ────────────────────────────────────────────────
add_executable(dialsort_sim
    main_sim.cpp

    algorithms/dialsort.cpp
    algorithms/parallel_radix_sort.cpp
    algorithms/parallel_merge_sort.cpp
    algorithms/parallel_sample_sort.cpp
    algorithms/parallel_intro_sort.cpp
    algorithms/registry.cpp

    simulator/visualizer.cpp

    utils/data_generator.cpp
    utils/memory.cpp
)

# ── Target 3: generador de datasets ───────────────────────────────────────────
add_executable(gen_datasets
    main_gen.cpp
    utils/data_generator.cpp
    utils/memory.cpp
)

# ── Configuración común ────────────────────────────────────────────────────────
foreach(target dialsort_bench dialsort_sim gen_datasets)
    target_include_directories(${target}
        PRIVATE ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(${target}
        PRIVATE Threads::Threads
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
            $<$<CONFIG:Release>:-O3 -funroll-loops>
            $<$<CONFIG:Debug>:-O0 -g>
        )
    endif()
endforeach()
