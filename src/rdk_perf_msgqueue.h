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

#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>


#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

//#define RDK_PERF_MSG_QUEUE_NAME "/test"
#define RDK_PERF_MSG_QUEUE_NAME "/RDKPerfServerQueue"
#define MAX_NAME_LEN 128

typedef enum _MessageType 
{
    eNoMessage       = -1,
    eEntry           = 1,
    eExit            = 2,
    eThreshold       = 3,
    eReportThread    = 4,
    eReportProcess   = 5,
    eCloseThread     = 6,
    eCloseProcess    = 7,
    eExitQueue       = 9998,
    eMaxType         = 9999
} MessageType;

typedef struct _EntryMessage 
{
    pid_t               pID;
    pthread_t           tID;
    char                szName[MAX_NAME_LEN];
    char                szThreadName[MAX_NAME_LEN];
    uint64_t            nTimeStamp;
    int32_t             nThresholdInUS;
} EntryMessage;

typedef struct _ExitMessage 
{
    pid_t               pID;
    pthread_t           tID;
    char                szName[MAX_NAME_LEN];
    uint64_t            nTimeStamp;
} ExitMessage;

typedef struct _ThresholdMessage 
{
    pid_t               pID;
    pthread_t           tID;
    char                szName[MAX_NAME_LEN];
    int32_t             nThresholdInUS;
} ThresholdMessage;

typedef struct _ReportThread 
{
    pid_t               pID;
    pthread_t           tID;
} ReportThread;

typedef struct _ReportProcess 
{
    pid_t               pID;
} ReportProcess;

typedef struct _CloseThread 
{
    pid_t               pID;
    pthread_t           tID;
} CloseThread;

typedef struct _CloseProcess 
{
    pid_t               pID;
} CloseProcess;

typedef union _MessageData
{
    EntryMessage        entry;
    ExitMessage         exit;
    ThresholdMessage    threshold;
    ReportThread        report_thread;
    ReportProcess       report_process;
    CloseThread         close_thread;
    CloseProcess        close_process;
} MessageData;

typedef struct _PerfMessage 
{
    MessageType         type;
    MessageData         msg_data;
} PerfMessage; 

#define PERF_MSG_SIZE sizeof(PerfMessage)

class PerfMsgQueue
{
public:
    PerfMsgQueue(const char* szQueueName, bool bService);
    ~PerfMsgQueue();

    uint32_t AddRef();
    uint32_t Release();

    bool SendMessage(MessageType type, const char* szName = NULL, uint64_t nTimeStamp = 0, int32_t nThresholdInUS = -1);
    bool SendMessage(PerfMessage* pMsg);
    bool ReceiveMessage(PerfMessage* pMsg, int32_t nTimeoutInMS = 0);

    static PerfMsgQueue* GetQueue(const char* szQueueName, bool bService);
    static bool IsQueueCreated(const char* szQueueName);
private:
    mqd_t               m_queue;
    struct mq_attr      m_queue_attr;
    bool                m_bService;
    char                m_szName[MAX_NAME_LEN];
    uint32_t            m_RefCount;
};


#endif // __RDK_PERFMSGQUEUE_H__