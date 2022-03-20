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
#include <errno.h>

static PerfMsgQueue* s_PerfQueue = NULL;

PerfMsgQueue::PerfMsgQueue(const char* szQueueName, bool bService)
: m_bService(bService)
, m_RefCount(0)
{
    int             flags = 0;
    mode_t          mode = S_IRWXU | S_IRWXG | S_IRWXO;
    struct mq_attr  new_attr = { 0 };

    snprintf(m_szName, MAX_NAME_LEN, "%s", szQueueName);

    if(m_bService) {
        flags = O_RDONLY | O_CREAT;
    }
    else {
        flags = O_WRONLY;
    }

    // m_queue = mq_open(szQueueName, flags);
    new_attr.mq_maxmsg = 10;
    new_attr.mq_msgsize = sizeof(PerfMessage);

    if(bService) {
        mq_unlink(szQueueName);
    }
    m_queue = mq_open(szQueueName, flags, mode, &new_attr);
    if(m_queue == (mqd_t)-1) {
        int errnum = errno;
        // failed to open queue
        LOG(eError, "mq_open failed for %s (0x%X)\n", szQueueName, flags);
        LOG(eError, "Error = %d, (%s)\n", errnum, strerror(errnum));
    }
    else {
        LOG(eWarning, "Message queue created %u\n", (unsigned int)m_queue);
        LOG(eWarning, "Setting queue attibutes\n");
         if(mq_setattr(m_queue, &new_attr, &m_queue_attr) == -1) {
           int errnum = errno;
           LOG(eWarning, "mq_setattr failed, Error = %d, (%s)\n", errnum, strerror(errnum));
        }
        else {        
            if (mq_getattr(m_queue, &m_queue_attr) != -1) {
                LOG(eWarning, "Maximum # of messages on queue:   %ld\n", m_queue_attr.mq_maxmsg);
                LOG(eWarning, "Maximum message size:             %ld\n", m_queue_attr.mq_msgsize);
                LOG(eWarning, "Current messages in queue:        %ld\n", m_queue_attr.mq_curmsgs);
                LOG(eWarning, "Message Queue Flags:              %lX\n", m_queue_attr.mq_flags);
            }
        }
    }
}

PerfMsgQueue::~PerfMsgQueue()
{
    mq_close(m_queue);
    if(m_bService) {
        mq_unlink(m_szName);
    }
}

bool PerfMsgQueue::SendMessage(MessageType type, const char* szName, uint64_t nTimeStamp, int32_t nThresholdInUS)
{
    bool retVal = true;
    PerfMessage msg;

    // Set message data to 0s
    memset((void*)&msg, 0, sizeof(PerfMessage));

    msg.type = type;
    switch(type) {
    case eEntry:
        msg.msg_data.entry.pID = getpid();
        msg.msg_data.entry.tID = pthread_self();
        msg.msg_data.entry.nTimeStamp = nTimeStamp;
        msg.msg_data.entry.nThresholdInUS = nThresholdInUS;
        pthread_getname_np(msg.msg_data.entry.tID, msg.msg_data.entry.szThreadName, MAX_NAME_LEN);
        memcpy((void*)msg.msg_data.entry.szName, (void*)szName, MIN((size_t)(MAX_NAME_LEN - 1), strlen(szName)));
        break;
    case eThreshold:
        msg.msg_data.entry.pID = getpid();
        msg.msg_data.entry.tID = pthread_self();
        msg.msg_data.entry.nThresholdInUS = nThresholdInUS;
        memcpy((void*)msg.msg_data.entry.szName, (void*)szName, MIN((size_t)(MAX_NAME_LEN - 1), strlen(szName)));
        break;
    case eExit:
        msg.msg_data.exit.pID = getpid();
        msg.msg_data.exit.tID = pthread_self();
        msg.msg_data.exit.nTimeStamp = nTimeStamp;
        memcpy((void*)msg.msg_data.entry.szName, (void*)szName, MIN((size_t)(MAX_NAME_LEN - 1), strlen(szName)));
        break;
    case eReportThread:
        msg.msg_data.report_thread.pID = getpid();
        msg.msg_data.report_thread.tID = pthread_self();
        break;
    case eReportProcess:
        msg.msg_data.report_process.pID = getpid();
        break;
    case eCloseThread:
        msg.msg_data.close_thread.pID = getpid();
        msg.msg_data.close_thread.tID = pthread_self();
        break;
    case eCloseProcess:
        msg.msg_data.close_process.pID = getpid();
        break;
    case eExitQueue:
        LOG(eWarning, "Sending Exit Queue message\n");
        break;
    default:
        LOG(eError, "Unknown Mesage type %d\n", type);
        retVal = false;
        break;
    }

    if(retVal == true) {
        retVal = SendMessage(&msg);
    }

    return retVal;
}

bool PerfMsgQueue::SendMessage(PerfMessage* pMsg)
{
    bool            retVal      = false;
    unsigned int    nPriority   = 5;

    int result = mq_send(m_queue, (const char*)pMsg, sizeof(PerfMessage), nPriority);
    if(result == 0) {
        // Success
        retVal = true;
    }
    else {
        LOG(eError, "Unable to send message of type %d\n", pMsg->type);
    }
    return retVal;
}

bool PerfMsgQueue::ReceiveMessage(PerfMessage* pMsg, int32_t nTimeoutInMS)
{
    bool        retVal      = false;
    size_t      nRead       = 0;
    uint32_t    priority    = 0;
    uint8_t     buffer[m_queue_attr.mq_msgsize] = { 0 };
    
    memset((void*)pMsg, 0, sizeof(PerfMessage));
    pMsg->type = eNoMessage;

    if(nTimeoutInMS == 0) {
        // wait forever
        nRead = mq_receive(m_queue, (char*)buffer, m_queue_attr.mq_msgsize, &priority);
        if(nRead < sizeof(PerfMessage) || nRead == (size_t)-1) {
            int errnum = errno;
            LOG(eError, "Did not read complete message.  queue = %d Size = %d, received = %d error = %d (%s)\n", 
                m_queue, sizeof(PerfMessage), nRead, errnum, strerror(errnum));
            pMsg->type = eMaxType;
        }
        else {
            LOG(eError, "Got message, queue = %d received = %d\n", m_queue, nRead);
            if(nRead == sizeof(PerfMessage)) {
                // Got expected message size
                memcpy(pMsg, buffer, nRead);
            }
            retVal = true;
        }
    }
    else {
        // use timeout
        struct   timespec tm;
        // clock_gettime(CLOCK_REALTIME, &tm);
        // tm.tv_sec += nTimeoutInMS / 1000;  // Convert to seconds
        // tm.tv_nsec += (nTimeoutInMS % 1000) * 1000 * 1000;  // Convert remainder to nano seconds

        tm.tv_sec = time(NULL) + nTimeoutInMS / 1000;  // Convert to seconds;
	    tm.tv_nsec = (nTimeoutInMS % 1000) * 1000 * 1000;  // Convert remainder to nano seconds
        nRead = mq_timedreceive(m_queue, (char*)buffer, m_queue_attr.mq_msgsize, &priority, &tm);
        if(nRead != sizeof(PerfMessage) || nRead == (size_t)-1) {
            int errnum = errno;
            if(errnum == ETIMEDOUT) {
                //LOG(eWarning, "No message, receive timed out\n");
                pMsg->type = eNoMessage;
            }
            else {
                LOG(eError, "Did not read complete message.  Size = %d, received = %d error = %d (%s)\n", 
                    sizeof(PerfMessage), nRead, errnum, strerror(errnum));
                pMsg->type = eMaxType;
            }
        }
        else {
            // Got expected buffer length
            memcpy(pMsg, buffer, nRead);
            retVal = true;
        }
    }

    if (mq_getattr(m_queue, &m_queue_attr) != -1) {
        if(m_queue_attr.mq_curmsgs > m_queue_attr.mq_maxmsg - 2) {
            LOG(eWarning, "Queue Depth at threshold %d max %d\n", m_queue_attr.mq_curmsgs, m_queue_attr.mq_maxmsg);
        }
    }    
    return retVal;
}

uint32_t PerfMsgQueue::AddRef()
{
    SCOPED_LOCK();
    m_RefCount++;

    return m_RefCount;
}
uint32_t PerfMsgQueue::Release()
{
    SCOPED_LOCK();
    uint32_t retVal = --m_RefCount;
    if(m_RefCount == 0) {
        LOG(eWarning, "RefCount at 0, deleting Queue\n");
        delete this;
    }
    return retVal;
}

PerfMsgQueue* PerfMsgQueue::GetQueue(const char* szQueueName, bool bService)
{
    SCOPED_LOCK();
    if(s_PerfQueue == NULL) {
        s_PerfQueue = new PerfMsgQueue(RDK_PERF_MSG_QUEUE_NAME, bService);
    }
    s_PerfQueue->AddRef();
    return s_PerfQueue;
}

bool PerfMsgQueue::IsQueueCreated(const char* szQueueName)
{
    bool retVal = false;

    SCOPED_LOCK();

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