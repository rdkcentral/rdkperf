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
#include "rdk_perf_record.h"

class PerfRecordTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(PerfRecordTest, ConstructorDestructor) {
    PerfRecord* record = new PerfRecord("test_function");
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->GetName(), "test_function");
    delete record;
}

TEST_F(PerfRecordTest, GetName) {
    PerfRecord record("my_test_function");
    EXPECT_EQ(record.GetName(), "my_test_function");
}

TEST_F(PerfRecordTest, GetThreadID) {
    PerfRecord record("thread_test");
    pthread_t currentThread = pthread_self();
    EXPECT_EQ(record.GetThreadID(), currentThread);
}

TEST_F(PerfRecordTest, GetStartTime) {
    PerfRecord record("time_test");
    uint64_t startTime = record.GetStartTime();
    
    // Start time should be non-zero and reasonable
    EXPECT_GT(startTime, 0);
}

TEST_F(PerfRecordTest, SetThreshold) {
    PerfRecord record("threshold_test");
    
    // Set a threshold
    record.SetThreshold(5000); // 5ms threshold
    
    // We can't directly test the threshold value as it's private,
    // but we can ensure the call doesn't crash
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
        PerfRecord record("scoped_test");
        usleep(5000); // 5ms
    }
    
    uint64_t end = PerfRecord::TimeStamp();
    uint64_t elapsed = end - start;
    
    // Should have elapsed at least 5ms
    EXPECT_GE(elapsed, 4000);
    EXPECT_LE(elapsed, 10000);
}

TEST_F(PerfRecordTest, MultipleRecords) {
    PerfRecord record1("record1");
    PerfRecord record2("record2");
    PerfRecord record3("record3");
    
    EXPECT_EQ(record1.GetName(), "record1");
    EXPECT_EQ(record2.GetName(), "record2");
    EXPECT_EQ(record3.GetName(), "record3");
    
    // All should have the same thread ID
    EXPECT_EQ(record1.GetThreadID(), record2.GetThreadID());
    EXPECT_EQ(record2.GetThreadID(), record3.GetThreadID());
}

TEST_F(PerfRecordTest, LongNameHandling) {
    std::string longName(100, 'a');
    PerfRecord record(longName);
    EXPECT_EQ(record.GetName(), longName);
}
