/**
 * @file GmshLoader.h
 * @brief Gmsh file loader
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha1
 * @date 2025-11-06
 *
 * This file defines the GmshLoader class for loading mesh files in gmsh format.
 * Supports both gmsh API (if available) and manual parsing of .msh files.
 */

#ifndef KOO_MESH_LOADER_GMSH_LOADER_H
#define KOO_MESH_LOADER_GMSH_LOADER_H

#include "mesh/core/MeshData.h"
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

// Optional gmsh library support
#ifdef USE_GMSH
#include <gmsh.h>
#endif

namespace koo {
namespace mesh {
namespace loader {

// ============================================================================
// Gmsh Loader Exception
// ============================================================================

/**
 * @brief Exception for gmsh loading errors
 */
class GmshLoaderException : public std::runtime_error {
public:
    explicit GmshLoaderException(const std::string& message)
        : std::runtime_error("GmshLoader: " + message) {}
};

// ============================================================================
// Gmsh Loader Class
// ============================================================================

/**
 * @brief Loader for gmsh mesh files
 *
 * Supports loading .msh files (ASCII format 2.2 and 4.1).
 * If gmsh library is available (USE_GMSH), can also use gmsh API.
 */
class GmshLoader {
public:
    /**
     * @brief Load mesh from gmsh file
     *
     * @param filename Path to .msh file
     * @return MeshData containing loaded mesh
     */
    static std::shared_ptr<core::MeshData> loadFile(const std::string& filename) {
#ifdef USE_GMSH
        return loadWithGmshAPI(filename);
#else
        return loadManual(filename);
#endif
    }

    /**
     * @brief Load mesh manually (without gmsh library)
     *
     * Parses .msh file format version 2.2 (ASCII)
     *
     * @param filename Path to .msh file
     * @return MeshData containing loaded mesh
     */
    static std::shared_ptr<core::MeshData> loadManual(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw GmshLoaderException("Cannot open file: " + filename);
        }

        auto meshData = std::make_shared<core::MeshData>();
        meshData->setName(filename);

        std::string line;
        while (std::getline(file, line)) {
            if (line == "$MeshFormat") {
                parseMeshFormat(file);
            } else if (line == "$PhysicalNames") {
                parsePhysicalNames(file, meshData);
            } else if (line == "$Nodes") {
                parseNodes(file, meshData);
            } else if (line == "$Elements") {
                parseElements(file, meshData);
            }
        }

        // Validate mesh
        auto errors = meshData->validate();
        if (!errors.empty()) {
            std::string msg = "Mesh validation failed:\n";
            for (const auto& error : errors) {
                msg += "  - " + error + "\n";
            }
            throw GmshLoaderException(msg);
        }

        return meshData;
    }

#ifdef USE_GMSH
    /**
     * @brief Load mesh using gmsh API
     *
     * @param filename Path to mesh file
     * @return MeshData containing loaded mesh
     */
    static std::shared_ptr<core::MeshData> loadWithGmshAPI(const std::string& filename) {
        gmsh::initialize();
        gmsh::option::setNumber("General.Terminal", 0); // Suppress output

        try {
            gmsh::open(filename);

            auto meshData = std::make_shared<core::MeshData>();
            meshData->setName(filename);

            // Get nodes
            std::vector<std::size_t> nodeTags;
            std::vector<double> nodeCoords;
            std::vector<double> parametricCoords;

            gmsh::model::mesh::getNodes(nodeTags, nodeCoords, parametricCoords);

            for (size_t i = 0; i < nodeTags.size(); ++i) {
                size_t nodeId = nodeTags[i];
                double x = nodeCoords[i * 3];
                double y = nodeCoords[i * 3 + 1];
                double z = nodeCoords[i * 3 + 2];
                meshData->addNode(core::Node(nodeId, x, y, z));
            }

            // Get elements
            std::vector<int> elementTypes;
            std::vector<std::vector<std::size_t>> elementTags;
            std::vector<std::vector<std::size_t>> nodeTags_elem;

            gmsh::model::mesh::getElements(elementTypes, elementTags, nodeTags_elem);

            for (size_t i = 0; i < elementTypes.size(); ++i) {
                core::ElementType type = gmshTypeToElementType(elementTypes[i]);

                for (size_t j = 0; j < elementTags[i].size(); ++j) {
                    size_t elemId = elementTags[i][j];

                    // Get node IDs for this element
                    std::vector<size_t> nodeIds;
                    size_t numNodes = getNumNodesForGmshType(elementTypes[i]);
                    for (size_t k = 0; k < numNodes; ++k) {
                        nodeIds.push_back(nodeTags_elem[i][j * numNodes + k]);
                    }

                    meshData->addElement(core::Element(elemId, type, nodeIds));
                }
            }

            gmsh::finalize();
            return meshData;

        } catch (const std::exception& e) {
            gmsh::finalize();
            throw GmshLoaderException(std::string("Gmsh API error: ") + e.what());
        }
    }
#endif

private:
    /**
     * @brief Parse mesh format section
     */
    static void parseMeshFormat(std::ifstream& file) {
        std::string line;
        std::getline(file, line);

        std::istringstream iss(line);
        double version;
        int fileType, dataSize;
        iss >> version >> fileType >> dataSize;

        if (version < 2.0 || version >= 5.0) {
            throw GmshLoaderException("Unsupported mesh format version: " +
                                     std::to_string(version));
        }

        if (fileType != 0) {
            throw GmshLoaderException("Binary mesh files not supported");
        }

        // Read end marker
        std::getline(file, line);
        if (line != "$EndMeshFormat") {
            throw GmshLoaderException("Expected $EndMeshFormat");
        }
    }

    /**
     * @brief Parse physical names section
     */
    static void parsePhysicalNames(std::ifstream& file,
                                   std::shared_ptr<core::MeshData> meshData) {
        std::string line;
        std::getline(file, line);
        int numNames = std::stoi(line);

        for (int i = 0; i < numNames; ++i) {
            std::getline(file, line);
            std::istringstream iss(line);
            int dimension, tag;
            std::string name;
            iss >> dimension >> tag;
            std::getline(iss, name); // Rest of line is name
            // Remove quotes and trim
            name.erase(0, name.find_first_not_of(" \t\""));
            name.erase(name.find_last_not_of(" \t\"") + 1);
            meshData->addPhysicalGroup(tag, name);
        }

        std::getline(file, line);
        if (line != "$EndPhysicalNames") {
            throw GmshLoaderException("Expected $EndPhysicalNames");
        }
    }

    /**
     * @brief Parse nodes section
     */
    static void parseNodes(std::ifstream& file,
                          std::shared_ptr<core::MeshData> meshData) {
        std::string line;
        std::getline(file, line);
        int numNodes = std::stoi(line);

        for (int i = 0; i < numNodes; ++i) {
            std::getline(file, line);
            std::istringstream iss(line);
            size_t nodeId;
            double x, y, z;
            iss >> nodeId >> x >> y >> z;
            meshData->addNode(core::Node(nodeId, x, y, z));
        }

        std::getline(file, line);
        if (line != "$EndNodes") {
            throw GmshLoaderException("Expected $EndNodes");
        }
    }

    /**
     * @brief Parse elements section
     */
    static void parseElements(std::ifstream& file,
                             std::shared_ptr<core::MeshData> meshData) {
        std::string line;
        std::getline(file, line);
        int numElements = std::stoi(line);

        for (int i = 0; i < numElements; ++i) {
            std::getline(file, line);
            std::istringstream iss(line);

            size_t elemId;
            int elemType, numTags;
            iss >> elemId >> elemType >> numTags;

            // Read tags
            int physicalTag = 0;
            for (int j = 0; j < numTags; ++j) {
                int tag;
                iss >> tag;
                if (j == 0) physicalTag = tag; // First tag is physical
            }

            // Convert gmsh element type to our ElementType
            core::ElementType type = gmshTypeToElementType(elemType);

            // Read node IDs
            std::vector<size_t> nodeIds;
            size_t nodeId;
            while (iss >> nodeId) {
                nodeIds.push_back(nodeId);
            }

            meshData->addElement(core::Element(elemId, type, nodeIds, physicalTag));
        }

        std::getline(file, line);
        if (line != "$EndElements") {
            throw GmshLoaderException("Expected $EndElements");
        }
    }

    /**
     * @brief Convert gmsh element type to ElementType
     */
    static koo::core::ElementType gmshTypeToElementType(int gmshType) {
        using koo::core::ElementType;
        switch (gmshType) {
            case 1:  return ElementType::LINE;
            case 2:  return ElementType::TRIANGLE;
            case 3:  return ElementType::QUADRILATERAL;
            case 4:  return ElementType::TETRAHEDRON;
            case 5:  return ElementType::HEXAHEDRON;
            case 6:  return ElementType::PRISM;
            case 7:  return ElementType::PYRAMID;
            case 15: return ElementType::VERTEX;
            default:
                throw GmshLoaderException("Unsupported gmsh element type: " +
                                         std::to_string(gmshType));
        }
    }

#ifdef USE_GMSH
    /**
     * @brief Get number of nodes for gmsh element type
     */
    static size_t getNumNodesForGmshType(int gmshType) {
        switch (gmshType) {
            case 1:  return 2;  // Line
            case 2:  return 3;  // Triangle
            case 3:  return 4;  // Quad
            case 4:  return 4;  // Tet
            case 5:  return 8;  // Hex
            case 6:  return 6;  // Prism
            case 7:  return 5;  // Pyramid
            case 15: return 1;  // Point
            default: return 0;
        }
    }
#endif
};

} // namespace loader
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_LOADER_GMSH_LOADER_H
