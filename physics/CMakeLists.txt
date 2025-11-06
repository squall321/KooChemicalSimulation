# Physics module
# Phase 21-30: Diffusion and surface chemistry models

add_subdirectory(diffusion)
add_subdirectory(transport)
add_subdirectory(coupling)
add_subdirectory(surface)

# Create physics INTERFACE library
add_library(koo_physics INTERFACE)
target_include_directories(koo_physics INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(koo_physics INTERFACE koo_core koo_utils)

message(STATUS "Physics module configured (Phase 21-23 complete - v0.5.0-alpha3)")
