# I/O module
# Phase 31-35: Input/output and VTK integration (v2.0.0-alpha1)

set(IO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io/InputReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io/OutputWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io/VTKWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io/Logger.h
)

# Header-only library
add_library(koo_io INTERFACE)
target_include_directories(koo_io INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

message(STATUS "I/O module configured (Phase 31-35 complete - v2.0.0-alpha1)")
