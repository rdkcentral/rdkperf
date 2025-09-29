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

#ifndef __RDK_PERF_H__
#define __RDK_PERF_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#ifdef __cplusplus

#include <string>
#include <list>
#include <map>
#include <stack>

#include "rdk_perf_record.h"
//#include "rdk_perf_node.h"
#ifdef PERF_SHOW_CPU
#include "rdk_perf_clock.h"
#endif

#define FUNC_METRICS_START(depth)                                   \
    {                                                               \
        static uint16_t x_nIdx = 0;                                 \
        static uint64_t x_data[depth] = { 0 };                      \
        uint16_t        x_depth = depth;                            \
        uint64_t        nStartTime = PerfRecord::TimeStamp();       \


#define FUNC_METRICS_END()                                          \
        if(x_nIdx < x_depth) {                                      \
            x_data[x_nIdx++] = PerfRecord::TimeStamp() - nStartTime;\
        } else {                                                    \
            uint64_t sum = 0;                                       \
            while(x_nIdx != 0) {                                    \
                sum += x_data[--x_nIdx];                            \
            }                                                       \
            printf("%s avg time is %0.6lf ms for %d iterations\n",  \
                    __FUNCTION__, ((double)sum) / (x_depth * 1000.0), x_depth); \
        }                                                           \
    }                                                               \

#ifdef NO_PERF
class RDKPerfEmpty
{
public:
    RDKPerfEmpty(const char* szName, uint32_t nThresholdInUS);
    RDKPerfEmpty(const char* szName);
    ~RDKPerfEmpty();

    void SetThreshhold(uint32_t nThresholdInUS);

private:
};
#endif

class RDKPerfInProc
{
public:
    RDKPerfInProc(const char* szName, uint32_t nThresholdInUS);
    RDKPerfInProc(const char* szName);
    ~RDKPerfInProc();

    void SetThreshhold(uint32_t nThresholdInUS);

private:
    PerfRecord        m_record;
};

class RDKPerfRemote
{
public:
    RDKPerfRemote(const char* szName, uint32_t nThresholdInUS);
    RDKPerfRemote(const char* szName);
    ~RDKPerfRemote();

    void SetThreshhold(uint32_t nThresholdInUS);

private:
    const char* m_szName;
    uint32_t    m_nThresholdInUS;
#ifdef PERF_SHOW_CPU
    PerfClock   m_clock;
#else
    uint64_t    m_StartTime;
#endif
};


#ifdef NO_PERF
    #define RDKPerf RDKPerfEmpty
#else
    #ifdef PERF_REMOTE
    #define RDKPerf RDKPerfRemote
    #else
    #define RDKPerf RDKPerfInProc
    #endif // PERF_REMOTE
#endif // NO_PERF

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void RDKPerf_ReportProcess(pid_t pID);
void RDKPerf_ReportThread(pthread_t tID);
void RDKPerf_CloseThread(pthread_t tID);
void RDKPerf_CloseProcess(pid_t pID);

typedef void* RDKPerfHandle;
RDKPerfHandle RDKPerfStart(const char* szName);
void RDKPerfStop(RDKPerfHandle hPerf);
void RDKPerfSetThreshold(RDKPerfHandle hPerf, uint32_t nThresholdInUS);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RDK_PERF_H__
