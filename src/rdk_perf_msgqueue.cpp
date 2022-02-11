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

#include <string>
#include <map>

#include "rdk_perf_msgqueue.h"

#include <glib.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

static PerfMsgQueue* s_PerfQueue = NULL;

static const char* RDK_PERF_QUEUE_NAME = "rdk_perf_queue";

PerfMsgQueue::PerfMsgQueue(const char* szQueueName, int flags)
{

}

PerfMsgQueue::~PerfMsgQueue()
{

}

bool PerfMsgQueue::SendMessage(PerfMessage** pMsg)
{
    bool retVal = false;

    return retVal;
}

bool PerfMsgQueue::ReceiveMessage(PerfMessage** ppMsg)
{
    bool retVal = false;
    
    return retVal;
}

bool PerfMsgQueue::DeleteMessage(PerfMessage* pMsg)
{
    bool retVal = false;
    
    return retVal;
}

PerfMsgQueue* PerfMsgQueue::GetQueue(const char* szQueueName, bool bService)
{
    if(!IsQueueCreated(RDK_PERF_QUEUE_NAME)) {
        s_PerfQueue = new PerfMsgQueue(RDK_PERF_QUEUE_NAME, 0);
    }
    return s_PerfQueue;
}

bool PerfMsgQueue::IsQueueCreated(const char* szQueueName)
{
    bool retVal = false;

    mqd_t queue = mq_open(szQueueName, O_RDONLY);
    if(queue == (mqd_t)-1) {
        // failed to open queue
        LOG(eWarning, "mq_open failed for %s\n", szQueueName);
        retVal = false;
    }
    else {
        // Queue exists
        LOG(eWarning, "mq_open succeeded for %s\n", szQueueName);
        mq_close(queue);
        retVal = true;
    }

    return retVal;
}