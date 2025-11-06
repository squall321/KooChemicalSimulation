#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "mesh/core/MeshData.h"
#include <iostream>
#include <cassert>

using koo::mesh::core::Node;
using koo::mesh::core::Element;
using koo::mesh::core::MeshData;
using ElementType = koo::core::ElementType;

int main() {
    std::cout << "Phase 6 Basic Tests\n";
    
    // Test Node
    Node n1(1, 0.0, 0.0, 0.0);
    assert(n1.getId() == 1);
    std::cout << "✓ Node works\n";
    
    // Test Element
    Element tri(1, ElementType::TRIANGLE, {1, 2, 3});
    assert(tri.getId() == 1);
    std::cout << "✓ Element works\n";
    
    // Test MeshData
    MeshData mesh;
    mesh.addNode(n1);
    assert(mesh.getNumNodes() == 1);
    std::cout << "✓ MeshData works\n";
    
    std::cout << "All Phase 6 tests passed!\n";
    return 0;
}
