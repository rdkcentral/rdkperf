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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <string>
#include <map>
#include <thread>
#include <chrono>

#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"
#include "rdk_perf.h"

#include <glib.h>
#include <unistd.h>

#define REPORTING_INITIAL_COUNT 5000
#define REPORTING_INTERVAL_COUNT 20000
#define TIMER_INTERVAL_SECONDS 1
#define MAX_DELAY 600


static std::map<pid_t, PerfProcess*>* sp_ProcessMap;

static void __attribute__((constructor)) PerfModuleInit();
static void __attribute__((destructor)) PerfModuleTerminate();

class TimerCallback {
public:
    TimerCallback (std::map<pid_t, PerfProcess*>* pProcessMap) 
    : m_ProcessMap(pProcessMap)
    , m_bContinue(false)
    , m_nDelay(0)
    , m_nCount(0)
    {
        LOG(eWarning, "Timer Created\n");
    };
    
    ~TimerCallback() {
        LOG(eWarning, "Timer destroyed\n");
    }; 

    void StopTask() {
        LOG(eWarning, "Stoping Timer Task\n");
        m_bContinue = false;
        return;
    };

    bool Loop() {
        bool bTimerContinue = true;
 
        LOG(eWarning, "Timer Callback! m_nCount = %d m_nDelay = %d\n", m_nCount, m_nDelay);

        // Validate that threads in process are still active
        auto it = m_ProcessMap->find(getpid());
        if(it != m_ProcessMap->end()) {
            // Found a process in the list
// TEST
            RDKPerf_ReportProcess(getpid()); return true;

            if(m_nCount > m_nDelay) {
                RDKPerf_ReportProcess(getpid());
                m_nCount = 0;
                if(m_nDelay < MAX_DELAY) {
                    m_nDelay+=5;
                }
                LOG(eWarning, "Next performance log in %d seconds\n", m_nDelay * TIMER_INTERVAL_SECONDS);
            }
            m_nCount++;
        }
        else {
            LOG(eError, "Could not find Process ID %X from map of size %d for reporting\n", (uint32_t)getpid(), m_ProcessMap->size());
        }

        return bTimerContinue;
    }

    void Task() {
        m_bContinue = true;
        LOG(eWarning, "Task Started\n");
        while(m_bContinue == true) {
            m_bContinue = Loop();
            LOG(eWarning, "Task sleeping %d seconds\n", TIMER_INTERVAL_SECONDS);
            sleep(TIMER_INTERVAL_SECONDS);
        }
        LOG(eWarning, "Task Completed\n");
        return;
    };
private:
    std::map<pid_t, PerfProcess*>*  m_ProcessMap;
    bool                            m_bContinue; 
    uint32_t                        m_nDelay;
    uint32_t                        m_nCount;

};

static std::thread*     s_thread = NULL;
static TimerCallback*   s_timer = NULL;

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
static void PerfModuleInit()
{
    // Test to see if message queue service is running
    
    char cmd[80] = { 0 };
    char strProcessName[PROCESS_NAMELEN];

    sprintf(cmd, "/proc/%d/cmdline", getpid());
    FILE* fp = fopen(cmd,"r");
    if(fp != NULL) {
        size_t size = fread(strProcessName, sizeof(char), PROCESS_NAMELEN - 1, fp);
        if(size <= 0) {
            LOG(eError, "Could not read process name\n");
        }
        fclose(fp);
    }

    LOG(eWarning, "RDK performance process initialize %X named %s\n", getpid(), strProcessName);       
    
    sp_ProcessMap = new std::map<pid_t, PerfProcess*>();
    sp_ProcessMap->clear();
    s_timer = new TimerCallback(sp_ProcessMap);

    if(s_thread == NULL) {
        s_thread = new std::thread(&TimerCallback::Task, s_timer);
        LOG(eWarning, "Created new timer (%X) with the context %p\n", s_thread->get_id(), sp_ProcessMap);
    }
    else {
        LOG(eWarning, "Timer already exists %X\n"), s_thread->get_id();
    }

    LOG(eTrace, "Exit init code\n");
}
  
// This function is assigned to execute as library unload
// using __attribute__((destructor))
static void PerfModuleTerminate()
{
    pid_t pID = getpid();

    LOG(eWarning, "RDK Performance process terminate %X\n", pID);
    RDKPerf_ReportProcess(pID);

    // Find thread in process map
    auto it = sp_ProcessMap->find(pID);
    if(it == sp_ProcessMap->end()) {
        LOG(eError, "Could not find Process ID %X for reporting\n", (uint32_t)pID);
    }
    else {
        LOG(eError, "Process Map size %d found entry for PID %X\n", sp_ProcessMap->size(), it->first);
        delete it->second;
        sp_ProcessMap->erase(it);
    }

    if(s_thread != NULL && s_thread->joinable()) {
        LOG(eWarning, "Cleaning up timer thread\n");
        s_timer->StopTask();
        s_thread->join();
        delete s_thread;
        delete s_timer;
        s_thread = NULL;
        s_timer = NULL;
    }
    else {
        LOG(eError, "Thread does not exist\n"); 
    }

    delete sp_ProcessMap;
}

//-------------------------------------------
RDKPerf::RDKPerf(const char* szName) 
: m_node(szName)
{
    SCOPED_LOCK();
    return;
}
RDKPerf::RDKPerf(const char* szName, uint32_t nThresholdInUS)
: m_node(szName)
{
    SCOPED_LOCK();
    m_node.SetThreshold(nThresholdInUS);
    return;
}

void RDKPerf::SetThreshhold(uint32_t nThresholdInUS)
{
    SCOPED_LOCK();
    m_node.SetThreshold(nThresholdInUS); 
}

RDKPerf::~RDKPerf()
{
    SCOPED_LOCK();
    return;
}
//-------------------------------------------
RDKPerfRemote::RDKPerfRemote(const char* szName) 
: m_szName(szName)
, m_nThresholdInUS(0)
{
    // Send enter event
    return;
}
RDKPerfRemote::RDKPerfRemote(const char* szName, uint32_t nThresholdInUS)
: m_szName(szName)
, m_nThresholdInUS(nThresholdInUS)
{
    // Send enter event
    return;
}

void RDKPerfRemote::SetThreshhold(uint32_t nThresholdInUS)
{
    // Send threshhold event
    m_nThresholdInUS = nThresholdInUS;
}

RDKPerfRemote::~RDKPerfRemote()
{
    // Send close event
    return;
}
//-------------------------------------------

PerfProcess* RDKPerf_FindProcess(pid_t pID)
{
    PerfProcess* retVal = NULL;

    SCOPED_LOCK();

    auto it = sp_ProcessMap->find(pID);
    if(it != sp_ProcessMap->end()) {
        retVal = it->second;
    }

    return retVal;
}
void RDKPerf_InsertProcess(pid_t pID, PerfProcess* pProcess)
{
    SCOPED_LOCK();
    sp_ProcessMap->insert(std::pair<pid_t, PerfProcess*>(pID, pProcess));
    LOG(eError, "Process Map %p size %d added entry for PID %X, pProcess %p\n", sp_ProcessMap, sp_ProcessMap->size(), pID, pProcess);
}

void RDKPerf_ReportProcess(pid_t pID)
{
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    auto it = sp_ProcessMap->find(pID);
    if(it == sp_ProcessMap->end()) {
        LOG(eError, "Could not find Process ID %X for reporting\n", (uint32_t)pID);
        return;
    }
    else {
        pProcess = it->second;
    }

    if(pProcess != NULL) {
        pProcess->ShowTrees();
        // Close threads that have no activity since the last report
        pProcess->CloseInactiveThreads();
        LOG(eWarning, "Printing process report for Process ID %X\n", (uint32_t)pID);
        pProcess->ReportData();
    }

    return;
}
void RDKPerf_ReportThread(pthread_t tID)
{
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    auto it = sp_ProcessMap->find(getpid());
    if(it == sp_ProcessMap->end()) {
        LOG(eError, "Could not find Process ID %X for reporting\n", (uint32_t)getpid());
        return;
    }
    else {
        pProcess = it->second;
    }

    PerfTree* pTree = pProcess->GetTree(tID);
    if(pTree != NULL) {
        LOG(eWarning, "Printing tree report for Task ID %X\n", (uint32_t)tID);
        pTree->ReportData();
    }

    return;

}

void RDKPerf_CloseThread(pthread_t tID)
{
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    auto it = sp_ProcessMap->find(getpid());
    if(it == sp_ProcessMap->end()) {
        LOG(eError, "Could not find Process ID %X for reporting\n", (uint32_t)getpid());
        return;
    }
    else {
        pProcess = it->second;
    }

    if(pProcess != NULL) {
        pProcess->RemoveTree(tID);
    }

    return;
}

extern "C" {
RDKPerfHandle RDKPerfStart(const char* szName)
{
    RDKPerfHandle retVal = NULL;
    SCOPED_LOCK();

    retVal = (RDKPerfHandle)new RDKPerf(szName);

    return retVal;
}

void RDKPerfStop(RDKPerfHandle hPerf)
{
    RDKPerf* perf = (RDKPerf*)hPerf;

    SCOPED_LOCK();

    if(perf != NULL) {
        delete perf;
    }
    return;
}

} // extern "C" 