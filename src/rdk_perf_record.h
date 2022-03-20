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


#ifndef __RDK_PERF_RECORD_H__
#define __RDK_PERF_RECORD_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <stack>

#define MAX_BUF_SIZE 2048


// Forward decls
class PerfTree;
class PerfNode;

class PerfRecord
{
public:
    PerfRecord(std::string elementName);
    ~PerfRecord();
    
    static uint64_t TimeStamp();

    std::string&    GetName()                       { return m_elementName; };
    pthread_t       GetThreadID()                   { return m_idThread; };
    uint64_t        GetStartTime()                  { return m_startTime; };
    void            SetThreshold(int32_t nUS)       { m_ThresholdInUS = (int32_t)nUS; };
    void            SetNodeInTree(PerfNode* pNode)  { m_nodeInTree = pNode; };
 
    void            ReportData(uint32_t nLevel, bool bShowOnlyDelta = false);

private:
    pthread_t               m_idThread;
    std::string             m_elementName;
    uint64_t                m_startTime;
    PerfNode*               m_nodeInTree;
    int32_t                 m_ThresholdInUS;
};

#endif // __RDK_PERF_RECORD_H__
