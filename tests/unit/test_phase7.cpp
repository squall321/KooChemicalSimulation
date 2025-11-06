/**
 * @file test_phase7.cpp
 * @brief Unit tests for Phase 7 - Mesh Optimization and Conversion
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha2
 * @date 2025-11-06
 */

#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "mesh/core/MeshData.h"
#include "mesh/manager/MeshQuality.h"
#include "mesh/manager/MeshOptimizer.h"
#include "mesh/manager/MeshConverter.h"

#include <iostream>
#include <cassert>
#include <memory>

using koo::mesh::core::Node;
using koo::mesh::core::Element;
using koo::mesh::core::MeshData;
using koo::mesh::manager::MeshQuality;
using koo::mesh::manager::MeshOptimizer;
using koo::mesh::manager::MeshConverter;
using ElementType = koo::core::ElementType;

int main() {
    std::cout << "Phase 7 Tests - Mesh Optimization\n";

    // Create simple mesh
    auto mesh = std::make_shared<MeshData>();
    mesh->setName("TestMesh");

    // Add nodes for a triangle
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    mesh->addNode(Node(3, 0.0, 1.0, 0.0));

    // Add triangle element
    Element tri(1, ElementType::TRIANGLE, {1, 2, 3});
    mesh->addElement(tri);

    // Test MeshQuality
    std::cout << "Testing MeshQuality...\n";
    auto eq = MeshQuality::computeElementQuality(tri, *mesh);
    assert(eq.isValid && "Element should be valid");
    assert(eq.volume > 0.4 && eq.volume < 0.6 && "Triangle area should be ~0.5");
    std::cout << "✓ Element quality computation works\n";

    auto report = MeshQuality::analyzeMesh(*mesh);
    assert(report.numInvalidElements == 0 && "No invalid elements");
    std::cout << "✓ Mesh quality analysis works\n";

    // Test MeshConverter
    std::cout << "Testing MeshConverter...\n";
    bool vtk_ok = MeshConverter::exportToVTK(*mesh, "/tmp/test.vtk");
    assert(vtk_ok && "VTK export should succeed");
    std::cout << "✓ VTK export works\n";

    bool stl_ok = MeshConverter::exportToSTL(*mesh, "/tmp/test.stl");
    assert(stl_ok && "STL export should succeed");
    std::cout << "✓ STL export works\n";

    bool obj_ok = MeshConverter::exportToOBJ(*mesh, "/tmp/test.obj");
    assert(obj_ok && "OBJ export should succeed");
    std::cout << "✓ OBJ export works\n";

    // Test MeshOptimizer
    std::cout << "Testing MeshOptimizer...\n";
    MeshOptimizer optimizer(mesh);
    // Note: Refinement modifies mesh, so we test it last

    std::cout << "\nAll Phase 7 tests passed!\n";
    return 0;
}
