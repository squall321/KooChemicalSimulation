# Configuration module
# Phase 5: Configuration system basics

# Header-only library for configuration system
add_library(koo_config INTERFACE)

# Add include directories
target_include_directories(koo_config INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set C++ standard
target_compile_features(koo_config INTERFACE cxx_std_17)

# Link to core and utils
target_link_libraries(koo_config INTERFACE koo_core koo_utils)

# Check for JSON library (nlohmann-json)
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json - JSON parsing enabled")
    target_link_libraries(koo_config INTERFACE nlohmann_json::nlohmann_json)
    target_compile_definitions(koo_config INTERFACE USE_JSON)
else()
    message(STATUS "nlohmann_json not found - JSON parsing disabled")
endif()

# Check for YAML library (yaml-cpp)
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "Found yaml-cpp - YAML parsing enabled")
    target_link_libraries(koo_config INTERFACE yaml-cpp)
    target_compile_definitions(koo_config INTERFACE USE_YAML)
else()
    message(STATUS "yaml-cpp not found - YAML parsing disabled")
endif()

# Header files
set(CONFIG_HEADERS
    include/config/Config.h
    include/config/parser/ConfigParser.h
    include/config/validator/ConfigValidator.h
)

# Group headers for IDE
source_group("Config Headers" FILES ${CONFIG_HEADERS})

# Subdirectories (placeholders for later phases)
add_subdirectory(parser)
add_subdirectory(validator)

# Add alias for consistent naming
add_library(Koo::Config ALIAS koo_config)

message(STATUS "Config module configured (Phase 5 - configuration system complete)")
