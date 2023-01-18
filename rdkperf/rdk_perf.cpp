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
#include <mutex>
#include <ctime>
#include <ratio>
#include <chrono>
#include <condition_variable>

#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"
#include "rdk_perf_msgqueue.h"
#include "rdk_perf_process.h"
#include "rdk_perf_tree.h"  // Needs to come after rdk_perf_process because of forward declaration of PerfTree
#include "rdk_perf.h"

#include <unistd.h>

#define REPORTING_INITIAL_COUNT 5000
#define REPORTING_INTERVAL_COUNT 20000
#define TIMER_INTERVAL_SECONDS 10
#define MAX_DELAY 600


#ifdef PERF_REMOTE
static PerfMsgQueue* s_pQueue = NULL;
#endif // PERF_REMOTE

static void __attribute__((constructor)) PerfModuleInit();
static void __attribute__((destructor)) PerfModuleTerminate();

class TimerCallback {
public:
    enum SignalResult {
        WAITING     = 0,
        TIMEOUT     = 1,
        EXIT_LOOP   = 2
    };

    TimerCallback (void* pContext) 
    : m_Context(pContext)
    , m_bContinue(false)
    , m_nDelay(0)
    , m_nCount(0)
    , m_current_state(WAITING)
    {
        LOG(eWarning, "Timer Created\n");
    };
    
    ~TimerCallback() {
        LOG(eWarning, "Timer destroyed\n");
    }; 

    void Signal (TimerCallback::SignalResult value)
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_current_state = value;
        m_cv.notify_one();
        lck.unlock();
    }

    SignalResult Wait(unsigned int time_in_seconds)
    {
        TimerCallback::SignalResult result;
        std::chrono::milliseconds ms(time_in_seconds * 1000);

        std::unique_lock<std::mutex> lck(m_mtx);

        if(m_current_state == WAITING) {
            if(m_cv.wait_for(lck, ms) == std::cv_status::timeout) {
                result = TIMEOUT;
            }
            else {
                result = m_current_state;
            }
        }
        else {
            // State change before the lock was called
            result = m_current_state;
        }
        m_current_state = WAITING;

        lck.unlock();

        return result;
    }

    void StopTask() {
        LOG(eWarning, "Stoping Timer Task\n");
        m_bContinue = false;
        Signal(EXIT_LOOP);
        return;
    };

    bool Loop() {
        bool bTimerContinue = true;
 
        LOG(eTrace, "Timer Callback! m_nCount = %d m_nDelay = %d\n", m_nCount, m_nDelay);

        // Validate that threads in process are still active
        if(RDKPerf_FindProcess(getpid()) != NULL) {
            // Found a process in the list
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
            LOG(eTrace, "Could not find Process ID %X from map of size %d for reporting\n", (uint32_t)getpid(), RDKPerf_GetMapSize());
        }

        return bTimerContinue;
    }

    void Task() {
        m_bContinue = true;
        LOG(eWarning, "Task Started\n");
        while(m_bContinue == true) {
            if(!Loop()) {
                LOG(eWarning, "Timer loop signaled for Exit..\n");
                m_bContinue = false;
                break;
            }
            LOG(eTrace, "Task sleeping %d seconds\n", TIMER_INTERVAL_SECONDS);
            SignalResult result = Wait(TIMER_INTERVAL_SECONDS);
            if(result == EXIT_LOOP) {
                LOG(eWarning, "Exit task loop has been signaled\n");
            }
        }
        LOG(eWarning, "Task Completed\n");
        return;
    };
private:
    void*       m_Context;
    bool        m_bContinue; 
    uint32_t    m_nDelay;
    uint32_t    m_nCount;
    // Timeout, signaling
    TimerCallback::SignalResult m_current_state;
    std::mutex   m_mtx;
    std::condition_variable m_cv;
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
    
    RDKPerf_InitializeMap();
    s_timer = new TimerCallback(NULL);

    if(s_thread == NULL) {
        s_thread = new std::thread(&TimerCallback::Task, s_timer);
        LOG(eWarning, "Created new timer (%X) with the context %p\n", s_thread->get_id(), NULL);
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
    // Print report
    RDKPerf_ReportProcess(pID);
    // Remove prosess from list
    RDKPerf_RemoveProcess(pID);
    // Wait for timer thread cleanup
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

#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->Release();
    }
#endif // PERF_REMOTE

    RDKPerf_DeleteMap();
}

#ifdef NO_PERF
//-------------------------------------------
RDKPerfEmpty::RDKPerfEmpty(const char* szName) 
{
    printf("RDKPerfEmpty --> %s\n", szName);fflush(stdout);
    return;
}
RDKPerfEmpty::RDKPerfEmpty(const char* szName, uint32_t nThresholdInUS)
{
    printf("RDKPerfEmpty(t) --> %s\n", szName);fflush(stdout);
    return;
}
void RDKPerfEmpty::SetThreshhold(uint32_t nThresholdInUS)
{
}
RDKPerfEmpty::~RDKPerfEmpty()
{
    return;
}
#endif
//-------------------------------------------
RDKPerfInProc::RDKPerfInProc(const char* szName) 
: m_record(szName)
{
    return;
}
RDKPerfInProc::RDKPerfInProc(const char* szName, uint32_t nThresholdInUS)
: m_record(szName)
{
    m_record.SetThreshold((int32_t)nThresholdInUS);
    return;
}

void RDKPerfInProc::SetThreshhold(uint32_t nThresholdInUS)
{
    m_record.SetThreshold((int32_t)nThresholdInUS); 
}

RDKPerfInProc::~RDKPerfInProc()
{
    return;
}

//-------------------------------------------
RDKPerfRemote::RDKPerfRemote(const char* szName) 
: m_szName(szName)
, m_nThresholdInUS(0)
, m_EndTime(0)
{
    m_StartTime = PerfRecord::TimeStamp();

    // Send enter event
#ifdef PERF_REMOTE
    if(s_pQueue == NULL) s_pQueue = PerfMsgQueue::GetQueue(RDK_PERF_MSG_QUEUE_NAME, false);
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eEntry, m_szName, m_StartTime, m_nThresholdInUS);
    }
    else {
        LOG(eError, "Could not get Message Queue to send perf events\n");
    }
#endif // PERF_REMOTE    
    return;
}
RDKPerfRemote::RDKPerfRemote(const char* szName, uint32_t nThresholdInUS)
: m_szName(szName)
, m_nThresholdInUS(nThresholdInUS)
{
    m_StartTime = PerfRecord::TimeStamp();
 
     // Send enter event
#ifdef PERF_REMOTE
    if(s_pQueue == NULL) s_pQueue = PerfMsgQueue::GetQueue(RDK_PERF_MSG_QUEUE_NAME, false);
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eEntry, m_szName, m_StartTime, nThresholdInUS);
    }
    else {
        LOG(eError, "Could not get Message Queue to send perf events\n");
    }
#endif // PERF_REMOTE    
   return;
}

void RDKPerfRemote::SetThreshhold(uint32_t nThresholdInUS)
{
    // Send threshhold event
    m_nThresholdInUS = nThresholdInUS;
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eThreshold, m_szName, 0, m_nThresholdInUS);
    }
#endif // PERF_REMOTE    
}

RDKPerfRemote::~RDKPerfRemote()
{
    m_EndTime = PerfRecord::TimeStamp();

    // Send close event
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eExit, m_szName, m_EndTime - m_StartTime);
    }
    else {
        LOG(eError, "Could not get Message Queue to send perf events\n");
    }
#endif // PERF_REMOTE    
    return;
}

//-------------------------------------------
extern "C" {

void RDKPerf_ReportProcess(pid_t pID)
{
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eReportProcess, NULL, 0, 0);
    }
#else // PERF_REMOTE
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    pProcess = RDKPerf_FindProcess(pID);
    
    if(pProcess != NULL) {
        pProcess->ShowTrees();
        // Close threads that have no activity since the last report
        pProcess->CloseInactiveThreads();
        LOG(eWarning, "Printing process report for Process ID %X\n", (uint32_t)pID);
        pProcess->ReportData();
    }
#endif // PERF_REMOTE  

    return;
}

void RDKPerf_ReportThread(pthread_t tID)
{
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eReportThread, NULL, 0, 0);
    }
#else // PERF_REMOTE
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    pProcess = RDKPerf_FindProcess(getpid());

    PerfTree* pTree = pProcess->GetTree(tID);
    if(pTree != NULL) {
        LOG(eWarning, "Printing tree report for Task ID %X\n", (uint32_t)tID);
        pTree->ReportData();
    }
#endif // PERF_REMOTE
    return;

}

void RDKPerf_CloseThread(pthread_t tID)
{
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eCloseThread, NULL, 0, 0);
    }
#else // PERF_REMOTE
    // Find Process ID in List
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // Find thread in process map
    pProcess = RDKPerf_FindProcess(getpid());

    if(pProcess != NULL) {
        pProcess->RemoveTree(tID);
    }
#endif // PERF_REMOTE
    return;
}

void RDKPerf_CloseProcess(pid_t pID)
{
#ifdef PERF_REMOTE
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eCloseProcess, NULL, 0, 0);
    }
#else // PERF_REMOTE
    // Find Process ID in List
    SCOPED_LOCK();

    RDKPerf_RemoveProcess(pID);
#endif // PERF_REMOTE
    return;
}

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

void RDKPerfSetThreshold(RDKPerfHandle hPerf, uint32_t nThresholdInUS)
{
    RDKPerf* perf = (RDKPerf*)hPerf;

    perf->SetThreshhold(nThresholdInUS);

    return;
}


} // extern "C" 
