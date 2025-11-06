# Phase 20: Chemical Reaction-PDE Coupling
# Provides coupling between reactions and PDE systems

set(COUPLING_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/coupling/ConcentrationField.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/coupling/ReactionTerm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/coupling/ReactionPDECoupler.h
)

# Header-only library, no source files needed
# All coupling components are included via koo_chemistry target

message(STATUS "  - chemistry/coupling (Phase 20 - Reaction-PDE coupling complete)")
