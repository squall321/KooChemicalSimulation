/**
 * @file BCManager.h
 * @brief Boundary condition manager
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 *
 * Manages collections of boundary conditions and provides query capabilities.
 */

#ifndef KOO_MESH_BOUNDARY_BC_MANAGER_H
#define KOO_MESH_BOUNDARY_BC_MANAGER_H

#include "BoundaryCondition.h"
#include "DirichletBC.h"
#include "NeumannBC.h"
#include "RobinBC.h"
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace koo {
namespace mesh {
namespace boundary {

/**
 * @brief Boundary condition manager
 *
 * Organizes and manages multiple boundary conditions for a simulation.
 */
class BCManager {
public:
    using BCPtr = std::shared_ptr<BoundaryCondition>;

    BCManager() = default;

    /**
     * @brief Add a boundary condition
     * @throws std::runtime_error if BC with same name already exists
     */
    void addBC(BCPtr bc) {
        if (!bc) {
            throw std::runtime_error("Cannot add null boundary condition");
        }

        const std::string& name = bc->getName();
        if (bcs_.find(name) != bcs_.end()) {
            throw std::runtime_error("BC with name '" + name + "' already exists");
        }

        bcs_[name] = bc;

        // Index by type
        bcsByType_[bc->getType()].push_back(bc);

        // Index by tag
        bcsByTag_[bc->getPhysicalTag()].push_back(bc);

        // Index by field
        if (!bc->getFieldName().empty()) {
            bcsByField_[bc->getFieldName()].push_back(bc);
        }
    }

    /**
     * @brief Remove a boundary condition by name
     */
    bool removeBC(const std::string& name) {
        auto it = bcs_.find(name);
        if (it == bcs_.end()) {
            return false;
        }

        BCPtr bc = it->second;

        // Remove from type index
        auto& typeVec = bcsByType_[bc->getType()];
        typeVec.erase(std::remove(typeVec.begin(), typeVec.end(), bc), typeVec.end());

        // Remove from tag index
        auto& tagVec = bcsByTag_[bc->getPhysicalTag()];
        tagVec.erase(std::remove(tagVec.begin(), tagVec.end(), bc), tagVec.end());

        // Remove from field index
        if (!bc->getFieldName().empty()) {
            auto& fieldVec = bcsByField_[bc->getFieldName()];
            fieldVec.erase(std::remove(fieldVec.begin(), fieldVec.end(), bc), fieldVec.end());
        }

        bcs_.erase(it);
        return true;
    }

    /**
     * @brief Get BC by name
     */
    BCPtr getBC(const std::string& name) const {
        auto it = bcs_.find(name);
        return (it != bcs_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Get all BCs of a specific type
     */
    std::vector<BCPtr> getBCsByType(BCType type) const {
        auto it = bcsByType_.find(type);
        return (it != bcsByType_.end()) ? it->second : std::vector<BCPtr>();
    }

    /**
     * @brief Get all BCs for a physical tag
     */
    std::vector<BCPtr> getBCsByTag(int tag) const {
        auto it = bcsByTag_.find(tag);
        return (it != bcsByTag_.end()) ? it->second : std::vector<BCPtr>();
    }

    /**
     * @brief Get all BCs for a field
     */
    std::vector<BCPtr> getBCsByField(const std::string& field) const {
        auto it = bcsByField_.find(field);
        return (it != bcsByField_.end()) ? it->second : std::vector<BCPtr>();
    }

    /**
     * @brief Get all Dirichlet BCs
     */
    std::vector<std::shared_ptr<DirichletBC>> getDirichletBCs() const {
        std::vector<std::shared_ptr<DirichletBC>> result;
        auto bcs = getBCsByType(BCType::DIRICHLET);
        for (const auto& bc : bcs) {
            if (auto dbc = std::dynamic_pointer_cast<DirichletBC>(bc)) {
                result.push_back(dbc);
            }
        }
        return result;
    }

    /**
     * @brief Get all Neumann BCs
     */
    std::vector<std::shared_ptr<NeumannBC>> getNeumannBCs() const {
        std::vector<std::shared_ptr<NeumannBC>> result;
        auto bcs = getBCsByType(BCType::NEUMANN);
        for (const auto& bc : bcs) {
            if (auto nbc = std::dynamic_pointer_cast<NeumannBC>(bc)) {
                result.push_back(nbc);
            }
        }
        return result;
    }

    /**
     * @brief Get all Robin BCs
     */
    std::vector<std::shared_ptr<RobinBC>> getRobinBCs() const {
        std::vector<std::shared_ptr<RobinBC>> result;
        auto bcs = getBCsByType(BCType::ROBIN);
        for (const auto& bc : bcs) {
            if (auto rbc = std::dynamic_pointer_cast<RobinBC>(bc)) {
                result.push_back(rbc);
            }
        }
        return result;
    }

    /**
     * @brief Get all BCs
     */
    std::vector<BCPtr> getAllBCs() const {
        std::vector<BCPtr> result;
        result.reserve(bcs_.size());
        for (const auto& pair : bcs_) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * @brief Get number of BCs
     */
    size_t getNumBCs() const {
        return bcs_.size();
    }

    /**
     * @brief Clear all BCs
     */
    void clear() {
        bcs_.clear();
        bcsByType_.clear();
        bcsByTag_.clear();
        bcsByField_.clear();
    }

    /**
     * @brief Check if BC exists
     */
    bool hasBC(const std::string& name) const {
        return bcs_.find(name) != bcs_.end();
    }

    /**
     * @brief Enable/disable BC by name
     */
    bool setEnabled(const std::string& name, bool enabled) {
        auto bc = getBC(name);
        if (!bc) return false;
        bc->setEnabled(enabled);
        return true;
    }

    /**
     * @brief Get enabled BCs only
     */
    std::vector<BCPtr> getEnabledBCs() const {
        std::vector<BCPtr> result;
        for (const auto& pair : bcs_) {
            if (pair.second->isEnabled()) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    /**
     * @brief Validate BCs
     * @return List of validation errors (empty if valid)
     */
    std::vector<std::string> validate() const {
        std::vector<std::string> errors;

        // Check for conflicting BCs on same tag
        for (const auto& tagPair : bcsByTag_) {
            int tag = tagPair.first;
            const auto& bcs = tagPair.second;

            // Count enabled BCs per field on this tag
            std::map<std::string, int> bcCountPerField;
            for (const auto& bc : bcs) {
                if (bc->isEnabled()) {
                    bcCountPerField[bc->getFieldName()]++;
                }
            }

            // Warn if multiple BCs for same field on same tag
            for (const auto& fieldCount : bcCountPerField) {
                if (fieldCount.second > 1) {
                    errors.push_back("Multiple BCs for field '" + fieldCount.first +
                                   "' on tag " + std::to_string(tag));
                }
            }
        }

        return errors;
    }

    /**
     * @brief Print summary
     */
    std::string getSummary() const {
        std::string result = "Boundary Condition Manager Summary:\n";
        result += "  Total BCs: " + std::to_string(bcs_.size()) + "\n";

        // Count by type
        result += "  By type:\n";
        for (const auto& pair : bcsByType_) {
            result += "    " + typeToString(pair.first) + ": " +
                     std::to_string(pair.second.size()) + "\n";
        }

        // Count by field
        if (!bcsByField_.empty()) {
            result += "  By field:\n";
            for (const auto& pair : bcsByField_) {
                result += "    " + pair.first + ": " +
                         std::to_string(pair.second.size()) + "\n";
            }
        }

        // List all BCs
        result += "  All BCs:\n";
        for (const auto& pair : bcs_) {
            result += "    " + pair.second->toString() + "\n";
        }

        return result;
    }

private:
    std::map<std::string, BCPtr> bcs_;                    ///< BCs by name
    std::map<BCType, std::vector<BCPtr>> bcsByType_;      ///< BCs by type
    std::map<int, std::vector<BCPtr>> bcsByTag_;          ///< BCs by physical tag
    std::map<std::string, std::vector<BCPtr>> bcsByField_;///< BCs by field name

    /**
     * @brief Helper to convert BC type to string (defined in BoundaryCondition)
     */
    static std::string typeToString(BCType type) {
        switch (type) {
            case BCType::DIRICHLET: return "Dirichlet";
            case BCType::NEUMANN:   return "Neumann";
            case BCType::ROBIN:     return "Robin";
            case BCType::PERIODIC:  return "Periodic";
            case BCType::INTERFACE: return "Interface";
            default:                return "Unknown";
        }
    }
};

} // namespace boundary
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_BOUNDARY_BC_MANAGER_H
