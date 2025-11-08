# Testing module
# Phase 3+: Test suite (Phase 3 has simple tests, Phase 46 will add Google Test)
# Priority D1: MPI tests (v6.0.0-alpha5)

add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(benchmarks)

# MPI tests
if(ENABLE_MPI AND MPI_FOUND)
    add_subdirectory(parallel)
endif()

message(STATUS "Tests module configured (Phase 3+, D1 tests available)")
