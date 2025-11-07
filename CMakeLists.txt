cmake_minimum_required(VERSION 3.20)

# Project definition
project(KooChemicalSimulation
    VERSION 0.1.0
    DESCRIPTION "Open-source chemical simulation solution using FEM solvers"
    LANGUAGES CXX
)

# ============================================================================
# Project Configuration
# ============================================================================

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ============================================================================
# Build Options
# ============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_DOCUMENTATION "Build documentation" OFF)
option(ENABLE_MPI "Enable MPI support for parallel computing" OFF)
option(ENABLE_OPENMP "Enable OpenMP support" ON)
option(ENABLE_NGSOLVE "Enable NGSolve FEM solver" OFF)
option(ENABLE_MFEM "Enable MFEM solver" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

# ============================================================================
# Compiler Flags
# ============================================================================

# Warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Woverloaded-virtual
        -Wunused
    )
endif()

# Optimization flags for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O3 -march=native)
    elseif(MSVC)
        add_compile_options(/O2)
    endif()
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g -O0)
    elseif(MSVC)
        add_compile_options(/Od /Zi)
    endif()
endif()

# ============================================================================
# Dependencies
# ============================================================================

# Use vcpkg if available
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Find packages (all optional for Phase 1)
# These will become REQUIRED in later phases when code is added
find_package(Eigen3 3.4 QUIET NO_MODULE)
if(Eigen3_FOUND)
    message(STATUS "Found Eigen3 ${Eigen3_VERSION}")
else()
    message(STATUS "Eigen3 not found (will be required in Phase 2+)")
endif()

find_package(fmt QUIET)
if(fmt_FOUND)
    message(STATUS "Found fmt ${fmt_VERSION}")
endif()

find_package(spdlog QUIET)
if(spdlog_FOUND)
    message(STATUS "Found spdlog ${spdlog_VERSION}")
endif()

find_package(nlohmann_json 3.11.0 QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json ${nlohmann_json_VERSION}")
endif()

find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "Found yaml-cpp")
endif()

# Find optional packages
if(BUILD_TESTING)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Found GTest - full testing enabled")
        enable_testing()
        include(GoogleTest)
    else()
        message(STATUS "GTest not found - simple tests available (Phase 3-4)")
        enable_testing()
    endif()
endif()

# VTK for output (optional for Phase 1, required later)
find_package(VTK QUIET COMPONENTS
    CommonCore
    CommonDataModel
    IOCore
    IOXML
    FiltersCore
)
if(VTK_FOUND)
    message(STATUS "Found VTK ${VTK_VERSION}")
else()
    message(WARNING "VTK not found. VTK output will be disabled.")
endif()

# HDF5 for checkpointing (optional for Phase 1)
find_package(HDF5 QUIET COMPONENTS CXX)
if(HDF5_FOUND)
    message(STATUS "Found HDF5 ${HDF5_VERSION}")
else()
    message(WARNING "HDF5 not found. Checkpoint/restart will be disabled.")
endif()

# MPI support
if(ENABLE_MPI)
    find_package(MPI REQUIRED COMPONENTS CXX)
    message(STATUS "MPI enabled")
endif()

# OpenMP support
if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP enabled")
    else()
        message(WARNING "OpenMP not found but ENABLE_OPENMP=ON")
    endif()
endif()

# NGSolve (custom find module needed - will be added in later phases)
if(ENABLE_NGSOLVE)
    # find_package(NGSolve REQUIRED)
    message(WARNING "NGSolve support will be added in Phase 12")
endif()

# MFEM (custom find module needed - will be added in later phases)
if(ENABLE_MFEM)
    # find_package(MFEM REQUIRED)
    message(WARNING "MFEM support will be added in Phase 13")
endif()

# ============================================================================
# Code Coverage
# ============================================================================

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

# ============================================================================
# Sanitizers
# ============================================================================

if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# ============================================================================
# Include Directories
# ============================================================================

# Add project-wide include directory
include_directories(${PROJECT_SOURCE_DIR})

# ============================================================================
# Subdirectories
# ============================================================================

# Core modules
add_subdirectory(core)
add_subdirectory(mesh)
add_subdirectory(solver)
add_subdirectory(chemistry)
add_subdirectory(physics)
add_subdirectory(io)
add_subdirectory(config)
add_subdirectory(parallel)
add_subdirectory(gpu)
add_subdirectory(utils)
add_subdirectory(simulation)

# Applications
add_subdirectory(apps)

# Python bindings (Phase 56)
option(ENABLE_PYTHON "Build Python bindings" ON)
if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()

# Tests (if enabled)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Examples (if enabled)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Benchmarks (Phase 70)
option(BUILD_BENCHMARKS "Build benchmark suite" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Documentation (if enabled)
if(BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif()

# ============================================================================
# Installation
# ============================================================================
# Installation rules will be added in Phase 2+ when libraries are created

# include(GNUInstallDirs)

# # Install headers
# install(DIRECTORY core/ mesh/ solver/ chemistry/ physics/ io/ config/ parallel/ utils/
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/koo
#     FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
# )

# # Export targets (will be enabled when libraries are created)
# install(EXPORT KooChemicalSimulationTargets
#     FILE KooChemicalSimulationTargets.cmake
#     NAMESPACE Koo::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KooChemicalSimulation
# )

# # Generate config file
# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KooChemicalSimulationConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/KooChemicalSimulationConfig.cmake
#     INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KooChemicalSimulation
# )

# write_basic_package_version_file(
#     ${CMAKE_CURRENT_BINARY_DIR}/KooChemicalSimulationConfigVersion.cmake
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY SameMajorVersion
# )

# install(FILES
#     ${CMAKE_CURRENT_BINARY_DIR}/KooChemicalSimulationConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/KooChemicalSimulationConfigVersion.cmake
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KooChemicalSimulation
# )

message(STATUS "Installation rules will be added in Phase 2+")

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  KooChemicalSimulation Configuration")
message(STATUS "========================================")
message(STATUS "Version:           ${PROJECT_VERSION}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "Build testing:     ${BUILD_TESTING}")
message(STATUS "Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "Enable MPI:        ${ENABLE_MPI}")
message(STATUS "Enable OpenMP:     ${ENABLE_OPENMP}")
message(STATUS "Enable NGSolve:    ${ENABLE_NGSOLVE}")
message(STATUS "Enable MFEM:       ${ENABLE_MFEM}")
message(STATUS "========================================")
message(STATUS "")
