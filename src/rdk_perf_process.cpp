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

#include "rdk_perf_node.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"
#include "rdk_perf_logging.h"

PerfProcess::PerfProcess(pid_t pID)
: m_idProcess(pID)
{
    LOG(eWarning, "Creating PerfProcess %p\n", this);
    memset(m_ProcessName, 0, PROCESS_NAMELEN);
    GetProcessName();
    LOG(eWarning, "Creating new process with named <%.*s>\n",
                  sizeof(m_ProcessName) <= PROCESS_NAMELEN ? (int)sizeof(m_ProcessName) : (int)PROCESS_NAMELEN, 
                  m_ProcessName);
    return;
}
PerfProcess::~PerfProcess()
{
    LOG(eWarning, "Deleting PerfProcess %p\n", this);
    auto it = m_mapThreads.begin();
    while(it != m_mapThreads.end()) {
        delete it->second;
        it++;
    }
    return;
}
bool PerfProcess::CloseInactiveThreads()
{
    bool retVal = false;

    auto it = m_mapThreads.begin();
    while(it != m_mapThreads.end()) {
        PerfTree* pTree = it->second;
        if(pTree->IsInactive()) {
            LOG(eWarning, "Thread %s is inactive, removing from process %.*s\n", 
                           pTree->GetName(),
                           sizeof(m_ProcessName) <= PROCESS_NAMELEN ? (int)sizeof(m_ProcessName) : (int)PROCESS_NAMELEN, 
                           m_ProcessName);
            // Remove inactive thread from tree
            delete it->second;
            it = m_mapThreads.erase(it);
            retVal = true;
        }
        else {
            it++;
        }
    }
    return retVal;
}

bool PerfProcess::RemoveTree(pthread_t tID)
{
    bool retVal = false;
 
    // Remove tree from process list.
    auto it = m_mapThreads.find(tID);
    if(it != m_mapThreads.end()) {
        // Remove from list
        delete it->second;
        it = m_mapThreads.erase(it);
        retVal = true;
    }    

    return retVal;
}

void PerfProcess::GetProcessName()
{
    char cmd[80] = { 0 };
    sprintf(cmd, "/proc/%d/cmdline", m_idProcess);
    FILE* fp = fopen(cmd,"r");
    if(fp != NULL) {
        size_t size = fread(m_ProcessName, sizeof(char), PROCESS_NAMELEN - 1, fp);
        if(size <= 0) {
            LOG(eError, "Could not read process name\n");
        }
        fclose(fp);
    }
    return;
}

PerfTree* PerfProcess::GetTree(pthread_t tID)
{
    PerfTree* pTree = NULL;

    auto it = m_mapThreads.find(tID);
    if(it != m_mapThreads.end()) {
        pTree = it->second;
    }

    return pTree;
}

PerfTree* PerfProcess::NewTree(pthread_t tID)
{
    PerfTree* pTree = NULL;

    auto it = m_mapThreads.find(tID);
    if(it == m_mapThreads.end()) {
        // Cound not find thread in list
        pTree = new PerfTree();
        m_mapThreads[tID] = pTree;
    }
    else {
        LOG(eError, "Trying to add a thread %d to a map where it already exists name = %s\n", 
                    tID, it->second->GetName());
    }

    return pTree;
}


void PerfProcess::ReportData()
{
    // Print reports for all the trees in this process.
    LOG(eWarning, "Found %d threads in this process %X named <%.*s>\n", 
                  m_mapThreads.size(), (uint32_t)m_idProcess, 
                  sizeof(m_ProcessName) <= PROCESS_NAMELEN ? (int)sizeof(m_ProcessName) : (int)PROCESS_NAMELEN, 
                  m_ProcessName);
    auto it = m_mapThreads.begin();
    while(it != m_mapThreads.end()) {
        it->second->ReportData();
        it++;
    }    
    
    return;
}
