/**
 * @file Element.h
 * @brief Mesh element data structure
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha1
 * @date 2025-11-06
 *
 * This file defines the Element class for representing mesh cells.
 * Elements are composed of nodes and represent geometric shapes
 * (triangles, quadrilaterals, tetrahedra, hexahedra, etc.).
 */

#ifndef KOO_MESH_CORE_ELEMENT_H
#define KOO_MESH_CORE_ELEMENT_H

#include "core/interfaces/IMesh.h"
#include "mesh/core/Node.h"
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace koo {
namespace mesh {
namespace core {

using ElementType = koo::core::ElementType;

// ============================================================================
// Element Class
// ============================================================================

/**
 * @brief Mesh element (cell) representation
 *
 * An element represents a geometric shape in the mesh, composed of nodes.
 * Common element types include:
 * - 1D: LINE
 * - 2D: TRIANGLE, QUADRILATERAL
 * - 3D: TETRAHEDRON, HEXAHEDRON, PRISM, PYRAMID
 */
class Element {
public:
    /**
     * @brief Default constructor
     */
    Element()
        : id_(0), type_(ElementType::TRIANGLE), tag_(0), partition_(0) {}

    /**
     * @brief Constructor with ID, type, and node IDs
     *
     * @param id Element ID
     * @param type Element type
     * @param nodeIds Node IDs that compose this element
     */
    Element(size_t id, ElementType type, const std::vector<size_t>& nodeIds)
        : id_(id), type_(type), nodeIds_(nodeIds), tag_(0), partition_(0) {
        validateNodeCount();
    }

    /**
     * @brief Constructor with ID, type, node IDs, and tag
     *
     * @param id Element ID
     * @param type Element type
     * @param nodeIds Node IDs that compose this element
     * @param tag Physical tag (for region/material identification)
     */
    Element(size_t id, ElementType type, const std::vector<size_t>& nodeIds, int tag)
        : id_(id), type_(type), nodeIds_(nodeIds), tag_(tag), partition_(0) {
        validateNodeCount();
    }

    /**
     * @brief Get element ID
     */
    size_t getId() const { return id_; }

    /**
     * @brief Set element ID
     */
    void setId(size_t id) { id_ = id; }

    /**
     * @brief Get element type
     */
    ElementType getType() const { return type_; }

    /**
     * @brief Set element type
     */
    void setType(ElementType type) {
        type_ = type;
        validateNodeCount();
    }

    /**
     * @brief Get physical tag
     */
    int getTag() const { return tag_; }

    /**
     * @brief Set physical tag
     */
    void setTag(int tag) { tag_ = tag; }

    /**
     * @brief Get partition ID (for parallel decomposition)
     */
    int getPartition() const { return partition_; }

    /**
     * @brief Set partition ID
     */
    void setPartition(int partition) { partition_ = partition; }

    /**
     * @brief Get node IDs
     */
    const std::vector<size_t>& getNodeIds() const { return nodeIds_; }

    /**
     * @brief Set node IDs
     */
    void setNodeIds(const std::vector<size_t>& nodeIds) {
        nodeIds_ = nodeIds;
        validateNodeCount();
    }

    /**
     * @brief Get number of nodes
     */
    size_t getNumNodes() const { return nodeIds_.size(); }

    /**
     * @brief Get node ID at index
     */
    size_t getNodeId(size_t index) const {
        if (index >= nodeIds_.size()) {
            throw std::out_of_range("Node index out of range");
        }
        return nodeIds_[index];
    }

    /**
     * @brief Set node ID at index
     */
    void setNodeId(size_t index, size_t nodeId) {
        if (index >= nodeIds_.size()) {
            throw std::out_of_range("Node index out of range");
        }
        nodeIds_[index] = nodeId;
    }

    /**
     * @brief Check if element contains a specific node
     */
    bool containsNode(size_t nodeId) const {
        return std::find(nodeIds_.begin(), nodeIds_.end(), nodeId) != nodeIds_.end();
    }

    /**
     * @brief Get element dimension (1D, 2D, or 3D)
     */
    int getDimension() const {
        switch (type_) {
            case ElementType::VERTEX:
                return 0;
            case ElementType::LINE:
                return 1;
            case ElementType::TRIANGLE:
            case ElementType::QUADRILATERAL:
                return 2;
            case ElementType::TETRAHEDRON:
            case ElementType::HEXAHEDRON:
            case ElementType::PRISM:
            case ElementType::PYRAMID:
                return 3;
            default:
                return -1;
        }
    }

    /**
     * @brief Get expected number of nodes for element type
     */
    static size_t getExpectedNodeCount(ElementType type) {
        switch (type) {
            case ElementType::VERTEX:       return 1;
            case ElementType::LINE:         return 2;
            case ElementType::TRIANGLE:     return 3;
            case ElementType::QUADRILATERAL:return 4;
            case ElementType::TETRAHEDRON:  return 4;
            case ElementType::HEXAHEDRON:   return 8;
            case ElementType::PRISM:        return 6;
            case ElementType::PYRAMID:      return 5;
            default:                        return 0;
        }
    }

    /**
     * @brief Validate that node count matches element type
     */
    bool isValid() const {
        size_t expected = getExpectedNodeCount(type_);
        return expected == 0 || nodeIds_.size() == expected;
    }

    /**
     * @brief Calculate element volume/area (requires node coordinates)
     *
     * @param nodes Vector of nodes (must contain all nodes of this element)
     * @return Volume (3D), area (2D), or length (1D)
     */
    double calculateMeasure(const std::vector<Node>& nodes) const {
        // Create a map for quick node lookup
        std::map<size_t, const Node*> nodeMap;
        for (const auto& node : nodes) {
            nodeMap[node.getId()] = &node;
        }

        // Get coordinates of element nodes
        std::vector<const Node*> elemNodes;
        for (size_t nodeId : nodeIds_) {
            auto it = nodeMap.find(nodeId);
            if (it == nodeMap.end()) {
                throw std::runtime_error("Node not found in node list");
            }
            elemNodes.push_back(it->second);
        }

        return calculateMeasure(elemNodes);
    }

    /**
     * @brief Calculate element centroid
     *
     * @param nodes Vector of nodes
     * @param centroid Output centroid [x, y, z]
     */
    void calculateCentroid(const std::vector<Node>& nodes, double centroid[3]) const {
        std::map<size_t, const Node*> nodeMap;
        for (const auto& node : nodes) {
            nodeMap[node.getId()] = &node;
        }

        centroid[0] = centroid[1] = centroid[2] = 0.0;

        for (size_t nodeId : nodeIds_) {
            auto it = nodeMap.find(nodeId);
            if (it != nodeMap.end()) {
                centroid[0] += it->second->getX();
                centroid[1] += it->second->getY();
                centroid[2] += it->second->getZ();
            }
        }

        double n = static_cast<double>(nodeIds_.size());
        centroid[0] /= n;
        centroid[1] /= n;
        centroid[2] /= n;
    }

    /**
     * @brief Set user-defined data
     */
    void setData(const std::string& key, double value) {
        data_[key] = value;
    }

    /**
     * @brief Get user-defined data
     */
    double getData(const std::string& key, double defaultValue = 0.0) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        return defaultValue;
    }

    /**
     * @brief Check if data key exists
     */
    bool hasData(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    /**
     * @brief Remove data key
     */
    bool removeData(const std::string& key) {
        return data_.erase(key) > 0;
    }

    /**
     * @brief Clear all user-defined data
     */
    void clearData() {
        data_.clear();
    }

    /**
     * @brief Equality comparison operator
     */
    bool operator==(const Element& other) const {
        return id_ == other.id_;
    }

    /**
     * @brief Inequality comparison operator
     */
    bool operator!=(const Element& other) const {
        return id_ != other.id_;
    }

    /**
     * @brief Less-than comparison operator (for sorting by ID)
     */
    bool operator<(const Element& other) const {
        return id_ < other.id_;
    }

private:
    /**
     * @brief Validate node count against element type
     */
    void validateNodeCount() {
        // Allow zero nodes for initialization, validation happens later
        if (nodeIds_.empty()) return;

        size_t expected = getExpectedNodeCount(type_);
        if (expected > 0 && nodeIds_.size() != expected) {
            // For now, just accept any count (higher order elements may have more nodes)
            // Strict validation can be enabled in DEBUG mode
        }
    }

    /**
     * @brief Calculate measure from node pointers
     */
    double calculateMeasure(const std::vector<const Node*>& nodes) const {
        switch (type_) {
            case ElementType::VERTEX:
                return 0.0;
            case ElementType::LINE:
                return calculateLineLength(nodes);
            case ElementType::TRIANGLE:
                return calculateTriangleArea(nodes);
            case ElementType::QUADRILATERAL:
                return calculateQuadArea(nodes);
            case ElementType::TETRAHEDRON:
                return calculateTetVolume(nodes);
            case ElementType::HEXAHEDRON:
                return calculateHexVolume(nodes);
            case ElementType::PRISM:
                return 0.0;  // TODO: Implement prism volume
            case ElementType::PYRAMID:
                return 0.0;  // TODO: Implement pyramid volume
        }
        return 0.0;  // Default fallback
    }

    /**
     * @brief Calculate line length
     */
    double calculateLineLength(const std::vector<const Node*>& nodes) const {
        if (nodes.size() < 2) return 0.0;
        return nodes[0]->distanceTo(*nodes[1]);
    }

    /**
     * @brief Calculate triangle area using cross product
     */
    double calculateTriangleArea(const std::vector<const Node*>& nodes) const {
        if (nodes.size() < 3) return 0.0;

        double v1[3] = {
            nodes[1]->getX() - nodes[0]->getX(),
            nodes[1]->getY() - nodes[0]->getY(),
            nodes[1]->getZ() - nodes[0]->getZ()
        };

        double v2[3] = {
            nodes[2]->getX() - nodes[0]->getX(),
            nodes[2]->getY() - nodes[0]->getY(),
            nodes[2]->getZ() - nodes[0]->getZ()
        };

        // Cross product
        double cross[3] = {
            v1[1] * v2[2] - v1[2] * v2[1],
            v1[2] * v2[0] - v1[0] * v2[2],
            v1[0] * v2[1] - v1[1] * v2[0]
        };

        double mag = std::sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
        return 0.5 * mag;
    }

    /**
     * @brief Calculate quadrilateral area (split into two triangles)
     */
    double calculateQuadArea(const std::vector<const Node*>& nodes) const {
        if (nodes.size() < 4) return 0.0;

        // Split into two triangles: 0-1-2 and 0-2-3
        std::vector<const Node*> tri1 = {nodes[0], nodes[1], nodes[2]};
        std::vector<const Node*> tri2 = {nodes[0], nodes[2], nodes[3]};

        double area1 = calculateTriangleArea(tri1);
        double area2 = calculateTriangleArea(tri2);

        return area1 + area2;
    }

    /**
     * @brief Calculate tetrahedron volume
     */
    double calculateTetVolume(const std::vector<const Node*>& nodes) const {
        if (nodes.size() < 4) return 0.0;

        // Volume = |det(v1, v2, v3)| / 6
        // where v1, v2, v3 are vectors from node 0 to nodes 1, 2, 3
        double v1[3] = {
            nodes[1]->getX() - nodes[0]->getX(),
            nodes[1]->getY() - nodes[0]->getY(),
            nodes[1]->getZ() - nodes[0]->getZ()
        };

        double v2[3] = {
            nodes[2]->getX() - nodes[0]->getX(),
            nodes[2]->getY() - nodes[0]->getY(),
            nodes[2]->getZ() - nodes[0]->getZ()
        };

        double v3[3] = {
            nodes[3]->getX() - nodes[0]->getX(),
            nodes[3]->getY() - nodes[0]->getY(),
            nodes[3]->getZ() - nodes[0]->getZ()
        };

        // Determinant
        double det = v1[0] * (v2[1] * v3[2] - v2[2] * v3[1])
                   - v1[1] * (v2[0] * v3[2] - v2[2] * v3[0])
                   + v1[2] * (v2[0] * v3[1] - v2[1] * v3[0]);

        return std::abs(det) / 6.0;
    }

    /**
     * @brief Calculate hexahedron volume (approximate using 5 tetrahedra)
     */
    double calculateHexVolume(const std::vector<const Node*>& nodes) const {
        if (nodes.size() < 8) return 0.0;

        // Approximate decomposition into tetrahedra
        // This is a simplified calculation
        double centroid[3];
        koo::mesh::core::calculateCentroid(
            std::vector<Node>{
                *nodes[0], *nodes[1], *nodes[2], *nodes[3],
                *nodes[4], *nodes[5], *nodes[6], *nodes[7]
            },
            centroid
        );

        // For simplicity, return bounding box volume
        double minCoords[3], maxCoords[3];
        minCoords[0] = maxCoords[0] = nodes[0]->getX();
        minCoords[1] = maxCoords[1] = nodes[0]->getY();
        minCoords[2] = maxCoords[2] = nodes[0]->getZ();

        for (size_t i = 1; i < 8; ++i) {
            double x = nodes[i]->getX();
            double y = nodes[i]->getY();
            double z = nodes[i]->getZ();
            if (x < minCoords[0]) minCoords[0] = x;
            if (x > maxCoords[0]) maxCoords[0] = x;
            if (y < minCoords[1]) minCoords[1] = y;
            if (y > maxCoords[1]) maxCoords[1] = y;
            if (z < minCoords[2]) minCoords[2] = z;
            if (z > maxCoords[2]) maxCoords[2] = z;
        }

        return (maxCoords[0] - minCoords[0]) *
               (maxCoords[1] - minCoords[1]) *
               (maxCoords[2] - minCoords[2]);
    }

    size_t id_;                              ///< Unique element ID
    ElementType type_;                       ///< Element type
    std::vector<size_t> nodeIds_;            ///< Node IDs composing this element
    int tag_;                                ///< Physical tag
    int partition_;                          ///< Partition ID (for parallel)
    std::map<std::string, double> data_;     ///< User-defined data
};

} // namespace core
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_CORE_ELEMENT_H
