# Parallel computing module
# Phase 41-45: MPI and HPC support (v4.0.0-alpha1)

# Header files
set(PARALLEL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/mpi/MPIWrapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/domain/DomainDecomposition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/linalg/ParallelVector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/io/ParallelIO.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/parallel/balance/LoadBalancing.h
)

# Header-only library
add_library(koo_parallel INTERFACE)
target_include_directories(koo_parallel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(koo_parallel INTERFACE koo_core koo_utils)

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

message(STATUS "Parallel module configured (Phase 41-45 complete - v4.0.0-alpha1)")
