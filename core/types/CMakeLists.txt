# Core types
# Phase 3: CommonTypes, PhysicalQuantity, and other type definitions

# Header files for types (header-only)
set(CORE_TYPES_HEADERS
    ../include/core/types/CommonTypes.h
    ../include/core/types/PhysicalQuantity.h
)

# Add to parent target (koo_core is INTERFACE library defined in core/CMakeLists.txt)
# Headers are already included via core's include directory

message(STATUS "  - core/types (Phase 3 - complete)")
