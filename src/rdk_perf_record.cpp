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
#include <unistd.h>

#ifdef NOT_YET

#include "rdk_perf_node.h"
#include "rdk_perf_record.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"
#include "rdk_perf_logging.h"
#include "rdk_perf.h"



PerfRecord::PerfRecord()
: m_elementName("root_node"), m_nodeInTree(NULL), m_Tree(NULL), m_TheshholdInUS(-1)
{
    m_startTime     = TimeStamp();
    m_idThread      = pthread_self();

    memset((void*)&m_stats, 0, sizeof(TimingStats));
    m_stats.nTotalMin     = INITIAL_MIN_VALUE;      // Preset Min values to pickup the inital value
    m_stats.nIntervalMin  = INITIAL_MIN_VALUE;
    m_stats.elementName   = m_elementName;

    LOG(eWarning, "Creating node for element %s\n", m_elementName.c_str());

    return;
}

PerfRecord::PerfRecord(PerfRecord* pRecord)
: m_nodeInTree(NULL), m_Tree(NULL), m_TheshholdInUS(-1)
{
    m_idThread      = pRecord->m_idThread;
    m_elementName   = pRecord->m_elementName;
    m_startTime     = pRecord->m_startTime;
    m_childRecords    = pRecord->m_childRecords;

    memset((void*)&m_stats, 0, sizeof(TimingStats));
    m_stats.nTotalMin     = INITIAL_MIN_VALUE;      // Preset Min values to pickup the inital value
    m_stats.nIntervalMin  = INITIAL_MIN_VALUE;
    m_stats.elementName   = m_elementName;

    return;
}

PerfRecord::PerfRecord(std::string elementName)
: m_elementName(elementName), m_nodeInTree(NULL), m_Tree(NULL), m_TheshholdInUS(-1)
{
    pid_t           pID = getpid();
    PerfProcess*    pProcess = NULL;

    LOG(eWarning, "Creating node for element %s pid %X\n", m_elementName.c_str(), pID);
    
    // Find thread in process map
    pProcess = RDKPerf_FindProcess(pID);
    if(pProcess == NULL) {
        // no existing PID in map
        pProcess = new PerfProcess(pID);
        RDKPerf_InsertProcess(pID, pProcess);
        LOG(eWarning, "Creating new process element %X for elemant %s\n", pProcess, m_elementName.c_str());
    }

    m_startTime = TimeStamp();
    m_idThread = pthread_self();

    // Found PID get element tree for current thread.
    PerfTree* pTree = pProcess->GetTree(m_idThread);
    if(pTree == NULL) {
        pTree = pProcess->NewTree(m_idThread);
    }
    
    if(pTree) {
        pTree->AddNode(this);
    }

    return;
}

PerfRecord::~PerfRecord()
{
    if(m_nodeInTree != NULL) {
        // This is a node in the code
        uint64_t deltaTime = TimeStamp() - m_startTime;
        m_nodeInTree->IncrementData(deltaTime);
        m_Tree->CloseActiveNode(m_nodeInTree);
        if(m_TheshholdInUS > 0 && deltaTime > (uint64_t)m_TheshholdInUS) {
            LOG(eWarning, "%s Threshold %ld exceeded, elapsed time = %0.3lf ms Avg time = %0.3lf (interval %0.3lf) ms\n", 
                          GetName().c_str(), 
                          m_TheshholdInUS / 1000,
                          ((double)deltaTime) / 1000.0,
                          ((double)m_nodeInTree->m_stats.nTotalTime / (double)m_nodeInTree->m_stats.nTotalCount) / 1000.0,
                          ((double)m_nodeInTree->m_stats.nIntervalTime / (double)m_nodeInTree->m_stats.nIntervalCount) / 1000.0);
            m_nodeInTree->ReportData(0, true);
        }
    }
    else {
        // This is a node in the tree and should be cleaned up
        auto it = m_childRecords.begin();
        while(it != m_childRecords.end()) {
            delete it->second;
            it++;
        }
    }

    return;
}
PerfRecord* PerfRecord::AddChild(PerfRecord * pRecord)
{
    // Does this node exist in the list of children
    auto it = m_childRecords.find(pRecord->GetName());
    if(it == m_childRecords.end()) {
        // new child
        // Create copy of Record for storage in the tree
        PerfNode* copyNode = new PerfNode(pNode);
        m_childNodes[pNode->GetName()] = copyNode;
        pNode->m_nodeInTree = copyNode;
    }
    else {
        pNode->m_nodeInTree = it->second;
    }

    return pNode->m_nodeInTree;
}

void PerfRecord::IncrementData(uint64_t deltaTime)
{
    // Increment totals
    m_stats.nLastDelta = deltaTime;
    m_stats.nTotalTime += deltaTime;
    m_stats.nTotalCount++;
    if(m_stats.nTotalMin > deltaTime) {
        m_stats.nTotalMin = deltaTime;
    }
    if(m_stats.nTotalMax < deltaTime) {
        m_stats.nTotalMax = deltaTime;
    }
    m_stats.nTotalAvg = (double)m_stats.nTotalTime / (double)m_stats.nTotalCount;

    // Increment intervals
    m_stats.nIntervalTime += deltaTime;
    m_stats.nIntervalCount++;
    if(m_stats.nIntervalMin > deltaTime) {
        m_stats.nIntervalMin = deltaTime;
    }
    if(m_stats.nIntervalMax < deltaTime) {
        m_stats.nIntervalMax = deltaTime;
    }
    m_stats.nIntervalAvg = (double)m_stats.nIntervalTime / (double)m_stats.nIntervalCount;

    return;
}

void PerfRecord::ResetInterval()
{
    m_stats.nIntervalTime     = 0;
    m_stats.nIntervalAvg      = 0;
    m_stats.nIntervalMax      = 0;
    m_stats.nIntervalMin      = INITIAL_MIN_VALUE;      // Need a large number here to get new min value
    m_stats.nIntervalCount    = 0;

    return;
}

void PerfRecord::ReportData(uint32_t nLevel, bool bShowOnlyDelta)
{
    char buffer[MAX_BUF_SIZE] = { 0 };
    char* ptr = &buffer[0];
    // Print the indent 
    for(uint32_t nIdx = 0; nIdx < nLevel; nIdx++) {
        snprintf(ptr, MAX_BUF_SIZE, "--");
        ptr += 2;
    }

    if(bShowOnlyDelta) {
        // Print only the current delta time data 
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s elapsed time %0.3lf\n",
                m_stats.elementName.c_str(),
                (double)m_stats.nLastDelta / 1000.0);
    }
    else {
        // Print data for this node
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s (Count, Max, Min, Avg) Total %lu, %0.3lf, %0.3lf, %0.3lf Interval %lu, %0.3lf, %0.3lf, %0.3lf",
                m_stats.elementName.c_str(),
                m_stats.nTotalCount, ((double)m_stats.nTotalMax) / 1000.0, ((double)m_stats.nTotalMin) / 1000.0, m_stats.nTotalAvg / 1000.0,
                m_stats.nIntervalCount, ((double)m_stats.nIntervalMax) / 1000.0, ((double)m_stats.nIntervalMin) / 1000.0, m_stats.nIntervalAvg / 1000.0);
    }
    LOG(eWarning, "%s\n", buffer);
    
    // Print data for all the children
    auto it = m_childRecords.begin();
    while(it != m_childRecords.end()) {
        it->second->ReportData(nLevel + 1, bShowOnlyDelta);
        it++;
    }

    if(!bShowOnlyDelta) {
        ResetInterval();
    }

    return;
}

uint64_t PerfRecord::TimeStamp() 
{
    struct timeval  timeStamp;
    uint64_t        retVal = 0;

    gettimeofday(&timeStamp, NULL);

    // Convert timestamp to Micro Seconds
    retVal = (uint64_t)(((uint64_t)timeStamp.tv_sec * 1000000) + timeStamp.tv_usec);

    return retVal;
}

#endif // NOT_YET