/**
 * @file Domain.h
 * @brief Domain representation for mesh regions
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha4
 * @date 2025-11-06
 *
 * Defines geometric domains and physical regions within a mesh.
 */

#ifndef KOO_MESH_DOMAIN_DOMAIN_H
#define KOO_MESH_DOMAIN_DOMAIN_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <algorithm>

namespace koo {
namespace mesh {
namespace domain {

/**
 * @brief Domain dimension
 */
enum class DomainDimension {
    POINT = 0,      ///< 0D point
    CURVE = 1,      ///< 1D curve
    SURFACE = 2,    ///< 2D surface
    VOLUME = 3      ///< 3D volume
};

/**
 * @brief Domain type
 */
enum class DomainType {
    PHYSICAL,       ///< Physical domain (from mesh)
    MATERIAL,       ///< Material region
    BOUNDARY,       ///< Boundary region
    INTERFACE,      ///< Interface between materials
    CUSTOM          ///< User-defined region
};

/**
 * @brief Domain representation
 *
 * A domain represents a geometric region in the mesh, typically
 * corresponding to physical entities, material regions, or boundaries.
 */
class Domain {
public:
    /**
     * @brief Constructor
     * @param id Unique domain ID
     * @param name Domain name
     * @param dimension Domain dimension
     * @param type Domain type
     */
    Domain(int id, const std::string& name, DomainDimension dimension, DomainType type)
        : id_(id), name_(name), dimension_(dimension), type_(type),
          physicalTag_(-1), materialId_(-1), enabled_(true) {}

    /**
     * @brief Get domain ID
     */
    int getId() const { return id_; }

    /**
     * @brief Get domain name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Set domain name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Get dimension
     */
    DomainDimension getDimension() const { return dimension_; }

    /**
     * @brief Get type
     */
    DomainType getType() const { return type_; }

    /**
     * @brief Set physical tag
     */
    void setPhysicalTag(int tag) { physicalTag_ = tag; }

    /**
     * @brief Get physical tag
     */
    int getPhysicalTag() const { return physicalTag_; }

    /**
     * @brief Set material ID
     */
    void setMaterialId(int matId) { materialId_ = matId; }

    /**
     * @brief Get material ID
     */
    int getMaterialId() const { return materialId_; }

    /**
     * @brief Add element to domain
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
     * @brief Check if element is in domain
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
     * @brief Add node to domain
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
     * @brief Check if node is in domain
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
     * @brief Set property
     */
    void setProperty(const std::string& key, double value) {
        properties_[key] = value;
    }

    /**
     * @brief Get property
     */
    double getProperty(const std::string& key, double defaultValue = 0.0) const {
        auto it = properties_.find(key);
        return (it != properties_.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Check if property exists
     */
    bool hasProperty(const std::string& key) const {
        return properties_.find(key) != properties_.end();
    }

    /**
     * @brief Get all property keys
     */
    std::vector<std::string> getPropertyKeys() const {
        std::vector<std::string> keys;
        keys.reserve(properties_.size());
        for (const auto& pair : properties_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    /**
     * @brief Enable/disable domain
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if domain is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Get string representation
     */
    std::string toString() const {
        std::string result = "Domain[" + std::to_string(id_) + "]: ";
        result += name_ + " (";
        result += dimensionToString(dimension_) + ", ";
        result += typeToString(type_) + ")";
        result += ", elements=" + std::to_string(elementIds_.size());
        result += ", nodes=" + std::to_string(nodeIds_.size());
        if (physicalTag_ >= 0) {
            result += ", tag=" + std::to_string(physicalTag_);
        }
        if (materialId_ >= 0) {
            result += ", material=" + std::to_string(materialId_);
        }
        result += ", enabled=" + std::string(enabled_ ? "yes" : "no");
        return result;
    }

    /**
     * @brief Convert dimension to string
     */
    static std::string dimensionToString(DomainDimension dim) {
        switch (dim) {
            case DomainDimension::POINT:   return "0D";
            case DomainDimension::CURVE:   return "1D";
            case DomainDimension::SURFACE: return "2D";
            case DomainDimension::VOLUME:  return "3D";
            default:                       return "unknown";
        }
    }

    /**
     * @brief Convert type to string
     */
    static std::string typeToString(DomainType type) {
        switch (type) {
            case DomainType::PHYSICAL:   return "physical";
            case DomainType::MATERIAL:   return "material";
            case DomainType::BOUNDARY:   return "boundary";
            case DomainType::INTERFACE:  return "interface";
            case DomainType::CUSTOM:     return "custom";
            default:                     return "unknown";
        }
    }

private:
    int id_;                        ///< Domain ID
    std::string name_;              ///< Domain name
    DomainDimension dimension_;     ///< Domain dimension
    DomainType type_;               ///< Domain type
    int physicalTag_;               ///< Physical tag from mesh
    int materialId_;                ///< Material ID
    bool enabled_;                  ///< Whether domain is active

    std::set<size_t> elementIds_;   ///< Element IDs in this domain
    std::set<size_t> nodeIds_;      ///< Node IDs in this domain
    std::map<std::string, double> properties_;  ///< Domain properties
};

} // namespace domain
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_DOMAIN_DOMAIN_H
