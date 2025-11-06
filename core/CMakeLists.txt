# Core module - Fundamental abstractions and interfaces
# Phase 2: Base classes and interfaces

# Header-only library for core interfaces
add_library(koo_core INTERFACE)

# Add include directories
target_include_directories(koo_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_core INTERFACE cxx_std_17)

# Header files
set(CORE_INTERFACE_HEADERS
    include/core/interfaces/ISimulatable.h
    include/core/interfaces/ISolver.h
    include/core/interfaces/IMesh.h
    include/core/interfaces/IChemicalSystem.h
)

set(CORE_BASE_HEADERS
    include/core/base/BaseObject.h
)

set(CORE_MEMORY_HEADERS
    include/core/memory/MemoryManager.h
    include/core/memory/ObjectPool.h
    include/core/memory/SmartPtr.h
)

# Group headers for IDE
source_group("Interface Headers" FILES ${CORE_INTERFACE_HEADERS})
source_group("Base Headers" FILES ${CORE_BASE_HEADERS})
source_group("Memory Headers" FILES ${CORE_MEMORY_HEADERS})

# Subdirectories
add_subdirectory(base)
add_subdirectory(interfaces)
add_subdirectory(types)
add_subdirectory(memory)

# Add alias for consistent naming
add_library(Koo::Core ALIAS koo_core)

# Test executable to verify interfaces compile
add_executable(test_interfaces_compile src/test_interfaces.cpp)
target_link_libraries(test_interfaces_compile PRIVATE koo_core)

# Set properties for the test
set_target_properties(test_interfaces_compile PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Core module configured (Phase 2-4 - interfaces, types, memory complete)")
