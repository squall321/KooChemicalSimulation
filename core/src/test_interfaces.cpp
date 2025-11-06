/**
 * @file test_interfaces.cpp
 * @brief Test file to verify that all interfaces compile correctly
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file contains simple test implementations of all Phase 2 interfaces
 * to verify that they compile correctly. This is NOT a unit test - it's
 * just a compilation check.
 */

#include "core/interfaces/ISimulatable.h"
#include "core/interfaces/ISolver.h"
#include "core/interfaces/IMesh.h"
#include "core/interfaces/IChemicalSystem.h"
#include "core/base/BaseObject.h"

using namespace koo::core;

// ============================================================================
// Test Implementation of ISimulatable
// ============================================================================

class TestSimulatable : public ISimulatable {
public:
    void initialize() override {}
    void step(double /*dt*/) override {}
    bool validate() const override { return true; }
    void finalize() override {}
    std::string getName() const override { return "TestSimulatable"; }
    double getCurrentTime() const override { return 0.0; }
    void reset() override {}
};

// ============================================================================
// Test Implementation of ISolver
// ============================================================================

class TestSolver : public ISolver {
public:
    void setup(std::shared_ptr<IMesh> /*mesh*/) override {}
    void assemble() override {}
    bool solve() override { return true; }
    const std::vector<double>& getSolution() const override { return solution_; }
    SolverType getType() const override { return SolverType::DIRECT; }
    SolverStatus getStatus() const override { return SolverStatus::READY; }
    std::string getName() const override { return "TestSolver"; }
    void setTolerance(double /*tol*/) override {}
    double getTolerance() const override { return 1e-6; }
    void setMaxIterations(int /*maxIter*/) override {}
    int getMaxIterations() const override { return 100; }
    int getIterationCount() const override { return 0; }
    double getResidual() const override { return 0.0; }
    void reset() override {}
    size_t getNumDOFs() const override { return 0; }

private:
    std::vector<double> solution_;
};

// ============================================================================
// Test Implementation of IMesh
// ============================================================================

class TestMesh : public IMesh {
public:
    void load(const std::string& /*filename*/) override {}
    void save(const std::string& /*filename*/) const override {}
    MeshDimension getDimension() const override { return MeshDimension::THREE_D; }
    size_t getNumNodes() const override { return 0; }
    size_t getNumElements() const override { return 0; }
    size_t getNumBoundaryElements() const override { return 0; }
    std::vector<double> getNodeCoordinates(size_t /*nodeId*/) const override {
        return {0.0, 0.0, 0.0};
    }
    std::vector<size_t> getElementNodes(size_t /*elementId*/) const override {
        return {};
    }
    ElementType getElementType(size_t /*elementId*/) const override {
        return ElementType::TETRAHEDRON;
    }
    int getElementMarker(size_t /*elementId*/) const override { return 0; }
    int getBoundaryMarker(size_t /*boundaryElementId*/) const override { return 0; }
    BoundingBox getBoundingBox() const override {
        return {0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    }
    void refine(int /*levels*/) override {}
    bool validate() const override { return true; }
    double getQuality() const override { return 1.0; }
    std::string getName() const override { return "TestMesh"; }
    std::vector<std::string> getPhysicalGroupNames() const override { return {}; }
    int getPhysicalGroupId(const std::string& /*name*/) const override { return -1; }
};

// ============================================================================
// Test Implementation of IChemicalSystem
// ============================================================================

class TestChemicalSystem : public IChemicalSystem {
public:
    void addSpecies(const std::string& /*name*/, double /*molecularWeight*/,
                   int /*charge*/) override {}
    void addReaction(const std::string& /*reactionString*/,
                    double /*rateConstant*/) override {}
    size_t getNumSpecies() const override { return 0; }
    size_t getNumReactions() const override { return 0; }
    int getSpeciesIndex(const std::string& /*name*/) const override { return -1; }
    std::string getSpeciesName(size_t /*index*/) const override { return ""; }
    double getMolecularWeight(size_t /*speciesIndex*/) const override { return 0.0; }
    int getCharge(size_t /*speciesIndex*/) const override { return 0; }
    std::vector<double> computeReactionRates(const std::vector<double>& /*concentrations*/,
                                             double /*temperature*/) const override {
        return {};
    }
    std::vector<double> computeSourceTerms(const std::vector<double>& /*concentrations*/,
                                           double /*temperature*/) const override {
        return {};
    }
    std::vector<std::vector<double>> computeJacobian(
        const std::vector<double>& /*concentrations*/,
        double /*temperature*/) const override {
        return {};
    }
    void setDiffusionCoefficient(size_t /*speciesIndex*/,
                                double /*diffusionCoeff*/) override {}
    double getDiffusionCoefficient(size_t /*speciesIndex*/) const override {
        return 0.0;
    }
    bool validate() const override { return true; }
    std::string getName() const override { return "TestChemicalSystem"; }
    void clear() override {}
    std::string toString() const override { return "TestChemicalSystem"; }
    std::vector<std::string> getAllSpeciesNames() const override { return {}; }
    void setArrheniusParameters(size_t /*reactionIndex*/, double /*preExponential*/,
                               double /*activationEnergy*/) override {}
};

// ============================================================================
// Test Implementation of BaseObject
// ============================================================================

class TestObject : public BaseObject {
public:
    explicit TestObject(const std::string& name = "TestObject")
        : BaseObject(name, "Test object for Phase 2 verification") {}

    std::string getTypeName() const override {
        return "TestObject";
    }

    std::shared_ptr<BaseObject> clone() const override {
        return std::make_shared<TestObject>(*this);
    }
};

// ============================================================================
// Main function to verify compilation
// ============================================================================

int main() {
    // Test ISimulatable
    {
        TestSimulatable sim;
        sim.initialize();
        sim.step(0.1);
        bool valid = sim.validate();
        (void)valid; // Suppress unused variable warning
        sim.finalize();
    }

    // Test ISolver
    {
        TestSolver solver;
        solver.setTolerance(1e-8);
        solver.setMaxIterations(1000);
        bool converged = solver.solve();
        (void)converged;
    }

    // Test IMesh
    {
        TestMesh mesh;
        auto dim = mesh.getDimension();
        (void)dim;
        auto numNodes = mesh.getNumNodes();
        (void)numNodes;
        bool meshValid = mesh.validate();
        (void)meshValid;
    }

    // Test IChemicalSystem
    {
        TestChemicalSystem chem;
        chem.addSpecies("H2O", 18.015, 0);
        size_t numSpecies = chem.getNumSpecies();
        (void)numSpecies;
        bool chemValid = chem.validate();
        (void)chemValid;
    }

    // Test BaseObject
    {
        TestObject obj("MyObject");
        obj.setDescription("A test object");
        std::string name = obj.getName();
        (void)name;
        auto objId = obj.getObjectId();
        (void)objId;
        auto clone = obj.clone();
        (void)clone;
    }

    // If we get here, all interfaces compile successfully
    return 0;
}
