# Code Coverage Guide

This document explains how to generate and analyze code coverage for KooChemicalSimulation.

---

## 📊 Overview

Code coverage measures how much of the source code is executed by the test suite. This project uses:
- **gcov/lcov**: GCC's coverage tools
- **Codecov**: Cloud-based coverage analysis and reporting

**Current Target**: 70% overall coverage, 80% for new code

---

## 🚀 Quick Start

### Generate Coverage Report Locally

```bash
# 1. Configure with coverage enabled
cmake -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_COVERAGE=ON \
  -DBUILD_TESTING=ON

# 2. Build
cmake --build build-coverage -j$(nproc)

# 3. Run coverage analysis (builds, runs tests, generates report)
cmake --build build-coverage --target coverage

# 4. View HTML report
xdg-open build-coverage/coverage/html/index.html  # Linux
# or
open build-coverage/coverage/html/index.html      # macOS
```

**That's it!** The `coverage` target handles everything automatically.

---

## 📋 Detailed Steps

### Manual Step-by-Step

If you want more control, use individual targets:

```bash
# 1. Initialize coverage counters
cmake --build build-coverage --target coverage-init

# 2. Run tests
cmake --build build-coverage --target test

# 3. Capture coverage data
cmake --build build-coverage --target coverage-capture

# 4. Generate HTML report
cmake --build build-coverage --target coverage-html
```

### Available Targets

| Target | Description |
|--------|-------------|
| `coverage` | **All-in-one**: init → test → capture → HTML |
| `coverage-init` | Initialize coverage counters |
| `coverage-capture` | Capture coverage data after running tests |
| `coverage-html` | Generate HTML report from captured data |

---

## 📈 Understanding Coverage Reports

### HTML Report

Open `build-coverage/coverage/html/index.html` in your browser to see:

- **Overall coverage percentage** (lines, functions, branches)
- **Per-directory breakdown**
- **Per-file analysis** with line-by-line coverage
- **Uncovered lines** highlighted in red

### Coverage Metrics

```
Lines Coverage: 75.3%     # Percentage of lines executed
Functions: 82.1%           # Percentage of functions called
Branches: 68.4%            # Percentage of branches taken
```

**Good coverage targets**:
- Lines: ≥ 70%
- Functions: ≥ 75%
- Branches: ≥ 60%

---

## 🎯 Coverage Goals

### Project-wide Targets

| Component | Target | Current |
|-----------|--------|---------|
| Core | 80% | - |
| Mesh | 75% | - |
| Chemistry | 70% | - |
| Utils | 85% | - |
| Overall | 70% | - |

### What's Excluded

Coverage analysis excludes:
- Test files (`tests/**`)
- Example programs (`examples/**`)
- Benchmarks (`benchmarks/**`)
- External dependencies
- System headers

---

## 🔄 CI/CD Integration

### GitHub Actions

Coverage runs automatically on every push and PR:

1. **Build with coverage flags**
2. **Run test suite**
3. **Generate lcov report**
4. **Upload to Codecov**
5. **Post results as PR comment**

View results:
- **Codecov Dashboard**: https://codecov.io/gh/squall321/KooChemicalSimulation
- **GitHub Actions**: Check "Code Coverage" workflow

### Codecov Features

- **Pull Request Comments**: Automatic coverage diff
- **Trend Graphs**: Track coverage over time
- **Sunburst Chart**: Visual coverage breakdown
- **Flags**: Separate coverage for different components

---

## 🛠️ Troubleshooting

### Problem: Coverage shows 0%

**Solution**: Ensure you built with `-DENABLE_COVERAGE=ON` and ran tests before capturing:

```bash
cmake -B build-coverage -DENABLE_COVERAGE=ON
cmake --build build-coverage
cd build-coverage && ctest
cmake --build build-coverage --target coverage-capture
```

### Problem: "lcov: command not found"

**Solution**: Install lcov:

```bash
# Ubuntu/Debian
sudo apt-get install lcov

# macOS
brew install lcov
```

### Problem: Coverage includes system headers

**Solution**: The `coverage-capture` target automatically filters these out. Check `.lcovrc` if you need custom filters.

### Problem: Can't open HTML report

**Solution**: The report is at `build-coverage/coverage/html/index.html`. Use:

```bash
# Linux
firefox build-coverage/coverage/html/index.html

# macOS
open build-coverage/coverage/html/index.html

# WSL (Windows Subsystem for Linux)
wslview build-coverage/coverage/html/index.html
```

---

## 📝 Best Practices

### Writing Testable Code

1. **Keep functions small** (< 50 lines)
2. **Single responsibility** per function
3. **Minimize dependencies** (easier to mock)
4. **Avoid global state**

### Improving Coverage

1. **Identify uncovered code**:
   ```bash
   lcov --summary build-coverage/coverage/coverage_cleaned.info
   ```

2. **Check specific file**:
   ```bash
   lcov --list build-coverage/coverage/coverage_cleaned.info | grep "src/core/MyFile.cpp"
   ```

3. **Focus on critical paths**:
   - Core algorithms
   - Error handling
   - Edge cases

4. **Don't chase 100%**:
   - 70-80% is excellent
   - Some code is hard to test (e.g., platform-specific)
   - Focus on valuable tests, not just coverage numbers

---

## 🔍 Advanced Usage

### Generate Coverage for Specific Tests

```bash
# Run only specific tests
cd build-coverage
ctest -R MeshTest
cmake --build . --target coverage-capture coverage-html
```

### Compare Coverage Between Branches

```bash
# Branch 1
git checkout main
cmake --build build-coverage --target coverage
cp build-coverage/coverage/coverage_cleaned.info /tmp/main.info

# Branch 2
git checkout feature-branch
cmake --build build-coverage --target coverage

# Compare
lcov --diff /tmp/main.info build-coverage/coverage/coverage_cleaned.info
```

### Export Coverage Summary

```bash
lcov --summary build-coverage/coverage/coverage_cleaned.info > coverage-summary.txt
```

---

## 📚 References

- [gcov Documentation](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html)
- [lcov Homepage](http://ltp.sourceforge.net/coverage/lcov.php)
- [Codecov Documentation](https://docs.codecov.com/)
- [CMake Code Coverage](https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html)

---

## ✅ Quick Reference

```bash
# Full coverage workflow
cmake -B build-coverage -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON
cmake --build build-coverage -j$(nproc)
cmake --build build-coverage --target coverage
xdg-open build-coverage/coverage/html/index.html

# View summary in terminal
lcov --summary build-coverage/coverage/coverage_cleaned.info

# Check specific file
lcov --list build-coverage/coverage/coverage_cleaned.info | grep "MyFile.cpp"

# Clean coverage data
rm -rf build-coverage/coverage
```

---

**Happy testing!** 🧪
