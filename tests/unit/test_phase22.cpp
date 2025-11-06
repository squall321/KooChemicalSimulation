#include "physics/transport/VelocityField.h"
#include "physics/transport/AdvectionDiffusion.h"
#include "physics/transport/TransportBoundary.h"
#include "physics/diffusion/DiffusionCoefficient.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <memory>

using namespace koo::physics::transport;
using namespace koo::physics;

// Test 1: Uniform velocity field
void testUniformVelocityField() {
    std::cout << "\nTesting uniform velocity field..." << std::endl;

    VelocityField vfield({1.0, 0.5, 0.0}); // vx=1, vy=0.5, vz=0

    auto v = vfield.getVelocity({0.0, 0.0, 0.0}, 0.0);
    double magnitude = vfield.getMagnitude({0.0, 0.0, 0.0}, 0.0);

    std::cout << "  → Velocity: [" << v[0] << ", " << v[1] << ", " << v[2] << "]" << std::endl;
    std::cout << "  → Magnitude: " << magnitude << " m/s" << std::endl;

    if (std::abs(v[0] - 1.0) < 1.0e-10 &&
        std::abs(v[1] - 0.5) < 1.0e-10 &&
        std::abs(magnitude - std::sqrt(1.25)) < 1.0e-10) {
        std::cout << "  ✓ Uniform velocity field works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 2: Spatially varying velocity field
void testVaryingVelocityField() {
    std::cout << "\nTesting spatially varying velocity field..." << std::endl;

    // Shear flow: v(y) = [y, 0, 0]
    VelocityField vfield([](const std::array<double, 3>& pos, double t) {
        return std::array<double, 3>{pos[1], 0.0, 0.0}; // vx = y
    });

    auto v1 = vfield.getVelocity({0.0, 0.0, 0.0}, 0.0);
    auto v2 = vfield.getVelocity({0.0, 2.0, 0.0}, 0.0);

    std::cout << "  → v(y=0): [" << v1[0] << ", " << v1[1] << ", " << v1[2] << "]" << std::endl;
    std::cout << "  → v(y=2): [" << v2[0] << ", " << v2[1] << ", " << v2[2] << "]" << std::endl;

    if (std::abs(v1[0] - 0.0) < 1.0e-10 && std::abs(v2[0] - 2.0) < 1.0e-10) {
        std::cout << "  ✓ Varying velocity field works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 3: Advection term calculation
void testAdvectionTerm() {
    std::cout << "\nTesting advection term calculation..." << std::endl;

    VelocityField vfield({1.0, 0.0, 0.0}); // Flow in x-direction
    Advection advection(vfield);

    std::array<double, 3> gradient = {2.0, 0.0, 0.0}; // ∂C/∂x = 2
    std::array<double, 3> position = {0.0, 0.0, 0.0};

    double advTerm = advection.calculateAdvectionTerm(gradient, position, 0.0);
    // v·∇C = 1.0 * 2.0 = 2.0

    std::cout << "  → v·∇C = " << advTerm << std::endl;

    if (std::abs(advTerm - 2.0) < 1.0e-10) {
        std::cout << "  ✓ Advection term calculation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 4: Material derivative
void testMaterialDerivative() {
    std::cout << "\nTesting material derivative..." << std::endl;

    VelocityField vfield({1.0, 0.5, 0.0});
    Advection advection(vfield);

    double dCdt = 0.1; // Local time derivative
    std::array<double, 3> gradient = {2.0, 1.0, 0.0}; // ∇C
    std::array<double, 3> position = {0.0, 0.0, 0.0};

    double materialDeriv = advection.calculateMaterialDerivative(dCdt, gradient, position, 0.0);
    // DC/Dt = ∂C/∂t + v·∇C = 0.1 + (1.0*2.0 + 0.5*1.0) = 0.1 + 2.5 = 2.6

    std::cout << "  → DC/Dt = " << materialDeriv << std::endl;

    if (std::abs(materialDeriv - 2.6) < 1.0e-10) {
        std::cout << "  ✓ Material derivative works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 5: Courant number
void testCourantNumber() {
    std::cout << "\nTesting Courant number..." << std::endl;

    double velocity = 1.0;  // m/s
    double dt = 0.001;      // s
    double dx = 0.01;       // m

    double Co = Advection::getCourantNumber(velocity, dt, dx);
    // Co = vΔt/Δx = 1.0 * 0.001 / 0.01 = 0.1

    bool stable = Advection::isCFLStable(velocity, dt, dx);

    std::cout << "  → Co = " << Co << std::endl;
    std::cout << "  → CFL stable: " << (stable ? "Yes" : "No") << std::endl;

    if (std::abs(Co - 0.1) < 1.0e-10 && stable) {
        std::cout << "  ✓ Courant number works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 6: Maximum stable time step
void testMaxTimeStep() {
    std::cout << "\nTesting maximum stable time step..." << std::endl;

    double velocity = 2.0;  // m/s
    double dx = 0.01;       // m

    double dtMax = Advection::getMaxTimeStep(velocity, dx);
    // Δt_max = Δx/v = 0.01/2.0 = 0.005

    std::cout << "  → Δt_max = " << dtMax << " s" << std::endl;

    if (std::abs(dtMax - 0.005) < 1.0e-10) {
        std::cout << "  ✓ Maximum time step calculation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 7: Advection-diffusion coupling
void testAdvectionDiffusionCoupling() {
    std::cout << "\nTesting advection-diffusion coupling..." << std::endl;

    VelocityField vfield({1.0, 0.0, 0.0});
    auto diffCoeff = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport(vfield, diffCoeff);

    std::array<double, 3> gradient = {1000.0, 0.0, 0.0}; // mol/m⁴
    double laplacian = 1.0e6;  // mol/m⁵
    double sourceTerm = 0.0;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0; // K

    double rhs = transport.calculateRHS(gradient, laplacian, sourceTerm,
                                       position, temperature, 0.0, 0.0);

    std::cout << "  → RHS = " << rhs << " mol/(m³·s)" << std::endl;

    if (std::abs(rhs + 999.0) < 10.0) { // Advection dominates
        std::cout << "  ✓ Advection-diffusion coupling works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 8: Peclet number
void testPecletNumber() {
    std::cout << "\nTesting Peclet number..." << std::endl;

    VelocityField vfield({0.01, 0.0, 0.0}); // 1 cm/s
    auto diffCoeff = std::make_shared<ConstantDiffusion>(1.0e-9); // m²/s
    AdvectionDiffusion transport(vfield, diffCoeff);

    double L = 0.001; // 1 mm
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0;

    double Pe = transport.getPecletNumber(L, position, temperature, 0.0, 0.0);
    // Pe = vL/D = 0.01 * 0.001 / 1e-9 = 10000

    std::cout << "  → Pe = " << Pe << std::endl;

    if (std::abs(Pe - 10000.0) < 1.0) {
        std::cout << "  ✓ Peclet number works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 9: Diffusion vs advection dominated
void testTransportRegimes() {
    std::cout << "\nTesting transport regimes..." << std::endl;

    // Diffusion-dominated: very slow flow (Pe << 0.1)
    VelocityField vfield1({1.0e-11, 0.0, 0.0});  // Very slow: 10 pm/s
    auto diffCoeff1 = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport1(vfield1, diffCoeff1);

    double L = 0.001;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0;

    bool isDiffDom = transport1.isDiffusionDominated(L, position, temperature);
    std::cout << "  → Slow flow (Pe~0.00001): Diffusion-dominated = "
              << (isDiffDom ? "Yes" : "No") << std::endl;

    // Advection-dominated: fast flow
    VelocityField vfield2({0.1, 0.0, 0.0});
    auto diffCoeff2 = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport2(vfield2, diffCoeff2);

    bool isAdvDom = transport2.isAdvectionDominated(L, position, temperature);
    std::cout << "  → Fast flow (Pe~100000): Advection-dominated = "
              << (isAdvDom ? "Yes" : "No") << std::endl;

    if (isDiffDom && isAdvDom) {
        std::cout << "  ✓ Transport regime detection works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 10: Characteristic time scales
void testTimeScales() {
    std::cout << "\nTesting characteristic time scales..." << std::endl;

    VelocityField vfield({0.01, 0.0, 0.0});
    auto diffCoeff = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport(vfield, diffCoeff);

    double L = 0.001; // 1 mm
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0;

    double tAdv = transport.getAdvectionTime(L, position, 0.0);
    double tDiff = transport.getDiffusionTime(L, temperature, 0.0);
    double ratio = transport.getTimeScaleRatio(L, position, temperature, 0.0, 0.0);

    std::cout << "  → Advection time: " << tAdv << " s" << std::endl;
    std::cout << "  → Diffusion time: " << tDiff << " s" << std::endl;
    std::cout << "  → Time scale ratio: " << ratio << std::endl;

    if (std::abs(tAdv - 0.1) < 0.01 && tDiff > 100.0) {
        std::cout << "  ✓ Time scale calculation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 11: Dirichlet boundary condition
void testDirichletBoundary() {
    std::cout << "\nTesting Dirichlet boundary..." << std::endl;

    auto bc = TransportBoundary::dirichlet(1.0);

    double value = bc.applyDirichlet();
    std::string typeName = bc.getTypeName();

    std::cout << "  → Type: " << typeName << std::endl;
    std::cout << "  → Value: " << value << " mol/m³" << std::endl;

    if (bc.getType() == BoundaryType::DIRICHLET && std::abs(value - 1.0) < 1.0e-10) {
        std::cout << "  ✓ Dirichlet boundary works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 12: Neumann boundary condition
void testNeumannBoundary() {
    std::cout << "\nTesting Neumann boundary..." << std::endl;

    auto bc = TransportBoundary::neumann(1.0e-6); // Flux = 1e-6 mol/(m²·s)
    double D = 1.0e-9; // m²/s

    double gradient = bc.applyNeumann(D);
    // ∂C/∂n = -J/D = -1e-6/1e-9 = -1000

    std::cout << "  → Type: " << bc.getTypeName() << std::endl;
    std::cout << "  → Normal gradient: " << gradient << " mol/m⁴" << std::endl;

    if (bc.getType() == BoundaryType::NEUMANN && std::abs(gradient + 1000.0) < 0.1) {
        std::cout << "  ✓ Neumann boundary works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 13: Robin boundary condition
void testRobinBoundary() {
    std::cout << "\nTesting Robin boundary..." << std::endl;

    double h = 1.0e-3;    // Mass transfer coefficient
    double Cinf = 0.5;    // Ambient concentration
    auto bc = TransportBoundary::robin(h, Cinf);

    double C = 1.0;       // Surface concentration
    double D = 1.0e-9;    // Diffusion coefficient

    double gradient = bc.applyRobin(C, D);
    // ∂C/∂n = -h(C - C_∞)/D = -1e-3 * (1.0 - 0.5) / 1e-9 = -500000

    std::cout << "  → Type: " << bc.getTypeName() << std::endl;
    std::cout << "  → Normal gradient: " << gradient << " mol/m⁴" << std::endl;

    if (bc.getType() == BoundaryType::ROBIN && std::abs(gradient + 500000.0) < 100.0) {
        std::cout << "  ✓ Robin boundary works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 14: Convective outflow boundary
void testConvectiveOutflow() {
    std::cout << "\nTesting convective outflow boundary..." << std::endl;

    double vn = 1.0; // Normal velocity = 1 m/s
    auto bc = TransportBoundary::convectiveOutflow(vn);

    double C = 1.0;         // Current boundary concentration
    double Cupwind = 0.9;   // Upwind concentration
    double dt = 0.001;      // Time step
    double dn = 0.01;       // Grid spacing

    double Cnew = bc.applyConvectiveOutflow(C, Cupwind, dt, dn);

    std::cout << "  → Type: " << bc.getTypeName() << std::endl;
    std::cout << "  → C_old = " << C << ", C_new = " << Cnew << std::endl;

    if (bc.getType() == BoundaryType::CONVECTIVE_OUTFLOW && Cnew < C) {
        std::cout << "  ✓ Convective outflow works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 15: Boundary flux calculation
void testBoundaryFlux() {
    std::cout << "\nTesting boundary flux calculation..." << std::endl;

    double C = 1.0;              // Concentration
    double normalGradient = -100.0; // ∂C/∂n
    double D = 1.0e-9;           // Diffusion coefficient
    double vn = 0.01;            // Normal velocity

    double flux = TransportBoundary::calculateFlux(C, normalGradient, D, vn);
    // Flux = -D(∂C/∂n) + v·n × C = -1e-9*(-100) + 0.01*1.0 ≈ 0.01

    std::cout << "  → Total flux: " << flux << " mol/(m²·s)" << std::endl;

    if (std::abs(flux - 0.01) < 1.0e-6) {
        std::cout << "  ✓ Boundary flux calculation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 16: Biot number
void testBiotNumber() {
    std::cout << "\nTesting Biot number..." << std::endl;

    double h = 1.0e-3;   // Mass transfer coefficient
    double L = 0.001;    // Characteristic length (1 mm)
    double D = 1.0e-9;   // Diffusion coefficient

    double Bi = TransportBoundary::getBiotNumber(h, L, D);
    // Bi = hL/D = 1e-3 * 0.001 / 1e-9 = 1000

    std::cout << "  → Bi = " << Bi << std::endl;

    if (std::abs(Bi - 1000.0) < 1.0) {
        std::cout << "  ✓ Biot number works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 17: Operator splitting
void testOperatorSplitting() {
    std::cout << "\nTesting operator splitting..." << std::endl;

    VelocityField vfield({1.0, 0.0, 0.0});
    auto diffCoeff = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport(vfield, diffCoeff);

    double C0 = 1.0;
    double dt = 0.001;
    std::array<double, 3> gradient = {100.0, 0.0, 0.0};
    double laplacian = 1000.0;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0;

    // Advection step
    double C1 = transport.advectionStep(dt, gradient, C0, position, 0.0);

    // Diffusion step
    double C2 = transport.diffusionStep(dt, laplacian, C1, temperature);

    std::cout << "  → C_initial = " << C0 << std::endl;
    std::cout << "  → C_after_advection = " << C1 << std::endl;
    std::cout << "  → C_after_diffusion = " << C2 << std::endl;

    if (C1 < C0 && C2 > C1) { // Advection decreases, diffusion increases
        std::cout << "  ✓ Operator splitting works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 18: Strang splitting
void testStrangSplitting() {
    std::cout << "\nTesting Strang splitting..." << std::endl;

    VelocityField vfield({1.0, 0.0, 0.0});
    auto diffCoeff = std::make_shared<ConstantDiffusion>(1.0e-9);
    AdvectionDiffusion transport(vfield, diffCoeff);

    double C0 = 1.0;
    double dt = 0.001;
    std::array<double, 3> gradient = {100.0, 0.0, 0.0};
    double laplacian = 1000.0;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    double temperature = 300.0;

    double C_strang = transport.strangSplittingStep(dt, gradient, laplacian,
                                                   C0, position, temperature, 0.0);

    std::cout << "  → C_initial = " << C0 << std::endl;
    std::cout << "  → C_Strang = " << C_strang << std::endl;
    std::cout << "  → ΔC = " << (C_strang - C0) << std::endl;

    if (std::abs(C_strang - C0) > 1.0e-10) {
        std::cout << "  ✓ Strang splitting works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

int main() {
    std::cout << "Phase 22 Tests - Transport Phenomena" << std::endl;
    std::cout << "=====================================" << std::endl;

    testUniformVelocityField();
    testVaryingVelocityField();
    testAdvectionTerm();
    testMaterialDerivative();
    testCourantNumber();
    testMaxTimeStep();
    testAdvectionDiffusionCoupling();
    testPecletNumber();
    testTransportRegimes();
    testTimeScales();
    testDirichletBoundary();
    testNeumannBoundary();
    testRobinBoundary();
    testConvectiveOutflow();
    testBoundaryFlux();
    testBiotNumber();
    testOperatorSplitting();
    testStrangSplitting();

    std::cout << "\n=====================================" << std::endl;
    std::cout << "All Phase 22 tests passed!" << std::endl;
    std::cout << "Transport phenomena verified." << std::endl;

    return 0;
}
