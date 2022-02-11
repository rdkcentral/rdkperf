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

#include <string>
#include <list>
#include <map>
#include <stack>

#include "rdk_perf_node.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"

class RDKPerf
{
public:
    RDKPerf(const char* szName, uint32_t nThresholdInUS);
    RDKPerf(const char* szName);
    ~RDKPerf();

    void SetThreshhold(uint32_t nThresholdInUS);

private:
    PerfNode        m_node;
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
};

PerfProcess* RDKPerf_FindProcess(pid_t pID);
void RDKPerf_InsertProcess(pid_t pID, PerfProcess*pProcess);
void RDKPerf_ReportProcess(pid_t pID);
void RDKPerf_ReportThread(pthread_t tID);
void RDKPerf_CloseThread(pthread_t tID);

extern "C" {

typedef void* RDKPerfHandle;
RDKPerfHandle RDKPerfStart(const char* szName);
void RDKPerfStop(RDKPerfHandle hPerf);

} // extern "C" 

#endif // __RDK_PERF_H__
