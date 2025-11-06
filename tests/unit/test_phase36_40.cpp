#include "config/Config.h"
#include "config/schema/ConfigSchema.h"
#include "config/preset/ConfigPreset.h"
#include "config/env/ConfigEnvironment.h"
#include "config/util/ConfigUtil.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdlib>

using namespace koo::config;

// Test 1: Schema Definition
void testSchemaDefinition() {
    std::cout << "\nTesting schema definition..." << std::endl;

    schema::ConfigSchema schema("Test", "1.0");

    // Define required field
    schema.defineRequired("solver.type", ConfigType::STRING, "Solver type", "solver")
          .addRule(validator::makeStringEnum({"explicit", "implicit"}));

    // Define optional field
    schema.defineOptional("solver.tolerance", ConfigType::DOUBLE, 1.0e-6,
                         "Tolerance", "solver")
          .addRule(validator::makePositive());

    // Check schema properties
    auto fields = schema.getFields();
    assert(fields.size() == 2);

    auto categories = schema.getCategories();
    assert(categories.size() == 1);
    assert(categories[0] == "solver");

    std::cout << "  ✓ Schema definition works" << std::endl;
}

// Test 2: Schema Validation - Valid Config
void testSchemaValidationValid() {
    std::cout << "\nTesting schema validation (valid)..." << std::endl;

    auto schema = schema::buildSolverSchema();
    auto config = std::make_shared<Config>();

    config->set("solver.type", std::string("implicit"));
    config->set("solver.tolerance", 1.0e-6);
    config->set("solver.max_iterations", 1000);

    auto report = schema.validate(config);

    assert(report.isValid());
    assert(report.getFailureCount() == 0);

    std::cout << "  ✓ Valid configuration passes validation" << std::endl;
}

// Test 3: Schema Validation - Invalid Config
void testSchemaValidationInvalid() {
    std::cout << "\nTesting schema validation (invalid)..." << std::endl;

    auto schema = schema::buildSolverSchema();
    auto config = std::make_shared<Config>();

    config->set("solver.type", std::string("invalid_type"));
    config->set("solver.tolerance", -1.0);  // Negative (invalid)

    auto report = schema.validate(config);

    assert(!report.isValid());
    assert(report.getFailureCount() > 0);

    std::cout << "  ✓ Invalid configuration fails validation" << std::endl;
}

// Test 4: Schema Apply Defaults
void testSchemaApplyDefaults() {
    std::cout << "\nTesting schema apply defaults..." << std::endl;

    auto schema = schema::buildSolverSchema();
    auto config = std::make_shared<Config>();

    config->set("solver.type", std::string("implicit"));

    size_t count = schema.applyDefaults(config);

    assert(count > 0);  // At least some defaults applied
    assert(config->has("solver.tolerance"));
    assert(std::abs(config->get<double>("solver.tolerance") - 1.0e-6) < 1e-15);

    std::cout << "  ✓ Schema applies defaults correctly" << std::endl;
}

// Test 5: Configuration Presets - Fast
void testPresetFast() {
    std::cout << "\nTesting fast preset..." << std::endl;

    auto preset = preset::PresetFactory::createFast();
    auto config = std::make_shared<Config>();

    preset.apply(config);

    assert(config->has("solver.type"));
    assert(config->get<std::string>("solver.type") == "explicit");
    assert(config->has("time.cfl"));

    std::cout << "  ✓ Fast preset applies correctly" << std::endl;
}

// Test 6: Configuration Presets - Accurate
void testPresetAccurate() {
    std::cout << "\nTesting accurate preset..." << std::endl;

    auto preset = preset::PresetFactory::createAccurate();
    auto config = std::make_shared<Config>();

    preset.apply(config);

    assert(config->get<std::string>("solver.type") == "implicit");
    assert(config->get<double>("solver.tolerance") < 1.0e-8);
    assert(config->get<int>("solver.max_iterations") > 1000);

    std::cout << "  ✓ Accurate preset applies correctly" << std::endl;
}

// Test 7: Configuration Presets - Surface Chemistry
void testPresetSurfaceChemistry() {
    std::cout << "\nTesting surface chemistry preset..." << std::endl;

    auto preset = preset::PresetFactory::createSurfaceChemistry();
    auto config = std::make_shared<Config>();

    preset.apply(config);

    assert(config->has("surface.mechanism"));
    assert(config->has("surface.site_density"));
    assert(config->get<double>("time.dt") < 1.0e-5);  // Microsecond scale

    std::cout << "  ✓ Surface chemistry preset applies correctly" << std::endl;
}

// Test 8: Preset Manager
void testPresetManager() {
    std::cout << "\nTesting preset manager..." << std::endl;

    auto& manager = preset::PresetManager::getInstance();
    manager.clear();

    auto customPreset = preset::ConfigPreset("Custom", "My custom preset");
    customPreset.set("my.param", 42);

    manager.registerPreset("custom1", customPreset);

    assert(manager.hasPreset("custom1"));
    auto retrieved = manager.getPreset("custom1");
    assert(retrieved.has_value());
    assert(retrieved->getName() == "Custom");

    std::cout << "  ✓ Preset manager works" << std::endl;
}

// Test 9: Environment Variable Loading
void testEnvironmentLoading() {
    std::cout << "\nTesting environment variable loading..." << std::endl;

    // Set test environment variables
    setenv("KOO_SOLVER_TYPE", "implicit", 1);
    setenv("KOO_SOLVER_TOLERANCE", "1e-8", 1);
    setenv("KOO_TIME_DT", "0.001", 1);

    env::EnvironmentLoader loader;
    auto config = std::make_shared<Config>();

    loader.loadVariable(config, "KOO_SOLVER_TYPE");
    loader.loadVariable(config, "KOO_SOLVER_TOLERANCE");
    loader.loadVariable(config, "KOO_TIME_DT");

    assert(config->has("solver.type"));
    assert(config->get<std::string>("solver.type") == "implicit");
    assert(config->has("solver.tolerance"));
    assert(config->get<double>("solver.tolerance") < 1.0e-7);

    // Cleanup
    unsetenv("KOO_SOLVER_TYPE");
    unsetenv("KOO_SOLVER_TOLERANCE");
    unsetenv("KOO_TIME_DT");

    std::cout << "  ✓ Environment variable loading works" << std::endl;
}

// Test 10: Environment Key Conversion
void testEnvironmentKeyConversion() {
    std::cout << "\nTesting environment key conversion..." << std::endl;

    env::EnvironmentLoader loader;

    std::string envVar = loader.configKeyToEnvVar("solver.type");
    assert(envVar == "KOO_SOLVER_TYPE");

    std::string key = loader.envVarToConfigKey("KOO_SOLVER_TYPE");
    assert(key == "solver.type");

    std::cout << "  ✓ Environment key conversion works" << std::endl;
}

// Test 11: Environment Config Builder
void testEnvironmentConfigBuilder() {
    std::cout << "\nTesting environment config builder..." << std::endl;

    // Set environment variable
    setenv("KOO_SOLVER_MAX_ITERATIONS", "5000", 1);

    auto defaults = std::make_shared<Config>();
    defaults->set("solver.max_iterations", 1000);
    defaults->set("solver.tolerance", 1.0e-6);

    env::EnvironmentConfigBuilder builder;
    auto config = builder.build(defaults);

    // Environment should override default
    assert(config->get<int>("solver.max_iterations") == 5000);
    // But default should still be there
    assert(config->has("solver.tolerance"));

    unsetenv("KOO_SOLVER_MAX_ITERATIONS");

    std::cout << "  ✓ Environment config builder works" << std::endl;
}

// Test 12: Configuration Diff - Identical
void testConfigDiffIdentical() {
    std::cout << "\nTesting config diff (identical)..." << std::endl;

    auto config1 = std::make_shared<Config>();
    config1->set("param1", 42);
    config1->set("param2", 3.14);

    auto config2 = std::make_shared<Config>();
    config2->set("param1", 42);
    config2->set("param2", 3.14);

    auto diff = util::ConfigComparator::compare(config1, config2);

    assert(diff.isIdentical());
    assert(diff.getTotalDiffs() == 0);

    std::cout << "  ✓ Identical configs detected" << std::endl;
}

// Test 13: Configuration Diff - Modified
void testConfigDiffModified() {
    std::cout << "\nTesting config diff (modified)..." << std::endl;

    auto config1 = std::make_shared<Config>();
    config1->set("param1", 42);
    config1->set("param2", 3.14);

    auto config2 = std::make_shared<Config>();
    config2->set("param1", 100);  // Modified
    config2->set("param2", 3.14);

    auto diff = util::ConfigComparator::compare(config1, config2);

    assert(!diff.isIdentical());
    assert(diff.getModifiedCount() == 1);
    assert(diff.getAddedCount() == 0);
    assert(diff.getRemovedCount() == 0);

    std::cout << "  ✓ Modified configs detected" << std::endl;
}

// Test 14: Configuration Diff - Added/Removed
void testConfigDiffAddedRemoved() {
    std::cout << "\nTesting config diff (added/removed)..." << std::endl;

    auto config1 = std::make_shared<Config>();
    config1->set("param1", 42);
    config1->set("param2", 3.14);

    auto config2 = std::make_shared<Config>();
    config2->set("param1", 42);
    config2->set("param3", std::string("new"));  // Added

    auto diff = util::ConfigComparator::compare(config1, config2);

    assert(diff.getAddedCount() == 1);
    assert(diff.getRemovedCount() == 1);

    std::cout << "  ✓ Added/removed keys detected" << std::endl;
}

// Test 15: Configuration Diff Report
void testConfigDiffReport() {
    std::cout << "\nTesting config diff report..." << std::endl;

    auto config1 = std::make_shared<Config>();
    config1->set("param1", 42);

    auto config2 = std::make_shared<Config>();
    config2->set("param1", 100);
    config2->set("param2", std::string("added"));

    auto diff = util::ConfigComparator::compare(config1, config2);

    std::string report = diff.report();
    assert(!report.empty());
    assert(report.find("Modified") != std::string::npos);
    assert(report.find("Added") != std::string::npos);

    std::cout << "  ✓ Diff report generated" << std::endl;
}

// Test 16: Documentation Generation
void testDocGeneration() {
    std::cout << "\nTesting documentation generation..." << std::endl;

    auto config = std::make_shared<Config>();
    config->set("solver.type", std::string("implicit"));
    config->set("solver.tolerance", 1.0e-6);
    config->set("time.dt", 0.001);

    std::string doc = util::ConfigDocGenerator::generate(config);

    assert(!doc.empty());
    assert(doc.find("Configuration Documentation") != std::string::npos);
    assert(doc.find("solver.type") != std::string::npos);

    std::cout << "  ✓ Documentation generation works" << std::endl;
}

// Test 17: Documentation with Schema
void testDocGenerationWithSchema() {
    std::cout << "\nTesting documentation with schema..." << std::endl;

    auto schema = schema::buildSolverSchema();
    auto config = std::make_shared<Config>();
    config->set("solver.type", std::string("implicit"));
    config->set("solver.tolerance", 1.0e-6);

    std::string doc = util::ConfigDocGenerator::generate(config, &schema);

    assert(!doc.empty());
    assert(doc.find("Schema:") != std::string::npos);
    assert(doc.find("## solver") != std::string::npos);

    std::cout << "  ✓ Documentation with schema works" << std::endl;
}

// Test 18: Complete Workflow - Schema + Preset + Validation
void testCompleteWorkflow() {
    std::cout << "\nTesting complete workflow..." << std::endl;

    // 1. Create schema
    auto schema = schema::buildSolverSchema();

    // 2. Apply preset
    auto preset = preset::PresetFactory::createBalanced();
    auto config = std::make_shared<Config>();
    preset.apply(config);

    // 3. Apply schema defaults
    schema.applyDefaults(config);

    // 4. Validate
    auto report = schema.validate(config);

    assert(report.isValid());
    assert(config->has("solver.type"));

    std::cout << "  ✓ Complete workflow works" << std::endl;
}

// Test 19: Schema Documentation Generation
void testSchemaDocumentation() {
    std::cout << "\nTesting schema documentation..." << std::endl;

    auto schema = schema::buildSolverSchema();
    std::string doc = schema.generateDocumentation();

    assert(!doc.empty());
    assert(doc.find("solver.type") != std::string::npos);
    assert(doc.find("[required]") != std::string::npos);

    std::cout << "  ✓ Schema documentation works" << std::endl;
}

// Test 20: Integration - All Features
void testIntegration() {
    std::cout << "\nTesting integration of all features..." << std::endl;

    // Set environment variable
    setenv("KOO_SOLVER_MAX_ITERATIONS", "2000", 1);

    // 1. Build schema
    auto schema = schema::buildSolverSchema();

    // 2. Create config with preset
    auto preset = preset::PresetFactory::createProduction();
    auto fileConfig = std::make_shared<Config>();
    preset.apply(fileConfig);

    // 3. Apply environment variables
    env::EnvironmentConfigBuilder builder;
    auto config = builder.build(fileConfig);

    // 4. Apply schema defaults
    schema.applyDefaults(config);

    // 5. Validate
    auto report = schema.validate(config);

    // 6. Generate documentation
    std::string doc = util::ConfigDocGenerator::generate(config, &schema);

    assert(report.isValid());
    assert(!doc.empty());
    assert(config->get<int>("solver.max_iterations") == 2000);  // From environment

    unsetenv("KOO_SOLVER_MAX_ITERATIONS");

    std::cout << "  ✓ Full integration works" << std::endl;
}

int main() {
    std::cout << "Phase 36-40 Tests - Configuration Management" << std::endl;
    std::cout << "===========================================" << std::endl;

    // Phase 36: Schema
    testSchemaDefinition();
    testSchemaValidationValid();
    testSchemaValidationInvalid();
    testSchemaApplyDefaults();

    // Phase 37: Presets
    testPresetFast();
    testPresetAccurate();
    testPresetSurfaceChemistry();
    testPresetManager();

    // Phase 38: Environment
    testEnvironmentLoading();
    testEnvironmentKeyConversion();
    testEnvironmentConfigBuilder();

    // Phase 39-40: Diff & Documentation
    testConfigDiffIdentical();
    testConfigDiffModified();
    testConfigDiffAddedRemoved();
    testConfigDiffReport();
    testDocGeneration();
    testDocGenerationWithSchema();

    // Integration
    testCompleteWorkflow();
    testSchemaDocumentation();
    testIntegration();

    std::cout << "\n===========================================" << std::endl;
    std::cout << "All Phase 36-40 tests passed!" << std::endl;
    std::cout << "Configuration management system verified." << std::endl;

    return 0;
}
