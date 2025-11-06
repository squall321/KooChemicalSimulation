/**
 * @file MeshManager.h
 * @brief Integrated mesh management system
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-beta
 * @date 2025-11-06
 *
 * Provides a unified interface for mesh operations, including loading,
 * optimization, boundary conditions, and domain management.
 */

#ifndef KOO_MESH_MESH_MANAGER_H
#define KOO_MESH_MESH_MANAGER_H

#include "mesh/core/MeshData.h"
#include "mesh/loader/GmshLoader.h"
#include "mesh/manager/MeshQuality.h"
#include "mesh/manager/MeshOptimizer.h"
#include "mesh/manager/MeshConverter.h"
#include "mesh/boundary/BCManager.h"
#include "mesh/domain/DomainManager.h"

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace koo {
namespace mesh {

/**
 * @brief Mesh manager status
 */
enum class MeshStatus {
    EMPTY,          ///< No mesh loaded
    LOADED,         ///< Mesh loaded successfully
    OPTIMIZED,      ///< Mesh has been optimized
    READY           ///< Mesh ready for simulation (BCs and domains set)
};

/**
 * @brief Integrated mesh manager
 *
 * Central manager for all mesh operations, providing a unified interface
 * for loading, optimization, boundary conditions, and domain management.
 */
class MeshManager {
public:
    MeshManager()
        : meshData_(std::make_shared<core::MeshData>()),
          bcManager_(std::make_shared<boundary::BCManager>()),
          domainManager_(std::make_shared<domain::DomainManager>()),
          status_(MeshStatus::EMPTY) {}

    /**
     * @brief Load mesh from file
     * @param filename Path to mesh file
     * @return True if successful
     */
    bool loadMesh(const std::string& filename) {
        try {
            meshData_ = loader::GmshLoader::loadFile(filename);
            if (!meshData_) {
                return false;
            }

            status_ = MeshStatus::LOADED;

            // Build domains from mesh physical groups
            domainManager_->buildFromMesh(*meshData_);

            return true;
        } catch (const std::exception& e) {
            lastError_ = std::string("Failed to load mesh: ") + e.what();
            return false;
        }
    }

    /**
     * @brief Get mesh data
     */
    std::shared_ptr<core::MeshData> getMesh() const {
        return meshData_;
    }

    /**
     * @brief Get mesh data (non-const)
     */
    std::shared_ptr<core::MeshData> getMesh() {
        return meshData_;
    }

    /**
     * @brief Set mesh data
     */
    void setMesh(std::shared_ptr<core::MeshData> mesh) {
        meshData_ = mesh;
        if (meshData_ && meshData_->getNumElements() > 0) {
            status_ = MeshStatus::LOADED;
            domainManager_->buildFromMesh(*meshData_);
        } else {
            status_ = MeshStatus::EMPTY;
        }
    }

    /**
     * @brief Get boundary condition manager
     */
    std::shared_ptr<boundary::BCManager> getBCManager() const {
        return bcManager_;
    }

    /**
     * @brief Get domain manager
     */
    std::shared_ptr<domain::DomainManager> getDomainManager() const {
        return domainManager_;
    }

    /**
     * @brief Get mesh status
     */
    MeshStatus getStatus() const {
        return status_;
    }

    /**
     * @brief Check if mesh is loaded
     */
    bool isLoaded() const {
        return status_ != MeshStatus::EMPTY && meshData_ && meshData_->getNumElements() > 0;
    }

    /**
     * @brief Check if mesh is ready for simulation
     */
    bool isReady() const {
        return status_ == MeshStatus::READY;
    }

    /**
     * @brief Mark mesh as ready
     */
    void markReady() {
        status_ = MeshStatus::READY;
    }

    /**
     * @brief Get last error message
     */
    const std::string& getLastError() const {
        return lastError_;
    }

    // === Mesh Quality Operations ===

    /**
     * @brief Compute mesh quality
     */
    manager::MeshQualityReport computeQuality() const {
        if (!meshData_) {
            return manager::MeshQualityReport();
        }
        return manager::MeshQuality::analyzeMesh(*meshData_);
    }

    /**
     * @brief Get quality score (0-100)
     */
    double getQualityScore() const {
        auto report = computeQuality();
        return report.qualityScore;
    }

    /**
     * @brief Check if mesh quality is acceptable
     * @param threshold Minimum quality score (default 50.0)
     */
    bool hasAcceptableQuality(double threshold = 50.0) const {
        return getQualityScore() >= threshold;
    }

    // === Mesh Optimization Operations ===

    /**
     * @brief Refine mesh uniformly
     * @param maxLevel Maximum refinement level
     * @return Number of elements added
     */
    size_t refineUniform(int maxLevel = 1) {
        if (!meshData_) {
            return 0;
        }

        manager::MeshOptimizer optimizer(meshData_);
        size_t added = optimizer.refineUniform(maxLevel);

        if (added > 0) {
            status_ = MeshStatus::OPTIMIZED;
        }

        return added;
    }

    /**
     * @brief Refine mesh based on quality
     * @param qualityThreshold Minimum quality threshold
     * @return Number of elements refined
     */
    size_t refineByQuality(double qualityThreshold = 0.3) {
        if (!meshData_) {
            return 0;
        }

        manager::MeshOptimizer optimizer(meshData_);
        size_t refined = optimizer.refineByQuality(qualityThreshold);

        if (refined > 0) {
            status_ = MeshStatus::OPTIMIZED;
        }

        return refined;
    }

    /**
     * @brief Smooth mesh
     * @param iterations Number of smoothing iterations
     * @param relaxation Relaxation factor (0-1)
     */
    void smoothMesh(int iterations = 5, double relaxation = 0.5) {
        if (!meshData_) {
            return;
        }

        manager::MeshOptimizer optimizer(meshData_);
        optimizer.smooth(iterations, relaxation);

        status_ = MeshStatus::OPTIMIZED;
    }

    /**
     * @brief Optimize mesh (combined refinement and smoothing)
     * @return Quality improvement score
     */
    double optimizeMesh() {
        if (!meshData_) {
            return 0.0;
        }

        manager::MeshOptimizer optimizer(meshData_);
        double improvement = optimizer.optimize();

        if (improvement > 0.0) {
            status_ = MeshStatus::OPTIMIZED;
        }

        return improvement;
    }

    // === Mesh Export Operations ===

    /**
     * @brief Export mesh to file
     * @param filename Output filename
     * @param format File format
     * @return True if successful
     */
    bool exportMesh(const std::string& filename, manager::MeshFormat format) const {
        if (!meshData_) {
            return false;
        }

        try {
            switch (format) {
                case manager::MeshFormat::VTK:
                    return manager::MeshConverter::exportToVTK(*meshData_, filename);
                case manager::MeshFormat::STL:
                    return manager::MeshConverter::exportToSTL(*meshData_, filename);
                case manager::MeshFormat::OBJ:
                    return manager::MeshConverter::exportToOBJ(*meshData_, filename);
                default:
                    return false;
            }
        } catch (const std::exception& e) {
            lastError_ = std::string("Failed to export mesh: ") + e.what();
            return false;
        }
    }

    /**
     * @brief Export mesh to VTK format
     */
    bool exportToVTK(const std::string& filename) const {
        return exportMesh(filename, manager::MeshFormat::VTK);
    }

    /**
     * @brief Export mesh to STL format
     */
    bool exportToSTL(const std::string& filename) const {
        return exportMesh(filename, manager::MeshFormat::STL);
    }

    /**
     * @brief Export mesh to OBJ format
     */
    bool exportToOBJ(const std::string& filename) const {
        return exportMesh(filename, manager::MeshFormat::OBJ);
    }

    // === Boundary Condition Operations ===

    /**
     * @brief Add boundary condition
     */
    void addBC(std::shared_ptr<boundary::BoundaryCondition> bc) {
        bcManager_->addBC(bc);
    }

    /**
     * @brief Get boundary condition by name
     */
    std::shared_ptr<boundary::BoundaryCondition> getBC(const std::string& name) const {
        return bcManager_->getBC(name);
    }

    /**
     * @brief Validate boundary conditions
     */
    std::vector<std::string> validateBCs() const {
        return bcManager_->validate();
    }

    /**
     * @brief Check if all BCs are valid
     */
    bool hasValidBCs() const {
        return validateBCs().empty();
    }

    // === Domain Operations ===

    /**
     * @brief Create domain
     */
    std::shared_ptr<domain::Domain> createDomain(
        const std::string& name,
        domain::DomainDimension dimension,
        domain::DomainType type) {
        return domainManager_->createDomain(name, dimension, type);
    }

    /**
     * @brief Get domain by name
     */
    std::shared_ptr<domain::Domain> getDomain(const std::string& name) const {
        return domainManager_->getDomainByName(name);
    }

    /**
     * @brief Get domain by ID
     */
    std::shared_ptr<domain::Domain> getDomain(int id) const {
        return domainManager_->getDomain(id);
    }

    /**
     * @brief Decompose domain into subdomains
     */
    std::vector<std::shared_ptr<domain::SubDomain>> decomposeDomain(
        int domainId,
        int numSubdomains,
        domain::PartitionStrategy strategy = domain::PartitionStrategy::UNIFORM) {
        return domainManager_->decomposeDomain(domainId, numSubdomains, strategy);
    }

    // === Statistics and Information ===

    /**
     * @brief Get mesh statistics
     */
    core::MeshData::Statistics getStatistics() const {
        if (!meshData_) {
            return core::MeshData::Statistics();
        }
        return meshData_->getStatistics();
    }

    /**
     * @brief Get comprehensive summary
     */
    std::string getSummary() const {
        std::string result = "=== Mesh Manager Summary ===\n\n";

        // Status
        result += "Status: " + statusToString(status_) + "\n";

        // Mesh statistics
        if (meshData_ && meshData_->getNumElements() > 0) {
            auto stats = meshData_->getStatistics();
            result += "\nMesh Statistics:\n";
            result += "  Nodes: " + std::to_string(stats.numNodes) + "\n";
            result += "  Elements: " + std::to_string(stats.numElements) + "\n";
            result += "  Triangles: " + std::to_string(stats.numTriangles) + "\n";
            result += "  Quadrilaterals: " + std::to_string(stats.numQuads) + "\n";
            result += "  Tetrahedra: " + std::to_string(stats.numTets) + "\n";
            result += "  Hexahedra: " + std::to_string(stats.numHexs) + "\n";
        }

        // Quality
        if (isLoaded()) {
            auto quality = computeQuality();
            result += "\nMesh Quality:\n";
            result += "  Quality Score: " + std::to_string(quality.qualityScore) + "/100\n";
            result += "  Invalid Elements: " + std::to_string(quality.numInvalidElements) + "\n";
        }

        // Boundary conditions
        result += "\nBoundary Conditions: " + std::to_string(bcManager_->getNumBCs()) + "\n";

        // Domains
        result += "Domains: " + std::to_string(domainManager_->getNumDomains()) + "\n";
        result += "SubDomains: " + std::to_string(domainManager_->getNumSubDomains()) + "\n";

        return result;
    }

    /**
     * @brief Print summary to output stream
     */
    void printSummary() const {
        std::cout << getSummary();
    }

    /**
     * @brief Validate entire mesh setup
     * @return List of validation errors
     */
    std::vector<std::string> validate() const {
        std::vector<std::string> errors;

        // Check if mesh is loaded
        if (!isLoaded()) {
            errors.push_back("No mesh loaded");
            return errors;
        }

        // Validate mesh data
        auto meshErrors = meshData_->validate();
        errors.insert(errors.end(), meshErrors.begin(), meshErrors.end());

        // Validate boundary conditions
        auto bcErrors = bcManager_->validate();
        errors.insert(errors.end(), bcErrors.begin(), bcErrors.end());

        // Check mesh quality
        if (!hasAcceptableQuality(30.0)) {
            errors.push_back("Mesh quality is poor (score < 30)");
        }

        return errors;
    }

    /**
     * @brief Clear all data
     */
    void clear() {
        meshData_ = std::make_shared<core::MeshData>();
        bcManager_ = std::make_shared<boundary::BCManager>();
        domainManager_ = std::make_shared<domain::DomainManager>();
        status_ = MeshStatus::EMPTY;
        lastError_.clear();
    }

private:
    std::shared_ptr<core::MeshData> meshData_;              ///< Mesh data
    std::shared_ptr<boundary::BCManager> bcManager_;        ///< Boundary condition manager
    std::shared_ptr<domain::DomainManager> domainManager_;  ///< Domain manager
    MeshStatus status_;                                     ///< Current status
    mutable std::string lastError_;                         ///< Last error message

    /**
     * @brief Convert status to string
     */
    static std::string statusToString(MeshStatus status) {
        switch (status) {
            case MeshStatus::EMPTY:     return "Empty";
            case MeshStatus::LOADED:    return "Loaded";
            case MeshStatus::OPTIMIZED: return "Optimized";
            case MeshStatus::READY:     return "Ready";
            default:                    return "Unknown";
        }
    }
};

} // namespace mesh
} // namespace koo

#endif // KOO_MESH_MESH_MANAGER_H
