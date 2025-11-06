# Python bindings for KooChemicalSimulation
# Phase 56: Core Python Interface

cmake_minimum_required(VERSION 3.15)

# Find Python
find_package(Python COMPONENTS Interpreter Development)

if(NOT Python_FOUND)
    message(WARNING "Python not found - Python bindings will not be built")
    return()
endif()

# Find pybind11
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found - attempting to use find_package with MODULE mode")
    find_package(pybind11 MODULE QUIET)
endif()

if(NOT pybind11_FOUND)
    message(WARNING "pybind11 not found - Python bindings will not be built")
    message(STATUS "  Install pybind11: pip install pybind11")
    return()
endif()

message(STATUS "Python bindings enabled")
message(STATUS "  Python version: ${Python_VERSION}")
message(STATUS "  Python executable: ${Python_EXECUTABLE}")
message(STATUS "  pybind11 version: ${pybind11_VERSION}")

# Create Python extension module
pybind11_add_module(_core
    src/bindings.cpp
    src/core.cpp
    src/mesh.cpp
    src/chemistry.cpp
    src/gpu.cpp
)

# Link libraries
target_link_libraries(_core PRIVATE
    koo_core
    koo_utils
    koo_mesh
    koo_chemistry
    koo_gpu
)

# Include directories
target_include_directories(_core PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/mesh/include
    ${CMAKE_SOURCE_DIR}/chemistry/include
    ${CMAKE_SOURCE_DIR}/gpu/include
    ${CMAKE_SOURCE_DIR}/utils/include
)

# Set properties
set_target_properties(_core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    PREFIX ""
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
)

# Install target
install(TARGETS _core
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/python/koolab
)

# Install Python package
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/koolab
    DESTINATION ${CMAKE_INSTALL_PREFIX}/python
    FILES_MATCHING PATTERN "*.py"
)

message(STATUS "Python bindings configured successfully")
