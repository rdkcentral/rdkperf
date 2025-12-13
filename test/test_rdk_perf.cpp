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
#include "rdk_perf.h"

class RDKPerfTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(RDKPerfTest, CppConstructorWithName) {
    RDKPerf* perf = new RDKPerf("test_function");
    ASSERT_NE(perf, nullptr);
    delete perf;
}

TEST_F(RDKPerfTest, CppConstructorWithThreshold) {
    RDKPerf* perf = new RDKPerf("test_function", 5000);
    ASSERT_NE(perf, nullptr);
    delete perf;
}

TEST_F(RDKPerfTest, ScopedUsage) {
    {
        RDKPerf perf("scoped_test");
        usleep(1000); // 1ms work
    }
    // Destructor should have been called
    SUCCEED();
}

TEST_F(RDKPerfTest, NestedScopes) {
    {
        RDKPerf outer("outer_function");
        {
            RDKPerf inner("inner_function");
            usleep(500);
        }
        usleep(500);
    }
    SUCCEED();
}

TEST_F(RDKPerfTest, SetThreshold) {
    RDKPerf perf("threshold_test");
    perf.SetThreshhold(10000); // 10ms threshold
    SUCCEED();
}

TEST_F(RDKPerfTest, CInterfaceStartStop) {
    RDKPerfHandle handle = RDKPerfStart("c_interface_test");
    ASSERT_NE(handle, nullptr);
    
    usleep(1000);
    
    RDKPerfStop(handle);
    SUCCEED();
}

TEST_F(RDKPerfTest, CInterfaceSetThreshold) {
    RDKPerfHandle handle = RDKPerfStart("c_threshold_test");
    ASSERT_NE(handle, nullptr);
    
    RDKPerfSetThreshold(handle, 5000);
    
    usleep(1000);
    
    RDKPerfStop(handle);
    SUCCEED();
}

TEST_F(RDKPerfTest, CInterfaceMultipleCalls) {
    RDKPerfHandle h1 = RDKPerfStart("c_test1");
    RDKPerfHandle h2 = RDKPerfStart("c_test2");
    RDKPerfHandle h3 = RDKPerfStart("c_test3");
    
    ASSERT_NE(h1, nullptr);
    ASSERT_NE(h2, nullptr);
    ASSERT_NE(h3, nullptr);
    
    usleep(500);
    
    RDKPerfStop(h3);
    RDKPerfStop(h2);
    RDKPerfStop(h1);
    
    SUCCEED();
}

TEST_F(RDKPerfTest, ReportThread) {
    {
        RDKPerf perf("report_test");
        usleep(1000);
    }
    
    RDKPerf_ReportThread(pthread_self());
    SUCCEED();
}

TEST_F(RDKPerfTest, ReportProcess) {
    {
        RDKPerf perf("process_test");
        usleep(1000);
    }
    
    RDKPerf_ReportProcess(getpid());
    SUCCEED();
}

TEST_F(RDKPerfTest, CloseThread) {
    {
        RDKPerf perf("close_thread_test");
        usleep(1000);
    }
    
    RDKPerf_CloseThread(pthread_self());
    SUCCEED();
}

TEST_F(RDKPerfTest, CloseProcess) {
    {
        RDKPerf perf("close_process_test");
        usleep(1000);
    }
    
    RDKPerf_CloseProcess(getpid());
    SUCCEED();
}

// Test function to be instrumented
void instrumented_function() {
    RDKPerf perf(__FUNCTION__);
    usleep(500);
}

TEST_F(RDKPerfTest, InstrumentedFunction) {
    instrumented_function();
    SUCCEED();
}

TEST_F(RDKPerfTest, MultipleInstrumentedCalls) {
    for (int i = 0; i < 10; i++) {
        instrumented_function();
    }
    SUCCEED();
}

void recursive_instrumented(int depth) {
    RDKPerf perf(__FUNCTION__);
    if (depth > 0) {
        recursive_instrumented(depth - 1);
    }
}

TEST_F(RDKPerfTest, RecursiveInstrumentation) {
    recursive_instrumented(5);
    SUCCEED();
}

TEST_F(RDKPerfTest, LongRunningFunction) {
    RDKPerf perf("long_running");
    usleep(10000); // 10ms
    SUCCEED();
}

TEST_F(RDKPerfTest, VeryShortFunction) {
    RDKPerf perf("very_short");
    // No work, just measure overhead
    SUCCEED();
}
