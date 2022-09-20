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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "rdk_perf.h"
#include "rdk_perf_logging.h"


void timer_sleep(uint32_t timeMS)
{
    // Timer, sleep
    PerfClock timer;
    PerfClock::Now(&timer, PerfClock::Marker);
    usleep(timeMS * 1000);
    PerfClock::Now(&timer, PerfClock::Elapsed);
    LOG(eWarning, "UNIT_TEST (expected time %lu ms): %s WallClock = %llu, User = %llu, System = %llu\n",
        timeMS, __FUNCTION__,
        timer.GetWallClock(PerfClock::millisecond), timer.GetUserCPU(PerfClock::millisecond), timer.GetSystemCPU(PerfClock::millisecond));

    return;
}

void do_work(uint32_t timeMS)
{
    struct timeval timeStamp;
    gettimeofday(&timeStamp, NULL);
    // Convert timestamp to Micro Seconds
    uint64_t inital_time = (uint64_t)(((uint64_t)timeStamp.tv_sec * 1000000) + timeStamp.tv_usec);
    uint64_t elapsed_time = inital_time;
    while(elapsed_time - inital_time < (timeMS * 1000)) {
        gettimeofday(&timeStamp, NULL);
        elapsed_time = (uint64_t)(((uint64_t)timeStamp.tv_sec * 1000000) + timeStamp.tv_usec);
    }

    return;
}

void timer_work(uint32_t timeMS)
{
    // Timer, work
    PerfClock timer;
    PerfClock::Now(&timer, PerfClock::Marker);

    do_work(timeMS);

    PerfClock::Now(&timer, PerfClock::Elapsed);
    LOG(eWarning, "UNIT_TEST (expected time %lu ms): %s WallClock = %llu, User = %llu, System = %llu\n",
        timeMS, __FUNCTION__,
        timer.GetWallClock(PerfClock::millisecond), timer.GetUserCPU(PerfClock::millisecond), timer.GetSystemCPU(PerfClock::millisecond));

    return;
}

void record_with_work(uint32_t timeMS)
{
    int idx = 0;
    while(idx < 1) {
        RDKPerf perf (__FUNCTION__);

        usleep((timeMS / 2) * 1000);
        do_work(timeMS / 2);

        idx++;
    }

    RDKPerf_ReportThread(pthread_self());

    return;
}

void record_with_threshold(uint32_t timeMS)
{
    int idx = 0;
    while(idx < 1) {
        RDKPerf perf (__FUNCTION__, timeMS/2);

        do_work(timeMS);

        idx++;
    }

    return;    
}

// Unit Tests entry point
#define DELAY_SHORT 2 * 1000 // 2s
#define DELAY_LONG 10 * 1000 // 2s

void unit_tests()
{
    LOG(eWarning, "---------------------- Unit Tests START --------------------\n");

    timer_sleep(DELAY_SHORT);

    timer_work(DELAY_SHORT);

    record_with_work(DELAY_SHORT);

    record_with_threshold(DELAY_SHORT);
     
    LOG(eWarning, "---------------------- Unit Tests END --------------------\n");
    return;
}

