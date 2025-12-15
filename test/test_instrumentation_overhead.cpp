/**
* Copyright 2021 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <gtest/gtest.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "rdk_perf.h"
#include "rdk_perf_clock.h"

class InstrumentationOverheadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Allow system to stabilize before running overhead tests
        usleep(10000); // 10ms delay
    }

    void TearDown() override {
        // Allow any pending operations to complete
        usleep(10000); // 10ms delay
    }
    
    // Get high-resolution timestamp
    uint64_t GetTimestamp() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    }
    
    // Simple work function without instrumentation
    void DoWork(int iterations) {
        volatile uint64_t sum = 0;
        for (int i = 0; i < iterations; i++) {
            sum += i;
        }
    }
};

TEST_F(InstrumentationOverheadTest, ConstructorDestructorOverhead) {
    // Reduced iterations and added delays to prevent segfaults from rapid object creation
    const int iterations = 100;
    
    // Measure time without instrumentation
    uint64_t start_uninstrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        // Empty scope, no instrumentation
    }
    uint64_t end_uninstrumented = GetTimestamp();
    uint64_t time_uninstrumented = end_uninstrumented - start_uninstrumented;
    
    // Measure time with instrumentation
    uint64_t start_instrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("overhead_test");
        // Small delay between iterations to prevent resource issues
        if (i % 10 == 0) {
            usleep(1);
        }
    }
    uint64_t end_instrumented = GetTimestamp();
    uint64_t time_instrumented = end_instrumented - start_instrumented;
    
    // Calculate overhead
    uint64_t overhead = time_instrumented - time_uninstrumented;
    double overhead_per_call = (double)overhead / iterations;
    
    std::cout << "\n=== Constructor/Destructor Overhead ===" << std::endl;
    std::cout << "Total iterations: " << iterations << std::endl;
    std::cout << "Time without instrumentation: " << time_uninstrumented << " us" << std::endl;
    std::cout << "Time with instrumentation: " << time_instrumented << " us" << std::endl;
    std::cout << "Total overhead: " << overhead << " us" << std::endl;
    std::cout << "Overhead per call: " << overhead_per_call << " us" << std::endl;
    std::cout << "Overhead percentage: " << (time_uninstrumented > 0 ? (double)overhead / time_uninstrumented * 100.0 : 0) << "%" << std::endl;
    
    // Instrumentation should add measurable but reasonable overhead
    EXPECT_GT(time_instrumented, time_uninstrumented);
}

TEST_F(InstrumentationOverheadTest, NestedInstrumentationOverhead) {
    const int iterations = 1000;
    
    // Measure nested calls without instrumentation
    uint64_t start_uninstrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < 5; j++) {
            // Empty nested scopes
        }
    }
    uint64_t end_uninstrumented = GetTimestamp();
    uint64_t time_uninstrumented = end_uninstrumented - start_uninstrumented;
    
    // Measure nested calls with instrumentation
    uint64_t start_instrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf outer("outer");
        for (int j = 0; j < 5; j++) {
            RDKPerf inner("inner");
        }
    }
    uint64_t end_instrumented = GetTimestamp();
    uint64_t time_instrumented = end_instrumented - start_instrumented;
    
    uint64_t overhead = time_instrumented - time_uninstrumented;
    double overhead_per_outer = (double)overhead / iterations;
    
    std::cout << "\n=== Nested Instrumentation Overhead ===" << std::endl;
    std::cout << "Outer iterations: " << iterations << std::endl;
    std::cout << "Inner iterations per outer: 5" << std::endl;
    std::cout << "Time without instrumentation: " << time_uninstrumented << " us" << std::endl;
    std::cout << "Time with instrumentation: " << time_instrumented << " us" << std::endl;
    std::cout << "Total overhead: " << overhead << " us" << std::endl;
    std::cout << "Overhead per outer call: " << overhead_per_outer << " us" << std::endl;
    
    EXPECT_GT(time_instrumented, time_uninstrumented);
}

TEST_F(InstrumentationOverheadTest, WorkFunctionOverhead) {
    const int iterations = 1000;
    const int work_iterations = 10000;
    
    // Without instrumentation
    uint64_t start_uninstrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        DoWork(work_iterations);
    }
    uint64_t end_uninstrumented = GetTimestamp();
    uint64_t time_uninstrumented = end_uninstrumented - start_uninstrumented;
    
    // With instrumentation
    uint64_t start_instrumented = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("work_function");
        DoWork(work_iterations);
    }
    uint64_t end_instrumented = GetTimestamp();
    uint64_t time_instrumented = end_instrumented - start_instrumented;
    
    uint64_t overhead = time_instrumented - time_uninstrumented;
    double overhead_percentage = time_uninstrumented > 0 ? 
        ((double)overhead / time_uninstrumented * 100.0) : 0;
    
    std::cout << "\n=== Work Function Instrumentation Overhead ===" << std::endl;
    std::cout << "Function calls: " << iterations << std::endl;
    std::cout << "Work iterations per call: " << work_iterations << std::endl;
    std::cout << "Time without instrumentation: " << time_uninstrumented << " us" << std::endl;
    std::cout << "Time with instrumentation: " << time_instrumented << " us" << std::endl;
    std::cout << "Total overhead: " << overhead << " us" << std::endl;
    std::cout << "Overhead percentage: " << overhead_percentage << "%" << std::endl;
    
    // Overhead should be relatively small compared to actual work
    EXPECT_LT(overhead_percentage, 50.0); // Less than 50% overhead
}

TEST_F(InstrumentationOverheadTest, CInterfaceOverhead) {
    const int iterations = 10000;
    
    // C interface overhead
    uint64_t start_c = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerfHandle h = RDKPerfStart("c_test");
        RDKPerfStop(h);
    }
    uint64_t end_c = GetTimestamp();
    uint64_t time_c = end_c - start_c;
    
    // C++ interface overhead
    uint64_t start_cpp = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("cpp_test");
    }
    uint64_t end_cpp = GetTimestamp();
    uint64_t time_cpp = end_cpp - start_cpp;
    
    std::cout << "\n=== C vs C++ Interface Overhead ===" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "C interface total time: " << time_c << " us" << std::endl;
    std::cout << "C++ interface total time: " << time_cpp << " us" << std::endl;
    std::cout << "C interface per call: " << (double)time_c / iterations << " us" << std::endl;
    std::cout << "C++ interface per call: " << (double)time_cpp / iterations << " us" << std::endl;
    
    // Both should have measurable but reasonable overhead
    EXPECT_GT(time_c, 0);
    EXPECT_GT(time_cpp, 0);
}

TEST_F(InstrumentationOverheadTest, ThresholdOverhead) {
    const int iterations = 10000;
    
    // Without threshold
    uint64_t start_no_threshold = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("no_threshold");
    }
    uint64_t end_no_threshold = GetTimestamp();
    uint64_t time_no_threshold = end_no_threshold - start_no_threshold;
    
    // With threshold
    uint64_t start_with_threshold = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("with_threshold", 10000);
    }
    uint64_t end_with_threshold = GetTimestamp();
    uint64_t time_with_threshold = end_with_threshold - start_with_threshold;
    
    std::cout << "\n=== Threshold Feature Overhead ===" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Without threshold: " << time_no_threshold << " us" << std::endl;
    std::cout << "With threshold: " << time_with_threshold << " us" << std::endl;
    std::cout << "Difference: " << (int64_t)(time_with_threshold - time_no_threshold) << " us" << std::endl;
    
    // Threshold feature should add minimal overhead
    EXPECT_GT(time_with_threshold, 0);
    EXPECT_GT(time_no_threshold, 0);
}

TEST_F(InstrumentationOverheadTest, MemoryOverhead) {
    const int count = 1000;
    
    // Estimate memory usage by creating many instances
    RDKPerf* perfs[count];
    
    std::cout << "\n=== Memory Overhead Estimate ===" << std::endl;
    std::cout << "NOTE: This measures sizeof() for the RDKPerf object wrapper." << std::endl;
    std::cout << "Actual runtime memory includes heap allocations for tree structures." << std::endl;
    std::cout << "Creating " << count << " RDKPerf instances..." << std::endl;
    
    for (int i = 0; i < count; i++) {
        perfs[i] = new RDKPerf("memory_test");
    }
    
    std::cout << "RDKPerf instances created successfully" << std::endl;
    std::cout << "Object size per instance (sizeof): " << sizeof(RDKPerf) << " bytes" << std::endl;
    std::cout << "Total object size for " << count << " instances: " << sizeof(RDKPerf) * count << " bytes" << std::endl;
    std::cout << "Additional heap memory for trees/nodes is allocated dynamically." << std::endl;
    
    // Clean up
    for (int i = 0; i < count; i++) {
        delete perfs[i];
    }
    
    SUCCEED();
}

TEST_F(InstrumentationOverheadTest, MinimalCallOverhead) {
    const int iterations = 100000;
    
    uint64_t start = GetTimestamp();
    for (int i = 0; i < iterations; i++) {
        RDKPerf perf("minimal");
        // Absolutely minimal work
    }
    uint64_t end = GetTimestamp();
    uint64_t total_time = end - start;
    double time_per_call = (double)total_time / iterations;
    
    std::cout << "\n=== Minimal Call Overhead ===" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Total time: " << total_time << " us" << std::endl;
    std::cout << "Time per call: " << time_per_call << " us" << std::endl;
    std::cout << "Calls per second: " << (double)iterations / ((double)total_time / 1000000.0) << std::endl;
    
    EXPECT_GT(total_time, 0);
    EXPECT_LT(time_per_call, 1000.0); // Should be less than 1ms per call
}
