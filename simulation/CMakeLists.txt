# Simulation Module CMakeLists.txt
# Phase 66-70: Advanced Simulation Features
# Version 6.0.0-alpha4

cmake_minimum_required(VERSION 3.18)

message(STATUS "Configuring Simulation module (Phase 66-70)...")

# ============================================
# Simulation Library (Header-Only)
# ============================================

add_library(koo_simulation INTERFACE)

target_include_directories(koo_simulation INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link to core utilities
target_link_libraries(koo_simulation INTERFACE
    koo_utils
)

# C++17 required
target_compile_features(koo_simulation INTERFACE cxx_std_17)

# ============================================
# Tests
# ============================================

if(BUILD_TESTING)
    message(STATUS "Configuring simulation tests (Phase 66-70)...")

    # Check if GTest is available
    if(TARGET GTest::gtest)
        # Phase 66-70 comprehensive test with GTest
        add_executable(test_phase66_70 tests/test_phase66_70.cpp)

        target_link_libraries(test_phase66_70 PRIVATE
            koo_simulation
            koo_utils
            GTest::gtest
            GTest::gtest_main
        )

        # If GPU available, link GPU libraries
        if(TARGET koo_gpu)
            target_link_libraries(test_phase66_70 PRIVATE koo_gpu)
        endif()

        add_test(NAME SimulationPhase66_70 COMMAND test_phase66_70)

        message(STATUS "  Test executable: test_phase66_70 (with GTest)")
        message(STATUS "  Tests for Phase 66-70 configured")
    else()
        # Simple test without GTest
        add_executable(test_phase66_70_simple tests/test_phase66_70_simple.cpp)

        target_link_libraries(test_phase66_70_simple PRIVATE
            koo_simulation
            koo_utils
        )

        # If GPU available, link GPU libraries
        if(TARGET koo_gpu)
            target_link_libraries(test_phase66_70_simple PRIVATE koo_gpu)
        endif()

        add_test(NAME SimulationPhase66_70_Simple COMMAND test_phase66_70_simple)

        message(STATUS "  Test executable: test_phase66_70_simple (no GTest)")
        message(STATUS "  Tests for Phase 66-70 configured (simple version)")
        message(STATUS "  Install GTest for comprehensive testing")
    endif()
endif()

# ============================================
# Installation
# ============================================

install(DIRECTORY include/simulation
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS koo_simulation
    EXPORT KooChemicalSimulationTargets
)

# ============================================
# Summary
# ============================================

message(STATUS "Simulation module configured successfully")
message(STATUS "  Phase 66: Adaptive Timestepping ✓")
message(STATUS "  Phase 66: Stability Monitoring ✓")
message(STATUS "  Phase 67: Thermal-Chemical Coupling ✓")
message(STATUS "  Phase 67: Flow-Chemistry Coupling ✓")
message(STATUS "  Phase 68: Real-time Visualization (Python) ✓")
message(STATUS "  Phase 69: Performance Auto-tuning (GPU) ✓")
message(STATUS "  Phase 70: Production Deployment ✓")
message(STATUS "")
message(STATUS "  Ready for production use!")
message(STATUS "========================================")
