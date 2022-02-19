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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "rdk_perf.h"
#include "rdk_perf_logging.h"
#include "rdk_perf_msgqueue.h"

bool HandleMessage(PerfMessage* pMsg)
{
    bool retVal = true;

    LOG(eWarning, "Got message of type %d\n", pMsg->type);

    return retVal;
}
void RunLoop(PerfMsgQueue* pQueue)
{
    bool bContiue = true;

    while(bContiue == true) {
        PerfMessage msg;
        // Get Message
        pQueue->ReceiveMessage(&msg, 5000);
        if(msg.type == eNoMessage || msg.type == eMaxType) {
            // Error case
            //TEST
            if(msg.type == eExitQueue || msg.type == eMaxType) {
                // Exit loop
                bContiue = false;
            }
        }
        else {
            // Handle message
            HandleMessage(&msg);
        }
    }

    LOG(eWarning, "RunLoop exiting\n");
    return;
}
int main(int argc, char *argv[])
{    
    LOG(eWarning, "Enter perfservice app %s\n", __DATE__);

    // // Does the queue exist
    // if(PerfMsgQueue::IsQueueCreated(RDK_PERF_MSG_QUEUE_NAME)) {
    //     // Queue exists, service is a duplicate
    //     exit(-1);
    // }

    // Create Queue
    PerfMsgQueue* pQueue = PerfMsgQueue::GetQueue(RDK_PERF_MSG_QUEUE_NAME, true);
    if(pQueue != NULL) {
        // Have Queue, start retrieven messages
        RunLoop(pQueue);

        // RunLoop exited, cleanup
        pQueue->Release();
    }

    LOG(eWarning, "Exit perfservice app %s\n", __DATE__);

    exit(1);
}

