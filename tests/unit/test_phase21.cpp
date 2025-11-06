/**
 * @file test_phase21.cpp
 * @brief Tests for Phase 21 - Diffusion Equation Model
 * @author KooChemicalSimulation Development Team
 * @date 2025-11-06
 */

#include "physics/diffusion/DiffusionCoefficient.h"
#include "physics/diffusion/FickDiffusion.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace koo::physics;

// Test constant diffusion coefficient
void testConstantDiffusion() {
    std::cout << "\nTesting constant diffusion...\n";

    double D0 = 1.0e-9;  // m²/s
    ConstantDiffusion diff(D0);

    assert(diff.getName() == "Constant");
    assert(diff.getD0() == D0);

    // Should be constant regardless of T and C
    assert(diff.calculate(300.0, 0.0) == D0);
    assert(diff.calculate(1000.0, 10.0) == D0);

    std::cout << "  ✓ Constant diffusion works\n";
}

// Test Arrhenius diffusion
void testArrheniusDiffusion() {
    std::cout << "\nTesting Arrhenius diffusion...\n";

    double D0 = 1.0e-5;  // m²/s
    double Ea = 50000.0;  // J/mol
    ArrheniusDiffusion diff(D0, Ea);

    assert(diff.getName() == "Arrhenius");
    assert(diff.getD0() == D0);
    assert(diff.getEa() == Ea);

    // Higher temperature should give higher D
    double D_300 = diff.calculate(300.0);
    double D_600 = diff.calculate(600.0);
    assert(D_600 > D_300);

    std::cout << "  → D(300K) = " << D_300 << " m²/s\n";
    std::cout << "  → D(600K) = " << D_600 << " m²/s\n";
    std::cout << "  ✓ Arrhenius diffusion works\n";
}

// Test Chapman-Enskog diffusion
void testChapmanEnskogDiffusion() {
    std::cout << "\nTesting Chapman-Enskog diffusion...\n";

    double A = 1.0e-7;
    double P = 101325.0;  // Pa (1 atm)
    ChapmanEnskogDiffusion diff(A, P);

    assert(diff.getName() == "Chapman-Enskog");

    // D ~ T^(3/2)
    double D_300 = diff.calculate(300.0);
    double D_600 = diff.calculate(600.0);

    std::cout << "  → D(300K) = " << D_300 << " m²/s\n";
    std::cout << "  → D(600K) = " << D_600 << " m²/s\n";

    // Should increase with temperature
    assert(D_600 > D_300);

    std::cout << "  ✓ Chapman-Enskog diffusion works\n";
}

// Test concentration-dependent diffusion
void testConcentrationDependentDiffusion() {
    std::cout << "\nTesting concentration-dependent diffusion...\n";

    double D0 = 1.0e-9;
    double alpha = 0.1;   // m³/mol
    double beta = 0.01;   // m⁶/mol²
    ConcentrationDependentDiffusion diff(D0, alpha, beta);

    assert(diff.getName() == "Concentration-Dependent");
    assert(diff.getD0() == D0);
    assert(diff.getAlpha() == alpha);
    assert(diff.getBeta() == beta);

    // D should increase with concentration
    double D_0 = diff.calculate(300.0, 0.0);
    double D_10 = diff.calculate(300.0, 10.0);

    std::cout << "  → D(C=0) = " << D_0 << " m²/s\n";
    std::cout << "  → D(C=10) = " << D_10 << " m²/s\n";

    assert(D_10 > D_0);

    std::cout << "  ✓ Concentration-dependent diffusion works\n";
}

// Test anisotropic diffusion tensor
void testAnisotropicDiffusion() {
    std::cout << "\nTesting anisotropic diffusion...\n";

    double Dx = 1.0e-9;
    double Dy = 2.0e-9;
    double Dz = 3.0e-9;
    AnisotropicDiffusion diff(Dx, Dy, Dz);

    auto diagonal = diff.getDiagonal();
    assert(diagonal[0] == Dx);
    assert(diagonal[1] == Dy);
    assert(diagonal[2] == Dz);

    assert(!diff.isIsotropic());

    double D_eff = diff.getEffective();
    assert(std::abs(D_eff - (Dx + Dy + Dz) / 3.0) < 1.0e-12);

    std::cout << "  → Dx = " << Dx << " m²/s\n";
    std::cout << "  → Dy = " << Dy << " m²/s\n";
    std::cout << "  → Dz = " << Dz << " m²/s\n";
    std::cout << "  → D_eff = " << D_eff << " m²/s\n";

    std::cout << "  ✓ Anisotropic diffusion works\n";
}

// Test isotropic check
void testIsotropicCheck() {
    std::cout << "\nTesting isotropic check...\n";

    AnisotropicDiffusion iso(1.0e-9, 1.0e-9, 1.0e-9);
    assert(iso.isIsotropic());

    AnisotropicDiffusion aniso(1.0e-9, 2.0e-9, 3.0e-9);
    assert(!aniso.isIsotropic());

    std::cout << "  ✓ Isotropic check works\n";
}

// Test Fick diffusion construction
void testFickDiffusionConstruction() {
    std::cout << "\nTesting Fick diffusion construction...\n";

    double D0 = 1.0e-9;
    FickDiffusion fick(D0);

    assert(fick.getModelName().find("Fick's Law") != std::string::npos);
    assert(fick.getDiffusionCoefficient(300.0) == D0);

    std::cout << "  → Model: " << fick.getModelName() << "\n";
    std::cout << "  ✓ Fick diffusion construction works\n";
}

// Test flux calculation
void testFluxCalculation() {
    std::cout << "\nTesting flux calculation...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    // Concentration gradient (mol/m⁴)
    std::array<double, 3> gradient = {1000.0, 500.0, 0.0};

    auto flux = fick.calculateFlux(gradient, 300.0);

    // J = -D × ∇C
    assert(flux[0] < 0.0);  // Opposite to gradient
    assert(flux[1] < 0.0);
    assert(flux[2] == 0.0);  // No gradient in z

    std::cout << "  → Flux: [" << flux[0] << ", " << flux[1] << ", " << flux[2] << "] mol/(m²·s)\n";

    std::cout << "  ✓ Flux calculation works\n";
}

// Test flux magnitude
void testFluxMagnitude() {
    std::cout << "\nTesting flux magnitude...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    double gradMag = 1000.0;  // mol/m⁴
    double fluxMag = fick.calculateFluxMagnitude(gradMag, 300.0);

    assert(fluxMag == D0 * gradMag);

    std::cout << "  → |J| = " << fluxMag << " mol/(m²·s)\n";
    std::cout << "  ✓ Flux magnitude works\n";
}

// Test source term calculation
void testSourceTermCalculation() {
    std::cout << "\nTesting source term calculation...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    double laplacian = 1.0e6;  // mol/m⁵
    double source = fick.calculateSourceTerm(laplacian, 300.0);

    // ∂C/∂t = D × ∇²C
    assert(source == D0 * laplacian);

    std::cout << "  → ∂C/∂t = " << source << " mol/(m³·s)\n";
    std::cout << "  ✓ Source term calculation works\n";
}

// Test diffusion time
void testDiffusionTime() {
    std::cout << "\nTesting diffusion time...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    double length = 1.0e-3;  // 1 mm
    double time = fick.getDiffusionTime(length, 300.0);

    // τ = L² / D
    double expected = (length * length) / D0;
    assert(std::abs(time - expected) < 1.0e-10);

    std::cout << "  → Diffusion time for L=" << length * 1000 << " mm: " << time << " s\n";
    std::cout << "  ✓ Diffusion time calculation works\n";
}

// Test diffusion length
void testDiffusionLength() {
    std::cout << "\nTesting diffusion length...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    double time = 1000.0;  // s
    double length = fick.getDiffusionLength(time, 300.0);

    // L = √(D × t)
    double expected = std::sqrt(D0 * time);
    assert(std::abs(length - expected) < 1.0e-12);

    std::cout << "  → Diffusion length for t=" << time << " s: " << length * 1000 << " mm\n";
    std::cout << "  ✓ Diffusion length calculation works\n";
}

// Test Peclet number
void testPecletNumber() {
    std::cout << "\nTesting Peclet number...\n";

    double D0 = 1.0e-9;  // m²/s
    FickDiffusion fick(D0);

    double velocity = 0.01;  // m/s
    double length = 0.001;   // m
    double Pe = fick.getPecletNumber(velocity, length, 300.0);

    // Pe = v × L / D
    double expected = (velocity * length) / D0;
    assert(std::abs(Pe - expected) < 1.0e-6);

    std::cout << "  → Pe = " << Pe << "\n";
    if (Pe < 1.0) {
        std::cout << "  → Diffusion dominated\n";
    } else {
        std::cout << "  → Convection dominated\n";
    }

    std::cout << "  ✓ Peclet number calculation works\n";
}

// Test anisotropic Fick diffusion
void testAnisotropicFickDiffusion() {
    std::cout << "\nTesting anisotropic Fick diffusion...\n";

    double Dx = 1.0e-9;
    double Dy = 2.0e-9;
    double Dz = 3.0e-9;
    AnisotropicFickDiffusion fick(Dx, Dy, Dz);

    assert(!fick.isIsotropic());

    std::array<double, 3> gradient = {1000.0, 500.0, 100.0};
    auto flux = fick.calculateFlux(gradient);

    // Jx = -Dx × ∂C/∂x, etc.
    assert(flux[0] == -Dx * gradient[0]);
    assert(flux[1] == -Dy * gradient[1]);
    assert(flux[2] == -Dz * gradient[2]);

    std::cout << "  → Flux: [" << flux[0] << ", " << flux[1] << ", " << flux[2] << "] mol/(m²·s)\n";
    std::cout << "  ✓ Anisotropic Fick diffusion works\n";
}

// Test anisotropic source term
void testAnisotropicSourceTerm() {
    std::cout << "\nTesting anisotropic source term...\n";

    AnisotropicFickDiffusion fick(1.0e-9, 2.0e-9, 3.0e-9);

    std::array<double, 3> secondDerivatives = {1.0e6, 5.0e5, 2.0e5};
    double source = fick.calculateSourceTerm(secondDerivatives);

    auto D = fick.getDiffusionTensor().getDiagonal();
    double expected = D[0] * secondDerivatives[0] +
                     D[1] * secondDerivatives[1] +
                     D[2] * secondDerivatives[2];

    assert(std::abs(source - expected) < 1.0e-12);

    std::cout << "  → ∂C/∂t = " << source << " mol/(m³·s)\n";
    std::cout << "  ✓ Anisotropic source term works\n";
}

int main() {
    std::cout << "Phase 21 Tests - Diffusion Equation Model\n";
    std::cout << "=========================================\n";

    testConstantDiffusion();
    testArrheniusDiffusion();
    testChapmanEnskogDiffusion();
    testConcentrationDependentDiffusion();
    testAnisotropicDiffusion();
    testIsotropicCheck();
    testFickDiffusionConstruction();
    testFluxCalculation();
    testFluxMagnitude();
    testSourceTermCalculation();
    testDiffusionTime();
    testDiffusionLength();
    testPecletNumber();
    testAnisotropicFickDiffusion();
    testAnisotropicSourceTerm();

    std::cout << "\n=========================================\n";
    std::cout << "All Phase 21 tests passed!\n";
    std::cout << "Diffusion equation model verified.\n";

    return 0;
}
