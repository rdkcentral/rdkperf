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

#ifndef __RDK_PERFMSGQUEUE_H__
#define __RDK_PERFMSGQUEUE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <string>
#include <map>

#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"

#include <glib.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

typedef struct _PerMessage {
    int type;
} PerfMessage; 

class PerfMsgQueue
{
public:
    PerfMsgQueue(const char* szQueueName, int flags);
    ~PerfMsgQueue();

    bool SendMessage(PerfMessage** pMsg);
    bool ReceiveMessage(PerfMessage** ppMsg);
    bool DeleteMessage(PerfMessage* pMsg);

    static PerfMsgQueue* GetQueue(const char* szQueueName, bool bService);
    static bool IsQueueCreated(const char* szQueueName);
private:
    mqd_t   m_queue;
};


#endif // __RDK_PERFMSGQUEUE_H__