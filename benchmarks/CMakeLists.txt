# Benchmarks CMakeLists.txt
# Phase 70: Production Deployment
# Version 6.0.0-alpha4

cmake_minimum_required(VERSION 3.18)

message(STATUS "Configuring benchmarks...")

# ============================================
# Benchmark Suite
# ============================================

# Note: benchmark_suite requires GPU support
# It uses CUDA/HIP headers that are not available in CPU-only mode
# GPU_BACKEND is set in gpu/CMakeLists.txt
if(GPU_BACKEND STREQUAL "CUDA" OR GPU_BACKEND STREQUAL "HIP")
    add_executable(benchmark_suite benchmark_suite.cpp)

    target_link_libraries(benchmark_suite PRIVATE
        koo_core
        koo_utils
        koo_simulation
        koo_gpu
    )

    target_compile_definitions(benchmark_suite PRIVATE KOO_USE_CUDA)

    # C++17 required
    target_compile_features(benchmark_suite PRIVATE cxx_std_17)

    # Installation
    install(TARGETS benchmark_suite
        RUNTIME DESTINATION bin
    )

    message(STATUS "  Benchmark suite: GPU benchmarks enabled")
else()
    message(STATUS "  Benchmark suite: Skipped (requires GPU support)")
    message(STATUS "  Enable GPU with: cmake -DENABLE_GPU=ON ..")
endif()

# ============================================
# CPU Benchmark Suite (Phase 70)
# ============================================
add_executable(cpu_benchmark_suite cpu_benchmark_suite.cpp)

target_link_libraries(cpu_benchmark_suite PRIVATE
    koo_core
    koo_mesh
    koo_chemistry
    koo_physics
    koo_utils
)

target_compile_features(cpu_benchmark_suite PRIVATE cxx_std_17)

# Installation
install(TARGETS cpu_benchmark_suite
    RUNTIME DESTINATION bin
)

message(STATUS "  CPU Benchmark suite: Enabled")

# ============================================
# MPI Domain Decomposition Benchmark
# ============================================
add_executable(mpi_domain_decomposition_benchmark mpi_domain_decomposition_benchmark.cpp)

target_link_libraries(mpi_domain_decomposition_benchmark PRIVATE
    koo_core
)

target_compile_features(mpi_domain_decomposition_benchmark PRIVATE cxx_std_17)

# Installation
install(TARGETS mpi_domain_decomposition_benchmark
    RUNTIME DESTINATION bin
)

message(STATUS "  MPI Domain Decomposition Benchmark: Enabled")

message(STATUS "Benchmarks configured successfully")
