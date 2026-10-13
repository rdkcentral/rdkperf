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


#ifndef __RDK_PERF_CLOCK_H__
#define __RDK_PERF_CLOCK_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>      // time_t

typedef struct _TimeStamp
{
    uint64_t    wallClock;
    uint64_t    userCPU;
    uint64_t    systemCPU;
} TimeStamp;

class PerfClock
{
public:
    typedef enum {
        Elapsed,
        Marker
    } Operation;

    typedef enum {
        microsecond     = 1,
        millisecond     = 1000,
    } TimeUnit;
    
    PerfClock(TimeStamp* pTS);
    PerfClock();
    ~PerfClock();
 
    void SetWallClock();
    void SetCPU();

    uint64_t GetWallClock(TimeUnit units = microsecond);
    uint64_t GetUserCPU(TimeUnit units = microsecond);
    uint64_t GetSystemCPU(TimeUnit units = microsecond);

    void SetWallClock(uint64_t wallClock) { m_timeStamp.wallClock = wallClock; };
    void SetUserCPU(uint64_t userCPU) { m_timeStamp.userCPU = userCPU; };
    void SetSystemCPU(uint64_t systemCPU) { m_timeStamp.systemCPU = systemCPU; };
    
    static PerfClock* Now();
    static void Now(PerfClock* pClock, Operation operation = Marker);

    const TimeStamp& GetTimeStamp() const;

private:
    uint64_t ConvertToUS(time_t sec, time_t usec);

    TimeStamp   m_timeStamp;
};

#endif // __RDK_PERF_CLOCK_H__ 