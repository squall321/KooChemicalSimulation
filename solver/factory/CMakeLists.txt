# Solver factory and strategy pattern
# Phase 14: SolverFactory, SolverStrategy

# Factory headers (header-only)
set(FACTORY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/solver/SolverFactory.h
)

# Header-only, no compilation needed
# Headers are already included via koo_solver interface library

message(STATUS "  - solver/factory (Phase 14 - SolverFactory complete)")
