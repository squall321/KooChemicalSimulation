# Solver factory and strategy pattern
# Phase 14: SolverFactory, SolverStrategy, SolverConfig

# Factory headers (header-only)
set(FACTORY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/solver/SolverFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/solver/SolverStrategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/solver/SolverConfig.h
)

# Header-only, no compilation needed
# Headers are already included via koo_solver interface library

message(STATUS "  - solver/factory (Phase 14 - Factory, Strategy, Config complete)")
