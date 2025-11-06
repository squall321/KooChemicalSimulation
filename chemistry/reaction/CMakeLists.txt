# Phase 17: Chemical Reaction System
# Provides reaction kinetics and mechanism management

set(REACTION_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/reaction/Reaction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/reaction/ReactionManager.h
)

# Header-only library, no source files needed
# All reaction components are included via koo_chemistry target

message(STATUS "  - chemistry/reaction (Phase 17 - Reaction system complete)")
