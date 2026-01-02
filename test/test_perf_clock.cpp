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
#include "rdk_perf_clock.h"

class PerfClockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(PerfClockTest, ConstructorDestructor) {
    PerfClock* clock = new PerfClock();
    ASSERT_NE(clock, nullptr);
    delete clock;
}

TEST_F(PerfClockTest, WallClockMeasurement) {
    PerfClock clock;
    PerfClock::Now(&clock, PerfClock::Marker);
    
    // Sleep for a known duration
    usleep(10000); // 10ms
    
    PerfClock::Now(&clock, PerfClock::Elapsed);
    
    uint64_t wallClock = clock.GetWallClock(PerfClock::microsecond);
    
    // Wall clock should be approximately 10000 microseconds (with some tolerance)
    EXPECT_GE(wallClock, 9000);  // At least 9ms
    EXPECT_LE(wallClock, 20000); // At most 20ms (generous tolerance for CI)
}

TEST_F(PerfClockTest, TimeUnitsConversion) {
    PerfClock clock;
    PerfClock::Now(&clock, PerfClock::Marker);
    usleep(10000); // 10ms
    PerfClock::Now(&clock, PerfClock::Elapsed);
    
    uint64_t microseconds = clock.GetWallClock(PerfClock::microsecond);
    uint64_t milliseconds = clock.GetWallClock(PerfClock::millisecond);
    
    // Milliseconds should be roughly 1/1000 of microseconds
    const double TIMING_TOLERANCE_MS = 5.0;
    EXPECT_NEAR(microseconds / 1000.0, milliseconds, TIMING_TOLERANCE_MS);
}

TEST_F(PerfClockTest, CPUTimeMeasurement) {
    PerfClock clock;
    PerfClock::Now(&clock, PerfClock::Marker);
    
    // Do some CPU-intensive work
    volatile uint64_t sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }
    
    PerfClock::Now(&clock, PerfClock::Elapsed);
    
    uint64_t userCPU = clock.GetUserCPU(PerfClock::microsecond);
    uint64_t systemCPU = clock.GetSystemCPU(PerfClock::microsecond);
    
    // User CPU time should be non-zero for CPU work
    EXPECT_GT(userCPU, 0);
    // System CPU time should be measurable (could be zero)
    EXPECT_GE(systemCPU, 0);
}

TEST_F(PerfClockTest, StaticNowMethod) {
    PerfClock* clock = PerfClock::Now();
    ASSERT_NE(clock, nullptr);
    
    // Clock should have valid wall clock time
    uint64_t wallClock = clock->GetWallClock(PerfClock::microsecond);
    EXPECT_GT(wallClock, 0);
    
    delete clock;
}

TEST_F(PerfClockTest, SettersAndGetters) {
    PerfClock clock;
    
    clock.SetWallClock(12345);
    clock.SetUserCPU(6789);
    clock.SetSystemCPU(4321);
    
    EXPECT_EQ(clock.GetWallClock(PerfClock::microsecond), 12345);
    EXPECT_EQ(clock.GetUserCPU(PerfClock::microsecond), 6789);
    EXPECT_EQ(clock.GetSystemCPU(PerfClock::microsecond), 4321);
}

TEST_F(PerfClockTest, MultipleMarkerElapsedCycles) {
    PerfClock clock;
    
    // First measurement
    PerfClock::Now(&clock, PerfClock::Marker);
    usleep(5000);
    PerfClock::Now(&clock, PerfClock::Elapsed);
    uint64_t first = clock.GetWallClock(PerfClock::microsecond);
    
    // Second measurement (should reset)
    PerfClock::Now(&clock, PerfClock::Marker);
    usleep(5000);
    PerfClock::Now(&clock, PerfClock::Elapsed);
    uint64_t second = clock.GetWallClock(PerfClock::microsecond);
    
    // Both measurements should be around 5ms
    EXPECT_GE(first, 4000);
    EXPECT_LE(first, 10000);
    EXPECT_GE(second, 4000);
    EXPECT_LE(second, 10000);
}
