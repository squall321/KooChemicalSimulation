/**
 * @file MeshData.h
 * @brief Mesh data container
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha1
 * @date 2025-11-06
 *
 * This file defines the MeshData class which contains all mesh information:
 * nodes, elements, and associated metadata.
 */

#ifndef KOO_MESH_CORE_MESH_DATA_H
#define KOO_MESH_CORE_MESH_DATA_H

#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "core/interfaces/IMesh.h"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>

namespace koo {
namespace mesh {
namespace core {

using ElementType = koo::core::ElementType;
using MeshDimension = koo::core::MeshDimension;

// ============================================================================
// MeshData Class
// ============================================================================

/**
 * @brief Container for mesh data
 *
 * MeshData stores all mesh information including nodes, elements,
 * physical groups, and mesh metadata.
 *
 * Thread-safety: Not thread-safe. Use external synchronization if needed.
 */
class MeshData {
public:
    /**
     * @brief Constructor
     */
    MeshData() : dimension_(MeshDimension::THREE_D) {}

    /**
     * @brief Destructor
     */
    ~MeshData() = default;

    // Copy and move semantics
    MeshData(const MeshData&) = default;
    MeshData& operator=(const MeshData&) = default;
    MeshData(MeshData&&) = default;
    MeshData& operator=(MeshData&&) = default;

    // ========================================================================
    // Node Operations
    // ========================================================================

    /**
     * @brief Add a node to the mesh
     *
     * @param node Node to add
     * @return true if node was added (false if ID already exists)
     */
    bool addNode(const Node& node) {
        size_t id = node.getId();
        if (nodeMap_.find(id) != nodeMap_.end()) {
            return false; // Node already exists
        }
        nodes_.push_back(node);
        nodeMap_[id] = nodes_.size() - 1;
        return true;
    }

    /**
     * @brief Get node by ID
     *
     * @param id Node ID
     * @return Pointer to node (nullptr if not found)
     */
    const Node* getNode(size_t id) const {
        auto it = nodeMap_.find(id);
        if (it == nodeMap_.end()) {
            return nullptr;
        }
        return &nodes_[it->second];
    }

    /**
     * @brief Get mutable node by ID
     */
    Node* getNode(size_t id) {
        auto it = nodeMap_.find(id);
        if (it == nodeMap_.end()) {
            return nullptr;
        }
        return &nodes_[it->second];
    }

    /**
     * @brief Get node by index
     */
    const Node& getNodeByIndex(size_t index) const {
        if (index >= nodes_.size()) {
            throw std::out_of_range("Node index out of range");
        }
        return nodes_[index];
    }

    /**
     * @brief Get mutable node by index
     */
    Node& getNodeByIndex(size_t index) {
        if (index >= nodes_.size()) {
            throw std::out_of_range("Node index out of range");
        }
        return nodes_[index];
    }

    /**
     * @brief Get all nodes
     */
    const std::vector<Node>& getNodes() const { return nodes_; }

    /**
     * @brief Get number of nodes
     */
    size_t getNumNodes() const { return nodes_.size(); }

    /**
     * @brief Check if node exists
     */
    bool hasNode(size_t id) const {
        return nodeMap_.find(id) != nodeMap_.end();
    }

    /**
     * @brief Clear all nodes
     */
    void clearNodes() {
        nodes_.clear();
        nodeMap_.clear();
    }

    // ========================================================================
    // Element Operations
    // ========================================================================

    /**
     * @brief Add an element to the mesh
     *
     * @param element Element to add
     * @return true if element was added (false if ID already exists)
     */
    bool addElement(const Element& element) {
        size_t id = element.getId();
        if (elementMap_.find(id) != elementMap_.end()) {
            return false; // Element already exists
        }
        elements_.push_back(element);
        elementMap_[id] = elements_.size() - 1;
        return true;
    }

    /**
     * @brief Get element by ID
     *
     * @param id Element ID
     * @return Pointer to element (nullptr if not found)
     */
    const Element* getElement(size_t id) const {
        auto it = elementMap_.find(id);
        if (it == elementMap_.end()) {
            return nullptr;
        }
        return &elements_[it->second];
    }

    /**
     * @brief Get mutable element by ID
     */
    Element* getElement(size_t id) {
        auto it = elementMap_.find(id);
        if (it == elementMap_.end()) {
            return nullptr;
        }
        return &elements_[it->second];
    }

    /**
     * @brief Get element by index
     */
    const Element& getElementByIndex(size_t index) const {
        if (index >= elements_.size()) {
            throw std::out_of_range("Element index out of range");
        }
        return elements_[index];
    }

    /**
     * @brief Get mutable element by index
     */
    Element& getElementByIndex(size_t index) {
        if (index >= elements_.size()) {
            throw std::out_of_range("Element index out of range");
        }
        return elements_[index];
    }

    /**
     * @brief Get all elements
     */
    const std::vector<Element>& getElements() const { return elements_; }

    /**
     * @brief Get number of elements
     */
    size_t getNumElements() const { return elements_.size(); }

    /**
     * @brief Check if element exists
     */
    bool hasElement(size_t id) const {
        return elementMap_.find(id) != elementMap_.end();
    }

    /**
     * @brief Clear all elements
     */
    void clearElements() {
        elements_.clear();
        elementMap_.clear();
    }

    /**
     * @brief Get elements of a specific type
     */
    std::vector<const Element*> getElementsByType(ElementType type) const {
        std::vector<const Element*> result;
        for (const auto& elem : elements_) {
            if (elem.getType() == type) {
                result.push_back(&elem);
            }
        }
        return result;
    }

    /**
     * @brief Get elements with a specific tag
     */
    std::vector<const Element*> getElementsByTag(int tag) const {
        std::vector<const Element*> result;
        for (const auto& elem : elements_) {
            if (elem.getTag() == tag) {
                result.push_back(&elem);
            }
        }
        return result;
    }

    // ========================================================================
    // Physical Groups
    // ========================================================================

    /**
     * @brief Add a physical group
     *
     * @param tag Physical tag
     * @param name Group name
     */
    void addPhysicalGroup(int tag, const std::string& name) {
        physicalGroups_[tag] = name;
    }

    /**
     * @brief Get physical group name
     *
     * @param tag Physical tag
     * @return Group name (empty if not found)
     */
    std::string getPhysicalGroupName(int tag) const {
        auto it = physicalGroups_.find(tag);
        if (it != physicalGroups_.end()) {
            return it->second;
        }
        return "";
    }

    /**
     * @brief Check if physical group exists
     */
    bool hasPhysicalGroup(int tag) const {
        return physicalGroups_.find(tag) != physicalGroups_.end();
    }

    /**
     * @brief Get all physical groups
     */
    const std::map<int, std::string>& getPhysicalGroups() const {
        return physicalGroups_;
    }

    /**
     * @brief Clear all physical groups
     */
    void clearPhysicalGroups() {
        physicalGroups_.clear();
    }

    // ========================================================================
    // Mesh Properties
    // ========================================================================

    /**
     * @brief Get mesh dimension
     */
    MeshDimension getDimension() const { return dimension_; }

    /**
     * @brief Set mesh dimension
     */
    void setDimension(MeshDimension dim) { dimension_ = dim; }

    /**
     * @brief Get mesh name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Set mesh name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Calculate bounding box
     *
     * @param min Output minimum coordinates [x, y, z]
     * @param max Output maximum coordinates [x, y, z]
     */
    void getBoundingBox(double min[3], double max[3]) const {
        calculateBoundingBox(nodes_, min, max);
    }

    /**
     * @brief Check if mesh is empty
     */
    bool isEmpty() const {
        return nodes_.empty() && elements_.empty();
    }

    /**
     * @brief Clear all mesh data
     */
    void clear() {
        clearNodes();
        clearElements();
        clearPhysicalGroups();
        name_.clear();
    }

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get mesh statistics
     */
    struct Statistics {
        size_t numNodes{0};
        size_t numElements{0};
        size_t numVertices{0};
        size_t numLines{0};
        size_t numTriangles{0};
        size_t numQuads{0};
        size_t numTets{0};
        size_t numHexs{0};
        size_t numPrisms{0};
        size_t numPyramids{0};
        size_t numPhysicalGroups{0};
        double minElementSize{0.0};
        double maxElementSize{0.0};
        double avgElementSize{0.0};
    };

    /**
     * @brief Calculate mesh statistics
     */
    Statistics getStatistics() const {
        Statistics stats;
        stats.numNodes = nodes_.size();
        stats.numElements = elements_.size();
        stats.numPhysicalGroups = physicalGroups_.size();

        double totalSize = 0.0;
        double minSize = std::numeric_limits<double>::max();
        double maxSize = 0.0;
        size_t measureCount = 0;

        for (const auto& elem : elements_) {
            // Count by type
            switch (elem.getType()) {
                case ElementType::VERTEX:       stats.numVertices++; break;
                case ElementType::LINE:         stats.numLines++; break;
                case ElementType::TRIANGLE:     stats.numTriangles++; break;
                case ElementType::QUADRILATERAL:stats.numQuads++; break;
                case ElementType::TETRAHEDRON:  stats.numTets++; break;
                case ElementType::HEXAHEDRON:   stats.numHexs++; break;
                case ElementType::PRISM:        stats.numPrisms++; break;
                case ElementType::PYRAMID:      stats.numPyramids++; break;
            }

            // Calculate element size
            try {
                double measure = elem.calculateMeasure(nodes_);
                if (measure > 0.0) {
                    totalSize += measure;
                    minSize = std::min(minSize, measure);
                    maxSize = std::max(maxSize, measure);
                    measureCount++;
                }
            } catch (...) {
                // Skip elements with missing nodes
            }
        }

        if (measureCount > 0) {
            stats.minElementSize = minSize;
            stats.maxElementSize = maxSize;
            stats.avgElementSize = totalSize / static_cast<double>(measureCount);
        }

        return stats;
    }

    // ========================================================================
    // Validation
    // ========================================================================

    /**
     * @brief Validate mesh integrity
     *
     * Checks that all element nodes exist in the mesh.
     *
     * @return Vector of error messages (empty if valid)
     */
    std::vector<std::string> validate() const {
        std::vector<std::string> errors;

        for (size_t i = 0; i < elements_.size(); ++i) {
            const auto& elem = elements_[i];
            const auto& nodeIds = elem.getNodeIds();

            for (size_t nodeId : nodeIds) {
                if (!hasNode(nodeId)) {
                    errors.push_back("Element " + std::to_string(elem.getId()) +
                                   " references non-existent node " +
                                   std::to_string(nodeId));
                }
            }

            if (!elem.isValid()) {
                errors.push_back("Element " + std::to_string(elem.getId()) +
                               " has invalid node count for its type");
            }
        }

        return errors;
    }

    /**
     * @brief Check if mesh is valid
     */
    bool isValid() const {
        return validate().empty();
    }

private:
    std::vector<Node> nodes_;                    ///< Mesh nodes
    std::vector<Element> elements_;              ///< Mesh elements
    std::map<size_t, size_t> nodeMap_;           ///< Node ID to index map
    std::map<size_t, size_t> elementMap_;        ///< Element ID to index map
    std::map<int, std::string> physicalGroups_;  ///< Physical group tags
    MeshDimension dimension_;                    ///< Mesh dimension
    std::string name_;                           ///< Mesh name
};

} // namespace core
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_CORE_MESH_DATA_H
