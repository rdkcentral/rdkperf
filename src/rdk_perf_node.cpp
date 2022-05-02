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

#include "rdk_perf_node.h"
#include "rdk_perf_record.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"
#include "rdk_perf_logging.h"

PerfNode::PerfNode()
: m_elementName("root_node"), m_Tree(NULL), m_ThresholdInUS(-1)
{
    m_startTime     = TimeStamp();
    m_idThread      = pthread_self();

    memset((void*)&m_stats, 0, sizeof(TimingStats));
    m_stats.nTotalMin      = INITIAL_MIN_VALUE;      // Preset Min values to pickup the inital value
    m_stats.nIntervalMin   = INITIAL_MIN_VALUE;
    m_stats.elementName    = m_elementName;
    m_stats.nTotalCount    = 1;
    m_stats.nIntervalCount = 1;

    // LOG(eWarning, "Creating node for element %s\n", m_elementName.c_str());

    return;
}

PerfNode::PerfNode(PerfRecord* pRecord)
: m_Tree(NULL), m_ThresholdInUS(-1)
{
    m_idThread      = pRecord->GetThreadID();
    m_elementName   = pRecord->GetName();
    m_startTime     = pRecord->GetStartTime();

    m_childNodes.clear();

    memset((void*)&m_stats, 0, sizeof(TimingStats));
    m_stats.nTotalMin     = INITIAL_MIN_VALUE;      // Preset Min values to pickup the inital value
    m_stats.nIntervalMin  = INITIAL_MIN_VALUE;
    m_stats.elementName   = m_elementName;

    return;
}

PerfNode::PerfNode(char* szName, pthread_t tID, uint64_t nStartTime)
: m_Tree(NULL), m_ThresholdInUS(-1)
{
    m_idThread      = tID;
    m_elementName   = std::string(szName);
    m_startTime     = nStartTime;

    m_childNodes.clear();

    memset((void*)&m_stats, 0, sizeof(TimingStats));
    m_stats.nTotalMin     = INITIAL_MIN_VALUE;      // Preset Min values to pickup the inital value
    m_stats.nIntervalMin  = INITIAL_MIN_VALUE;
    m_stats.elementName   = m_elementName;

    return;
}

PerfNode::~PerfNode()
{
    // LOG(eWarning, "Deleting Node %s\n", GetName().c_str());

    // This is a node in the tree and should be cleaned up
    auto it = m_childNodes.begin();
    while(it != m_childNodes.end()) {
        delete it->second;
        it++;
    }

    return;
}
PerfNode* PerfNode::AddChild(PerfRecord * pRecord)
{
    PerfNode* pNode = NULL;
    // Does this node exist in the list of children
    auto it = m_childNodes.find(pRecord->GetName());
    if(it == m_childNodes.end()) {
        // new child
        // Create copy of Node for storage in the tree
        pNode = new PerfNode(pRecord);
        m_childNodes[pRecord->GetName()] = pNode;
    }
    else {
        pNode = it->second;
    }

    pRecord->SetNodeInTree(pNode);
    return pNode;
}

PerfNode* PerfNode::AddChild(char* szName, pthread_t tID, uint64_t nStartTime)
{
    PerfNode* pNode = NULL;
    // Does this node exist in the list of children
    auto it = m_childNodes.find(std::string(szName));
    if(it == m_childNodes.end()) {
        // new child
        // Create copy of Node for storage in the tree
        pNode = new PerfNode(szName, tID, nStartTime);
        m_childNodes[szName] = pNode;
    }
    else {
        pNode = it->second;
    }

    return pNode;
}

void PerfNode::CloseNode()
{
    if(m_Tree != NULL) {
        m_Tree->CloseActiveNode(this);
    }
}

void PerfNode::IncrementData(uint64_t deltaTime, uint64_t userCPU, uint64_t systemCPU)
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

    m_stats.nUserCPU = userCPU;
    m_stats.nSystemCPU = systemCPU;
    m_stats.nTotalUserCPU += userCPU;
    m_stats.nTotalSystemCPU += systemCPU;

    return;
}

void PerfNode::ResetInterval()
{
    m_stats.nIntervalTime     = 0;
    m_stats.nIntervalAvg      = 0;
    m_stats.nIntervalMax      = 0;
    m_stats.nIntervalMin      = INITIAL_MIN_VALUE;      // Need a large number here to get new min value
    m_stats.nIntervalCount    = 0;

    return;
}

void PerfNode::ReportData(uint32_t nLevel, bool bShowOnlyDelta)
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
#ifdef PERF_SHOW_CPU
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s elapsed time %0.3lf CPU User %0.3lf, System %0.3lf\n",
                m_stats.elementName.c_str(),
                (double)m_stats.nLastDelta / 1000.0,
                (double)m_stats.nUserCPU / 1000.0, (double)m_stats.nSystemCPU / 1000.0);
#else
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s elapsed time %0.3lf\n",
                m_stats.elementName.c_str(),
                (double)m_stats.nLastDelta / 1000.0);
#endif
    }
    else {
        // Print data for this node
#ifdef PERF_SHOW_CPU
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s CPU User %0.3lf, System %0.3lf (Count, Max, Min, Avg) Total %llu, %0.3lf, %0.3lf, %0.3lf Interval %llu, %0.3lf, %0.3lf, %0.3lf",
                m_stats.elementName.c_str(),
                (double)m_stats.nTotalUserCPU / 1000.0 / m_stats.nTotalCount, (double)m_stats.nTotalSystemCPU / 1000.0 / m_stats.nTotalCount,
                m_stats.nTotalCount, ((double)m_stats.nTotalMax) / 1000.0, ((double)m_stats.nTotalMin) / 1000.0, m_stats.nTotalAvg / 1000.0,
                m_stats.nIntervalCount, ((double)m_stats.nIntervalMax) / 1000.0, ((double)m_stats.nIntervalMin) / 1000.0, m_stats.nIntervalAvg / 1000.0);
#else
        snprintf(ptr, MAX_BUF_SIZE - strlen(buffer), "| %s (Count, Max, Min, Avg) Total %llu, %0.3lf, %0.3lf, %0.3lf Interval %llu, %0.3lf, %0.3lf, %0.3lf",
                m_stats.elementName.c_str(),
                m_stats.nTotalCount, ((double)m_stats.nTotalMax) / 1000.0, ((double)m_stats.nTotalMin) / 1000.0, m_stats.nTotalAvg / 1000.0,
                m_stats.nIntervalCount, ((double)m_stats.nIntervalMax) / 1000.0, ((double)m_stats.nIntervalMin) / 1000.0, m_stats.nIntervalAvg / 1000.0);
#endif
    }
    LOG(eWarning, "%s\n", buffer);
    
    // Print data for all the children
    auto it = m_childNodes.begin();
    while(it != m_childNodes.end()) {
        it->second->ReportData(nLevel + 1, bShowOnlyDelta);
        it++;
    }

    if(!bShowOnlyDelta) {
        ResetInterval();
    }

    return;
}

uint64_t PerfNode::TimeStamp() 
{
    struct timeval  timeStamp;
    uint64_t        retVal = 0;

    gettimeofday(&timeStamp, NULL);

    // Convert timestamp to Micro Seconds
    retVal = (uint64_t)(((uint64_t)timeStamp.tv_sec * 1000000) + timeStamp.tv_usec);

    return retVal;
}
