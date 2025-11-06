# PDE abstraction layer
# Phase 11: Solver interfaces and types

# Header files for PDE solver interfaces
set(PDE_HEADERS
    include/solver/pde/SolverTypes.h
    include/solver/pde/LinearSystem.h
    include/solver/pde/SolverOptions.h
    include/solver/pde/ISolver.h
)

# Group headers for IDE
source_group("PDE Headers" FILES ${PDE_HEADERS})

message(STATUS "  - solver/pde (Phase 11 - Solver interfaces complete)")
