# Chemical species management
# Phase 16: Species class and database

# Species headers (header-only)
set(SPECIES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/species/Species.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chemistry/species/SpeciesManager.h
)

# Header-only, no compilation needed
# Headers will be included via parent koo_chemistry library

message(STATUS "  - chemistry/species (Phase 16 - Species system complete)")
