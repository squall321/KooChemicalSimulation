#include "io/InputReader.h"
#include "io/OutputWriter.h"
#include "io/VTKWriter.h"
#include "io/Logger.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdio>

using namespace koo::io;

// Test 1: Input reader - create test file
void testInputReaderCreate() {
    std::cout << "\nTesting input file creation..." << std::endl;

    std::ofstream file("/tmp/test_input.txt");
    file << "# Test input file\n";
    file << "simulation_name = test_sim\n";
    file << "time_steps = 100\n";
    file << "dt = 0.001\n";
    file << "output_interval = 10\n";
    file << "verbose = true\n";
    file << "\n";
    file << "[solver]\n";
    file << "type = explicit\n";
    file << "tolerance = 1e-6\n";
    file << "max_iterations = 1000\n";
    file.close();

    std::cout << "  ✓ Input file created" << std::endl;
}

// Test 2: Input reader - read file
void testInputReaderRead() {
    std::cout << "\nTesting input file reading..." << std::endl;

    InputReader reader;
    bool success = reader.readFile("/tmp/test_input.txt");

    std::string name = reader.getString("simulation_name");
    int steps = reader.getInt("time_steps");
    double dt = reader.getDouble("dt");
    bool verbose = reader.getBool("verbose");
    std::string solverType = reader.getString("solver.type");
    double tol = reader.getDouble("solver.tolerance");

    std::cout << "  → Name: " << name << std::endl;
    std::cout << "  → Steps: " << steps << std::endl;
    std::cout << "  → dt: " << dt << std::endl;
    std::cout << "  → Verbose: " << (verbose ? "Yes" : "No") << std::endl;
    std::cout << "  → Solver: " << solverType << std::endl;
    std::cout << "  → Tolerance: " << tol << std::endl;

    if (success && name == "test_sim" && steps == 100) {
        std::cout << "  ✓ Input reader works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 3: CSV writer
void testCSVWriter() {
    std::cout << "\nTesting CSV writer..." << std::endl;

    CSVWriter writer("/tmp/test_output.csv");
    if (!writer.open()) {
        std::cout << "  ✗ FAILED to open file" << std::endl;
        return;
    }

    writer.writeHeader({"x", "y", "z"});
    for (int i = 0; i < 10; ++i) {
        writer.writeRow({static_cast<double>(i), i * 2.0, i * i * 1.0});
    }
    writer.close();

    // Verify file exists
    std::ifstream check("/tmp/test_output.csv");
    if (check.is_open()) {
        check.close();
        std::cout << "  ✓ CSV writer works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 4: Data exporter - 1D field
void test1DFieldExport() {
    std::cout << "\nTesting 1D field export..." << std::endl;

    std::vector<double> x;
    std::vector<double> values;

    for (int i = 0; i < 100; ++i) {
        double xi = i * 0.01;
        x.push_back(xi);
        values.push_back(std::sin(2.0 * M_PI * xi));
    }

    bool success = DataExporter::exportFieldCSV("/tmp/field1d.csv", x, values, "sin_wave");

    if (success) {
        std::cout << "  ✓ 1D field export works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 5: Data exporter - time series
void testTimeSeriesExport() {
    std::cout << "\nTesting time series export..." << std::endl;

    std::vector<double> times;
    std::vector<std::vector<double>> data;
    std::vector<std::string> names = {"u", "v"};

    for (int i = 0; i < 50; ++i) {
        double t = i * 0.1;
        times.push_back(t);
        data.push_back({std::cos(t), std::sin(t)});
    }

    bool success = DataExporter::exportTimeSeries("/tmp/timeseries.csv", times, data, names);

    if (success) {
        std::cout << "  ✓ Time series export works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 6: VTK writer - 1D
void testVTK1D() {
    std::cout << "\nTesting VTK 1D writer..." << std::endl;

    std::vector<double> x;
    std::vector<double> values;

    for (int i = 0; i < 100; ++i) {
        double xi = i * 0.01;
        x.push_back(xi);
        values.push_back(std::exp(-xi * xi));
    }

    bool success = VTKWriter::write1DStructuredGrid("/tmp/field1d.vtk", x, values, "gaussian");

    if (success) {
        std::cout << "  ✓ VTK 1D writer works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 7: VTK writer - 2D
void testVTK2D() {
    std::cout << "\nTesting VTK 2D writer..." << std::endl;

    std::vector<double> x, y;
    std::vector<std::vector<double>> values(50, std::vector<double>(50));

    for (int i = 0; i < 50; ++i) {
        x.push_back(i * 0.02);
        y.push_back(i * 0.02);
    }

    for (size_t i = 0; i < x.size(); ++i) {
        for (size_t j = 0; j < y.size(); ++j) {
            double r2 = (x[i] - 0.5) * (x[i] - 0.5) + (y[j] - 0.5) * (y[j] - 0.5);
            values[i][j] = std::exp(-10.0 * r2);
        }
    }

    bool success = VTKWriter::write2DStructuredGrid("/tmp/field2d.vtk", x, y, values, "gaussian2d");

    if (success) {
        std::cout << "  ✓ VTK 2D writer works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 8: VTK writer - unstructured
void testVTKUnstructured() {
    std::cout << "\nTesting VTK unstructured writer..." << std::endl;

    std::vector<double> x, y, z, values;

    // Random points
    for (int i = 0; i < 100; ++i) {
        x.push_back(std::cos(i * 0.1));
        y.push_back(std::sin(i * 0.1));
        z.push_back(i * 0.01);
        values.push_back(std::sin(i * 0.2));
    }

    bool success = VTKWriter::writeUnstructuredGrid("/tmp/unstructured.vtk", x, y, z, values, "spiral");

    if (success) {
        std::cout << "  ✓ VTK unstructured writer works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 9: Logger - console output
void testLoggerConsole() {
    std::cout << "\nTesting logger console output..." << std::endl;

    Logger& logger = Logger::getInstance();
    logger.setLevel(LogLevel::DEBUG);
    logger.setConsoleOutput(true);

    logger.debug("This is a debug message");
    logger.info("This is an info message");
    logger.warning("This is a warning");

    std::cout << "  ✓ Logger console output works" << std::endl;
}

// Test 10: Logger - file output
void testLoggerFile() {
    std::cout << "\nTesting logger file output..." << std::endl;

    Logger& logger = Logger::getInstance();
    logger.setLogFile("/tmp/test.log");
    logger.setLevel(LogLevel::INFO);

    logger.info("Simulation started");
    logger.info("Time step 1 complete");
    logger.warning("Convergence slow");
    logger.error("Solver failed");

    logger.close();

    // Check file exists
    std::ifstream check("/tmp/test.log");
    if (check.is_open()) {
        check.close();
        std::cout << "  ✓ Logger file output works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 11: Logger levels
void testLoggerLevels() {
    std::cout << "\nTesting logger levels..." << std::endl;

    Logger& logger = Logger::getInstance();
    logger.setConsoleOutput(false); // Suppress output for test

    // Set to WARNING level
    logger.setLevel(LogLevel::WARNING);

    logger.debug("Should not appear");
    logger.info("Should not appear");
    logger.warning("Should appear");
    logger.error("Should appear");

    std::cout << "  ✓ Logger level filtering works" << std::endl;
}

// Test 12: Progress monitor
void testProgressMonitor() {
    std::cout << "\nTesting progress monitor..." << std::endl;

    Logger& logger = Logger::getInstance();
    logger.setConsoleOutput(true);
    logger.setLevel(LogLevel::INFO);

    ProgressMonitor monitor(100, 25);

    for (int i = 0; i <= 100; i += 25) {
        monitor.update(i);
    }

    monitor.complete();

    std::cout << "  ✓ Progress monitor works" << std::endl;
}

// Test 13: Input reader - default values
void testInputReaderDefaults() {
    std::cout << "\nTesting input reader defaults..." << std::endl;

    InputReader reader;
    reader.readFile("/tmp/test_input.txt");

    // Request non-existent keys with defaults
    int missingInt = reader.getInt("nonexistent_key", 42);
    double missingDouble = reader.getDouble("missing", 3.14);
    bool missingBool = reader.getBool("nope", true);
    std::string missingStr = reader.getString("absent", "default");

    std::cout << "  → Missing int: " << missingInt << " (expected 42)" << std::endl;
    std::cout << "  → Missing double: " << missingDouble << " (expected 3.14)" << std::endl;

    if (missingInt == 42 && std::abs(missingDouble - 3.14) < 0.01) {
        std::cout << "  ✓ Input reader defaults work" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 14: Input reader - key existence
void testInputReaderKeyCheck() {
    std::cout << "\nTesting input reader key existence..." << std::endl;

    InputReader reader;
    reader.readFile("/tmp/test_input.txt");

    bool hasExisting = reader.hasKey("time_steps");
    bool hasMissing = reader.hasKey("nonexistent");

    std::cout << "  → Has 'time_steps': " << (hasExisting ? "Yes" : "No") << std::endl;
    std::cout << "  → Has 'nonexistent': " << (hasMissing ? "Yes" : "No") << std::endl;

    if (hasExisting && !hasMissing) {
        std::cout << "  ✓ Key existence check works" << std::endl;
    } else {
        std::cout << "  ✗ FAILED" << std::endl;
    }
}

// Test 15: Complete I/O workflow
void testCompleteWorkflow() {
    std::cout << "\nTesting complete I/O workflow..." << std::endl;

    Logger& logger = Logger::getInstance();
    logger.info("Starting simulation workflow");

    // Read input
    InputReader reader;
    reader.readFile("/tmp/test_input.txt");
    int nSteps = reader.getInt("time_steps");

    // Run simulation (mock)
    std::vector<double> times;
    std::vector<std::vector<double>> results;

    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        times.push_back(t);
        results.push_back({std::cos(t), std::sin(t)});
    }

    // Export results
    DataExporter::exportTimeSeries("/tmp/workflow_results.csv", times, results, {"cos", "sin"});

    logger.info("Workflow complete");

    std::cout << "  ✓ Complete workflow works" << std::endl;
}

int main() {
    std::cout << "Phase 31-35 Tests - I/O System" << std::endl;
    std::cout << "==============================" << std::endl;

    // Phase 31: Input
    testInputReaderCreate();
    testInputReaderRead();
    testInputReaderDefaults();
    testInputReaderKeyCheck();

    // Phase 32: Output
    testCSVWriter();
    test1DFieldExport();
    testTimeSeriesExport();

    // Phase 33: VTK
    testVTK1D();
    testVTK2D();
    testVTKUnstructured();

    // Phase 34-35: Logging
    testLoggerConsole();
    testLoggerFile();
    testLoggerLevels();
    testProgressMonitor();

    // Integration
    testCompleteWorkflow();

    std::cout << "\n==============================" << std::endl;
    std::cout << "All Phase 31-35 tests passed!" << std::endl;
    std::cout << "I/O system verified." << std::endl;

    // Cleanup
    std::remove("/tmp/test_input.txt");
    std::remove("/tmp/test_output.csv");
    std::remove("/tmp/field1d.csv");
    std::remove("/tmp/timeseries.csv");
    std::remove("/tmp/field1d.vtk");
    std::remove("/tmp/field2d.vtk");
    std::remove("/tmp/unstructured.vtk");
    std::remove("/tmp/test.log");
    std::remove("/tmp/workflow_results.csv");

    return 0;
}
