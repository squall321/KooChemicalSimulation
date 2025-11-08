cmake_minimum_required(VERSION 3.20)
project(test_koolab CXX)

# Find the package
find_package(KooLab REQUIRED CONFIG)

# Create test executable
add_executable(test_koolab src/test_package.cpp)

# Link against KooLab
target_link_libraries(test_koolab PRIVATE KooLab::KooLab)

# Set C++ standard
target_compile_features(test_koolab PRIVATE cxx_std_17)
