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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "rdk_perf_logging.h"
#include "rdk_perf_msgqueue.h"
#include "rdk_perf_process.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_node.h"

#define MESSAGE_TIMEOUT 10000
//#define MAX_TIMEOUT 60     // ~ 10 minutes
#define MAX_TIMEOUT 6     // ~ 1 minutes

PerfTree* GetTree(pid_t pID, pthread_t tID, char* szName, bool bCreate = false) 
{
    // Find thread in process map
    PerfProcess* pProcess = RDKPerf_FindProcess(pID);
    if(pProcess == NULL) {
        if(!bCreate) {
            LOG(eError, "Create not enabled, but process %X not found\n", pID);
            return NULL;
        }
        // no existing PID in map
        pProcess = new PerfProcess(pID);
        RDKPerf_InsertProcess(pID, pProcess);
        LOG(eWarning, "Creating new process element %X for node element %s\n", pProcess, szName);
    }

    // Found PID get element tree for current thread.
    PerfTree* pTree = pProcess->GetTree(tID);
    if(pTree == NULL) {
        if(bCreate) {
            pTree = pProcess->NewTree(tID);
            LOG(eError, "Tree created %X\n", tID);
        }
        else {
            LOG(eError, "Tree not found %X but create not enabled\n", tID);
        }
    }

    return pTree;
}

bool HandleEntry(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.entry.pID;
    pthread_t       tID = pMsg->msg_data.entry.tID;
    char*           szName = pMsg->msg_data.entry.szName;
    char*           szThreadName = pMsg->msg_data.entry.szThreadName;

    // Handle the node entry message
    LOG(eTrace, "Creating node for element %s pid %X tid %X\n", 
                szName, pID, tID);
    
    PerfTree* pTree = GetTree(pID, tID, szName, true);
    
    if(pTree) { 
        PerfNode* pNode = pTree->AddNode(szName,
                                         tID,
                                         szThreadName,
#ifdef PERF_SHOW_CPU
                                         0
#else
                                         pMsg->msg_data.entry.nTimeStamp
#endif
                                         );
        if(pMsg->msg_data.entry.nThresholdInUS > 0) {
            pNode->SetThreshold(pMsg->msg_data.entry.nThresholdInUS);
        }
        retVal = true;
    }

    return retVal;
}

bool HandleThreshold(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.threshold.pID;
    pthread_t       tID = pMsg->msg_data.threshold.tID;
    char*           szName = pMsg->msg_data.threshold.szName;
    int32_t         nThreshold = pMsg->msg_data.threshold.nThresholdInUS;

    // Handle the node threshold message
    LOG(eTrace, "Setting threshold %d node for element %s pid %X tid %X\n", 
                nThreshold, szName, pID, tID);
    
    PerfTree* pTree = GetTree(pID, tID, szName);
    
    if(pTree) { 
        PerfNode* pNode = pTree->GetStack()->top();
        if(pNode != NULL && nThreshold > 0) {
            pNode->SetThreshold(nThreshold);
            retVal = true;
        }
    }

    return retVal;
}

bool HandleExit(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.exit.pID;
    pthread_t       tID = pMsg->msg_data.exit.tID;
    char*           szName = pMsg->msg_data.exit.szName;

    // Handle the node exit message
    LOG(eTrace, "closing node for element %s pid %X tid %X\n", 
                szName, pID, tID);
    
    PerfTree* pTree = GetTree(pID, tID, szName);
    
    if(pTree) { 
        PerfNode* pNode = pTree->GetStack()->top();
        if(pNode != NULL && 
            pNode->GetName().compare(szName) == 0) {
#ifdef PERF_SHOW_CPU
            PerfClock deltaClock(&pMsg->msg_data.exit.clkTimeStamp);
            pNode->IncrementData(deltaClock.GetWallClock(),
                                 deltaClock.GetUserCPU(),
                                 deltaClock.GetSystemCPU());
#else
            pNode->IncrementData(pMsg->msg_data.exit.nTimeStamp, 0, 0);
#endif
            pNode->CloseNode();
            retVal = true;
        }
    }

    return retVal;
}

bool HandleReportThread(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.report_thread.pID;
    pthread_t       tID = pMsg->msg_data.report_thread.tID;

    // Handle the node exit message
    LOG(eWarning, "Reporting Thread pid %X tid %X\n", pID, tID);
    
    PerfTree* pTree = GetTree(pID, tID, "ReportThread Event");    

    if(pTree != NULL) {
        LOG(eWarning, "Printing tree report for Task ID %X\n", (uint32_t)tID);
        pTree->ReportData();
        retVal = true;
    }

    return retVal;
}

bool HandleReportProcess(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.report_process.pID;

    // Handle the node exit message
    LOG(eWarning, "Reporting Process pid %X\n", pID);
    
    PerfProcess* pProcess = RDKPerf_FindProcess(pID);
    if(pProcess != NULL) {
        pProcess->ShowTrees();
        // Close threads that have no activity since the last report
        pProcess->CloseInactiveThreads();
        LOG(eWarning, "Printing process report for process ID %X\n", (uint32_t)pID);
        pProcess->ReportData();
        retVal = true;
    }

    return retVal;
}

bool HandleCloseThread(PerfMessage* pMsg)
{
    bool            retVal = false;
    pid_t           pID = pMsg->msg_data.close_thread.pID;
    pthread_t       tID = pMsg->msg_data.close_thread.tID;

    // Handle the node exit message
    LOG(eWarning, "Closing Thread pid %X tid %X\n", pID, tID);
    
    PerfProcess* pProcess = RDKPerf_FindProcess(pID);
    if(pProcess != NULL) {
        retVal = pProcess->RemoveTree(tID);
    }

    return retVal;
}

bool HandleCloseProcess(PerfMessage* pMsg)
{
    bool            retVal = true;
    pid_t           pID = pMsg->msg_data.close_process.pID;

    // Handle the node exit message
    LOG(eWarning, "Closing Process pid %X\n", pID);
    
    RDKPerf_RemoveProcess(pID);

    return retVal;
}

bool HandleMessage(PerfMessage* pMsg)
{
    bool retVal = true;

    LOG(eTrace, "Got message of type %d\n", pMsg->type);
    switch(pMsg->type) {
    case eEntry:
        retVal = HandleEntry(pMsg);
        break;
    case eThreshold:
        retVal = HandleThreshold(pMsg);
        break;
    case eExit:
        retVal = HandleExit(pMsg);
        break;
    case eReportThread:
        retVal = HandleReportThread(pMsg);
        break;
    case eReportProcess:
        retVal = HandleReportProcess(pMsg);
        break;
    case eCloseThread:
        retVal = HandleCloseThread(pMsg);
        break;
    case eCloseProcess:
        retVal = HandleCloseProcess(pMsg);
        break;
    default:
        LOG(eError, "Unknown Mesage type %d\n", pMsg->type);
        retVal = false;
        break;
    }

    return retVal;
}

void RunLoop(PerfMsgQueue* pQueue)
{
    bool        bContinue       = true;
    uint32_t    nTimeoutCount   = 0;

    while(bContinue == true) {
        PerfMessage msg;
        // Get Message
        pQueue->ReceiveMessage(&msg, MESSAGE_TIMEOUT);
        //TEST
        if(msg.type == eExitQueue || msg.type == eMaxType) {
            LOG(eWarning, "Exit loop\n");
            bContinue = false;
        }
        else if(msg.type == eNoMessage) {
            // Timeout
            nTimeoutCount++;
            if(nTimeoutCount > MAX_TIMEOUT) {
                LOG(eError, "Max number of message timeouts reached exiting\n");
                bContinue = false;
            }
            else {
                LOG(eWarning, "No message, timeout %d\n", nTimeoutCount);
            }
        }
        else {
            // Reset Timeout Count
            nTimeoutCount = 0;
            // Handle message
            HandleMessage(&msg);
        }
    }

    LOG(eWarning, "RunLoop exiting\n");
    return;
}

static PerfMsgQueue* s_pQueue = NULL;

static void ctrlc_handler_callback(int, siginfo_t* ,void* contex)
{
    LOG(eWarning, "Ctrl+C pressed\n");
    if(s_pQueue != NULL) {
        s_pQueue->SendMessage(eExitQueue, NULL, 0, 0);
    }
}

int main(int argc, char *argv[])
{    
    LOG(eWarning, "Enter perfservice app %s\n", __DATE__);

    // Register Ctrl+C handler
    struct sigaction sa;
    sa.sa_sigaction = ctrlc_handler_callback;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    RDKPerf_InitializeMap();

    // // Does the queue exist
    if(PerfMsgQueue::IsQueueCreated(RDK_PERF_MSG_QUEUE_NAME)) {
        LOG(eError, "Queue exists, service is a duplicate\n");
        exit(-1);
    }

    // Create Queue
    s_pQueue = PerfMsgQueue::GetQueue(RDK_PERF_MSG_QUEUE_NAME, true);
    if(s_pQueue != NULL) {
        // Have Queue, start retrieven messages
        RunLoop(s_pQueue);

        // RunLoop exited, cleanup
        s_pQueue->Release();
    }

    RDKPerf_DeleteMap();
    LOG(eWarning, "Exit perfservice app %s\n", __DATE__);

    exit(1);
}

