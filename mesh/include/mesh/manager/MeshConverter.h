/**
 * @file MeshConverter.h
 * @brief Mesh format conversion utilities
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha2
 * @date 2025-11-06
 *
 * Provides conversion between different mesh formats.
 */

#ifndef KOO_MESH_MANAGER_MESH_CONVERTER_H
#define KOO_MESH_MANAGER_MESH_CONVERTER_H

#include "mesh/core/MeshData.h"
#include <string>
#include <fstream>
#include <sstream>

namespace koo {
namespace mesh {
namespace manager {

/**
 * @brief Supported mesh formats
 */
enum class MeshFormat {
    GMSH,              ///< Gmsh .msh format
    VTK,               ///< VTK legacy format
    STL,               ///< STL format (surface only)
    OBJ,               ///< Wavefront OBJ format
    PLY                ///< PLY format
};

/**
 * @brief Mesh format converter
 */
class MeshConverter {
public:
    /**
     * @brief Export mesh to VTK legacy format
     */
    static bool exportToVTK(const core::MeshData& mesh, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        // VTK header
        file << "# vtk DataFile Version 3.0\n";
        file << mesh.getName() << "\n";
        file << "ASCII\n";
        file << "DATASET UNSTRUCTURED_GRID\n";

        // Points
        file << "POINTS " << mesh.getNumNodes() << " double\n";
        for (const auto& node : mesh.getNodes()) {
            file << node.getX() << " " << node.getY() << " " << node.getZ() << "\n";
        }

        // Cells
        size_t totalSize = 0;
        for (const auto& elem : mesh.getElements()) {
            totalSize += 1 + elem.getNumNodes(); // 1 for count + node IDs
        }

        file << "\nCELLS " << mesh.getNumElements() << " " << totalSize << "\n";
        for (const auto& elem : mesh.getElements()) {
            file << elem.getNumNodes();
            for (size_t i = 0; i < elem.getNumNodes(); ++i) {
                file << " " << (elem.getNodeId(i) - 1); // VTK uses 0-based indexing
            }
            file << "\n";
        }

        // Cell types
        file << "\nCELL_TYPES " << mesh.getNumElements() << "\n";
        for (const auto& elem : mesh.getElements()) {
            file << getVTKCellType(elem.getType()) << "\n";
        }

        // Cell data (tags)
        file << "\nCELL_DATA " << mesh.getNumElements() << "\n";
        file << "SCALARS physical_tag int 1\n";
        file << "LOOKUP_TABLE default\n";
        for (const auto& elem : mesh.getElements()) {
            file << elem.getTag() << "\n";
        }

        file.close();
        return true;
    }

    /**
     * @brief Export surface mesh to STL format
     */
    static bool exportToSTL(const core::MeshData& mesh, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        file << "solid " << mesh.getName() << "\n";

        // Export only triangular surface elements
        for (const auto& elem : mesh.getElements()) {
            if (elem.getType() == koo::core::ElementType::TRIANGLE && elem.getNumNodes() >= 3) {
                const core::Node* n0 = mesh.getNode(elem.getNodeId(0));
                const core::Node* n1 = mesh.getNode(elem.getNodeId(1));
                const core::Node* n2 = mesh.getNode(elem.getNodeId(2));

                if (n0 && n1 && n2) {
                    // Compute normal
                    double v1[3] = {n1->getX() - n0->getX(), n1->getY() - n0->getY(), n1->getZ() - n0->getZ()};
                    double v2[3] = {n2->getX() - n0->getX(), n2->getY() - n0->getY(), n2->getZ() - n0->getZ()};

                    double normal[3] = {
                        v1[1] * v2[2] - v1[2] * v2[1],
                        v1[2] * v2[0] - v1[0] * v2[2],
                        v1[0] * v2[1] - v1[1] * v2[0]
                    };

                    double mag = std::sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
                    if (mag > 1e-12) {
                        normal[0] /= mag;
                        normal[1] /= mag;
                        normal[2] /= mag;
                    }

                    file << "  facet normal " << normal[0] << " " << normal[1] << " " << normal[2] << "\n";
                    file << "    outer loop\n";
                    file << "      vertex " << n0->getX() << " " << n0->getY() << " " << n0->getZ() << "\n";
                    file << "      vertex " << n1->getX() << " " << n1->getY() << " " << n1->getZ() << "\n";
                    file << "      vertex " << n2->getX() << " " << n2->getY() << " " << n2->getZ() << "\n";
                    file << "    endloop\n";
                    file << "  endfacet\n";
                }
            }
        }

        file << "endsolid " << mesh.getName() << "\n";
        file.close();
        return true;
    }

    /**
     * @brief Export to OBJ format
     */
    static bool exportToOBJ(const core::MeshData& mesh, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        file << "# " << mesh.getName() << "\n";
        file << "# Vertices: " << mesh.getNumNodes() << "\n";
        file << "# Faces: " << mesh.getNumElements() << "\n\n";

        // Vertices
        for (const auto& node : mesh.getNodes()) {
            file << "v " << node.getX() << " " << node.getY() << " " << node.getZ() << "\n";
        }

        file << "\n";

        // Faces (only triangles and quads)
        for (const auto& elem : mesh.getElements()) {
            if (elem.getType() == koo::core::ElementType::TRIANGLE ||
                elem.getType() == koo::core::ElementType::QUADRILATERAL) {

                file << "f";
                for (size_t i = 0; i < elem.getNumNodes(); ++i) {
                    file << " " << elem.getNodeId(i); // OBJ uses 1-based indexing
                }
                file << "\n";
            }
        }

        file.close();
        return true;
    }

    /**
     * @brief Convert mesh to different format
     */
    static bool convert(const core::MeshData& mesh, const std::string& outputFile,
                       MeshFormat format) {
        switch (format) {
            case MeshFormat::VTK:
                return exportToVTK(mesh, outputFile);
            case MeshFormat::STL:
                return exportToSTL(mesh, outputFile);
            case MeshFormat::OBJ:
                return exportToOBJ(mesh, outputFile);
            default:
                return false;
        }
    }

private:
    /**
     * @brief Get VTK cell type code
     */
    static int getVTKCellType(koo::core::ElementType type) {
        using ElementType = koo::core::ElementType;
        switch (type) {
            case ElementType::VERTEX:        return 1;
            case ElementType::LINE:          return 3;
            case ElementType::TRIANGLE:      return 5;
            case ElementType::QUADRILATERAL: return 9;
            case ElementType::TETRAHEDRON:   return 10;
            case ElementType::HEXAHEDRON:    return 12;
            case ElementType::PRISM:         return 13;
            case ElementType::PYRAMID:       return 14;
            default:                         return 0;
        }
    }
};

} // namespace manager
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_MANAGER_MESH_CONVERTER_H
