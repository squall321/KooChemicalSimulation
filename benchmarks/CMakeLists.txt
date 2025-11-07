# Benchmarks CMakeLists.txt
# Phase 70: Production Deployment
# Version 6.0.0-alpha4

cmake_minimum_required(VERSION 3.18)

message(STATUS "Configuring benchmarks...")

# ============================================
# Benchmark Suite
# ============================================

add_executable(benchmark_suite benchmark_suite.cpp)

target_link_libraries(benchmark_suite PRIVATE
    koo_core
    koo_utils
    koo_simulation
)

# Link GPU if available
if(TARGET koo_gpu)
    target_link_libraries(benchmark_suite PRIVATE koo_gpu)
    target_compile_definitions(benchmark_suite PRIVATE KOO_USE_CUDA)
    message(STATUS "  Benchmark suite: GPU benchmarks enabled")
else()
    message(STATUS "  Benchmark suite: CPU benchmarks only")
endif()

# C++17 required
target_compile_features(benchmark_suite PRIVATE cxx_std_17)

# ============================================
# Installation
# ============================================

install(TARGETS benchmark_suite
    RUNTIME DESTINATION bin
)

message(STATUS "Benchmarks configured successfully")
