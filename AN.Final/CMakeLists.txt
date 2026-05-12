cmake_minimum_required(VERSION 3.20)

project(sorting_benchmark
    VERSION     0.7.0
    DESCRIPTION "Comparative benchmark of integer sorting algorithms"
    LANGUAGES   CXX
)

# ── C++17 estricto ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -march=native")

# ── Única dependencia: pthreads ───────────────────────────────────────────────
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ── Registrar headers explícitamente para que CLion los indexe ────────────────
# Sin esto, la carpeta include/ es invisible en el árbol del proyecto.
set(PROJECT_HEADERS
    include/types.h
    include/algorithms.h
    include/dialsort.h
    include/benchmark.h
    include/simulator.h
    include/data_generator.h
    include/memory.h
    include/parallel_utils.h
)

# ── Registrar archivos extra (dashboard, docs) para que aparezcan en CLion ───
# CMake no los compila, pero CLion los muestra en el árbol al estar listados.
set(EXTRA_FILES
    dashboard/index.html
    dashboard/README.md
    README.md
    AUTHORS.private
    .gitignore
)

# Target ficticio que agrupa headers y archivos extra — solo para visibilidad
# en CLion. No genera ningún binario.
add_custom_target(project_files
    SOURCES
        ${PROJECT_HEADERS}
        ${EXTRA_FILES}
)

# ── Subdirectorio con los targets reales ──────────────────────────────────────
add_subdirectory(src)

message(STATUS "")
message(STATUS "=== Sorting Benchmark v0.7.0 ===")
message(STATUS "  Build  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  CXX    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Deps   : pthreads only (no OpenMP, no external libs)")
message(STATUS "")
message(STATUS "  Targets:")
message(STATUS "    dialsort_bench  — full benchmark + HTML report")
message(STATUS "    dialsort_sim    — interactive ASCII visualizer")
message(STATUS "    gen_datasets    — dataset generator")
message(STATUS "")
