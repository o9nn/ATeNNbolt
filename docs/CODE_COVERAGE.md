# Code Coverage Reporting

Bolt C++ includes comprehensive code coverage reporting using gcov and lcov. This allows developers to measure which parts of the codebase are exercised by the test suite and identify areas that need additional testing.

## Prerequisites

Install lcov (includes genhtml):

```bash
# Ubuntu/Debian
sudo apt-get install lcov

# macOS
brew install lcov

# Fedora/RHEL
sudo dnf install lcov
```

## Quick Start

### 1. Configure with Coverage Enabled

```bash
mkdir build-coverage && cd build-coverage
cmake -DENABLE_COVERAGE=ON ..
```

### 2. Build the Project

```bash
make -j$(nproc)
```

### 3. Generate Coverage Report

```bash
make coverage
```

This will:
1. Clean old coverage data
2. Run all tests via CTest
3. Collect coverage information
4. Filter out system headers and test files
5. Generate an HTML report in `build-coverage/coverage/`

### 4. View the Report

Open the coverage report in your browser:

```bash
# Linux
xdg-open coverage/index.html

# macOS
open coverage/index.html

# Or just navigate to the file
firefox coverage/index.html
```

## Available Targets

| Target | Description |
|--------|-------------|
| `make coverage` | Generate complete coverage report |
| `make coverage-clean` | Remove all coverage data and reports |

## Understanding the Report

The HTML report provides several views:

### Coverage Metrics

- **Line Coverage**: Percentage of code lines executed
- **Function Coverage**: Percentage of functions called
- **Branch Coverage**: Percentage of conditional branches taken

### Color Coding

- **Green**: Code executed by tests (good coverage)
- **Red**: Code not executed by tests (needs coverage)
- **Orange/Yellow**: Partially covered branches

### Drill-Down Navigation

1. **Directory Level**: Shows coverage for each source directory
2. **File Level**: Shows coverage for individual files
3. **Source Level**: Shows line-by-line coverage with execution counts

## Coverage Goals

Recommended coverage targets for Bolt C++:

- **Overall Coverage**: ≥80%
- **Core Components**: ≥90%
- **AI/ML Modules**: ≥85%
- **Editor Features**: ≥80%
- **Utility Functions**: ≥95%

## Excluding Code from Coverage

Some code shouldn't be included in coverage metrics:

- Third-party code (automatically excluded)
- Generated code
- Debug-only code
- Platform-specific fallbacks

These are automatically excluded by the coverage configuration:
- `/usr/*` - System headers
- `*/test/*` - Test files themselves
- `*/ggml/*` - GGML submodule
- `*/vcpkg_installed/*` - Dependencies

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: Code Coverage

on: [push, pull_request]

jobs:
  coverage:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y lcov cmake build-essential
      
      - name: Configure with coverage
        run: |
          mkdir build && cd build
          cmake -DENABLE_COVERAGE=ON ..
      
      - name: Build
        run: cd build && make -j$(nproc)
      
      - name: Generate coverage
        run: cd build && make coverage
      
      - name: Upload to Codecov
        uses: codecov/codecov-action@v3
        with:
          files: ./build/coverage.info.cleaned
```

## Manual Coverage Analysis

For detailed analysis, you can use lcov commands directly:

### View Coverage Summary

```bash
cd build
lcov --summary coverage.info.cleaned
```

### Extract Coverage for Specific Directory

```bash
lcov --extract coverage.info.cleaned '*/src/bolt/editor/*' \
     --output-file editor-coverage.info
genhtml editor-coverage.info --output-directory editor-coverage
```

### Compare Coverage Between Branches

```bash
# Generate baseline coverage
git checkout main
make coverage
cp coverage.info.cleaned ../baseline.info

# Generate feature coverage
git checkout feature-branch
make coverage
cp coverage.info.cleaned ../feature.info

# Compare
lcov --diff ../baseline.info ../feature.info --output-file diff.info
```

## Troubleshooting

### No Coverage Data

If you see "no coverage data":

1. Ensure you configured with `-DENABLE_COVERAGE=ON`
2. Rebuild all targets: `make clean && make`
3. Run tests: `make test` or `ctest`
4. Try `make coverage` again

### Low Coverage Numbers

If coverage is lower than expected:

1. Check that all tests are actually running
2. Verify tests are calling the code you expect
3. Look for dead code or unreachable branches
4. Check for tests that are disabled or skipped

### Missing Files in Report

If files are missing from the coverage report:

1. Ensure files are compiled with coverage flags
2. Check that files are linked into test executables
3. Verify files aren't in the exclude list
4. Make sure tests actually load/use those files

## Best Practices

### Writing Testable Code

1. **Small Functions**: Easier to test completely
2. **Dependency Injection**: Makes mocking easier
3. **Avoid Global State**: Improves test isolation
4. **Clear Interfaces**: Define clear contracts

### Improving Coverage

1. **Start with Critical Paths**: Test main workflows first
2. **Add Edge Cases**: Test boundary conditions
3. **Test Error Handling**: Cover error paths
4. **Branch Coverage**: Ensure all if/else paths are tested

### Maintaining Coverage

1. **Run Coverage Regularly**: Part of development workflow
2. **Review Coverage in PRs**: Don't let coverage drop
3. **Track Trends**: Monitor coverage over time
4. **Set Goals**: Establish team coverage targets

## Performance Considerations

Coverage instrumentation adds overhead:

- **Build Time**: ~15-20% slower
- **Runtime**: ~10-15% slower
- **Disk Space**: Additional files for .gcno and .gcda

For this reason, coverage is **not** enabled by default. Only enable it when you need coverage reports.

## Coverage Report Structure

```
coverage/
├── index.html              # Main dashboard
├── src/
│   └── bolt/
│       ├── index.html      # Bolt source overview
│       ├── core/
│       │   └── index.html  # Core module coverage
│       ├── editor/
│       │   └── index.html  # Editor module coverage
│       └── ai/
│           └── index.html  # AI module coverage
└── ...
```

## Advanced Usage

### Incremental Coverage

For large projects, you might want incremental coverage:

```bash
# Don't zero counters
lcov --directory . --capture --output-file coverage.info

# Accumulate with previous data
lcov -a previous.info -a coverage.info -o combined.info
```

### Branch Coverage

To see branch coverage statistics:

```bash
lcov --summary coverage.info.cleaned --rc lcov_branch_coverage=1
```

### Differential Coverage

To see only newly added lines:

```bash
# After making changes
make coverage
lcov --diff baseline.info coverage.info.cleaned --output-file new-code.info
genhtml new-code.info --output-directory new-coverage
```

## Example Output

```
Summary coverage rate:
  lines......: 85.2% (12458 of 14621 lines)
  functions..: 88.5% (1245 of 1407 functions)
  branches...: 72.3% (3821 of 5285 branches)
```

## Resources

- [lcov Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [gcov Manual](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html)
- [C++ Code Coverage Guide](https://github.com/gcovr/gcovr)

## See Also

- [TESTING.md](../TESTING.md) - Test framework documentation
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
- [CMake CodeCoverage.cmake](../cmake/CodeCoverage.cmake) - Coverage configuration
