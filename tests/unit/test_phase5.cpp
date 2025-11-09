/**
 * @file test_phase5.cpp
 * @brief Unit tests for Phase 5 - Configuration System
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Tests:
 * 1. Config basic operations (set, get, has, remove)
 * 2. Config type safety and error handling
 * 3. Config hierarchical keys and prefixes
 * 4. Config default values and reset
 * 5. Config merging and sub-configuration
 * 6. ConfigParser JSON parsing (if available)
 * 7. ConfigParser YAML parsing (if available)
 * 8. ConfigValidator range rules
 * 9. ConfigValidator enum and regex rules
 * 10. ConfigValidator composite rules (AND, OR)
 * 11. ConfigValidator custom rules
 * 12. Full validation workflow
 */

#include "config/Config.h"
#include "config/parser/ConfigParser.h"
#include "config/validator/ConfigValidator.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>

using namespace koo::config;
using namespace koo::config::parser;
using namespace koo::config::validator;

// ============================================================================
// Test 1: Config Basic Operations
// ============================================================================

void testConfigBasics() {
    std::cout << "\n[Test 1] Config Basic Operations\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();

    // Test set and get
    config->set("solver.tolerance", 1e-6);
    config->set("solver.max_iterations", 1000);
    config->set("mesh.filename", std::string("mesh.msh"));
    config->set("solver.verbose", true);

    assert(config->get<double>("solver.tolerance") == 1e-6 && "Should get tolerance");
    assert(config->get<int>("solver.max_iterations") == 1000 && "Should get max_iterations");
    assert(config->get<std::string>("mesh.filename") == "mesh.msh" && "Should get filename");
    assert(config->get<bool>("solver.verbose") == true && "Should get verbose");

    // Test has
    assert(config->has("solver.tolerance") && "Should have solver.tolerance");
    assert(!config->has("nonexistent") && "Should not have nonexistent key");

    // Test size
    assert(config->size() == 4 && "Should have 4 entries");
    assert(!config->empty() && "Should not be empty");

    // Test remove
    assert(config->remove("solver.verbose") && "Should remove existing key");
    assert(!config->has("solver.verbose") && "Key should be removed");
    assert(config->size() == 3 && "Should have 3 entries after removal");

    // Test clear
    config->clear();
    assert(config->empty() && "Should be empty after clear");
    assert(config->size() == 0 && "Size should be 0");

    std::cout << "✓ Set/get operations work\n";
    std::cout << "✓ Has/remove operations work\n";
    std::cout << "✓ Size/empty/clear operations work\n";
}

// ============================================================================
// Test 2: Config Type Safety
// ============================================================================

void testConfigTypeSafety() {
    std::cout << "\n[Test 2] Config Type Safety\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();

    // Set different types
    config->set("int_value", 42);
    config->set("double_value", 3.14);
    config->set("string_value", std::string("hello"));
    config->set("bool_value", false);

    // Test type checking
    assert(config->getType("int_value") == ConfigType::INT && "Should be INT");
    assert(config->getType("double_value") == ConfigType::DOUBLE && "Should be DOUBLE");
    assert(config->getType("string_value") == ConfigType::STRING && "Should be STRING");
    assert(config->getType("bool_value") == ConfigType::BOOL && "Should be BOOL");
    assert(config->getType("nonexistent") == ConfigType::NONE && "Should be NONE");

    // Test getOrDefault
    int val1 = config->getOrDefault("int_value", 0);
    (void)val1;  // Used in assert
    assert(val1 == 42 && "Should get actual value");

    int val2 = config->getOrDefault("nonexistent_int", 99);
    (void)val2;  // Used in assert
    assert(val2 == 99 && "Should get default value");

    // Test arrays
    std::vector<int> intArray = {1, 2, 3, 4, 5};
    std::vector<double> doubleArray = {1.1, 2.2, 3.3};
    std::vector<std::string> stringArray = {"a", "b", "c"};

    config->set("int_array", intArray);
    config->set("double_array", doubleArray);
    config->set("string_array", stringArray);

    auto retrievedIntArray = config->get<std::vector<int>>("int_array");
    assert(retrievedIntArray.size() == 5 && "Should have 5 elements");
    assert(retrievedIntArray[0] == 1 && "First element should be 1");
    assert(retrievedIntArray[4] == 5 && "Last element should be 5");

    auto retrievedStringArray = config->get<std::vector<std::string>>("string_array");
    assert(retrievedStringArray.size() == 3 && "Should have 3 elements");
    assert(retrievedStringArray[0] == "a" && "First element should be 'a'");

    std::cout << "✓ Type detection works\n";
    std::cout << "✓ getOrDefault works\n";
    std::cout << "✓ Array types work\n";
}

// ============================================================================
// Test 3: Config Hierarchical Keys
// ============================================================================

void testConfigHierarchicalKeys() {
    std::cout << "\n[Test 3] Config Hierarchical Keys\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();

    // Set hierarchical keys
    config->set("solver.linear.tolerance", 1e-8);
    config->set("solver.linear.max_iter", 500);
    config->set("solver.nonlinear.tolerance", 1e-6);
    config->set("solver.nonlinear.max_iter", 100);
    config->set("mesh.refine.level", 3);
    config->set("output.format", std::string("vtk"));

    // Test getKeys
    auto keys = config->getKeys();
    assert(keys.size() == 6 && "Should have 6 keys");

    // Test getKeysWithPrefix
    auto solverKeys = config->getKeysWithPrefix("solver.");
    assert(solverKeys.size() == 4 && "Should have 4 solver keys");

    auto linearKeys = config->getKeysWithPrefix("solver.linear.");
    assert(linearKeys.size() == 2 && "Should have 2 linear solver keys");

    auto meshKeys = config->getKeysWithPrefix("mesh.");
    assert(meshKeys.size() == 1 && "Should have 1 mesh key");

    std::cout << "✓ Hierarchical keys work\n";
    std::cout << "✓ getKeysWithPrefix works\n";
}

// ============================================================================
// Test 4: Config Default Values
// ============================================================================

void testConfigDefaults() {
    std::cout << "\n[Test 4] Config Default Values\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();

    // Set with default
    config->setWithDefault("solver.tolerance", 1e-3, 1e-6,
                          "Solver tolerance", false);
    config->setWithDefault("solver.max_iter", 200, 1000,
                          "Maximum iterations", false);

    assert(config->get<double>("solver.tolerance") == 1e-3 && "Should have current value");
    assert(config->get<int>("solver.max_iter") == 200 && "Should have current value");

    // Test reset to default
    assert(config->resetToDefault("solver.tolerance") && "Should reset");
    assert(config->get<double>("solver.tolerance") == 1e-6 && "Should have default value");

    assert(config->resetToDefault("solver.max_iter") && "Should reset");
    assert(config->get<int>("solver.max_iter") == 1000 && "Should have default value");

    // Set more values with defaults
    config->setWithDefault("mesh.refine", 2, 1, "Refinement level");
    config->setWithDefault("output.enabled", false, true, "Enable output");

    // Reset all to defaults
    size_t resetCount = config->resetAllToDefaults();
    (void)resetCount;  // Used in assert
    assert(resetCount == 4 && "Should reset 4 entries");

    assert(config->get<int>("mesh.refine") == 1 && "Should have default");
    assert(config->get<bool>("output.enabled") == true && "Should have default");

    std::cout << "✓ Default values work\n";
    std::cout << "✓ Reset to default works\n";
    std::cout << "✓ Reset all to defaults works\n";
}

// ============================================================================
// Test 5: Config Merging and Sub-Configuration
// ============================================================================

void testConfigMergingAndSub() {
    std::cout << "\n[Test 5] Config Merging and Sub-Configuration\n";
    std::cout << "==========================================\n";

    auto config1 = std::make_shared<Config>();
    auto config2 = std::make_shared<Config>();

    config1->set("solver.tolerance", 1e-6);
    config1->set("solver.max_iter", 1000);
    config1->set("mesh.filename", std::string("mesh1.msh"));

    config2->set("solver.tolerance", 1e-8); // Conflict
    config2->set("output.format", std::string("vtk"));
    config2->set("output.frequency", 10);

    // Merge without overwrite
    config1->merge(*config2, false);
    assert(config1->get<double>("solver.tolerance") == 1e-6 && "Should not overwrite");
    assert(config1->has("output.format") && "Should add new keys");
    assert(config1->get<std::string>("output.format") == "vtk");

    // Merge with overwrite
    config1->merge(*config2, true);
    assert(config1->get<double>("solver.tolerance") == 1e-8 && "Should overwrite");

    // Test sub-configuration
    auto solverConfig = config1->getSubConfig("solver.");
    assert(solverConfig->has("tolerance") && "Should have tolerance (without prefix)");
    assert(solverConfig->has("max_iter") && "Should have max_iter");
    assert(!solverConfig->has("solver.tolerance") && "Should not have full key");
    assert(solverConfig->get<double>("tolerance") == 1e-8 && "Should get value");

    auto outputConfig = config1->getSubConfig("output.");
    assert(outputConfig->size() == 2 && "Should have 2 output keys");
    assert(outputConfig->has("format") && "Should have format");
    assert(outputConfig->has("frequency") && "Should have frequency");

    std::cout << "✓ Merging without overwrite works\n";
    std::cout << "✓ Merging with overwrite works\n";
    std::cout << "✓ Sub-configuration works\n";
}

// ============================================================================
// Test 6: ConfigValidator Range Rules
// ============================================================================

void testValidatorRangeRules() {
    std::cout << "\n[Test 6] ConfigValidator Range Rules\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();
    config->set("tolerance", 1e-6);
    config->set("max_iter", 1000);
    config->set("temperature", 300.0);

    ConfigValidator validator;

    // Add range rules
    validator.addRule("tolerance", makeDoubleRange(1e-10, 1e-3));
    validator.addRule("max_iter", makeIntRange(1, 10000));
    validator.addRule("temperature", makeDoubleRange(0.0, 1000.0));

    // Validate
    auto result1 = validator.validateKey(config, "tolerance");
    assert(result1.valid && "Tolerance should be valid");

    auto result2 = validator.validateKey(config, "max_iter");
    assert(result2.valid && "Max iter should be valid");

    // Test invalid values
    config->set("tolerance", 0.1); // Out of range
    auto result3 = validator.validateKey(config, "tolerance");
    assert(!result3.valid && "Tolerance should be invalid");

    config->set("max_iter", 20000); // Out of range
    auto result4 = validator.validateKey(config, "max_iter");
    assert(!result4.valid && "Max iter should be invalid");

    std::cout << "✓ Range validation works for valid values\n";
    std::cout << "✓ Range validation detects invalid values\n";
}

// ============================================================================
// Test 7: ConfigValidator Enum and Regex Rules
// ============================================================================

void testValidatorEnumAndRegex() {
    std::cout << "\n[Test 7] ConfigValidator Enum and Regex Rules\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();
    config->set("solver_type", std::string("direct"));
    config->set("output_format", std::string("vtk"));
    config->set("email", std::string("user@example.com"));

    ConfigValidator validator;

    // Add enum rules
    validator.addRule("solver_type",
                     makeStringEnum({"direct", "iterative", "multigrid"}));
    validator.addRule("output_format",
                     makeStringEnum({"vtk", "hdf5", "csv"}));

    // Add regex rule for email
    validator.addRule("email",
                     makeRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"));

    // Validate valid values
    auto result1 = validator.validateKey(config, "solver_type");
    assert(result1.valid && "Solver type should be valid");

    auto result2 = validator.validateKey(config, "email");
    assert(result2.valid && "Email should be valid");

    // Test invalid enum
    config->set("solver_type", std::string("invalid_solver"));
    auto result3 = validator.validateKey(config, "solver_type");
    assert(!result3.valid && "Invalid solver type should fail");

    // Test invalid regex
    config->set("email", std::string("not-an-email"));
    auto result4 = validator.validateKey(config, "email");
    assert(!result4.valid && "Invalid email should fail");

    std::cout << "✓ Enum validation works\n";
    std::cout << "✓ Regex validation works\n";
}

// ============================================================================
// Test 8: ConfigValidator Custom Rules
// ============================================================================

void testValidatorCustomRules() {
    std::cout << "\n[Test 8] ConfigValidator Custom Rules\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();
    config->set("grid_size", 100);
    config->set("filename", std::string("output.txt"));

    ConfigValidator validator;

    // Add custom rule: grid size must be power of 2
    auto isPowerOfTwo = [](int n) -> bool {
        return n > 0 && (n & (n - 1)) == 0;
    };
    validator.addRule("grid_size",
                     std::make_shared<CustomRule<int>>(
                         isPowerOfTwo, "Grid size must be power of 2"));

    // Add non-empty rule
    validator.addRule("filename", makeNonEmpty());

    // Validate
    auto result1 = validator.validateKey(config, "grid_size");
    assert(!result1.valid && "100 is not power of 2");

    config->set("grid_size", 128); // 128 is 2^7
    auto result2 = validator.validateKey(config, "grid_size");
    assert(result2.valid && "128 is power of 2");

    auto result3 = validator.validateKey(config, "filename");
    assert(result3.valid && "Filename is not empty");

    config->set("filename", std::string(""));
    auto result4 = validator.validateKey(config, "filename");
    assert(!result4.valid && "Empty filename should fail");

    std::cout << "✓ Custom validation rules work\n";
    std::cout << "✓ Non-empty validation works\n";
}

// ============================================================================
// Test 9: ConfigValidator Composite Rules
// ============================================================================

void testValidatorCompositeRules() {
    std::cout << "\n[Test 9] ConfigValidator Composite Rules\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();
    config->set("value", 50.0);

    ConfigValidator validator;

    // Create AND rule: value must be in [0, 100] AND positive
    auto andRule = std::make_shared<AndRule>();
    andRule->addRule(makeDoubleRange(0.0, 100.0));
    andRule->addRule(makePositive());

    validator.addRule("value", andRule);

    // Valid case
    auto result1 = validator.validateKey(config, "value");
    assert(result1.valid && "50 satisfies both conditions");

    // Invalid case (out of range)
    config->set("value", 150.0);
    auto result2 = validator.validateKey(config, "value");
    assert(!result2.valid && "150 fails range check");

    // Create OR rule: string must be "auto" OR match pattern
    config->set("mode", std::string("auto"));
    auto orRule = std::make_shared<OrRule>();
    orRule->addRule(makeStringEnum({"auto"}));
    orRule->addRule(makeRegex(R"(\d+x\d+)")); // Pattern like "10x10"

    validator.addRule("mode", orRule);

    auto result3 = validator.validateKey(config, "mode");
    assert(result3.valid && "'auto' matches enum");

    config->set("mode", std::string("10x20"));
    auto result4 = validator.validateKey(config, "mode");
    assert(result4.valid && "'10x20' matches pattern");

    config->set("mode", std::string("invalid"));
    auto result5 = validator.validateKey(config, "mode");
    assert(!result5.valid && "'invalid' matches neither");

    std::cout << "✓ AND composite rules work\n";
    std::cout << "✓ OR composite rules work\n";
}

// ============================================================================
// Test 10: Full Validation Workflow
// ============================================================================

void testFullValidationWorkflow() {
    std::cout << "\n[Test 10] Full Validation Workflow\n";
    std::cout << "==========================================\n";

    auto config = std::make_shared<Config>();

    // Set configuration values
    config->set("solver.tolerance", 1e-6, "Solver tolerance", false);
    config->set("solver.max_iterations", 1000, "Max iterations", false);
    config->set("solver.type", std::string("direct"), "Solver type", false);
    config->set("mesh.filename", std::string("mesh.msh"), "Mesh file", true);
    config->set("output.format", std::string("vtk"), "Output format", false);

    // Create validator
    ConfigValidator validator;

    // Add validation rules
    validator.addRule("solver.tolerance", makeDoubleRange(1e-12, 1e-3));
    validator.addRule("solver.max_iterations", makeIntRange(1, 100000));
    validator.addRule("solver.type", makeStringEnum({"direct", "iterative", "multigrid"}));
    validator.addRule("mesh.filename", makeNonEmpty());
    validator.addRule("output.format", makeStringEnum({"vtk", "hdf5", "csv"}));

    // Validate entire configuration
    auto report = validator.validate(config);

    assert(report.isValid() && "Configuration should be valid");
    assert(report.getFailureCount() == 0 && "Should have no failures");

    // Introduce errors
    config->set("solver.tolerance", 1.0); // Out of range
    config->set("solver.type", std::string("invalid")); // Not in enum

    auto report2 = validator.validate(config);

    assert(!report2.isValid() && "Configuration should be invalid");
    assert(report2.getFailureCount() == 2 && "Should have 2 failures");

    auto failures = report2.getFailures();
    assert(failures.size() == 2 && "Should have 2 failure entries");

    std::cout << "✓ Full validation workflow works\n";
    std::cout << "✓ Validation report works\n";
    std::cout << "✓ Failure detection and reporting works\n";
}

// ============================================================================
// Test 11: JSON Parser (if available)
// ============================================================================

#ifdef USE_JSON
void testJsonParser() {
    std::cout << "\n[Test 11] JSON Parser\n";
    std::cout << "==========================================\n";

    // Create JSON content
    std::string jsonContent = R"(
    {
        "solver": {
            "tolerance": 1e-6,
            "max_iterations": 1000,
            "type": "direct"
        },
        "mesh": {
            "filename": "mesh.msh",
            "dimensions": [10, 20, 30]
        },
        "output": {
            "enabled": true,
            "format": "vtk"
        }
    }
    )";

    // Parse JSON
    JsonParser parser;
    auto config = std::make_shared<Config>();
    parser.parseString(jsonContent, config);

    // Verify parsed values
    assert(config->has("solver.tolerance") && "Should have solver.tolerance");
    assert(config->get<double>("solver.tolerance") == 1e-6 && "Tolerance should match");
    assert(config->get<int>("solver.max_iterations") == 1000 && "Max iterations should match");
    assert(config->get<std::string>("solver.type") == "direct" && "Type should match");
    assert(config->get<std::string>("mesh.filename") == "mesh.msh" && "Filename should match");
    assert(config->get<bool>("output.enabled") == true && "Enabled should match");

    auto dims = config->get<std::vector<int>>("mesh.dimensions");
    assert(dims.size() == 3 && "Should have 3 dimensions");
    assert(dims[0] == 10 && dims[1] == 20 && dims[2] == 30 && "Dimensions should match");

    // Write to file
    std::string filename = "/tmp/test_config.json";
    parser.writeFile(filename, config);

    // Read back
    auto config2 = std::make_shared<Config>();
    parser.parseFile(filename, config2);

    assert(config2->get<double>("solver.tolerance") == 1e-6 && "Should reload tolerance");
    assert(config2->get<int>("solver.max_iterations") == 1000 && "Should reload max_iter");

    std::cout << "✓ JSON parsing from string works\n";
    std::cout << "✓ JSON parsing from file works\n";
    std::cout << "✓ JSON writing to file works\n";
}
#endif

// ============================================================================
// Test 12: YAML Parser (if available)
// ============================================================================

#ifdef USE_YAML
void testYamlParser() {
    std::cout << "\n[Test 12] YAML Parser\n";
    std::cout << "==========================================\n";

    // Create YAML content
    std::string yamlContent = R"(
solver:
  tolerance: 1.0e-6
  max_iterations: 1000
  type: direct
mesh:
  filename: mesh.msh
  dimensions:
    - 10
    - 20
    - 30
output:
  enabled: true
  format: vtk
    )";

    // Parse YAML
    YamlParser parser;
    auto config = std::make_shared<Config>();
    parser.parseString(yamlContent, config);

    // Verify parsed values
    assert(config->has("solver.tolerance") && "Should have solver.tolerance");
    assert(std::abs(config->get<double>("solver.tolerance") - 1e-6) < 1e-10 && "Tolerance should match");
    assert(config->get<int>("solver.max_iterations") == 1000 && "Max iterations should match");
    assert(config->get<std::string>("solver.type") == "direct" && "Type should match");
    assert(config->get<bool>("output.enabled") == true && "Enabled should match");

    auto dims = config->get<std::vector<int>>("mesh.dimensions");
    assert(dims.size() == 3 && "Should have 3 dimensions");
    assert(dims[0] == 10 && dims[1] == 20 && dims[2] == 30 && "Dimensions should match");

    std::cout << "✓ YAML parsing from string works\n";
}
#endif

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Phase 5 Unit Tests\n";
    std::cout << "  Configuration System\n";
    std::cout << "========================================\n";

    try {
        // Config tests
        testConfigBasics();
        testConfigTypeSafety();
        testConfigHierarchicalKeys();
        testConfigDefaults();
        testConfigMergingAndSub();

        // Validator tests
        testValidatorRangeRules();
        testValidatorEnumAndRegex();
        testValidatorCustomRules();
        testValidatorCompositeRules();
        testFullValidationWorkflow();

#ifdef USE_JSON
        testJsonParser();
#else
        std::cout << "\n[Test 11] JSON Parser - SKIPPED (nlohmann_json not available)\n";
#endif

#ifdef USE_YAML
        testYamlParser();
#else
        std::cout << "\n[Test 12] YAML Parser - SKIPPED (yaml-cpp not available)\n";
#endif

        std::cout << "\n========================================\n";
        std::cout << "  All Tests Passed! ✓\n";
        std::cout << "========================================\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n========================================\n";
        std::cerr << "  Test Failed! ✗\n";
        std::cerr << "  Error: " << e.what() << "\n";
        std::cerr << "========================================\n\n";
        return 1;
    }
}
