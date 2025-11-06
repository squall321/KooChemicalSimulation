# Surface chemistry models
# Phase 24-26: Surface species, adsorption, surface reactions

set(SURFACE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/surface/SurfaceSpecies.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/surface/AdsorptionKinetics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/surface/SurfaceReaction.h
)

# Header-only library - no source files needed

add_subdirectory(corrosion)
add_subdirectory(migration)

message(STATUS "  - physics/surface (Phase 24-26 complete)")
