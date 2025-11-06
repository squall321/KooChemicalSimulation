# Unit tests
# Phase 3+: Unit tests (Google Test in Phase 46, simple tests for now)

# Phase 3 test executable
add_executable(test_phase3 test_phase3.cpp)
target_link_libraries(test_phase3 PRIVATE koo_core koo_utils)

# Set properties
set_target_properties(test_phase3 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Phase 4 test executable
add_executable(test_phase4 test_phase4.cpp)
target_link_libraries(test_phase4 PRIVATE koo_core koo_utils)

# Set properties
set_target_properties(test_phase4 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Phase 5 test executable
add_executable(test_phase5 test_phase5.cpp)
target_link_libraries(test_phase5 PRIVATE koo_core koo_utils koo_config)

# Set properties
set_target_properties(test_phase5 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "  - tests/unit (Phase 3-5 tests added)")
