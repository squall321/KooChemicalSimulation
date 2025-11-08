# MPI Parallel tests
# Priority D1: MPI Parallelization (v6.0.0-alpha5)

# Find Google Test
find_package(GTest REQUIRED)

# MPI Diffusion tests
add_executable(test_mpi_diffusion test_mpi_diffusion.cpp)
target_link_libraries(test_mpi_diffusion PRIVATE
    koo_core
    koo_utils
    koo_physics
    koo_parallel
    GTest::GTest
    GTest::Main
)
set_target_properties(test_mpi_diffusion PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add to CTest
add_test(NAME MPIDiffusionTests COMMAND test_mpi_diffusion)

# For MPI tests, you can also run with mpirun:
# add_test(NAME MPIDiffusionTestsMPI COMMAND mpirun -np 4 test_mpi_diffusion)

message(STATUS "MPI parallel tests configured (Priority D1)")
