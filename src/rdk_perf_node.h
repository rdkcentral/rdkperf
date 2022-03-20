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


#ifndef __RDK_PERF_NODE_H__
#define __RDK_PERF_NODE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <stack>

#define INITIAL_MIN_VALUE 1000000000
#define MAX_BUF_SIZE 2048

typedef struct _TimingStats
{
    std::string         elementName;
    uint64_t            nTotalTime;
    double              nTotalAvg;
    uint64_t            nTotalMax;
    uint64_t            nTotalMin;
    uint64_t            nTotalCount;
    uint64_t            nIntervalTime;
    double              nIntervalAvg;
    uint64_t            nIntervalMax;
    uint64_t            nIntervalMin;
    uint64_t            nIntervalCount;
    uint64_t            nLastDelta;
} TimingStats;

// Forward decls
class PerfTree;
class PerfRecord;
class PerfNode
{
public:
    PerfNode(); // For root node
    PerfNode(PerfRecord* pRecord);
    PerfNode(char* szName, pthread_t tID, uint64_t nStartTime);
    ~PerfNode();
    
    PerfNode* AddChild(PerfRecord * pNode);
    PerfNode* AddChild(char* szName, pthread_t tID, uint64_t nStartTime);
    static uint64_t TimeStamp();

    std::string& GetName() { return m_elementName; };
    TimingStats* GetStats() { return &m_stats; };
    void SetTree(PerfTree* pTree) { m_Tree = pTree; };
    void SetThreshold(int32_t nThreshold) { m_ThresholdInUS = nThreshold; };

    void CloseNode();
    void IncrementData(uint64_t deltaTime);
    void ResetInterval();

    void ReportData(uint32_t nLevel, bool bShowOnlyDelta = false);

private:
    pthread_t               m_idThread;
    std::string             m_elementName;
    TimingStats             m_stats;
    uint64_t                m_startTime;
    PerfTree*               m_Tree;
    int32_t                 m_ThresholdInUS;
    std::map<std::string, PerfNode*>    m_childNodes;
};

#endif // __RDK_PERF_NODE_H__
