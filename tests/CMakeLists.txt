# Testing module
# Phase 3+: Test suite (Phase 3 has simple tests, Phase 46 will add Google Test)

add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(benchmarks)

message(STATUS "Tests module configured (Phase 3+ tests available)")
