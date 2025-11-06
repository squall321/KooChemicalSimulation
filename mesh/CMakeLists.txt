# Mesh management module
# Phase 6-10: gmsh integration and mesh handling

# Header-only library for mesh system
add_library(koo_mesh INTERFACE)

# Add include directories
target_include_directories(koo_mesh INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_mesh INTERFACE cxx_std_17)

# Link to core
target_link_libraries(koo_mesh INTERFACE koo_core)

# Check for gmsh library (optional)
find_package(gmsh QUIET)
if(gmsh_FOUND)
    message(STATUS "Found gmsh - API support enabled")
    target_link_libraries(koo_mesh INTERFACE gmsh)
    target_compile_definitions(koo_mesh INTERFACE USE_GMSH)
else()
    message(STATUS "gmsh not found - using manual .msh parser")
endif()

# Header files
set(MESH_CORE_HEADERS
    include/mesh/core/Node.h
    include/mesh/core/Element.h
    include/mesh/core/MeshData.h
)

set(MESH_LOADER_HEADERS
    include/mesh/loader/GmshLoader.h
)

set(MESH_MANAGER_HEADERS
    include/mesh/manager/MeshQuality.h
    include/mesh/manager/MeshOptimizer.h
    include/mesh/manager/MeshConverter.h
)

# Group headers for IDE
source_group("Mesh Core Headers" FILES ${MESH_CORE_HEADERS})
source_group("Mesh Loader Headers" FILES ${MESH_LOADER_HEADERS})
source_group("Mesh Manager Headers" FILES ${MESH_MANAGER_HEADERS})

# Subdirectories (placeholders for later phases)
add_subdirectory(core)
add_subdirectory(loader)
add_subdirectory(manager)
add_subdirectory(boundary)

# Add alias for consistent naming
add_library(Koo::Mesh ALIAS koo_mesh)

message(STATUS "Mesh module configured (Phase 6-7 - gmsh integration and optimization complete)")
