/**
 * @file test_phase9.cpp
 * @brief Unit tests for Phase 9 - Domain and Subdomain Management
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha4
 * @date 2025-11-06
 */

#include "mesh/domain/Domain.h"
#include "mesh/domain/SubDomain.h"
#include "mesh/domain/DomainManager.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <algorithm>

using namespace koo::mesh::domain;

void testDomain() {
    std::cout << "Testing Domain...\n";

    // Create a domain
    Domain domain(1, "test_domain", DomainDimension::VOLUME, DomainType::PHYSICAL);

    // Test basic properties
    assert(domain.getId() == 1);
    assert(domain.getName() == "test_domain");
    assert(domain.getDimension() == DomainDimension::VOLUME);
    assert(domain.getType() == DomainType::PHYSICAL);
    assert(domain.isEnabled());
    std::cout << "  ✓ Basic properties work\n";

    // Test name change
    domain.setName("new_name");
    assert(domain.getName() == "new_name");
    std::cout << "  ✓ Name change works\n";

    // Test physical tag
    domain.setPhysicalTag(100);
    assert(domain.getPhysicalTag() == 100);
    std::cout << "  ✓ Physical tag works\n";

    // Test material ID
    domain.setMaterialId(5);
    assert(domain.getMaterialId() == 5);
    std::cout << "  ✓ Material ID works\n";

    // Test element management
    domain.addElement(1);
    domain.addElement(2);
    domain.addElement(3);
    assert(domain.getNumElements() == 3);
    assert(domain.hasElement(1));
    assert(domain.hasElement(2));
    assert(domain.hasElement(3));
    assert(!domain.hasElement(4));
    std::cout << "  ✓ Element management works\n";

    // Test bulk element addition
    std::vector<size_t> elements = {10, 11, 12, 13, 14};
    domain.addElements(elements);
    assert(domain.getNumElements() == 8);  // 3 + 5
    assert(domain.hasElement(10));
    std::cout << "  ✓ Bulk element addition works\n";

    // Test element removal
    bool removed = domain.removeElement(2);
    (void)removed;  // Used in assert
    assert(removed);
    assert(!domain.hasElement(2));
    assert(domain.getNumElements() == 7);
    std::cout << "  ✓ Element removal works\n";

    // Test node management
    domain.addNode(100);
    domain.addNode(101);
    domain.addNode(102);
    assert(domain.getNumNodes() == 3);
    assert(domain.hasNode(100));
    std::cout << "  ✓ Node management works\n";

    // Test bulk node addition
    std::vector<size_t> nodes = {200, 201, 202};
    domain.addNodes(nodes);
    assert(domain.getNumNodes() == 6);
    std::cout << "  ✓ Bulk node addition works\n";

    // Test properties
    domain.setProperty("density", 1000.0);
    domain.setProperty("temperature", 300.0);
    assert(domain.hasProperty("density"));
    assert(domain.getProperty("density") == 1000.0);
    assert(domain.getProperty("temperature") == 300.0);
    assert(domain.getProperty("nonexistent", 42.0) == 42.0);
    std::cout << "  ✓ Property system works\n";

    // Test property keys
    auto keys = domain.getPropertyKeys();
    assert(keys.size() == 2);
    assert(std::find(keys.begin(), keys.end(), "density") != keys.end());
    assert(std::find(keys.begin(), keys.end(), "temperature") != keys.end());
    std::cout << "  ✓ Property keys retrieval works\n";

    // Test enable/disable
    domain.setEnabled(false);
    assert(!domain.isEnabled());
    domain.setEnabled(true);
    assert(domain.isEnabled());
    std::cout << "  ✓ Enable/disable works\n";

    // Test element ID retrieval
    auto elementIds = domain.getElementIds();
    assert(elementIds.size() == 7);
    std::cout << "  ✓ Element ID retrieval works\n";

    // Test clear
    domain.clearElements();
    assert(domain.getNumElements() == 0);
    domain.clearNodes();
    assert(domain.getNumNodes() == 0);
    std::cout << "  ✓ Clear operations work\n";

    // Test toString
    std::string str = domain.toString();
    assert(!str.empty());
    std::cout << "  ✓ toString works\n";
}

void testSubDomain() {
    std::cout << "\nTesting SubDomain...\n";

    // Create a subdomain
    SubDomain subdomain(1, 10, "sub1");

    // Test basic properties
    assert(subdomain.getId() == 1);
    assert(subdomain.getParentDomainId() == 10);
    assert(subdomain.getName() == "sub1");
    assert(subdomain.isEnabled());
    std::cout << "  ✓ Basic properties work\n";

    // Test rank settings
    subdomain.setRank(2);
    subdomain.setNumRanks(4);
    assert(subdomain.getRank() == 2);
    assert(subdomain.getNumRanks() == 4);
    std::cout << "  ✓ Rank settings work\n";

    // Test element management
    subdomain.addElement(1);
    subdomain.addElement(2);
    subdomain.addElement(3);
    assert(subdomain.getNumElements() == 3);
    assert(subdomain.hasElement(2));
    std::cout << "  ✓ Element management works\n";

    // Test bulk element addition
    std::vector<size_t> elements = {10, 11, 12};
    subdomain.addElements(elements);
    assert(subdomain.getNumElements() == 6);
    std::cout << "  ✓ Bulk element addition works\n";

    // Test node management
    subdomain.addNode(100);
    subdomain.addNode(101);
    assert(subdomain.getNumNodes() == 2);
    std::cout << "  ✓ Node management works\n";

    // Test neighbor management
    subdomain.addNeighbor(2);
    subdomain.addNeighbor(3);
    assert(subdomain.hasNeighbor(2));
    assert(subdomain.hasNeighbor(3));
    assert(!subdomain.hasNeighbor(4));
    assert(subdomain.getNumNeighbors() == 2);
    std::cout << "  ✓ Neighbor management works\n";

    // Test neighbor removal
    bool removed = subdomain.removeNeighbor(3);
    (void)removed;  // Used in assert
    assert(removed);
    assert(!subdomain.hasNeighbor(3));
    assert(subdomain.getNumNeighbors() == 1);
    std::cout << "  ✓ Neighbor removal works\n";

    // Test interface nodes
    subdomain.addInterfaceNode(100, 2);  // Node 100 shared with neighbor 2
    subdomain.addInterfaceNode(101, 2);
    subdomain.addInterfaceNode(102, 3);  // Node 102 shared with neighbor 3

    auto interface2 = subdomain.getInterfaceNodes(2);
    assert(interface2.size() == 2);
    assert(std::find(interface2.begin(), interface2.end(), 100) != interface2.end());

    auto interface3 = subdomain.getInterfaceNodes(3);
    assert(interface3.size() == 1);

    auto allInterface = subdomain.getAllInterfaceNodes();
    assert(allInterface.size() == 3);
    std::cout << "  ✓ Interface node management works\n";

    // Test overlap detection
    SubDomain subdomain2(2, 10, "sub2");
    subdomain2.addElement(2);  // Shares element 2 with subdomain
    subdomain2.addElement(3);  // Shares element 3
    subdomain2.addElement(4);

    assert(subdomain.overlaps(subdomain2));
    size_t overlap = subdomain.computeOverlap(subdomain2);
    (void)overlap;  // Used in assert
    assert(overlap == 2);  // Elements 2 and 3
    std::cout << "  ✓ Overlap detection works\n";

    // Test no overlap
    SubDomain subdomain3(3, 10, "sub3");
    subdomain3.addElement(100);
    subdomain3.addElement(101);
    assert(!subdomain.overlaps(subdomain3));
    assert(subdomain.computeOverlap(subdomain3) == 0);
    std::cout << "  ✓ No overlap detection works\n";

    // Test toString
    std::string str = subdomain.toString();
    assert(!str.empty());
    std::cout << "  ✓ toString works\n";
}

void testDomainManager() {
    std::cout << "\nTesting DomainManager...\n";

    DomainManager manager;

    // Create domains
    auto domain1 = manager.createDomain("domain1", DomainDimension::VOLUME, DomainType::PHYSICAL);
    auto domain2 = manager.createDomain("domain2", DomainDimension::SURFACE, DomainType::BOUNDARY);
    auto domain3 = manager.createDomain("domain3", DomainDimension::VOLUME, DomainType::MATERIAL);

    assert(manager.getNumDomains() == 3);
    std::cout << "  ✓ Domain creation works\n";

    // Test get by ID
    auto retrieved = manager.getDomain(domain1->getId());
    assert(retrieved != nullptr);
    assert(retrieved->getName() == "domain1");
    std::cout << "  ✓ Get by ID works\n";

    // Test get by name
    auto byName = manager.getDomainByName("domain2");
    assert(byName != nullptr);
    assert(byName->getId() == domain2->getId());
    std::cout << "  ✓ Get by name works\n";

    // Test get by dimension
    auto volumeDomains = manager.getDomainsByDimension(DomainDimension::VOLUME);
    assert(volumeDomains.size() == 2);  // domain1 and domain3

    auto surfaceDomains = manager.getDomainsByDimension(DomainDimension::SURFACE);
    assert(surfaceDomains.size() == 1);  // domain2
    std::cout << "  ✓ Get by dimension works\n";

    // Test get by type
    auto physicalDomains = manager.getDomainsByType(DomainType::PHYSICAL);
    assert(physicalDomains.size() == 1);

    auto boundaryDomains = manager.getDomainsByType(DomainType::BOUNDARY);
    assert(boundaryDomains.size() == 1);

    auto materialDomains = manager.getDomainsByType(DomainType::MATERIAL);
    assert(materialDomains.size() == 1);
    std::cout << "  ✓ Get by type works\n";

    // Test get all domains
    auto allDomains = manager.getAllDomains();
    assert(allDomains.size() == 3);
    std::cout << "  ✓ Get all domains works\n";

    // Test hasDomain
    assert(manager.hasDomain(domain1->getId()));
    assert(!manager.hasDomain(999));
    std::cout << "  ✓ hasDomain works\n";

    // Test remove domain
    bool removed = manager.removeDomain(domain2->getId());
    (void)removed;  // Used in assert
    assert(removed);
    assert(manager.getNumDomains() == 2);
    assert(!manager.hasDomain(domain2->getId()));
    std::cout << "  ✓ Domain removal works\n";

    // Test subdomain creation
    auto sub1 = manager.createSubDomain(domain1->getId(), "subdomain1");
    auto sub2 = manager.createSubDomain(domain1->getId(), "subdomain2");
    assert(manager.getNumSubDomains() == 2);
    std::cout << "  ✓ SubDomain creation works\n";

    // Test get subdomain by ID
    auto retrievedSub = manager.getSubDomain(sub1->getId());
    assert(retrievedSub != nullptr);
    assert(retrievedSub->getName() == "subdomain1");
    std::cout << "  ✓ Get subdomain by ID works\n";

    // Test get subdomains by parent
    auto subsByParent = manager.getSubDomainsByParent(domain1->getId());
    assert(subsByParent.size() == 2);
    std::cout << "  ✓ Get subdomains by parent works\n";

    // Test get all subdomains
    auto allSubs = manager.getAllSubDomains();
    assert(allSubs.size() == 2);
    std::cout << "  ✓ Get all subdomains works\n";

    // Test remove subdomain
    bool subRemoved = manager.removeSubDomain(sub2->getId());
    assert(subRemoved);
    assert(manager.getNumSubDomains() == 1);
    std::cout << "  ✓ SubDomain removal works\n";

    // Clear subdomains before decomposition test
    manager.clearSubDomains();
    assert(manager.getNumSubDomains() == 0);

    // Test domain decomposition
    domain1->addElements({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    auto decomposed = manager.decomposeDomain(domain1->getId(), 3, PartitionStrategy::UNIFORM);
    assert(decomposed.size() == 3);

    // Check that elements were distributed
    size_t totalElements = 0;
    for (const auto& sub : decomposed) {
        totalElements += sub->getNumElements();
    }
    assert(totalElements == 10);
    std::cout << "  ✓ Domain decomposition works\n";

    // Test getSummary
    std::string summary = manager.getSummary();
    assert(!summary.empty());
    std::cout << "  ✓ Summary generation works\n";

    // Test clear
    manager.clearDomains();
    assert(manager.getNumDomains() == 0);
    assert(manager.getNumSubDomains() == 0);
    std::cout << "  ✓ Clear operations work\n";
}

void testDomainDecomposition() {
    std::cout << "\nTesting Domain Decomposition Utilities...\n";

    // Create a test domain with elements
    Domain domain(1, "test", DomainDimension::VOLUME, DomainType::PHYSICAL);
    for (size_t i = 1; i <= 20; ++i) {
        domain.addElement(i);
    }

    // Test uniform decomposition
    auto subdomains = decomposition::createUniformDecomposition(domain, 4);
    assert(subdomains.size() == 4);

    // Check that all elements are assigned
    size_t totalElements = 0;
    for (const auto& sub : subdomains) {
        totalElements += sub->getNumElements();
        assert(sub->getRank() >= 0 && sub->getRank() < 4);
        assert(sub->getNumRanks() == 4);
    }
    assert(totalElements == 20);
    std::cout << "  ✓ Uniform decomposition works\n";

    // Test empty domain decomposition
    Domain emptyDomain(2, "empty", DomainDimension::VOLUME, DomainType::PHYSICAL);
    auto emptySubdomains = decomposition::createUniformDecomposition(emptyDomain, 2);
    assert(emptySubdomains.empty());
    std::cout << "  ✓ Empty domain decomposition handled\n";

    // Test neighbor detection (after adding overlap)
    subdomains[0]->addElement(100);  // Add shared element
    subdomains[1]->addElement(100);  // Same element in another subdomain
    decomposition::detectNeighbors(subdomains);

    // subdomains[0] and subdomains[1] should be neighbors now
    assert(subdomains[0]->hasNeighbor(subdomains[1]->getId()));
    assert(subdomains[1]->hasNeighbor(subdomains[0]->getId()));
    std::cout << "  ✓ Neighbor detection works\n";
}

void testDomainStringConversions() {
    std::cout << "\nTesting Domain String Conversions...\n";

    // Test dimension to string
    assert(Domain::dimensionToString(DomainDimension::POINT) == "0D");
    assert(Domain::dimensionToString(DomainDimension::CURVE) == "1D");
    assert(Domain::dimensionToString(DomainDimension::SURFACE) == "2D");
    assert(Domain::dimensionToString(DomainDimension::VOLUME) == "3D");
    std::cout << "  ✓ Dimension to string works\n";

    // Test type to string
    assert(Domain::typeToString(DomainType::PHYSICAL) == "physical");
    assert(Domain::typeToString(DomainType::MATERIAL) == "material");
    assert(Domain::typeToString(DomainType::BOUNDARY) == "boundary");
    assert(Domain::typeToString(DomainType::INTERFACE) == "interface");
    assert(Domain::typeToString(DomainType::CUSTOM) == "custom");
    std::cout << "  ✓ Type to string works\n";
}

int main() {
    std::cout << "Phase 9 Tests - Domain and Subdomain Management\n";
    std::cout << "================================================\n\n";

    testDomain();
    testSubDomain();
    testDomainManager();
    testDomainDecomposition();
    testDomainStringConversions();

    std::cout << "\n================================================\n";
    std::cout << "All Phase 9 tests passed!\n";
    return 0;
}
