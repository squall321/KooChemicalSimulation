# Phase 18: Chemical Kinetics Solver
# Provides time integration for chemical systems

set(KINETICS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/kinetics/ChemicalSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/kinetics/KineticsIntegrator.h
)

# Header-only library, no source files needed
# All kinetics components are included via koo_chemistry target

message(STATUS "  - chemistry/kinetics (Phase 18 - Kinetics solver complete)")
