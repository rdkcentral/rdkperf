# RDKPerf Test Suite

This directory contains a comprehensive test suite for RDKPerf using Google Test (gtest).

## Overview

The test suite consists of multiple test files that cover all major components of the RDKPerf library:

- **test_main.cpp**: Main entry point for the test suite
- **test_perf_clock.cpp**: Tests for PerfClock (timing and CPU measurement)
- **test_perf_record.cpp**: Tests for PerfRecord (performance record management)
- **test_perf_node.cpp**: Tests for PerfNode (tree node operations and statistics)
- **test_perf_tree.cpp**: Tests for PerfTree (tree management and reporting)
- **test_perf_process.cpp**: Tests for PerfProcess (process-level operations)
- **test_rdk_perf.cpp**: Tests for the main RDKPerf API (both C++ and C interfaces)
- **test_instrumentation_overhead.cpp**: Tests that quantify the cost of instrumentation

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

### Instrumentation Overhead Tests (7 tests)

These tests quantify the performance cost of using RDKPerf instrumentation on the target platform:

1. **Constructor/Destructor Overhead**
   - Measures the overhead of creating and destroying RDKPerf objects
   - Reports overhead per call and percentage overhead

2. **Nested Instrumentation Overhead**
   - Measures overhead with nested function calls
   - Reports overhead for multi-level call stacks

3. **Work Function Overhead**
   - Measures overhead impact on actual CPU work
   - Ensures instrumentation doesn't significantly slow down the application

4. **C vs C++ Interface Overhead**
   - Compares performance of C and C++ APIs
   - Helps choose the most efficient interface

5. **Threshold Feature Overhead**
   - Measures additional cost of threshold checking
   - Verifies threshold feature is lightweight

6. **Memory Overhead**
   - Measures sizeof() for RDKPerf object wrappers
   - Note: Does not include dynamically allocated tree/node structures
   - Reports object size only; actual runtime memory is higher

7. **Minimal Call Overhead**
   - Measures overhead with minimal or no work
   - Reports calls per second achievable

## Test Results

All 74 tests should pass. The instrumentation overhead tests provide detailed output showing:

- Time without instrumentation
- Time with instrumentation
- Total overhead in microseconds
- Overhead per call
- Overhead percentage
- Calls per second

Typical overhead results:
- Per-call overhead: ~0.6-0.7 microseconds
- Overhead on real work: ~2-5%
- Memory per instance: ~88 bytes
- Throughput: ~1.6 million calls/second

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
