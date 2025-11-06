# Chemistry module
# Phase 16-20: Chemical species, reactions, and kinetics

# Header-only library for chemistry system
add_library(koo_chemistry INTERFACE)

# Add include directories
target_include_directories(koo_chemistry INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_chemistry INTERFACE cxx_std_17)

# Link to core
target_link_libraries(koo_chemistry INTERFACE koo_core)

add_subdirectory(species)
add_subdirectory(parser)
add_subdirectory(kinetics)
add_subdirectory(reaction)
add_subdirectory(coupling)

# Add alias for consistent naming
add_library(Koo::Chemistry ALIAS koo_chemistry)

message(STATUS "Chemistry module configured (Phase 16-20 complete - v0.4.0-beta)")
