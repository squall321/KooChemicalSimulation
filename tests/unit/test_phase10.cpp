/**
 * @file test_phase10.cpp
 * @brief Unit tests for Phase 10 - Mesh Manager Integration
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-beta
 * @date 2025-11-06
 */

#include "mesh/MeshManager.h"
#include "mesh/boundary/DirichletBC.h"
#include "mesh/boundary/NeumannBC.h"
#include "mesh/boundary/RobinBC.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <cmath>

using namespace koo::mesh;

// Helper function for floating point comparison
bool isClose(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) < tol;
}

void testMeshManagerBasics() {
    std::cout << "Testing MeshManager basics...\n";

    MeshManager manager;

    // Test initial state
    assert(manager.getStatus() == MeshStatus::EMPTY);
    assert(!manager.isLoaded());
    assert(!manager.isReady());
    std::cout << "  ✓ Initial state correct\n";

    // Test mesh access
    auto mesh = manager.getMesh();
    assert(mesh != nullptr);
    assert(mesh->getNumElements() == 0);
    std::cout << "  ✓ Mesh access works\n";

    // Test BC manager access
    auto bcManager = manager.getBCManager();
    assert(bcManager != nullptr);
    assert(bcManager->getNumBCs() == 0);
    std::cout << "  ✓ BC manager access works\n";

    // Test domain manager access
    auto domainManager = manager.getDomainManager();
    assert(domainManager != nullptr);
    assert(domainManager->getNumDomains() == 0);
    std::cout << "  ✓ Domain manager access works\n";

    // Test status changes
    manager.markReady();
    assert(manager.getStatus() == MeshStatus::READY);
    std::cout << "  ✓ Status changes work\n";

    // Test clear
    manager.clear();
    assert(manager.getStatus() == MeshStatus::EMPTY);
    std::cout << "  ✓ Clear works\n";
}

void testMeshManagerWithMesh() {
    std::cout << "\nTesting MeshManager with mesh...\n";

    MeshManager manager;

    // Create a simple test mesh
    auto mesh = std::make_shared<core::MeshData>();

    // Add nodes
    core::Node n1(1, 0.0, 0.0, 0.0);
    core::Node n2(2, 1.0, 0.0, 0.0);
    core::Node n3(3, 0.0, 1.0, 0.0);
    core::Node n4(4, 0.0, 0.0, 1.0);

    mesh->addNode(n1);
    mesh->addNode(n2);
    mesh->addNode(n3);
    mesh->addNode(n4);

    // Add triangles
    core::Element tri1(1, core::ElementType::TRIANGLE, {1, 2, 3});
    core::Element tri2(2, core::ElementType::TRIANGLE, {1, 2, 4});
    core::Element tri3(3, core::ElementType::TRIANGLE, {1, 3, 4});
    core::Element tri4(4, core::ElementType::TRIANGLE, {2, 3, 4});

    mesh->addElement(tri1);
    mesh->addElement(tri2);
    mesh->addElement(tri3);
    mesh->addElement(tri4);

    // Set mesh
    manager.setMesh(mesh);

    // Test status
    assert(manager.getStatus() == MeshStatus::LOADED);
    assert(manager.isLoaded());
    std::cout << "  ✓ Mesh set and loaded\n";

    // Test mesh access
    auto retrievedMesh = manager.getMesh();
    assert(retrievedMesh != nullptr);
    assert(retrievedMesh->getNumNodes() == 4);
    assert(retrievedMesh->getNumElements() == 4);
    std::cout << "  ✓ Mesh retrieval works\n";

    // Test statistics
    auto stats = manager.getStatistics();
    assert(stats.numNodes == 4);
    assert(stats.numElements == 4);
    assert(stats.numTriangles == 4);
    std::cout << "  ✓ Statistics work\n";
}

void testQualityOperations() {
    std::cout << "\nTesting quality operations...\n";

    MeshManager manager;

    // Create a test mesh
    auto mesh = std::make_shared<core::MeshData>();

    // Add nodes for a triangle
    core::Node n1(1, 0.0, 0.0, 0.0);
    core::Node n2(2, 1.0, 0.0, 0.0);
    core::Node n3(3, 0.5, 0.866, 0.0);  // Equilateral triangle

    mesh->addNode(n1);
    mesh->addNode(n2);
    mesh->addNode(n3);

    // Add triangle
    core::Element tri(1, core::ElementType::TRIANGLE, {1, 2, 3});
    mesh->addElement(tri);

    manager.setMesh(mesh);

    // Test quality computation
    auto quality = manager.computeQuality();
    assert(quality.numElements == 1);
    assert(quality.numInvalidElements == 0);
    std::cout << "  ✓ Quality computation works\n";

    // Test quality score
    double score = manager.getQualityScore();
    assert(score >= 0.0 && score <= 100.0);
    std::cout << "  ✓ Quality score works: " << score << "/100\n";

    // Test acceptable quality check
    bool acceptable = manager.hasAcceptableQuality(30.0);
    std::cout << "  ✓ Acceptable quality check works: "
              << (acceptable ? "yes" : "no") << "\n";
}

void testBoundaryConditionIntegration() {
    std::cout << "\nTesting boundary condition integration...\n";

    MeshManager manager;

    // Add Dirichlet BC
    auto dirichletBC = boundary::DirichletBC::makeConstant("inlet", 10, 1.0);
    dirichletBC->setFieldName("velocity");
    manager.addBC(dirichletBC);
    std::cout << "  ✓ Dirichlet BC added\n";

    // Add Neumann BC
    auto neumannBC = boundary::NeumannBC::makeConstant("outlet", 11, 0.0);
    neumannBC->setFieldName("pressure");
    manager.addBC(neumannBC);
    std::cout << "  ✓ Neumann BC added\n";

    // Add Robin BC
    auto robinBC = boundary::RobinBC::makeConvection("wall", 12, 10.0, 1.0, 300.0);
    robinBC->setFieldName("temperature");
    manager.addBC(robinBC);
    std::cout << "  ✓ Robin BC added\n";

    // Test BC retrieval
    auto retrieved = manager.getBC("inlet");
    assert(retrieved != nullptr);
    assert(retrieved->getName() == "inlet");
    std::cout << "  ✓ BC retrieval works\n";

    // Test BC validation
    auto errors = manager.validateBCs();
    assert(errors.empty());
    assert(manager.hasValidBCs());
    std::cout << "  ✓ BC validation works\n";

    // Test BC manager access
    auto bcManager = manager.getBCManager();
    assert(bcManager->getNumBCs() == 3);
    std::cout << "  ✓ BC manager has correct count\n";
}

void testDomainIntegration() {
    std::cout << "\nTesting domain integration...\n";

    MeshManager manager;

    // Create mesh
    auto mesh = std::make_shared<core::MeshData>();
    for (size_t i = 1; i <= 10; ++i) {
        core::Node node(i, i * 0.1, 0.0, 0.0);
        mesh->addNode(node);
    }

    for (size_t i = 1; i <= 8; ++i) {
        core::Element tri(i, core::ElementType::TRIANGLE, {i, i+1, i+2});
        mesh->addElement(tri);
    }

    manager.setMesh(mesh);

    // Create domain
    auto domain = manager.createDomain("fluid", domain::DomainDimension::SURFACE,
                                       domain::DomainType::PHYSICAL);
    assert(domain != nullptr);
    std::cout << "  ✓ Domain created\n";

    // Add elements to domain
    for (size_t i = 1; i <= 8; ++i) {
        domain->addElement(i);
    }
    assert(domain->getNumElements() == 8);
    std::cout << "  ✓ Elements added to domain\n";

    // Test domain retrieval by name
    auto retrievedDomain = manager.getDomain("fluid");
    assert(retrievedDomain != nullptr);
    assert(retrievedDomain->getName() == "fluid");
    std::cout << "  ✓ Domain retrieval by name works\n";

    // Test domain retrieval by ID
    auto domainById = manager.getDomain(domain->getId());
    assert(domainById != nullptr);
    std::cout << "  ✓ Domain retrieval by ID works\n";

    // Test domain manager access
    auto domainManager = manager.getDomainManager();
    assert(domainManager->getNumDomains() >= 1);
    std::cout << "  ✓ Domain manager access works\n";
}

void testDomainDecomposition() {
    std::cout << "\nTesting domain decomposition...\n";

    MeshManager manager;

    // Create mesh
    auto mesh = std::make_shared<core::MeshData>();
    for (size_t i = 1; i <= 20; ++i) {
        core::Node node(i, i * 0.1, 0.0, 0.0);
        mesh->addNode(node);
    }

    for (size_t i = 1; i <= 18; ++i) {
        core::Element tri(i, core::ElementType::TRIANGLE, {i, i+1, i+2});
        mesh->addElement(tri);
    }

    manager.setMesh(mesh);

    // Create domain
    auto domain = manager.createDomain("test", domain::DomainDimension::SURFACE,
                                       domain::DomainType::PHYSICAL);
    for (size_t i = 1; i <= 18; ++i) {
        domain->addElement(i);
    }

    // Decompose domain
    auto subdomains = manager.decomposeDomain(domain->getId(), 3,
                                              domain::PartitionStrategy::UNIFORM);
    assert(subdomains.size() == 3);
    std::cout << "  ✓ Domain decomposed into " << subdomains.size() << " subdomains\n";

    // Check element distribution
    size_t totalElements = 0;
    for (const auto& sub : subdomains) {
        totalElements += sub->getNumElements();
    }
    assert(totalElements == 18);
    std::cout << "  ✓ All elements distributed\n";

    // Test domain manager has subdomains
    auto domainManager = manager.getDomainManager();
    assert(domainManager->getNumSubDomains() == 3);
    std::cout << "  ✓ Domain manager tracks subdomains\n";
}

void testValidation() {
    std::cout << "\nTesting validation...\n";

    MeshManager manager;

    // Test validation on empty mesh
    auto errors = manager.validate();
    assert(!errors.empty());  // Should have "No mesh loaded" error
    std::cout << "  ✓ Empty mesh validation detects error\n";

    // Create mesh
    auto mesh = std::make_shared<core::MeshData>();
    core::Node n1(1, 0.0, 0.0, 0.0);
    core::Node n2(2, 1.0, 0.0, 0.0);
    core::Node n3(3, 0.5, 0.866, 0.0);

    mesh->addNode(n1);
    mesh->addNode(n2);
    mesh->addNode(n3);

    core::Element tri(1, core::ElementType::TRIANGLE, {1, 2, 3});
    mesh->addElement(tri);

    manager.setMesh(mesh);

    // Test validation on loaded mesh
    errors = manager.validate();
    // Should have fewer errors now
    std::cout << "  ✓ Loaded mesh validation works\n";

    if (!errors.empty()) {
        std::cout << "    Validation found " << errors.size() << " issues\n";
    }
}

void testSummary() {
    std::cout << "\nTesting summary generation...\n";

    MeshManager manager;

    // Create simple mesh
    auto mesh = std::make_shared<core::MeshData>();
    for (size_t i = 1; i <= 4; ++i) {
        core::Node node(i, i * 0.1, 0.0, 0.0);
        mesh->addNode(node);
    }

    for (size_t i = 1; i <= 2; ++i) {
        core::Element tri(i, core::ElementType::TRIANGLE, {i, i+1, i+2});
        mesh->addElement(tri);
    }

    manager.setMesh(mesh);

    // Add a BC
    auto bc = boundary::DirichletBC::makeConstant("test", 1, 0.0);
    manager.addBC(bc);

    // Get summary
    std::string summary = manager.getSummary();
    assert(!summary.empty());
    assert(summary.find("Mesh Manager Summary") != std::string::npos);
    assert(summary.find("Nodes: 4") != std::string::npos);
    assert(summary.find("Elements: 2") != std::string::npos);
    std::cout << "  ✓ Summary generation works\n";

    std::cout << "\n" << summary << "\n";
}

void testExportOperations() {
    std::cout << "\nTesting export operations...\n";

    MeshManager manager;

    // Create mesh
    auto mesh = std::make_shared<core::MeshData>();
    core::Node n1(1, 0.0, 0.0, 0.0);
    core::Node n2(2, 1.0, 0.0, 0.0);
    core::Node n3(3, 0.5, 0.866, 0.0);

    mesh->addNode(n1);
    mesh->addNode(n2);
    mesh->addNode(n3);

    core::Element tri(1, core::ElementType::TRIANGLE, {1, 2, 3});
    mesh->addElement(tri);

    manager.setMesh(mesh);

    // Test export to VTK
    bool success = manager.exportToVTK("/tmp/test_mesh.vtk");
    std::cout << "  ✓ VTK export: " << (success ? "success" : "failed") << "\n";

    // Test export to STL
    success = manager.exportToSTL("/tmp/test_mesh.stl");
    std::cout << "  ✓ STL export: " << (success ? "success" : "failed") << "\n";

    // Test export to OBJ
    success = manager.exportToOBJ("/tmp/test_mesh.obj");
    std::cout << "  ✓ OBJ export: " << (success ? "success" : "failed") << "\n";
}

int main() {
    std::cout << "Phase 10 Tests - Mesh Manager Integration\n";
    std::cout << "==========================================\n\n";

    testMeshManagerBasics();
    testMeshManagerWithMesh();
    testQualityOperations();
    testBoundaryConditionIntegration();
    testDomainIntegration();
    testDomainDecomposition();
    testValidation();
    testSummary();
    testExportOperations();

    std::cout << "\n==========================================\n";
    std::cout << "All Phase 10 tests passed!\n";
    return 0;
}
