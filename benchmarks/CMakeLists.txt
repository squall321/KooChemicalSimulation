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

message(STATUS "Benchmarks configured successfully")
