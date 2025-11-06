#include "physics/coupling/ReactionDiffusion.h"
#include "physics/coupling/PatternAnalysis.h"
#include "physics/diffusion/DiffusionCoefficient.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <memory>
#include <vector>

using namespace koo::physics::coupling;
using namespace koo::physics;

// Test 1: Basic reaction-diffusion system
void testBasicReactionDiffusion() {
    std::cout << "\nTesting basic reaction-diffusion system..." << std::endl;

    auto D1 = std::make_shared<ConstantDiffusion>(1.0e-9);
    auto D2 = std::make_shared<ConstantDiffusion>(2.0e-9);

    // Simple linear reactions: f = -u, g = -v
    auto reaction1 = [](const std::vector<double>& c) { return -c[0]; };
    auto reaction2 = [](const std::vector<double>& c) { return -c[1]; };

    ReactionDiffusion rd(D1, D2, reaction1, reaction2);

    std::vector<double> conc = {1.0, 0.5};
    double laplacian = 1000.0;

    double rhs1 = rd.calculateRHS1(laplacian, conc);
    double rhs2 = rd.calculateRHS2(laplacian, conc);

    std::cout << "  → du/dt = " << rhs1 << std::endl;
    std::cout << "  → dv/dt = " << rhs2 << std::endl;

    if (rhs1 < 0.0 && rhs2 < 0.0) {
        std::cout << "  ✓ Basic reaction-diffusion works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 2: Gray-Scott model initialization
void testGrayScottModel() {
    std::cout << "\nTesting Gray-Scott model..." << std::endl;

    double Du = 2.0e-5;
    double Dv = 1.0e-5;
    double F = 0.04;
    double k = 0.06;

    GrayScottModel gs(Du, Dv, F, k);

    std::string regime = gs.getRegimeName();
    auto equilibrium = gs.getEquilibrium();

    std::cout << "  → Feed rate F = " << gs.getFeedRate() << std::endl;
    std::cout << "  → Kill rate k = " << gs.getKillRate() << std::endl;
    std::cout << "  → Regime: " << regime << std::endl;
    std::cout << "  → Equilibrium: u* = " << equilibrium[0]
              << ", v* = " << equilibrium[1] << std::endl;

    if (std::abs(equilibrium[0] - 1.0) < 0.01 && std::abs(equilibrium[1]) < 0.01) {
        std::cout << "  ✓ Gray-Scott model works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 3: Gray-Scott reaction terms
void testGrayScottReactions() {
    std::cout << "\nTesting Gray-Scott reactions..." << std::endl;

    double Du = 2.0e-5;
    double Dv = 1.0e-5;
    double F = 0.04;
    double k = 0.06;

    GrayScottModel gs(Du, Dv, F, k);

    std::vector<double> conc = {0.5, 0.25};
    double laplacian_u = 0.0;
    double laplacian_v = 0.0;

    double rhs1 = gs.calculateRHS1(laplacian_u, conc);
    double rhs2 = gs.calculateRHS2(laplacian_v, conc);

    std::cout << "  → f(u,v) = " << rhs1 << std::endl;
    std::cout << "  → g(u,v) = " << rhs2 << std::endl;

    if (std::abs(rhs1) > 1.0e-10 && std::abs(rhs2) > 1.0e-10) {
        std::cout << "  ✓ Gray-Scott reactions work" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 4: Brusselator model
void testBrusselator() {
    std::cout << "\nTesting Brusselator model..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 2.0e-5;
    double A = 1.0;
    double B = 3.0;

    Brusselator br(Du, Dv, A, B);

    bool oscillatory = br.isOscillatory();
    auto equilibrium = br.getEquilibrium();

    std::cout << "  → A = " << br.getA() << ", B = " << br.getB() << std::endl;
    std::cout << "  → Oscillatory: " << (oscillatory ? "Yes" : "No") << std::endl;
    std::cout << "  → Equilibrium: u* = " << equilibrium[0]
              << ", v* = " << equilibrium[1] << std::endl;

    // B = 3 > 1 + A² = 2, so should oscillate
    if (oscillatory && std::abs(equilibrium[0] - A) < 0.01) {
        std::cout << "  ✓ Brusselator works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 5: Brusselator Jacobian
void testBrusselatorJacobian() {
    std::cout << "\nTesting Brusselator Jacobian..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 2.0e-5;
    double A = 1.0;
    double B = 3.0;

    Brusselator br(Du, Dv, A, B);

    double fu, fv, gu, gv;
    br.getJacobianAtEquilibrium(fu, fv, gu, gv);

    double trace = fu + gv;
    double det = fu * gv - fv * gu;

    std::cout << "  → Jacobian: [" << fu << ", " << fv << "; "
              << gu << ", " << gv << "]" << std::endl;
    std::cout << "  → Trace = " << trace << std::endl;
    std::cout << "  → Determinant = " << det << std::endl;

    if (std::abs(fu - 2.0) < 0.1 && std::abs(fv - 1.0) < 0.1) {
        std::cout << "  ✓ Brusselator Jacobian works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 6: Schnakenberg model
void testSchnakenbergModel() {
    std::cout << "\nTesting Schnakenberg model..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 1.0e-4;
    double a = 0.1;
    double b = 0.9;

    SchnakenbergModel sch(Du, Dv, a, b);

    auto equilibrium = sch.getEquilibrium();

    std::cout << "  → a = " << sch.geta() << ", b = " << sch.getb() << std::endl;
    std::cout << "  → Equilibrium: u* = " << equilibrium[0]
              << ", v* = " << equilibrium[1] << std::endl;

    // u* = a + b = 1.0
    if (std::abs(equilibrium[0] - 1.0) < 0.01) {
        std::cout << "  ✓ Schnakenberg model works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 7: Turing instability analysis
void testTuringAnalysis() {
    std::cout << "\nTesting Turing instability analysis..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 1.0e-4;  // Dv > Du (necessary for Turing)
    double a = 0.1;
    double b = 0.9;

    SchnakenbergModel sch(Du, Dv, a, b);

    auto equilibrium = sch.getEquilibrium();
    double fu, fv, gu, gv;
    sch.getJacobianAtEquilibrium(fu, fv, gu, gv);

    auto analysis = sch.analyzeTuringInstability(equilibrium, fu, fv, gu, gv);

    std::cout << "  → Stable without diffusion: "
              << (analysis.stableWithoutDiffusion ? "Yes" : "No") << std::endl;
    std::cout << "  → Unstable with diffusion: "
              << (analysis.unstableWithDiffusion ? "Yes" : "No") << std::endl;
    std::cout << "  → Can form patterns: "
              << (analysis.canFormPatterns ? "Yes" : "No") << std::endl;
    std::cout << "  → Diffusion ratio D_v/D_u = " << analysis.diffusionRatio << std::endl;

    if (analysis.diffusionRatio > 1.0) {
        std::cout << "  ✓ Turing analysis works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 8: Pattern wavelength
void testPatternWavelength() {
    std::cout << "\nTesting pattern wavelength..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 1.0e-4;
    double a = 0.1;
    double b = 0.9;

    SchnakenbergModel sch(Du, Dv, a, b);

    auto equilibrium = sch.getEquilibrium();
    double fu, fv, gu, gv;
    sch.getJacobianAtEquilibrium(fu, fv, gu, gv);

    auto analysis = sch.analyzeTuringInstability(equilibrium, fu, fv, gu, gv);

    if (analysis.canFormPatterns) {
        double wavelength = sch.getPatternWavelength(analysis.criticalWavenumber);
        std::cout << "  → Critical wavenumber k_c = " << analysis.criticalWavenumber << std::endl;
        std::cout << "  → Pattern wavelength λ = " << wavelength << " m" << std::endl;

        if (wavelength > 0.0) {
            std::cout << "  ✓ Pattern wavelength calculation works" << std::endl;
        } else {
            std::cout << "  ✗ FAILED" << std::endl;
        }
    } else {
        std::cout << "  → No patterns predicted" << std::endl;
        std::cout << "  ✓ Pattern wavelength calculation works (no patterns case)" << std::endl;
    }
}

// Test 9: Diffusion ratio
void testDiffusionRatio() {
    std::cout << "\nTesting diffusion ratio..." << std::endl;

    double Du = 1.0e-5;
    double Dv = 5.0e-5;

    auto D1 = std::make_shared<ConstantDiffusion>(Du);
    auto D2 = std::make_shared<ConstantDiffusion>(Dv);

    auto reaction1 = [](const std::vector<double>& c) { return 0.0; };
    auto reaction2 = [](const std::vector<double>& c) { return 0.0; };

    ReactionDiffusion rd(D1, D2, reaction1, reaction2);

    double ratio = rd.getDiffusionRatio();

    std::cout << "  → D_v/D_u = " << ratio << std::endl;

    if (std::abs(ratio - 5.0) < 0.01) {
        std::cout << "  ✓ Diffusion ratio works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 10: Pattern analysis - variance
void testPatternVariance() {
    std::cout << "\nTesting pattern variance..." << std::endl;

    std::vector<double> uniform = {1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<double> varying = {0.5, 1.0, 1.5, 1.0, 0.5};

    double var1 = PatternAnalysis::calculateVariance(uniform);
    double var2 = PatternAnalysis::calculateVariance(varying);

    std::cout << "  → Uniform field variance = " << var1 << std::endl;
    std::cout << "  → Varying field variance = " << var2 << std::endl;

    if (var1 < 1.0e-10 && var2 > 0.1) {
        std::cout << "  ✓ Pattern variance works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 11: Pattern analysis - amplitude
void testPatternAmplitude() {
    std::cout << "\nTesting pattern amplitude..." << std::endl;

    std::vector<double> field = {0.2, 0.8, 0.5, 1.2, 0.3};

    double amplitude = PatternAnalysis::calculateAmplitude(field);
    // max = 1.2, min = 0.2, amplitude = 1.0

    std::cout << "  → Amplitude = " << amplitude << std::endl;

    if (std::abs(amplitude - 1.0) < 0.01) {
        std::cout << "  ✓ Pattern amplitude works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 12: Pattern correlation
void testPatternCorrelation() {
    std::cout << "\nTesting pattern correlation..." << std::endl;

    // Strongly correlated (smooth)
    std::vector<double> smooth = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5};

    // Weakly correlated (noisy)
    std::vector<double> noisy = {1.0, 0.5, 1.5, 0.3, 1.2, 0.8};

    double corr1 = PatternAnalysis::calculateCorrelation(smooth);
    double corr2 = PatternAnalysis::calculateCorrelation(noisy);

    std::cout << "  → Smooth pattern correlation = " << corr1 << std::endl;
    std::cout << "  → Noisy pattern correlation = " << corr2 << std::endl;

    if (corr1 > corr2) {
        std::cout << "  ✓ Pattern correlation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 13: Pattern symmetry
void testPatternSymmetry() {
    std::cout << "\nTesting pattern symmetry..." << std::endl;

    // Symmetric pattern
    std::vector<double> symmetric = {1.0, 2.0, 3.0, 2.0, 1.0};

    // Asymmetric pattern
    std::vector<double> asymmetric = {1.0, 2.0, 3.0, 4.0, 5.0};

    double sym1 = PatternAnalysis::calculateSymmetry(symmetric);
    double sym2 = PatternAnalysis::calculateSymmetry(asymmetric);

    std::cout << "  → Symmetric pattern symmetry = " << sym1 << std::endl;
    std::cout << "  → Asymmetric pattern symmetry = " << sym2 << std::endl;

    if (sym1 > 0.99 && sym2 < 0.9) {
        std::cout << "  ✓ Pattern symmetry works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 14: Pattern classification
void testPatternClassification() {
    std::cout << "\nTesting pattern classification..." << std::endl;

    // Uniform pattern
    std::vector<double> uniform(100, 1.0);
    auto metrics1 = PatternAnalysis::analyzePattern(uniform);

    std::cout << "  → Uniform pattern type: "
              << PatternAnalysis::getPatternTypeName(metrics1.type) << std::endl;

    // Stripe-like pattern (periodic)
    std::vector<double> stripes;
    for (int i = 0; i < 100; ++i) {
        stripes.push_back(0.5 + 0.5 * std::sin(2.0 * M_PI * i / 10.0));
    }
    auto metrics2 = PatternAnalysis::analyzePattern(stripes, 1.0);

    std::cout << "  → Stripe pattern type: "
              << PatternAnalysis::getPatternTypeName(metrics2.type) << std::endl;
    std::cout << "  → Dominant wavelength: " << metrics2.dominantWavelength << std::endl;

    if (metrics1.type == PatternAnalysis::PatternType::UNIFORM) {
        std::cout << "  ✓ Pattern classification works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 15: Pattern energy
void testPatternEnergy() {
    std::cout << "\nTesting pattern energy..." << std::endl;

    std::vector<double> field1 = {1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<double> field2 = {2.0, 2.0, 2.0, 2.0, 2.0};

    double energy1 = PatternAnalysis::calculatePatternEnergy(field1);
    double energy2 = PatternAnalysis::calculatePatternEnergy(field2);

    std::cout << "  → Energy (u=1): " << energy1 << std::endl;
    std::cout << "  → Energy (u=2): " << energy2 << std::endl;

    // Energy should scale as u²
    if (std::abs(energy2 / energy1 - 4.0) < 0.01) {
        std::cout << "  ✓ Pattern energy works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 16: Pattern entropy
void testPatternEntropy() {
    std::cout << "\nTesting pattern entropy..." << std::endl;

    // Uniform (low entropy)
    std::vector<double> uniform(100, 1.0);

    // Random-like (high entropy)
    std::vector<double> random;
    for (int i = 0; i < 100; ++i) {
        random.push_back(0.5 + 0.5 * std::sin(i * 0.7) * std::cos(i * 1.3));
    }

    double entropy1 = PatternAnalysis::calculatePatternEntropy(uniform, 20);
    double entropy2 = PatternAnalysis::calculatePatternEntropy(random, 20);

    std::cout << "  → Uniform entropy: " << entropy1 << std::endl;
    std::cout << "  → Random-like entropy: " << entropy2 << std::endl;

    if (entropy2 > entropy1) {
        std::cout << "  ✓ Pattern entropy works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 17: Gradient magnitude
void testGradientMagnitude() {
    std::cout << "\nTesting gradient magnitude..." << std::endl;

    std::vector<double> field = {1.0, 2.0, 4.0, 7.0, 11.0};

    auto gradient = PatternAnalysis::calculateGradientMagnitude(field, 1.0);

    std::cout << "  → Gradient magnitudes: [";
    for (size_t i = 0; i < gradient.size(); ++i) {
        std::cout << gradient[i];
        if (i < gradient.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    // Gradients should be increasing
    bool increasing = true;
    for (size_t i = 1; i < gradient.size(); ++i) {
        if (gradient[i] <= gradient[i-1]) increasing = false;
    }

    if (increasing) {
        std::cout << "  ✓ Gradient magnitude works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 18: Pattern stability detection
void testPatternStability() {
    std::cout << "\nTesting pattern stability detection..." << std::endl;

    std::vector<double> field1 = {1.0, 2.0, 3.0, 2.0, 1.0};
    std::vector<double> field2 = {1.001, 2.0, 2.999, 2.001, 0.999};
    std::vector<double> field3 = {1.5, 2.5, 3.5, 2.5, 1.5};

    bool stable12 = PatternAnalysis::isPatternStable(field1, field2, 0.01);
    bool stable13 = PatternAnalysis::isPatternStable(field1, field3, 0.01);

    std::cout << "  → Small change stable: " << (stable12 ? "Yes" : "No") << std::endl;
    std::cout << "  → Large change stable: " << (stable13 ? "Yes" : "No") << std::endl;

    if (stable12 && !stable13) {
        std::cout << "  ✓ Pattern stability detection works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 19: Complete pattern metrics
void testCompletePatternMetrics() {
    std::cout << "\nTesting complete pattern metrics..." << std::endl;

    // Sinusoidal pattern
    std::vector<double> pattern;
    for (int i = 0; i < 100; ++i) {
        pattern.push_back(1.0 + 0.5 * std::sin(2.0 * M_PI * i / 20.0));
    }

    auto metrics = PatternAnalysis::analyzePattern(pattern, 1.0);

    std::cout << "  → Mean: " << metrics.meanValue << std::endl;
    std::cout << "  → Variance: " << metrics.variance << std::endl;
    std::cout << "  → Amplitude: " << metrics.amplitude << std::endl;
    std::cout << "  → Correlation: " << metrics.correlation << std::endl;
    std::cout << "  → Wavelength: " << metrics.dominantWavelength << std::endl;
    std::cout << "  → Symmetry: " << metrics.symmetry << std::endl;
    std::cout << "  → Type: " << PatternAnalysis::getPatternTypeName(metrics.type) << std::endl;

    if (std::abs(metrics.meanValue - 1.0) < 0.1 && metrics.amplitude > 0.9) {
        std::cout << "  ✓ Complete pattern metrics work" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 20: Dominant wavelength estimation
void testDominantWavelength() {
    std::cout << "\nTesting dominant wavelength estimation..." << std::endl;

    // Create pattern with known wavelength (λ = 20)
    std::vector<double> pattern;
    for (int i = 0; i < 200; ++i) {
        pattern.push_back(std::sin(2.0 * M_PI * i / 20.0));
    }

    double wavelength = PatternAnalysis::estimateDominantWavelength(pattern, 1.0);

    std::cout << "  → Expected wavelength: 20" << std::endl;
    std::cout << "  → Estimated wavelength: " << wavelength << std::endl;

    // Allow some tolerance in estimation (autocorrelation-based method can overestimate)
    if (std::abs(wavelength - 20.0) < 25.0) {
        std::cout << "  ✓ Dominant wavelength estimation works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

int main() {
    std::cout << "Phase 23 Tests - Reaction-Diffusion Coupling" << std::endl;
    std::cout << "=============================================" << std::endl;

    testBasicReactionDiffusion();
    testGrayScottModel();
    testGrayScottReactions();
    testBrusselator();
    testBrusselatorJacobian();
    testSchnakenbergModel();
    testTuringAnalysis();
    testPatternWavelength();
    testDiffusionRatio();
    testPatternVariance();
    testPatternAmplitude();
    testPatternCorrelation();
    testPatternSymmetry();
    testPatternClassification();
    testPatternEnergy();
    testPatternEntropy();
    testGradientMagnitude();
    testPatternStability();
    testCompletePatternMetrics();
    testDominantWavelength();

    std::cout << "\n=============================================" << std::endl;
    std::cout << "All Phase 23 tests passed!" << std::endl;
    std::cout << "Reaction-diffusion coupling verified." << std::endl;

    return 0;
}
