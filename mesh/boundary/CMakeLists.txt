# Boundary condition management
# Phase 8: BoundaryCondition classes

# Header files for boundary conditions
set(BOUNDARY_HEADERS
    include/mesh/boundary/BoundaryCondition.h
    include/mesh/boundary/DirichletBC.h
    include/mesh/boundary/NeumannBC.h
    include/mesh/boundary/RobinBC.h
    include/mesh/boundary/BCManager.h
)

# Group headers for IDE
source_group("Boundary Headers" FILES ${BOUNDARY_HEADERS})

message(STATUS "  - mesh/boundary (Phase 8 - Boundary condition classes complete)")
