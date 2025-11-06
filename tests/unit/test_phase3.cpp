/**
 * @file test_phase3.cpp
 * @brief Unit tests for Phase 3 components
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha3
 * @date 2025-11-06
 *
 * This file contains tests for CommonTypes, PhysicalQuantity, Exception, and Logger.
 */

#include "core/types/CommonTypes.h"
#include "core/types/PhysicalQuantity.h"
#include "utils/error/Exception.h"
#include "utils/logger/Logger.h"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace koo::core::types;
using namespace koo::utils::error;
using namespace koo::utils::logger;

// ============================================================================
// Test CommonTypes
// ============================================================================

void testVector3D() {
    std::cout << "Testing Vector3D..." << std::endl;

    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);

    // Test addition
    Vector3D v3 = v1 + v2;
    assert(std::abs(v3[0] - 5.0) < 1e-10);
    assert(std::abs(v3[1] - 7.0) < 1e-10);
    assert(std::abs(v3[2] - 9.0) < 1e-10);

    // Test scalar multiplication
    Vector3D v4 = v1 * 2.0;
    assert(std::abs(v4[0] - 2.0) < 1e-10);
    assert(std::abs(v4[1] - 4.0) < 1e-10);
    assert(std::abs(v4[2] - 6.0) < 1e-10);

    // Test dot product
    double dot = v1.dot(v2);
    assert(std::abs(dot - 32.0) < 1e-10);  // 1*4 + 2*5 + 3*6 = 32

    // Test norm
    Vector3D v5(3.0, 4.0, 0.0);
    assert(std::abs(v5.norm() - 5.0) < 1e-10);  // 3-4-5 triangle

    std::cout << "  Vector3D tests passed!" << std::endl;
}

void testVectorXd() {
    std::cout << "Testing VectorXd..." << std::endl;

    VectorXd v(5);
    for (size_t i = 0; i < 5; ++i) {
        v[i] = static_cast<double>(i);
    }

    assert(v.size() == 5);
    assert(std::abs(v[0] - 0.0) < 1e-10);
    assert(std::abs(v[4] - 4.0) < 1e-10);

    std::cout << "  VectorXd tests passed!" << std::endl;
}

void testMatrixXd() {
    std::cout << "Testing MatrixXd..." << std::endl;

    MatrixXd m(3, 3);
    m(0, 0) = 1.0;
    m(1, 1) = 2.0;
    m(2, 2) = 3.0;

    assert(m.rows() == 3);
    assert(m.cols() == 3);
    assert(std::abs(m(0, 0) - 1.0) < 1e-10);
    assert(std::abs(m(1, 1) - 2.0) < 1e-10);
    assert(std::abs(m(2, 2) - 3.0) < 1e-10);

    std::cout << "  MatrixXd tests passed!" << std::endl;
}

// ============================================================================
// Test PhysicalQuantity
// ============================================================================

void testPhysicalQuantity() {
    std::cout << "Testing PhysicalQuantity..." << std::endl;

    // Test basic quantities
    auto length = Units::Meter(5.0);
    auto time = Units::Second(2.0);

    // Test division to get velocity
    auto velocity = length / time;
    assert(std::abs(velocity.value() - 2.5) < 1e-10);

    // Test multiplication
    auto area = Units::Meter(3.0) * Units::Meter(4.0);
    assert(std::abs(area.value() - 12.0) < 1e-10);

    // Test addition (same dimensions)
    auto length2 = Units::Meter(3.0);
    auto totalLength = length + length2;
    assert(std::abs(totalLength.value() - 8.0) < 1e-10);

    // Test unit conversion
    auto cm = Units::Centimeter(100.0);
    auto m = Units::Meter(1.0);
    auto sum = cm + m;  // Should work since both are lengths
    assert(std::abs(sum.value() - 2.0) < 1e-10);  // 1.0 + 1.0 = 2.0 meters

    // Test temperature
    auto tempK = Units::Kelvin(298.15);
    auto tempC = Units::Celsius(25.0);
    assert(std::abs(tempK.value() - tempC.value()) < 0.01);

    // Test comparison
    assert(Units::Meter(5.0) > Units::Meter(3.0));
    assert(Units::Meter(2.0) < Units::Meter(5.0));

    std::cout << "  PhysicalQuantity tests passed!" << std::endl;
}

void testPhysicalQuantityErrors() {
    std::cout << "Testing PhysicalQuantity error handling..." << std::endl;

    bool exceptionCaught = false;

    try {
        auto length = Units::Meter(5.0);
        auto mass = Units::Kilogram(2.0);
        // This should throw - cannot add length and mass
        auto invalid = length + mass;
        (void)invalid;
    } catch (const std::invalid_argument& e) {
        exceptionCaught = true;
    }

    assert(exceptionCaught);
    std::cout << "  PhysicalQuantity error handling tests passed!" << std::endl;
}

// ============================================================================
// Test Exception System
// ============================================================================

void testExceptions() {
    std::cout << "Testing Exception system..." << std::endl;

    // Test basic exception
    try {
        throw InvalidArgumentException("Test error message");
    } catch (const KooException& e) {
        assert(std::string(e.message()) == "Test error message");
        assert(std::string(e.type()) == "InvalidArgumentException");
    }

    // Test exception with location
    try {
        THROW_EXCEPTION(RuntimeException, "Runtime error test");
    } catch (const KooException& e) {
        // Message should contain file and line information
        std::string msg(e.what());
        assert(msg.find("Runtime error test") != std::string::npos);
    }

    // Test different exception types
    try {
        throw OutOfRangeException("Out of range");
    } catch (const KooException& e) {
        assert(std::string(e.type()) == "OutOfRangeException");
    }

    try {
        throw ConvergenceException("Failed to converge");
    } catch (const KooException& e) {
        assert(std::string(e.type()) == "ConvergenceException");
    }

    std::cout << "  Exception system tests passed!" << std::endl;
}

void testAssertMacros() {
    std::cout << "Testing assertion macros..." << std::endl;

    // Test KOO_ASSERT (should not throw)
    KOO_ASSERT(true, "This should not throw");

    // Test KOO_ASSERT (should throw)
    bool caught = false;
    try {
        KOO_ASSERT(false, "This should throw");
    } catch (const LogicException&) {
        caught = true;
    }
    assert(caught);

    // Test KOO_CHECK_RANGE
    KOO_CHECK_RANGE(5, 0, 10, "value");

    caught = false;
    try {
        KOO_CHECK_RANGE(15, 0, 10, "value");
    } catch (const OutOfRangeException&) {
        caught = true;
    }
    assert(caught);

    std::cout << "  Assertion macro tests passed!" << std::endl;
}

// ============================================================================
// Test Logger
// ============================================================================

void testLogger() {
    std::cout << "Testing Logger system..." << std::endl;

    // Initialize logger
    Logger::initialize("TestLogger");
    Logger::setLevel(LogLevel::DEBUG);

    // Test different log levels
    Logger::info("This is an info message");
    Logger::debug("This is a debug message");
    Logger::warning("This is a warning message");
    Logger::error("This is an error message");

    // Test macros
    LOG_INFO("Testing LOG_INFO macro");
    LOG_DEBUG("Testing LOG_DEBUG macro");
    LOG_WARNING("Testing LOG_WARNING macro");

    // No assertions - just make sure it doesn't crash
    std::cout << "  Logger system tests passed!" << std::endl;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Phase 3 Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        // Test CommonTypes
        testVector3D();
        testVectorXd();
        testMatrixXd();

        // Test PhysicalQuantity
        testPhysicalQuantity();
        testPhysicalQuantityErrors();

        // Test Exception system
        testExceptions();
        testAssertMacros();

        // Test Logger
        testLogger();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  All Phase 3 tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
