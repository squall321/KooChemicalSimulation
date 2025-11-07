# Example applications
# Phase 48: Complete example gallery (v5.0.0)

# Reaction example
add_executable(reaction_example reaction_example.cpp)
target_link_libraries(reaction_example PRIVATE koo_core koo_utils koo_chemistry koo_io)
set_target_properties(reaction_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Diffusion example
add_executable(diffusion_example diffusion_example.cpp)
target_link_libraries(diffusion_example PRIVATE koo_core koo_utils koo_physics koo_io)
set_target_properties(diffusion_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Surface catalysis example
add_executable(surface_example surface_example.cpp)
target_link_libraries(surface_example PRIVATE koo_core koo_utils koo_physics koo_io)
set_target_properties(surface_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Full simulation example (Phase 70)
add_executable(full_simulation_example full_simulation_example.cpp)
target_link_libraries(full_simulation_example PRIVATE
    koo_core
    koo_utils
    koo_simulation
)

# Link GPU if available
if(TARGET koo_gpu)
    target_link_libraries(full_simulation_example PRIVATE koo_gpu)
endif()

set_target_properties(full_simulation_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Examples configured (Phase 48, 70 complete - v6.0.0-alpha4)")
