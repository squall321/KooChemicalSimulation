# Utilities module
# Phase 3-4: Logging, error handling, math utilities

# Header-only library for utilities
add_library(koo_utils INTERFACE)

# Add include directories
target_include_directories(koo_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_utils INTERFACE cxx_std_17)

# Header files
set(UTILS_ERROR_HEADERS
    include/utils/error/Exception.h
)

set(UTILS_LOGGER_HEADERS
    include/utils/logger/Logger.h
)

# Group headers for IDE
source_group("Error Headers" FILES ${UTILS_ERROR_HEADERS})
source_group("Logger Headers" FILES ${UTILS_LOGGER_HEADERS})

add_subdirectory(logger)
add_subdirectory(error)
add_subdirectory(math)

# Add alias for consistent naming
add_library(Koo::Utils ALIAS koo_utils)

message(STATUS "Utils module configured (Phase 3 - error handling and logging complete)")
