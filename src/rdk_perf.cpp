/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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
*/


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <string>
#include <map>

#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"
#include "rdk_perf.h"

#include <glib.h>
#include <unistd.h>

#define REPORTING_INITIAL_COUNT 5000
#define REPORTING_INTERVAL_COUNT 20000
#define TIMER_INTERVAL_SECONDS 10
#define MAX_DELAY 60


static std::map<pid_t, PerfProcess*>* sp_ProcessMap;

static void __attribute__((constructor)) PerfModuleInit();
static void __attribute__((destructor)) PerfModuleTerminate();

static guint s_timerID = 0;

static bool TimerCallback (gpointer pData)
{
    SCOPED_LOCK();
    bool bTimerContinue = true;
    static uint32_t nDelay = 0;
    static uint32_t nCount = 0;

    // Validate that threads in process are still active
    auto it = sp_ProcessMap->find(getpid());
    if(it != sp_ProcessMap->end()) {
        // Found a process in the list
        if(nCount > nDelay) {
            RDKPerf_ReportProcess(getpid());
            nCount = 0;
            if(nDelay < MAX_DELAY) {
                nDelay+=5;
            }
            LOG(eWarning, "Next performance log in %d seconds\n", nDelay * TIMER_INTERVAL_SECONDS);
        }
        nCount++;
    }
    else {
        LOG(eTrace, "Could not find Process ID %X for reporting\n", (uint32_t)getpid());
    }

    // Print report for process.
    return bTimerContinue;
}

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
static void PerfModuleInit()
{
    SCOPED_LOCK();
    sp_ProcessMap = new std::map<pid_t, PerfProcess*>();
    sp_ProcessMap->clear();

    LOG(eWarning, "RDK Performance process initialize %X\n", getpid());
    if(s_timerID == 0) {
        s_timerID = g_timeout_add_seconds(TIMER_INTERVAL_SECONDS, (GSourceFunc)TimerCallback, (gpointer)sp_ProcessMap);
        LOG(eTrace, "Created new timer (%u) with the context %p\n", s_timerID, NULL);
    }
}
  
// This function is assigned to execute as library unload
// using __attribute__((destructor))
static void PerfModuleTerminate()
{
    SCOPED_LOCK();
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

    // Stop the timer
    g_source_remove(s_timerID);
    s_timerID = 0;

    delete sp_ProcessMap;
}

RDKPerf::RDKPerf(const char* szName) 
: m_node(szName)
{
    return;
}
RDKPerf::RDKPerf(const char* szName, uint32_t nThresholdInUS)
: m_node(szName)
{
    m_node.SetThreshold(nThresholdInUS);
    return;
}

void RDKPerf::SetThreshhold(uint32_t nThresholdInUS)
{
    m_node.SetThreshold(nThresholdInUS); 
}

RDKPerf::~RDKPerf()
{
    return;
}

PerfProcess* RDKPerf_FindProcess(pid_t pID)
{
    SCOPED_LOCK();
    PerfProcess* retVal = NULL;

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
    SCOPED_LOCK();
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

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
        // Close threads that have no activity since the last report
        pProcess->CloseInactiveThreads();
        LOG(eWarning, "Printing process report for Process ID %X\n", (uint32_t)pID);
        pProcess->ReportData();
    }

    return;
}
void RDKPerf_ReportThread(pthread_t tID)
{
    SCOPED_LOCK();
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

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
    SCOPED_LOCK();
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

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
    retVal = (RDKPerfHandle)new RDKPerf(szName);

    return retVal;
}

void RDKPerfStop(RDKPerfHandle hPerf)
{
    RDKPerf* perf = (RDKPerf*)hPerf;
    if(perf != NULL) {
        delete perf;
    }
    return;
}

} // extern "C" 