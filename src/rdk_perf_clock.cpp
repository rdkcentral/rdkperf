/**
* Copyright 2022 Comcast Cable Communications Management, LLC
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <unistd.h> 
#include <time.h>
#include <sys/time.h> 
#include <sys/times.h> 
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

#include "rdk_perf_clock.h"
#include "rdk_perf_logging.h"


static uint32_t s_ticksPerSecond = 0;


static void __attribute__((constructor)) PerfClockModuleInit();
static void __attribute__((destructor)) PerfClockModuleTerminate();

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
static void PerfClockModuleInit()
{
    s_ticksPerSecond = sysconf(_SC_CLK_TCK);
    LOG(eWarning, "Ticks/Second = %ld\n", s_ticksPerSecond);
}

// This function is assigned to execute as library unload
// using __attribute__((destructor))
static void PerfClockModuleTerminate()
{
    LOG(eTrace, "Terminate");
}

PerfClock::PerfClock()
{
    m_timeStamp.wallClock = 0;
    m_timeStamp.userCPU = 0;
    m_timeStamp.systemCPU = 0;
}

PerfClock::PerfClock(TimeStamp* pTS)
{
    m_timeStamp.wallClock   = pTS->wallClock;
    m_timeStamp.userCPU     = pTS->userCPU;
    m_timeStamp.systemCPU   = pTS->systemCPU;
}

PerfClock::~PerfClock()
{

}

void PerfClock::SetWallClock()
{
    struct timeval  timeStamp;

    gettimeofday(&timeStamp, NULL);

    // Convert timestamp to Micro Seconds
    m_timeStamp.wallClock = ConvertToUS(timeStamp.tv_sec, timeStamp.tv_usec);

    return;
}

void PerfClock::SetCPU()
{
    struct rusage data;
    // int who = RUSAGE_SELF; 
    int who = RUSAGE_THREAD; 
    if(getrusage(who, &data) == -1) {
         LOG(eError, "getrusage failure %d: %s\n", errno, strerror(errno));
    }

    if(data.ru_stime.tv_sec == 0 && data.ru_stime.tv_usec == 0) {
        LOG(eError, "getrusage failed values = 0\n");   
        m_timeStamp.systemCPU = 0;
        m_timeStamp.userCPU = 0;       
    }
    else {
        m_timeStamp.systemCPU   = ConvertToUS(data.ru_stime.tv_sec, data.ru_stime.tv_usec);
        m_timeStamp.userCPU     = ConvertToUS(data.ru_utime.tv_sec, data.ru_utime.tv_usec);
    }

    return;
}

uint64_t PerfClock::ConvertToUS(time_t sec, time_t usec)
{
    return (((uint64_t)sec) * 1000000) + (uint64_t)usec;
}

uint64_t PerfClock::GetWallClock(TimeUnit units)
{
    return m_timeStamp.wallClock / units;
}

uint64_t PerfClock::GetUserCPU(TimeUnit units)
{
    return m_timeStamp.userCPU / units;
}

uint64_t PerfClock::GetSystemCPU(TimeUnit units)
{
    return m_timeStamp.systemCPU / units;
}

// Static Methods ---------------------------
void PerfClock::Now(PerfClock* pClock, Operation operation)
{
    if(operation == Marker) {
        pClock->SetWallClock();
        pClock->SetCPU();
        // LOG(eWarning, "Got Time Marker %0.3lf\n", (double)pClock->GetWallClock() / 1000.0);
    }
    else if(operation == Elapsed) {
        PerfClock elapsed;
        elapsed.SetWallClock();
        elapsed.SetCPU();

        pClock->SetWallClock(elapsed.GetWallClock() - pClock->GetWallClock());
        pClock->SetUserCPU(elapsed.GetUserCPU() - pClock->GetUserCPU());
        pClock->SetSystemCPU(elapsed.GetSystemCPU() - pClock->GetSystemCPU());
        LOG(eWarning, "Got Time Elapsed (computed) %0.3lf User %0.3lf System %0.3lf\n", 
            (double)pClock->GetWallClock() / 1000.0,
            (double)pClock->GetUserCPU() / 1000.0,
            (double)pClock->GetSystemCPU() / 1000.0);
    }
    else {
        LOG(eError, "Unknown operation mode\n");
    }
}

PerfClock* PerfClock::Now()
{
    PerfClock* retVal = new PerfClock();
    
    Now(retVal);

    return retVal;
}
