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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "rdk_perf_node.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"
#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"
#include "rdk_perf_clock.h"

static std::map<pid_t, PerfProcess*>* sp_ProcessMap;

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
            LOG(eWarning, "Thread %s is inactive, removing from process %.*s Thread Map size %d\n", 
                           pTree->GetName(),
                           sizeof(m_ProcessName) <= PROCESS_NAMELEN ? (int)sizeof(m_ProcessName) : (int)PROCESS_NAMELEN, 
                           m_ProcessName,
                           m_mapThreads.size());
            // Remove inactive thread from tree
            delete it->second;
            it = m_mapThreads.erase(it);
            retVal = true;
            LOG(eWarning, "After removal map size %d\n", m_mapThreads.size());
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
    else {
        LOG(eWarning, "Could not find %X in thread map Possible matches are:\n", tID);
        auto it2 = m_mapThreads.begin();
        while(it2 != m_mapThreads.end()) {
            LOG(eWarning, "\tKey %X, ID %X\n", it2->first, it2->second->GetThreadID());
            it2++;
        }
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

    PerfClock::Now(&m_clock, PerfClock::Marker);
    
    return pTree;
}


void PerfProcess::ShowTrees()
{
    LOG(eWarning, "Displaying %d threads in thread map for process %s\n", m_mapThreads.size(), m_ProcessName);
    auto it = m_mapThreads.begin();
    while(it != m_mapThreads.end()) {
        LOG(eWarning, "Tree = %p\n", it->second);
        if(it->second != NULL) {
            ShowTree(it->second);
        }
        it++;
    }
}

void PerfProcess::ShowTree(PerfTree* pTree)
{
    // Describe Thread
    LOG(eWarning, "Found Thread %X in tree named %s with stack size %d\n", 
        pTree->GetThreadID(),
        pTree->GetName(),
        pTree->GetStack()->size());
    // Show Stack
    LOG(eWarning, "Top Node = %p name %s\n", pTree->GetStack()->top(), pTree->GetStack()->top()->GetName().c_str());

    return;
}

void PerfProcess::ReportData()
{
    // Print reports for all the trees in this process.
    LOG(eWarning, "Found %d threads in this process %X named <%.*s>\n", 
                  m_mapThreads.size(), (uint32_t)m_idProcess, 
                  sizeof(m_ProcessName) <= PROCESS_NAMELEN ? (int)sizeof(m_ProcessName) : (int)PROCESS_NAMELEN, 
                  m_ProcessName);
    auto it = m_mapThreads.begin();

    if(it != m_mapThreads.end()) {
        PerfClock::Now(&m_clock, PerfClock::Elapsed);

        const uint64_t msIntervalTime = m_clock.GetWallClock(PerfClock::millisecond);

        const float userCPU = (m_clock.GetUserCPU(PerfClock::millisecond) * 100.0f) / (float)msIntervalTime;
        const float systemCPU = (m_clock.GetSystemCPU(PerfClock::millisecond) * 100.0f) / (float)msIntervalTime;

        LOG(eWarning, "CPU user: %llu ms (%0.1f%%) CPU system: %llu ms (%0.1f%%)\n",
            m_clock.GetUserCPU(PerfClock::millisecond), userCPU,
            m_clock.GetSystemCPU(PerfClock::millisecond), systemCPU);

        PerfClock::Now(&m_clock, PerfClock::Marker);

        while(it != m_mapThreads.end()) {
            it->second->ReportData(msIntervalTime);
            it++;
        }
    } 
    
    return;
}


//--------------------- Process Map Tools ----------------------
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

void RDKPerf_RemoveProcess(pid_t pID)
{
    SCOPED_LOCK();

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
}

void RDKPerf_InitializeMap()
{
    if(sp_ProcessMap == NULL) {
        sp_ProcessMap = new std::map<pid_t, PerfProcess*>();
        sp_ProcessMap->clear();
    }
    else {
        LOG(eError, "Map already exists\n");
    }
}

void RDKPerf_DeleteMap()
{
    if(sp_ProcessMap != NULL) {
        delete sp_ProcessMap;
        sp_ProcessMap = NULL;
    }
    else {
        LOG(eError, "Can not delete map\n");
    }
}

size_t RDKPerf_GetMapSize()
{
    if(sp_ProcessMap != NULL) {
        return sp_ProcessMap->size();
    }
    else {
        return 0;
    }    
}

