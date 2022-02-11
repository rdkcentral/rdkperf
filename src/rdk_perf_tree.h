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


#ifndef __RDK_PERF_TREE_H__
#define __RDK_PERF_TREE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <stack>


#define THREAD_NAMELEN 16

// Forward decls
class PerfNode;

class PerfTree
{
public:
    PerfTree();
    ~PerfTree();

    void AddNode(PerfNode * pNode);
    void CloseActiveNode(PerfNode * pTreeNode);
    void ReportData();

    bool IsInactive();
    char * GetName() { return m_ThreadName; };
    std::stack<PerfNode*> GetStack() { return m_activeNode; }
    pthread_t GetThreadID() { return m_idThread; };

private:
    pthread_t               m_idThread;
    PerfNode*               m_rootNode;
    char                    m_ThreadName[THREAD_NAMELEN];
    uint64_t                m_ActivityCount;
    uint64_t                m_CountAtLastReport;
    std::stack<PerfNode*>   m_activeNode;
};


#endif // __RDK_PERF_TREE_H__