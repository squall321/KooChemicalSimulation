/**
 * @file SubDomain.h
 * @brief Subdomain management for domain decomposition
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha4
 * @date 2025-11-06
 *
 * Defines subdomains for parallel domain decomposition and local refinement.
 */

#ifndef KOO_MESH_DOMAIN_SUBDOMAIN_H
#define KOO_MESH_DOMAIN_SUBDOMAIN_H

#include "Domain.h"
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <algorithm>

namespace koo {
namespace mesh {
namespace domain {

/**
 * @brief Subdomain partitioning strategy
 */
enum class PartitionStrategy {
    NONE,           ///< No partitioning
    UNIFORM,        ///< Uniform spatial partitioning
    METIS,          ///< METIS graph partitioning
    RECURSIVE,      ///< Recursive coordinate bisection
    CUSTOM          ///< User-defined partitioning
};

/**
 * @brief Subdomain representation
 *
 * A subdomain is a partition of a domain, typically used for:
 * - Parallel domain decomposition
 * - Local mesh refinement
 * - Multi-physics coupling
 */
class SubDomain {
public:
    /**
     * @brief Constructor
     * @param id Subdomain ID
     * @param parentDomainId Parent domain ID
     * @param name Subdomain name
     */
    SubDomain(int id, int parentDomainId, const std::string& name)
        : id_(id), parentDomainId_(parentDomainId), name_(name),
          rank_(0), numRanks_(1), enabled_(true) {}

    /**
     * @brief Get subdomain ID
     */
    int getId() const { return id_; }

    /**
     * @brief Get parent domain ID
     */
    int getParentDomainId() const { return parentDomainId_; }

    /**
     * @brief Get name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Set name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Set MPI rank
     */
    void setRank(int rank) { rank_ = rank; }

    /**
     * @brief Get MPI rank
     */
    int getRank() const { return rank_; }

    /**
     * @brief Set number of MPI ranks
     */
    void setNumRanks(int numRanks) { numRanks_ = numRanks; }

    /**
     * @brief Get number of MPI ranks
     */
    int getNumRanks() const { return numRanks_; }

    /**
     * @brief Add element to subdomain
     */
    void addElement(size_t elementId) {
        elementIds_.insert(elementId);
    }

    /**
     * @brief Add multiple elements
     */
    void addElements(const std::vector<size_t>& elementIds) {
        elementIds_.insert(elementIds.begin(), elementIds.end());
    }

    /**
     * @brief Remove element
     */
    bool removeElement(size_t elementId) {
        return elementIds_.erase(elementId) > 0;
    }

    /**
     * @brief Check if element is in subdomain
     */
    bool hasElement(size_t elementId) const {
        return elementIds_.find(elementId) != elementIds_.end();
    }

    /**
     * @brief Get number of elements
     */
    size_t getNumElements() const {
        return elementIds_.size();
    }

    /**
     * @brief Get all element IDs
     */
    std::vector<size_t> getElementIds() const {
        return std::vector<size_t>(elementIds_.begin(), elementIds_.end());
    }

    /**
     * @brief Get element IDs as set
     */
    const std::set<size_t>& getElementIdSet() const {
        return elementIds_;
    }

    /**
     * @brief Clear all elements
     */
    void clearElements() {
        elementIds_.clear();
    }

    /**
     * @brief Add node to subdomain
     */
    void addNode(size_t nodeId) {
        nodeIds_.insert(nodeId);
    }

    /**
     * @brief Add multiple nodes
     */
    void addNodes(const std::vector<size_t>& nodeIds) {
        nodeIds_.insert(nodeIds.begin(), nodeIds.end());
    }

    /**
     * @brief Check if node is in subdomain
     */
    bool hasNode(size_t nodeId) const {
        return nodeIds_.find(nodeId) != nodeIds_.end();
    }

    /**
     * @brief Get number of nodes
     */
    size_t getNumNodes() const {
        return nodeIds_.size();
    }

    /**
     * @brief Get all node IDs
     */
    std::vector<size_t> getNodeIds() const {
        return std::vector<size_t>(nodeIds_.begin(), nodeIds_.end());
    }

    /**
     * @brief Get node IDs as set
     */
    const std::set<size_t>& getNodeIdSet() const {
        return nodeIds_;
    }

    /**
     * @brief Clear all nodes
     */
    void clearNodes() {
        nodeIds_.clear();
    }

    /**
     * @brief Add neighbor subdomain
     */
    void addNeighbor(int subdomainId) {
        neighborIds_.insert(subdomainId);
    }

    /**
     * @brief Remove neighbor
     */
    bool removeNeighbor(int subdomainId) {
        return neighborIds_.erase(subdomainId) > 0;
    }

    /**
     * @brief Check if subdomain is a neighbor
     */
    bool hasNeighbor(int subdomainId) const {
        return neighborIds_.find(subdomainId) != neighborIds_.end();
    }

    /**
     * @brief Get neighbor IDs
     */
    std::vector<int> getNeighborIds() const {
        return std::vector<int>(neighborIds_.begin(), neighborIds_.end());
    }

    /**
     * @brief Get number of neighbors
     */
    size_t getNumNeighbors() const {
        return neighborIds_.size();
    }

    /**
     * @brief Add interface node (shared with neighbors)
     */
    void addInterfaceNode(size_t nodeId, int neighborId) {
        interfaceNodes_[neighborId].insert(nodeId);
    }

    /**
     * @brief Get interface nodes with a neighbor
     */
    std::vector<size_t> getInterfaceNodes(int neighborId) const {
        auto it = interfaceNodes_.find(neighborId);
        if (it != interfaceNodes_.end()) {
            return std::vector<size_t>(it->second.begin(), it->second.end());
        }
        return std::vector<size_t>();
    }

    /**
     * @brief Get all interface nodes
     */
    std::set<size_t> getAllInterfaceNodes() const {
        std::set<size_t> result;
        for (const auto& pair : interfaceNodes_) {
            result.insert(pair.second.begin(), pair.second.end());
        }
        return result;
    }

    /**
     * @brief Get number of interface nodes with neighbor
     */
    size_t getNumInterfaceNodes(int neighborId) const {
        auto it = interfaceNodes_.find(neighborId);
        return (it != interfaceNodes_.end()) ? it->second.size() : 0;
    }

    /**
     * @brief Enable/disable subdomain
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if subdomain is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Compute overlap with another subdomain
     */
    size_t computeOverlap(const SubDomain& other) const {
        std::vector<size_t> intersection;
        std::set_intersection(
            elementIds_.begin(), elementIds_.end(),
            other.elementIds_.begin(), other.elementIds_.end(),
            std::back_inserter(intersection)
        );
        return intersection.size();
    }

    /**
     * @brief Check if this subdomain overlaps with another
     */
    bool overlaps(const SubDomain& other) const {
        return computeOverlap(other) > 0;
    }

    /**
     * @brief Get string representation
     */
    std::string toString() const {
        std::string result = "SubDomain[" + std::to_string(id_) + "]: ";
        result += name_ + " (parent=" + std::to_string(parentDomainId_) + ")";
        result += ", elements=" + std::to_string(elementIds_.size());
        result += ", nodes=" + std::to_string(nodeIds_.size());
        result += ", neighbors=" + std::to_string(neighborIds_.size());
        result += ", rank=" + std::to_string(rank_) + "/" + std::to_string(numRanks_);
        result += ", enabled=" + std::string(enabled_ ? "yes" : "no");
        return result;
    }

private:
    int id_;                        ///< Subdomain ID
    int parentDomainId_;            ///< Parent domain ID
    std::string name_;              ///< Subdomain name
    int rank_;                      ///< MPI rank
    int numRanks_;                  ///< Total number of ranks
    bool enabled_;                  ///< Whether subdomain is active

    std::set<size_t> elementIds_;   ///< Element IDs in this subdomain
    std::set<size_t> nodeIds_;      ///< Node IDs in this subdomain
    std::set<int> neighborIds_;     ///< Neighbor subdomain IDs

    /// Interface nodes shared with each neighbor
    std::map<int, std::set<size_t>> interfaceNodes_;
};

/**
 * @brief Domain decomposition utilities
 */
namespace decomposition {

/**
 * @brief Create uniform subdomain decomposition
 * @param domain Domain to decompose
 * @param numSubdomains Number of subdomains
 * @return Vector of subdomains
 */
inline std::vector<std::shared_ptr<SubDomain>> createUniformDecomposition(
    const Domain& domain, int numSubdomains) {

    std::vector<std::shared_ptr<SubDomain>> subdomains;
    auto elementIds = domain.getElementIds();

    if (elementIds.empty() || numSubdomains <= 0) {
        return subdomains;
    }

    // Simple round-robin distribution
    size_t elemsPerSubdomain = (elementIds.size() + numSubdomains - 1) / numSubdomains;

    for (int i = 0; i < numSubdomains; ++i) {
        auto subdomain = std::make_shared<SubDomain>(
            i, domain.getId(),
            domain.getName() + "_sub" + std::to_string(i)
        );
        subdomain->setRank(i);
        subdomain->setNumRanks(numSubdomains);

        // Assign elements
        size_t start = i * elemsPerSubdomain;
        size_t end = std::min(start + elemsPerSubdomain, elementIds.size());
        for (size_t j = start; j < end; ++j) {
            subdomain->addElement(elementIds[j]);
        }

        subdomains.push_back(subdomain);
    }

    return subdomains;
}

/**
 * @brief Detect neighboring subdomains
 * @param subdomains Vector of subdomains
 */
inline void detectNeighbors(std::vector<std::shared_ptr<SubDomain>>& subdomains) {
    for (size_t i = 0; i < subdomains.size(); ++i) {
        for (size_t j = i + 1; j < subdomains.size(); ++j) {
            if (subdomains[i]->overlaps(*subdomains[j])) {
                subdomains[i]->addNeighbor(subdomains[j]->getId());
                subdomains[j]->addNeighbor(subdomains[i]->getId());
            }
        }
    }
}

} // namespace decomposition

} // namespace domain
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_DOMAIN_SUBDOMAIN_H
