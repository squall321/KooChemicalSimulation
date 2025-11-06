# Phase 21: Diffusion Equation Model (v0.5.0-alpha1)

set(DIFFUSION_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/diffusion/DiffusionCoefficient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/diffusion/FickDiffusion.h
)

# Header-only library - no source files needed

message(STATUS "  - physics/diffusion (Phase 21 - Diffusion models complete)")
