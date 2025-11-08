# GPU Module CMakeLists.txt
# Phase 51: GPU Abstraction Layer
# Version 6.0.0-alpha1

cmake_minimum_required(VERSION 3.18)

message(STATUS "Configuring GPU module (Phase 51)...")

# ============================================
# GPU Backend Detection
# ============================================

# Option to enable GPU support
option(ENABLE_GPU "Enable GPU acceleration (CUDA or HIP)" OFF)
option(ENABLE_CUDA "Force CUDA backend" OFF)
option(ENABLE_HIP "Force HIP backend" OFF)

set(GPU_BACKEND "None")

if(ENABLE_GPU OR ENABLE_CUDA OR ENABLE_HIP)
    # Try to find CUDA first
    if(ENABLE_CUDA OR (NOT ENABLE_HIP))
        find_package(CUDAToolkit QUIET)
        if(CUDAToolkit_FOUND)
            set(GPU_BACKEND "CUDA")
            message(STATUS "  GPU Backend: CUDA")
            message(STATUS "  CUDA Version: ${CUDAToolkit_VERSION}")
            message(STATUS "  CUDA Compiler: ${CMAKE_CUDA_COMPILER}")
        endif()
    endif()

    # Try HIP if CUDA not found
    if(ENABLE_HIP OR (NOT CUDAToolkit_FOUND AND ENABLE_GPU))
        find_package(HIP QUIET)
        if(HIP_FOUND)
            set(GPU_BACKEND "HIP")
            message(STATUS "  GPU Backend: HIP/ROCm")
            message(STATUS "  HIP Version: ${HIP_VERSION}")
        endif()
    endif()

    if(GPU_BACKEND STREQUAL "None")
        message(STATUS "  GPU Backend: None (GPU requested but not found)")
        message(STATUS "  Falling back to CPU-only mode")
    endif()
else()
    message(STATUS "  GPU Backend: Disabled (CPU-only mode)")
endif()

# ============================================
# GPU Library (Header-Only)
# ============================================

add_library(koo_gpu INTERFACE)

target_include_directories(koo_gpu INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link to core utilities
target_link_libraries(koo_gpu INTERFACE
    koo_utils
)

# ============================================
# CUDA Configuration
# ============================================

if(GPU_BACKEND STREQUAL "CUDA")
    # Enable CUDA language
    enable_language(CUDA)

    # Set CUDA standard
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    # CUDA architectures (customize as needed)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        # Default: Support Pascal (6.0) through Ada/Hopper (8.9, 9.0)
        set(CMAKE_CUDA_ARCHITECTURES 60 70 75 80 86 89 90)
        message(STATUS "  CUDA Architectures: ${CMAKE_CUDA_ARCHITECTURES}")
    endif()

    # Link CUDA libraries
    target_link_libraries(koo_gpu INTERFACE
        CUDA::cudart
        CUDA::cublas
        CUDA::cusparse
    )

    # Compile definitions
    target_compile_definitions(koo_gpu INTERFACE
        KOO_CUDA_ENABLED
    )

    message(STATUS "  CUDA libraries: cudart, cublas, cusparse")
endif()

# ============================================
# HIP Configuration
# ============================================

if(GPU_BACKEND STREQUAL "HIP")
    # Enable HIP language
    enable_language(HIP)

    # Link HIP libraries
    target_link_libraries(koo_gpu INTERFACE
        hip::host
        roc::rocblas
        roc::rocsparse
    )

    # Compile definitions
    target_compile_definitions(koo_gpu INTERFACE
        KOO_HIP_ENABLED
        __HIP_PLATFORM_AMD__
    )

    message(STATUS "  HIP libraries: hip::host, rocblas, rocsparse")
endif()

# ============================================
# CPU Fallback
# ============================================

if(GPU_BACKEND STREQUAL "None")
    target_compile_definitions(koo_gpu INTERFACE
        KOO_CPU_FALLBACK
    )
    message(STATUS "  Using CPU fallback implementation")
endif()

# ============================================
# Installation
# ============================================

install(DIRECTORY include/gpu
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS koo_gpu
    EXPORT KooChemicalSimulationTargets
)

# ============================================
# Summary
# ============================================

message(STATUS "GPU module configured successfully")
message(STATUS "  Backend: ${GPU_BACKEND}")
message(STATUS "  Phase 51: GPU Abstraction Layer complete")

if(GPU_BACKEND STREQUAL "CUDA")
    message(STATUS "  Device Management: ✓")
    message(STATUS "  Memory Management: ✓")
    message(STATUS "  Stream Management: ✓")
    message(STATUS "  Kernel Utilities: ✓")
    message(STATUS "")
    message(STATUS "  Ready for Phase 52: GPU Linear Algebra")
elseif(GPU_BACKEND STREQUAL "HIP")
    message(STATUS "  Device Management: ✓ (HIP)")
    message(STATUS "  Memory Management: ✓ (HIP)")
    message(STATUS "  Stream Management: ✓ (HIP)")
    message(STATUS "  Kernel Utilities: ✓ (HIP)")
    message(STATUS "")
    message(STATUS "  Ready for Phase 52: GPU Linear Algebra")
else()
    message(STATUS "  Running in CPU-only mode")
    message(STATUS "  To enable GPU: cmake -DENABLE_GPU=ON ..")
endif()

message(STATUS "========================================")

# ============================================
# Tests (Phase 61-65)
# ============================================

if(BUILD_TESTING AND GPU_BACKEND STREQUAL "CUDA")
    message(STATUS "Configuring GPU tests (Phase 61-65)...")

    # Phase 61-65 comprehensive test
    add_executable(test_phase61_65 tests/test_phase61_65.cpp)
    target_link_libraries(test_phase61_65 PRIVATE
        koo_gpu
        GTest::gtest
        GTest::gtest_main
    )

    # Add NVTX support if available
    find_library(NVTX_LIBRARY nvToolsExt PATHS ${CUDAToolkit_LIBRARY_DIR})
    if(NVTX_LIBRARY)
        target_link_libraries(test_phase61_65 PRIVATE ${NVTX_LIBRARY})
        message(STATUS "  NVTX library found: ${NVTX_LIBRARY}")
    else()
        message(STATUS "  NVTX library not found (markers will be no-ops)")
    endif()

    # Set CUDA properties
    set_target_properties(test_phase61_65 PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )

    add_test(NAME GPUPhase61_65 COMMAND test_phase61_65)

    message(STATUS "  Test executable: test_phase61_65")
    message(STATUS "  Tests for Phase 61-65 configured")
endif()
