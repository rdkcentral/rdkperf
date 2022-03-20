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
#include "rdk_perf_scopedlock.h"

PerfRecord::PerfRecord(std::string elementName)
: m_elementName(elementName), m_nodeInTree(NULL), m_ThresholdInUS(-1)
{
    pid_t           pID = getpid();
    PerfProcess*    pProcess = NULL;

    SCOPED_LOCK();

    // LOG(eWarning, "Creating node for element %s pid %X\n", m_elementName.c_str(), pID);
    
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
    SCOPED_LOCK();

    uint64_t deltaTime = TimeStamp() - m_startTime;
    m_nodeInTree->IncrementData(deltaTime);
    m_nodeInTree->CloseNode();
    if(m_ThresholdInUS > 0 && deltaTime > (uint64_t)m_ThresholdInUS) {
        TimingStats* stats = m_nodeInTree->GetStats();
        LOG(eWarning, "%s Threshold %ld exceeded, elapsed time = %0.3lf ms Avg time = %0.3lf (interval %0.3lf) ms\n", 
                        GetName().c_str(), 
                        m_ThresholdInUS / 1000,
                        ((double)deltaTime) / 1000.0,
                        ((double)stats->nTotalTime / (double)stats->nTotalCount) / 1000.0,
                        ((double)stats->nIntervalTime / (double)stats->nIntervalCount) / 1000.0);
        m_nodeInTree->ReportData(0, true);
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
