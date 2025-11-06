/**
 * @file MeshQuality.h
 * @brief Mesh quality assessment and metrics
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha2
 * @date 2025-11-06
 *
 * Provides metrics for assessing mesh quality including aspect ratio,
 * skewness, angles, and element validity checks.
 */

#ifndef KOO_MESH_MANAGER_MESH_QUALITY_H
#define KOO_MESH_MANAGER_MESH_QUALITY_H

#include "mesh/core/MeshData.h"
#include "mesh/core/Element.h"
#include "mesh/core/Node.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace koo {
namespace mesh {
namespace manager {

/**
 * @brief Quality metrics for a single element
 */
struct ElementQuality {
    double aspectRatio{0.0};      ///< Ratio of longest to shortest edge
    double skewness{0.0};         ///< Deviation from ideal shape (0=ideal, 1=degenerate)
    double volume{0.0};           ///< Element volume/area
    double minAngle{0.0};         ///< Minimum angle (degrees)
    double maxAngle{0.0};         ///< Maximum angle (degrees)
    bool isValid{true};           ///< Whether element is geometrically valid
};

/**
 * @brief Quality statistics for entire mesh
 */
struct MeshQualityReport {
    double minAspectRatio{0.0};
    double maxAspectRatio{0.0};
    double avgAspectRatio{0.0};

    double minSkewness{0.0};
    double maxSkewness{0.0};
    double avgSkewness{0.0};

    double minAngle{0.0};
    double maxAngle{0.0};

    size_t numInvalidElements{0};
    size_t numDegenerateElements{0};

    double qualityScore{0.0};     ///< Overall quality score (0-100)
};

/**
 * @brief Mesh quality analyzer
 */
class MeshQuality {
public:
    /**
     * @brief Compute quality metrics for a single element
     */
    static ElementQuality computeElementQuality(const core::Element& element,
                                               const core::MeshData& mesh) {
        ElementQuality quality;

        // Get element nodes
        std::vector<const core::Node*> nodes;
        for (size_t i = 0; i < element.getNumNodes(); ++i) {
            const core::Node* node = mesh.getNode(element.getNodeId(i));
            if (!node) {
                quality.isValid = false;
                return quality;
            }
            nodes.push_back(node);
        }

        // Compute metrics based on element type
        using ElementType = koo::core::ElementType;
        switch (element.getType()) {
            case ElementType::TRIANGLE:
                return computeTriangleQuality(nodes);
            case ElementType::QUADRILATERAL:
                return computeQuadQuality(nodes);
            case ElementType::TETRAHEDRON:
                return computeTetQuality(nodes);
            default:
                quality.isValid = true;
                quality.volume = element.calculateMeasure(mesh.getNodes());
                return quality;
        }
    }

    /**
     * @brief Analyze entire mesh quality
     */
    static MeshQualityReport analyzeMesh(const core::MeshData& mesh) {
        MeshQualityReport report;

        double sumAspectRatio = 0.0;
        double sumSkewness = 0.0;
        size_t count = 0;

        report.minAspectRatio = std::numeric_limits<double>::max();
        report.maxAspectRatio = 0.0;
        report.minSkewness = std::numeric_limits<double>::max();
        report.maxSkewness = 0.0;
        report.minAngle = std::numeric_limits<double>::max();
        report.maxAngle = 0.0;

        for (const auto& element : mesh.getElements()) {
            ElementQuality eq = computeElementQuality(element, mesh);

            if (!eq.isValid) {
                report.numInvalidElements++;
                continue;
            }

            if (eq.volume < 1e-12) {
                report.numDegenerateElements++;
                continue;
            }

            // Update statistics
            report.minAspectRatio = std::min(report.minAspectRatio, eq.aspectRatio);
            report.maxAspectRatio = std::max(report.maxAspectRatio, eq.aspectRatio);
            sumAspectRatio += eq.aspectRatio;

            report.minSkewness = std::min(report.minSkewness, eq.skewness);
            report.maxSkewness = std::max(report.maxSkewness, eq.skewness);
            sumSkewness += eq.skewness;

            if (eq.minAngle > 0.0) {
                report.minAngle = std::min(report.minAngle, eq.minAngle);
                report.maxAngle = std::max(report.maxAngle, eq.maxAngle);
            }

            count++;
        }

        if (count > 0) {
            report.avgAspectRatio = sumAspectRatio / count;
            report.avgSkewness = sumSkewness / count;

            // Compute overall quality score (simple heuristic)
            double aspectScore = std::max(0.0, 100.0 - (report.avgAspectRatio - 1.0) * 20.0);
            double skewnessScore = std::max(0.0, 100.0 - report.avgSkewness * 100.0);
            double angleScore = (report.minAngle > 20.0 && report.maxAngle < 140.0) ? 100.0 : 50.0;

            report.qualityScore = (aspectScore + skewnessScore + angleScore) / 3.0;
        }

        return report;
    }

private:
    /**
     * @brief Compute triangle quality
     */
    static ElementQuality computeTriangleQuality(const std::vector<const core::Node*>& nodes) {
        ElementQuality quality;

        if (nodes.size() < 3) {
            quality.isValid = false;
            return quality;
        }

        // Compute edge lengths
        double l1 = nodes[0]->distanceTo(*nodes[1]);
        double l2 = nodes[1]->distanceTo(*nodes[2]);
        double l3 = nodes[2]->distanceTo(*nodes[0]);

        // Aspect ratio
        double maxEdge = std::max({l1, l2, l3});
        double minEdge = std::min({l1, l2, l3});
        quality.aspectRatio = (minEdge > 1e-12) ? (maxEdge / minEdge) : 1000.0;

        // Area using Heron's formula
        double s = (l1 + l2 + l3) / 2.0;
        quality.volume = std::sqrt(std::max(0.0, s * (s - l1) * (s - l2) * (s - l3)));

        // Angles
        if (quality.volume > 1e-12) {
            double a1 = std::acos(std::clamp((l1*l1 + l3*l3 - l2*l2) / (2*l1*l3), -1.0, 1.0));
            double a2 = std::acos(std::clamp((l1*l1 + l2*l2 - l3*l3) / (2*l1*l2), -1.0, 1.0));
            double a3 = std::acos(std::clamp((l2*l2 + l3*l3 - l1*l1) / (2*l2*l3), -1.0, 1.0));

            quality.minAngle = std::min({a1, a2, a3}) * 180.0 / M_PI;
            quality.maxAngle = std::max({a1, a2, a3}) * 180.0 / M_PI;

            // Skewness (normalized)
            double idealAngle = 60.0;
            quality.skewness = std::max({
                std::abs(a1 * 180.0 / M_PI - idealAngle) / idealAngle,
                std::abs(a2 * 180.0 / M_PI - idealAngle) / idealAngle,
                std::abs(a3 * 180.0 / M_PI - idealAngle) / idealAngle
            });
        }

        return quality;
    }

    /**
     * @brief Compute quadrilateral quality
     */
    static ElementQuality computeQuadQuality(const std::vector<const core::Node*>& nodes) {
        ElementQuality quality;

        if (nodes.size() < 4) {
            quality.isValid = false;
            return quality;
        }

        // Compute edge lengths
        double l1 = nodes[0]->distanceTo(*nodes[1]);
        double l2 = nodes[1]->distanceTo(*nodes[2]);
        double l3 = nodes[2]->distanceTo(*nodes[3]);
        double l4 = nodes[3]->distanceTo(*nodes[0]);

        double maxEdge = std::max({l1, l2, l3, l4});
        double minEdge = std::min({l1, l2, l3, l4});
        quality.aspectRatio = (minEdge > 1e-12) ? (maxEdge / minEdge) : 1000.0;

        // Approximate area (split into two triangles)
        std::vector<const core::Node*> tri1 = {nodes[0], nodes[1], nodes[2]};
        std::vector<const core::Node*> tri2 = {nodes[0], nodes[2], nodes[3]};

        ElementQuality q1 = computeTriangleQuality(tri1);
        ElementQuality q2 = computeTriangleQuality(tri2);

        quality.volume = q1.volume + q2.volume;
        quality.skewness = std::max(q1.skewness, q2.skewness);

        return quality;
    }

    /**
     * @brief Compute tetrahedron quality
     */
    static ElementQuality computeTetQuality(const std::vector<const core::Node*>& nodes) {
        ElementQuality quality;

        if (nodes.size() < 4) {
            quality.isValid = false;
            return quality;
        }

        // Compute edge lengths
        double l01 = nodes[0]->distanceTo(*nodes[1]);
        double l02 = nodes[0]->distanceTo(*nodes[2]);
        double l03 = nodes[0]->distanceTo(*nodes[3]);
        double l12 = nodes[1]->distanceTo(*nodes[2]);
        double l13 = nodes[1]->distanceTo(*nodes[3]);
        double l23 = nodes[2]->distanceTo(*nodes[3]);

        double maxEdge = std::max({l01, l02, l03, l12, l13, l23});
        double minEdge = std::min({l01, l02, l03, l12, l13, l23});
        quality.aspectRatio = (minEdge > 1e-12) ? (maxEdge / minEdge) : 1000.0;

        // Volume calculation
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

        double det = v1[0] * (v2[1] * v3[2] - v2[2] * v3[1])
                   - v1[1] * (v2[0] * v3[2] - v2[2] * v3[0])
                   + v1[2] * (v2[0] * v3[1] - v2[1] * v3[0]);

        quality.volume = std::abs(det) / 6.0;

        // Simple skewness based on volume and edge lengths
        double avgEdge = (l01 + l02 + l03 + l12 + l13 + l23) / 6.0;
        double idealVolume = avgEdge * avgEdge * avgEdge / (6.0 * std::sqrt(2.0));
        quality.skewness = (idealVolume > 1e-12) ?
            std::abs(quality.volume - idealVolume) / idealVolume : 0.0;

        return quality;
    }
};

} // namespace manager
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_MANAGER_MESH_QUALITY_H
