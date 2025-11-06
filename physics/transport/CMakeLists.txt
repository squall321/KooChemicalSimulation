# Phase 22: Transport Phenomena (v0.5.0-alpha2)

set(TRANSPORT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/transport/VelocityField.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/transport/AdvectionDiffusion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/physics/transport/TransportBoundary.h
)

# Header-only library - no source files needed

message(STATUS "  - physics/transport (Phase 22 - Transport phenomena complete)")
