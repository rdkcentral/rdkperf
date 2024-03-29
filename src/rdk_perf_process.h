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


#ifndef __RDK_PERF_PROCESS_H__
#define __RDK_PERF_PROCESS_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <stack>
#include "rdk_perf_clock.h"

#define PROCESS_NAMELEN 80

// Forward decls
class PerfTree;

class PerfProcess
{
public:
    PerfProcess(pid_t pID);
    ~PerfProcess();

    PerfTree* GetTree(pthread_t tID);
    PerfTree* NewTree(pthread_t tID);
    void ShowTrees();
    void ShowTree(PerfTree* pTree);
    void ReportData();
    void GetProcessName();
    bool CloseInactiveThreads();
    bool RemoveTree(pthread_t tID);

private:
    pid_t                           m_idProcess;
    char                            m_ProcessName[PROCESS_NAMELEN];
    std::map<pthread_t, PerfTree*>  m_mapThreads;
    PerfClock                       m_clock;
};

PerfProcess* RDKPerf_FindProcess(pid_t pID);
void RDKPerf_InsertProcess(pid_t pID, PerfProcess* pProcess);
void RDKPerf_RemoveProcess(pid_t pID);
void RDKPerf_InitializeMap();
void RDKPerf_DeleteMap();
size_t RDKPerf_GetMapSize();


#endif // __RDK_PERF_PROCESS_H__
