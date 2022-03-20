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

#include "rdk_perf_node.h"
#include "rdk_perf_record.h"
#include "rdk_perf_msgqueue.h"
#include "rdk_perf_tree.h"
#include "rdk_perf_process.h"
#include "rdk_perf_logging.h"

PerfTree::PerfTree()
:m_idThread(0), m_rootNode(NULL), m_ActivityCount(0), m_CountAtLastReport(0)
{
    memset(m_ThreadName, 0, THREAD_NAMELEN);
    return;
}
PerfTree::~PerfTree()
{
    LOG(eWarning, "Deleting Tree %s\n", m_ThreadName);
    delete m_rootNode;

    return;
}

PerfNode* PerfTree::AddNode(PerfRecord* pRecord)
{
    PerfNode* pNode     = NULL;
    PerfNode* pTop      = NULL;

    //Get last opended node
    if(m_activeNode.size() > 0) {
        pTop = m_activeNode.top();
    }
    else {
        // New Tree
        m_rootNode = new PerfNode();    // root node special constructor
        m_activeNode.push(m_rootNode);
        m_idThread = pthread_self();
        pthread_getname_np(m_idThread, m_ThreadName, THREAD_NAMELEN);
        pTop = m_activeNode.top();
        LOG(eWarning, "Creating new Tree stack size = %d for node %s, thread name %s\n", 
            m_activeNode.size(), pRecord->GetName().c_str(), m_ThreadName);        
    }
    pNode = pTop->AddChild(pRecord);
    m_activeNode.push(pNode);
    m_ActivityCount++;

    pNode->SetTree(this);

    return pNode;
}

PerfNode* PerfTree::AddNode(char* szName, pthread_t tID, char* szThreadName, uint64_t nStartTime)
{
    PerfNode* pNode     = NULL;
    PerfNode* pTop      = NULL;

    //Get last opended node
    if(m_activeNode.size() > 0) {
        pTop = m_activeNode.top();
    }
    else {
        // New Tree
        m_rootNode = new PerfNode();    // root node special constructor
        m_activeNode.push(m_rootNode);
        m_idThread = tID;
        memcpy(m_ThreadName, szThreadName, strlen(szThreadName));
        pTop = m_activeNode.top();
        LOG(eWarning, "Creating new Tree stack size = %d for node %s, thread name %s\n", 
            m_activeNode.size(), szName, szThreadName);        
    }

    pNode = pTop->AddChild(szName, tID, nStartTime);
    m_activeNode.push(pNode);
    m_ActivityCount++;

    pNode->SetTree(this);

    return pNode;
}

bool PerfTree::IsInactive()
{
    bool retVal = false;

    if(m_ActivityCount == m_CountAtLastReport) {
        // No new open nodes since last report
        if(m_activeNode.top() == m_rootNode) {
            // AND there are no open nodes on the stack 
            retVal = true;
        }
    }

    return retVal;
}

void PerfTree::CloseActiveNode(PerfNode* pTreeNode)
{
    //Get last opended node
    PerfNode* pTop = m_activeNode.top();
    if(pTop != NULL) {
        // There is an active node
        if(pTreeNode != pTop) {
            // Error
            LOG(eError, "Not closeing the active node(%s != %s)\n", 
                pTop->GetName().c_str(), pTreeNode->GetName().c_str());
        }
        else {
            // LOG(eTrace, "Closeing the active node %s\n", 
            //             pTop->GetName().c_str());
            m_activeNode.pop();
        }
    }

    return;
}

void PerfTree::ReportData()
{
    // Get the root node and walk down the tree
    LOG(eWarning, "Printing report on %X thread named %s\n", (uint32_t)m_idThread, m_ThreadName);
    m_rootNode->ReportData(0);
    
    // Update the activity monitor
    m_CountAtLastReport = m_ActivityCount;

    return;
}
