/**
 * @file DomainManager.h
 * @brief Domain and subdomain manager
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha4
 * @date 2025-11-06
 *
 * Manages collections of domains and subdomains with query capabilities.
 */

#ifndef KOO_MESH_DOMAIN_DOMAIN_MANAGER_H
#define KOO_MESH_DOMAIN_DOMAIN_MANAGER_H

#include "Domain.h"
#include "SubDomain.h"
#include "mesh/core/MeshData.h"
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace koo {
namespace mesh {
namespace domain {

/**
 * @brief Domain and subdomain manager
 *
 * Organizes and manages domains and subdomains for a mesh.
 */
class DomainManager {
public:
    using DomainPtr = std::shared_ptr<Domain>;
    using SubDomainPtr = std::shared_ptr<SubDomain>;

    DomainManager() : nextDomainId_(0), nextSubdomainId_(0) {}

    /**
     * @brief Create and add a new domain
     */
    DomainPtr createDomain(const std::string& name, DomainDimension dimension, DomainType type) {
        int id = nextDomainId_++;
        auto domain = std::make_shared<Domain>(id, name, dimension, type);
        domains_[id] = domain;

        // Index by name
        domainsByName_[name] = domain;

        // Index by dimension
        domainsByDimension_[dimension].push_back(domain);

        // Index by type
        domainsByType_[type].push_back(domain);

        return domain;
    }

    /**
     * @brief Add an existing domain
     */
    void addDomain(DomainPtr domain) {
        if (!domain) {
            throw std::runtime_error("Cannot add null domain");
        }

        int id = domain->getId();
        if (domains_.find(id) != domains_.end()) {
            throw std::runtime_error("Domain with ID " + std::to_string(id) + " already exists");
        }

        domains_[id] = domain;
        domainsByName_[domain->getName()] = domain;
        domainsByDimension_[domain->getDimension()].push_back(domain);
        domainsByType_[domain->getType()].push_back(domain);

        // Update next ID
        if (id >= nextDomainId_) {
            nextDomainId_ = id + 1;
        }
    }

    /**
     * @brief Remove domain by ID
     */
    bool removeDomain(int id) {
        auto it = domains_.find(id);
        if (it == domains_.end()) {
            return false;
        }

        DomainPtr domain = it->second;

        // Remove from name index
        domainsByName_.erase(domain->getName());

        // Remove from dimension index
        auto& dimVec = domainsByDimension_[domain->getDimension()];
        dimVec.erase(std::remove(dimVec.begin(), dimVec.end(), domain), dimVec.end());

        // Remove from type index
        auto& typeVec = domainsByType_[domain->getType()];
        typeVec.erase(std::remove(typeVec.begin(), typeVec.end(), domain), typeVec.end());

        // Remove associated subdomains
        auto subdomains = getSubDomainsByParent(id);
        for (const auto& subdomain : subdomains) {
            removeSubDomain(subdomain->getId());
        }

        domains_.erase(it);
        return true;
    }

    /**
     * @brief Get domain by ID
     */
    DomainPtr getDomain(int id) const {
        auto it = domains_.find(id);
        return (it != domains_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Get domain by name
     */
    DomainPtr getDomainByName(const std::string& name) const {
        auto it = domainsByName_.find(name);
        return (it != domainsByName_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Get domains by dimension
     */
    std::vector<DomainPtr> getDomainsByDimension(DomainDimension dimension) const {
        auto it = domainsByDimension_.find(dimension);
        return (it != domainsByDimension_.end()) ? it->second : std::vector<DomainPtr>();
    }

    /**
     * @brief Get domains by type
     */
    std::vector<DomainPtr> getDomainsByType(DomainType type) const {
        auto it = domainsByType_.find(type);
        return (it != domainsByType_.end()) ? it->second : std::vector<DomainPtr>();
    }

    /**
     * @brief Get all domains
     */
    std::vector<DomainPtr> getAllDomains() const {
        std::vector<DomainPtr> result;
        result.reserve(domains_.size());
        for (const auto& pair : domains_) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * @brief Get number of domains
     */
    size_t getNumDomains() const {
        return domains_.size();
    }

    /**
     * @brief Check if domain exists
     */
    bool hasDomain(int id) const {
        return domains_.find(id) != domains_.end();
    }

    /**
     * @brief Clear all domains
     */
    void clearDomains() {
        domains_.clear();
        domainsByName_.clear();
        domainsByDimension_.clear();
        domainsByType_.clear();
        clearSubDomains();
    }

    /**
     * @brief Create and add a new subdomain
     */
    SubDomainPtr createSubDomain(int parentDomainId, const std::string& name) {
        if (!hasDomain(parentDomainId)) {
            throw std::runtime_error("Parent domain " + std::to_string(parentDomainId) + " does not exist");
        }

        int id = nextSubdomainId_++;
        auto subdomain = std::make_shared<SubDomain>(id, parentDomainId, name);
        subdomains_[id] = subdomain;

        // Index by parent
        subdomainsByParent_[parentDomainId].push_back(subdomain);

        return subdomain;
    }

    /**
     * @brief Add an existing subdomain
     */
    void addSubDomain(SubDomainPtr subdomain) {
        if (!subdomain) {
            throw std::runtime_error("Cannot add null subdomain");
        }

        int id = subdomain->getId();
        if (subdomains_.find(id) != subdomains_.end()) {
            throw std::runtime_error("SubDomain with ID " + std::to_string(id) + " already exists");
        }

        subdomains_[id] = subdomain;
        subdomainsByParent_[subdomain->getParentDomainId()].push_back(subdomain);

        // Update next ID
        if (id >= nextSubdomainId_) {
            nextSubdomainId_ = id + 1;
        }
    }

    /**
     * @brief Remove subdomain by ID
     */
    bool removeSubDomain(int id) {
        auto it = subdomains_.find(id);
        if (it == subdomains_.end()) {
            return false;
        }

        SubDomainPtr subdomain = it->second;

        // Remove from parent index
        auto& parentVec = subdomainsByParent_[subdomain->getParentDomainId()];
        parentVec.erase(std::remove(parentVec.begin(), parentVec.end(), subdomain), parentVec.end());

        subdomains_.erase(it);
        return true;
    }

    /**
     * @brief Get subdomain by ID
     */
    SubDomainPtr getSubDomain(int id) const {
        auto it = subdomains_.find(id);
        return (it != subdomains_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Get subdomains by parent domain
     */
    std::vector<SubDomainPtr> getSubDomainsByParent(int parentId) const {
        auto it = subdomainsByParent_.find(parentId);
        return (it != subdomainsByParent_.end()) ? it->second : std::vector<SubDomainPtr>();
    }

    /**
     * @brief Get all subdomains
     */
    std::vector<SubDomainPtr> getAllSubDomains() const {
        std::vector<SubDomainPtr> result;
        result.reserve(subdomains_.size());
        for (const auto& pair : subdomains_) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * @brief Get number of subdomains
     */
    size_t getNumSubDomains() const {
        return subdomains_.size();
    }

    /**
     * @brief Check if subdomain exists
     */
    bool hasSubDomain(int id) const {
        return subdomains_.find(id) != subdomains_.end();
    }

    /**
     * @brief Clear all subdomains
     */
    void clearSubDomains() {
        subdomains_.clear();
        subdomainsByParent_.clear();
    }

    /**
     * @brief Build domains from mesh physical groups
     */
    void buildFromMesh(const core::MeshData& mesh) {
        // This is a placeholder for integration with MeshData
        // In a full implementation, this would:
        // 1. Iterate through mesh physical groups
        // 2. Create domains for each group
        // 3. Assign elements to domains based on physical tags

        // For now, create a simple domain containing all elements
        auto domain = createDomain("mesh_domain", DomainDimension::VOLUME, DomainType::PHYSICAL);

        // Add all elements (in a real implementation, filter by physical tags)
        for (size_t i = 0; i < mesh.getNumElements(); ++i) {
            domain->addElement(i + 1);  // Assuming 1-based indexing
        }

        // Add all nodes
        for (size_t i = 0; i < mesh.getNumNodes(); ++i) {
            domain->addNode(i + 1);  // Assuming 1-based indexing
        }
    }

    /**
     * @brief Decompose domain into subdomains
     */
    std::vector<SubDomainPtr> decomposeDomain(int domainId, int numSubdomains, PartitionStrategy strategy = PartitionStrategy::UNIFORM) {
        auto domain = getDomain(domainId);
        if (!domain) {
            throw std::runtime_error("Domain " + std::to_string(domainId) + " not found");
        }

        std::vector<SubDomainPtr> result;

        switch (strategy) {
            case PartitionStrategy::UNIFORM: {
                auto subdomains = decomposition::createUniformDecomposition(*domain, numSubdomains);
                for (auto& subdomain : subdomains) {
                    addSubDomain(subdomain);
                    result.push_back(subdomain);
                }
                break;
            }
            case PartitionStrategy::NONE:
            case PartitionStrategy::METIS:
            case PartitionStrategy::RECURSIVE:
            case PartitionStrategy::CUSTOM:
            default:
                throw std::runtime_error("Partitioning strategy not yet implemented");
        }

        return result;
    }

    /**
     * @brief Get summary
     */
    std::string getSummary() const {
        std::string result = "Domain Manager Summary:\n";
        result += "  Total Domains: " + std::to_string(domains_.size()) + "\n";
        result += "  Total SubDomains: " + std::to_string(subdomains_.size()) + "\n";

        // Count by dimension
        result += "  Domains by dimension:\n";
        for (const auto& pair : domainsByDimension_) {
            result += "    " + Domain::dimensionToString(pair.first) + ": " +
                     std::to_string(pair.second.size()) + "\n";
        }

        // Count by type
        result += "  Domains by type:\n";
        for (const auto& pair : domainsByType_) {
            result += "    " + Domain::typeToString(pair.first) + ": " +
                     std::to_string(pair.second.size()) + "\n";
        }

        // List all domains
        if (!domains_.empty()) {
            result += "  All Domains:\n";
            for (const auto& pair : domains_) {
                result += "    " + pair.second->toString() + "\n";
            }
        }

        // List all subdomains
        if (!subdomains_.empty()) {
            result += "  All SubDomains:\n";
            for (const auto& pair : subdomains_) {
                result += "    " + pair.second->toString() + "\n";
            }
        }

        return result;
    }

private:
    int nextDomainId_;              ///< Next domain ID
    int nextSubdomainId_;           ///< Next subdomain ID

    std::map<int, DomainPtr> domains_;                          ///< Domains by ID
    std::map<std::string, DomainPtr> domainsByName_;            ///< Domains by name
    std::map<DomainDimension, std::vector<DomainPtr>> domainsByDimension_;  ///< Domains by dimension
    std::map<DomainType, std::vector<DomainPtr>> domainsByType_;            ///< Domains by type

    std::map<int, SubDomainPtr> subdomains_;                    ///< SubDomains by ID
    std::map<int, std::vector<SubDomainPtr>> subdomainsByParent_;  ///< SubDomains by parent
};

} // namespace domain
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_DOMAIN_DOMAIN_MANAGER_H
