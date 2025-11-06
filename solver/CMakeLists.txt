# PDE Solver module
# Phase 11-15: PDE abstraction and solver integration

# Header-only library for solver system
add_library(koo_solver INTERFACE)

# Add include directories
target_include_directories(koo_solver INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_solver INTERFACE cxx_std_17)

# Link to core and mesh
target_link_libraries(koo_solver INTERFACE koo_core koo_mesh)

# Add subdirectories
add_subdirectory(pde)
add_subdirectory(ngsolve)
add_subdirectory(mfem)
add_subdirectory(factory)

# Add alias for consistent naming
add_library(Koo::Solver ALIAS koo_solver)

message(STATUS "Solver module configured (Phase 11-15 - PDE interfaces complete)")
