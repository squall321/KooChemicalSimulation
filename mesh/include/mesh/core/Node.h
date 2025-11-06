/**
 * @file Node.h
 * @brief Mesh node data structure
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha1
 * @date 2025-11-06
 *
 * This file defines the Node class for representing mesh vertices.
 * Nodes store position, tags, and associated data.
 */

#ifndef KOO_MESH_CORE_NODE_H
#define KOO_MESH_CORE_NODE_H

#include "core/types/CommonTypes.h"
#include <cstddef>
#include <vector>
#include <map>
#include <string>

namespace koo {
namespace mesh {
namespace core {

// ============================================================================
// Node Class
// ============================================================================

/**
 * @brief Mesh node (vertex) representation
 *
 * A node represents a point in the mesh with spatial coordinates
 * and associated metadata.
 */
class Node {
public:
    /**
     * @brief Default constructor
     */
    Node() : id_(0), x_(0.0), y_(0.0), z_(0.0), tag_(0) {}

    /**
     * @brief Constructor with ID and coordinates
     *
     * @param id Node ID
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    Node(size_t id, double x, double y, double z = 0.0)
        : id_(id), x_(x), y_(y), z_(z), tag_(0) {}

    /**
     * @brief Constructor with ID, coordinates, and tag
     *
     * @param id Node ID
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param tag Physical tag (for boundary/region identification)
     */
    Node(size_t id, double x, double y, double z, int tag)
        : id_(id), x_(x), y_(y), z_(z), tag_(tag) {}

#ifdef USE_EIGEN
    /**
     * @brief Constructor with ID and Vector3D
     *
     * @param id Node ID
     * @param position Position vector
     */
    Node(size_t id, const koo::core::types::Vector3D& position)
        : id_(id), x_(position[0]), y_(position[1]), z_(position[2]), tag_(0) {}
#endif

    /**
     * @brief Get node ID
     */
    size_t getId() const { return id_; }

    /**
     * @brief Set node ID
     */
    void setId(size_t id) { id_ = id; }

    /**
     * @brief Get X coordinate
     */
    double getX() const { return x_; }

    /**
     * @brief Get Y coordinate
     */
    double getY() const { return y_; }

    /**
     * @brief Get Z coordinate
     */
    double getZ() const { return z_; }

    /**
     * @brief Set coordinates
     */
    void setCoordinates(double x, double y, double z = 0.0) {
        x_ = x;
        y_ = y;
        z_ = z;
    }

    /**
     * @brief Get coordinates as array
     */
    void getCoordinates(double coords[3]) const {
        coords[0] = x_;
        coords[1] = y_;
        coords[2] = z_;
    }

#ifdef USE_EIGEN
    /**
     * @brief Get position as Vector3D
     */
    koo::core::types::Vector3D getPosition() const {
        return koo::core::types::Vector3D(x_, y_, z_);
    }

    /**
     * @brief Set position from Vector3D
     */
    void setPosition(const koo::core::types::Vector3D& position) {
        x_ = position[0];
        y_ = position[1];
        z_ = position[2];
    }
#endif

    /**
     * @brief Get physical tag
     *
     * Physical tags are used in gmsh to identify boundaries,
     * regions, or material properties.
     */
    int getTag() const { return tag_; }

    /**
     * @brief Set physical tag
     */
    void setTag(int tag) { tag_ = tag; }

    /**
     * @brief Check if node has a specific tag
     */
    bool hasTag(int tag) const { return tag_ == tag; }

    /**
     * @brief Calculate distance to another node
     */
    double distanceTo(const Node& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        double dz = z_ - other.z_;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    /**
     * @brief Calculate squared distance to another node (faster)
     */
    double distanceSquaredTo(const Node& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        double dz = z_ - other.z_;
        return dx*dx + dy*dy + dz*dz;
    }

    /**
     * @brief Check if node is approximately equal to another
     *
     * @param other Other node
     * @param tolerance Tolerance for comparison
     */
    bool isApproximatelyEqual(const Node& other, double tolerance = 1e-10) const {
        return distanceSquaredTo(other) < tolerance * tolerance;
    }

    /**
     * @brief Set user-defined data
     *
     * Allows attaching arbitrary double values to nodes.
     *
     * @param key Data key
     * @param value Data value
     */
    void setData(const std::string& key, double value) {
        data_[key] = value;
    }

    /**
     * @brief Get user-defined data
     *
     * @param key Data key
     * @param defaultValue Default value if key not found
     * @return Data value
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
     * @brief Get all data keys
     */
    std::vector<std::string> getDataKeys() const {
        std::vector<std::string> keys;
        keys.reserve(data_.size());
        for (const auto& pair : data_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    /**
     * @brief Equality comparison operator
     */
    bool operator==(const Node& other) const {
        return id_ == other.id_;
    }

    /**
     * @brief Inequality comparison operator
     */
    bool operator!=(const Node& other) const {
        return id_ != other.id_;
    }

    /**
     * @brief Less-than comparison operator (for sorting by ID)
     */
    bool operator<(const Node& other) const {
        return id_ < other.id_;
    }

private:
    size_t id_;                              ///< Unique node ID
    double x_;                               ///< X coordinate
    double y_;                               ///< Y coordinate
    double z_;                               ///< Z coordinate
    int tag_;                                ///< Physical tag
    std::map<std::string, double> data_;     ///< User-defined data
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Calculate centroid of a set of nodes
 *
 * @param nodes Vector of nodes
 * @param centroid Output centroid coordinates [x, y, z]
 */
inline void calculateCentroid(const std::vector<Node>& nodes, double centroid[3]) {
    centroid[0] = 0.0;
    centroid[1] = 0.0;
    centroid[2] = 0.0;

    if (nodes.empty()) return;

    for (const auto& node : nodes) {
        centroid[0] += node.getX();
        centroid[1] += node.getY();
        centroid[2] += node.getZ();
    }

    double n = static_cast<double>(nodes.size());
    centroid[0] /= n;
    centroid[1] /= n;
    centroid[2] /= n;
}

/**
 * @brief Calculate bounding box of a set of nodes
 *
 * @param nodes Vector of nodes
 * @param min Output minimum coordinates [x, y, z]
 * @param max Output maximum coordinates [x, y, z]
 */
inline void calculateBoundingBox(const std::vector<Node>& nodes,
                                 double min[3], double max[3]) {
    if (nodes.empty()) {
        min[0] = min[1] = min[2] = 0.0;
        max[0] = max[1] = max[2] = 0.0;
        return;
    }

    min[0] = max[0] = nodes[0].getX();
    min[1] = max[1] = nodes[0].getY();
    min[2] = max[2] = nodes[0].getZ();

    for (size_t i = 1; i < nodes.size(); ++i) {
        double x = nodes[i].getX();
        double y = nodes[i].getY();
        double z = nodes[i].getZ();

        if (x < min[0]) min[0] = x;
        if (x > max[0]) max[0] = x;
        if (y < min[1]) min[1] = y;
        if (y > max[1]) max[1] = y;
        if (z < min[2]) min[2] = z;
        if (z > max[2]) max[2] = z;
    }
}

} // namespace core
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_CORE_NODE_H
