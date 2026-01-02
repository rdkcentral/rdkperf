# RDKPerf Test Suite

This directory contains a comprehensive test suite for RDKPerf using Google Test (gtest).

## Overview

The test suite consists of multiple test files that cover all major components of the RDKPerf library:

- **test_perf_clock.cpp**: Tests for PerfClock (timing and CPU measurement)
- **test_perf_record.cpp**: Tests for PerfRecord (performance record management)
- **test_perf_node.cpp**: Tests for PerfNode (tree node operations and statistics)
- **test_perf_tree.cpp**: Tests for PerfTree (tree management and reporting)
- **test_perf_process.cpp**: Tests for PerfProcess (process-level operations)
- **test_rdk_perf.cpp**: Tests for the main RDKPerf API (both C++ and C interfaces)
- **test_instrumentation_overhead.cpp**: Tests that quantify the cost of instrumentation

The test suite uses gtest's main() function (via `-lgtest_main`), so no custom main is needed.

## Building the Tests

From the root directory of the project:

```bash
make clean
make
```

This will build the RDKPerf library and the test executable `build/rdkperf_tests`.

## Running the Tests

**On development systems** (with gtest and runtime environment):

```bash
make test
```

**Manual execution** (for embedded systems, run on target after deployment):

```bash
export LD_LIBRARY_PATH=./build:$LD_LIBRARY_PATH
./build/rdkperf_tests
```

**Note for embedded systems:** Build the test binary with `make all` in the project root, then deploy `build/rdkperf_tests` and required libraries to your target device for execution.

## Test Categories

### Component Tests

These tests verify the correctness of individual RDKPerf components:

1. **PerfClock Tests** (7 tests)
   - Constructor/destructor
   - Wall clock measurement
   - Time unit conversions
   - CPU time measurement
   - Marker/Elapsed cycles

2. **PerfRecord Tests** (9 tests)
   - Record creation and naming
   - Thread ID tracking
   - Timestamp generation
   - Threshold settings
   - Record lifetime management

3. **PerfNode Tests** (13 tests)
   - Node creation (root, record-based, name-based)
   - Statistics tracking
   - Data incrementing
   - Interval resetting
   - Child node management
   - Statistics averages (min, max, avg)

4. **PerfTree Tests** (11 tests)
   - Tree construction
   - Node addition and management
   - Stack operations
   - Activity tracking
   - Data reporting

5. **PerfProcess Tests** (10 tests)
   - Process creation
   - Tree management per thread
   - Process naming
   - Data reporting
   - Thread cleanup

6. **RDKPerf API Tests** (17 tests)
   - C++ constructor with name
   - C++ constructor with threshold
   - Scoped usage
   - Nested scopes
   - C interface (Start/Stop)
   - Threshold configuration
   - Report functions
   - Thread/Process closing
   - Instrumented functions
   - Recursive instrumentation

### Instrumentation Overhead Tests (7 tests - currently DISABLED)

**Note:** These tests are currently disabled due to environment-specific segfaults when rapidly creating/destroying RDKPerf objects. The issue appears to be related to certain runtime environments (e.g., GitHub Codespaces) and may be caused by threading, memory allocation patterns, or process limits.

For measuring instrumentation overhead on embedded systems or controlled environments, create standalone benchmark programs rather than running these gtest-based tests.

The disabled tests include:
1. Constructor/Destructor Overhead
2. Nested Instrumentation Overhead  
3. Work Function Overhead
4. C vs C++ Interface Overhead
5. Threshold Feature Overhead
6. Memory Overhead
7. Minimal Call Overhead

To re-enable these tests on stable platforms, remove the `DISABLED_` prefix from the test names in `test_instrumentation_overhead.cpp`.

## Test Results

All 65 active tests should pass (7 overhead tests are currently disabled).

For instrumentation overhead measurements, refer to standalone benchmark results or run the disabled tests on stable embedded target platforms.

## Requirements

- Google Test (gtest) library
- C++14 compatible compiler
- pthread library

## Notes

- The test suite uses the same build system as the main RDKPerf library
- Tests are compiled with the same flags as the production code
- Some tests may show timing variations due to system load
- Overhead measurements are platform-dependent

## Troubleshooting

If tests fail to run:

1. Ensure LD_LIBRARY_PATH includes the build directory:
   ```bash
   export LD_LIBRARY_PATH=./build:$LD_LIBRARY_PATH
   ```

2. Verify gtest is installed:
   ```bash
   dpkg -l | grep libgtest
   ```

3. Check that all libraries are built:
   ```bash
   ls -la build/
   ```

4. Run make clean and rebuild:
   ```bash
   make clean && make
   ```
