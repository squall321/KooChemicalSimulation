# Domain and subdomain management
# Phase 9: Domain decomposition and management

# Header files for domain management
set(DOMAIN_HEADERS
    include/mesh/domain/Domain.h
    include/mesh/domain/SubDomain.h
    include/mesh/domain/DomainManager.h
)

# Group headers for IDE
source_group("Domain Headers" FILES ${DOMAIN_HEADERS})

message(STATUS "  - mesh/domain (Phase 9 - Domain and subdomain management complete)")
