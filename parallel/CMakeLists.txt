# Parallel computing module
# Phase 41-45: MPI and HPC support (v4.0.0-alpha1)
# Priority D1: MPI Parallelization (v6.0.0-alpha5)
# Priority D2: PINN Integration (v6.0.0-alpha5)

# Header files
set(PARALLEL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/mpi/MPIWrapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/mpi/MPIAsync.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/domain/DomainDecomposition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/linalg/ParallelVector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/io/ParallelIO.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/balance/LoadBalancing.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/solver/MPIDiffusionSolver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/solver/MPIReactionDiffusionSolver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/solver/MPIDiffusionSolverOptimized.h
)

# Source files
set(PARALLEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mpi/MPIAsync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver/MPIDiffusionSolver1D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver/MPIDiffusionSolver2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver/MPIReactionDiffusionSolver1D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver/MPIReactionDiffusionSolver2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/solver/MPIDiffusionSolver1DOptimized.cpp
)

# PINN headers and sources (optional, requires LibTorch)
set(PINN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/ml/PINNSolver.h
)

set(PINN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ml/PINNSolver.cpp
)

# Create library (now with sources)
add_library(koo_parallel ${PARALLEL_SOURCES} ${PARALLEL_HEADERS})
target_include_directories(koo_parallel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(koo_parallel PUBLIC koo_core koo_utils koo_physics)

# Check for MPI
if(ENABLE_MPI)
    find_package(MPI)
    if(MPI_FOUND)
        message(STATUS "MPI enabled")
        target_link_libraries(koo_parallel INTERFACE MPI::MPI_CXX)
        target_compile_definitions(koo_parallel INTERFACE USE_MPI)
    else()
        message(STATUS "MPI not found - parallel features will run in serial mode")
    endif()
else()
    message(STATUS "MPI disabled - parallel features will run in serial mode")
endif()

# Check for LibTorch (optional for PINN support)
if(ENABLE_PINN)
    find_package(Torch QUIET)
    if(Torch_FOUND)
        message(STATUS "LibTorch found - PINN C++ support enabled")

        # Add PINN sources to library
        target_sources(koo_parallel PRIVATE ${PINN_SOURCES} ${PINN_HEADERS})

        # Link LibTorch
        target_link_libraries(koo_parallel PUBLIC "${TORCH_LIBRARIES}")
        target_compile_definitions(koo_parallel PUBLIC USE_LIBTORCH)

        # Set C++17 for LibTorch compatibility
        set_property(TARGET koo_parallel PROPERTY CXX_STANDARD 17)

        message(STATUS "PINN module integrated with LibTorch")
    else()
        message(STATUS "LibTorch not found - PINN C++ support disabled")
        message(STATUS "  To enable: install LibTorch and set CMAKE_PREFIX_PATH")
        message(STATUS "  Download from: https://pytorch.org/get-started/locally/")
    endif()
else()
    message(STATUS "PINN disabled (set ENABLE_PINN=ON to enable)")
endif()

message(STATUS "Parallel module configured (Phase 41-45 complete - v4.0.0-alpha1)")
message(STATUS "PINN integration ready (Priority D2 - v6.0.0-alpha5)")
