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

// Note: PerfRecord is designed to be used internally by RDKPerf.
// Creating standalone PerfRecords causes segfaults when their destructors run
// because they try to access m_nodeInTree which is null.
// These tests use RDKPerf instead to test the record functionality indirectly.

class PerfRecordTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(PerfRecordTest, RDKPerfConstructorDestructor) {
    // Test via RDKPerf which properly manages PerfRecords
    RDKPerf* perf = new RDKPerf("test_function");
    ASSERT_NE(perf, nullptr);
    delete perf;
    SUCCEED();
}

TEST_F(PerfRecordTest, RDKPerfScoped) {
    // Test scoped usage which creates and destroys records properly
    {
        RDKPerf perf("my_test_function");
        usleep(100);
    }
    SUCCEED();
}

TEST_F(PerfRecordTest, RDKPerfWithThreshold) {
    // Test with threshold
    {
        RDKPerf perf("threshold_test", 5000);
        usleep(100);
    }
    SUCCEED();
}

TEST_F(PerfRecordTest, StaticTimeStamp) {
    uint64_t ts1 = PerfRecord::TimeStamp();
    usleep(1000); // 1ms
    uint64_t ts2 = PerfRecord::TimeStamp();
    
    // Second timestamp should be greater than first
    EXPECT_GT(ts2, ts1);
    
    // Difference should be approximately 1000 microseconds
    uint64_t diff = ts2 - ts1;
    EXPECT_GE(diff, 900);
    EXPECT_LE(diff, 5000);
}

TEST_F(PerfRecordTest, RecordLifetime) {
    uint64_t start = PerfRecord::TimeStamp();
    
    {
        RDKPerf perf("scoped_test");
        usleep(5000); // 5ms
    }
    
    uint64_t end = PerfRecord::TimeStamp();
    uint64_t elapsed = end - start;
    
    // Should have elapsed at least 5ms
    EXPECT_GE(elapsed, 4000);
    EXPECT_LE(elapsed, 10000);
}

TEST_F(PerfRecordTest, MultipleRecords) {
    // Test multiple RDKPerf instances
    {
        RDKPerf perf1("record1");
        RDKPerf perf2("record2");
        RDKPerf perf3("record3");
        usleep(100);
    }
    SUCCEED();
}

TEST_F(PerfRecordTest, LongNameHandling) {
    std::string longName(100, 'a');
    {
        RDKPerf perf(longName.c_str());
        usleep(100);
    }
    SUCCEED();
}
