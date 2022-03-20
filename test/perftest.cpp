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


uint32_t Func3(uint32_t nCount)
{
    RDKPerf perf(__FUNCTION__);

    usleep(100);
    nCount++;
    return nCount;
}

void Func2()
{
    RDKPerf perf(__FUNCTION__);
    for(int nIdx = 0; nIdx < 2; nIdx++) {
        Func3(nIdx);
    }
    sleep(1);
}

void Func1()
{
    // RDKPerfRemote perfRemote(__FUNCTION__);
    RDKPerf pref(__FUNCTION__);
    sleep(2);
    Func2();
}

//#define MAX_LOOP 1024 * 1024 * 1
#define MAX_LOOP 1000
void* task1(void* pData)
{
    pthread_setname_np(pthread_self(), __FUNCTION__);
    RDKPerf perf(__FUNCTION__);
    Func1();

    sleep(4);

    RDKPerfHandle hPerf = RDKPerfStart("Func3_Wrapper");
    int nCount = 0;
    while(nCount < MAX_LOOP) {
        nCount = Func3(nCount);   
    }
    RDKPerfStop(hPerf);
    return NULL;
}

void* task2(void* pData)
{
    pthread_setname_np(pthread_self(), __FUNCTION__);
    RDKPerf perf(__FUNCTION__);
    Func1();
    Func2();

    RDKPerfHandle hPerf = RDKPerfStart("test_c");
    sleep(2);
    RDKPerfStop(hPerf);
    
    return NULL;
}

void test_inline()
{
    uint32_t sleep_interval = 5;

    FUNC_METRICS_START(100);

    while(sleep_interval != 0) {
        sleep_interval--;
    }

    FUNC_METRICS_END();

    return;
}

int main(int argc, char *argv[])
{    
    LOG(eWarning, "Enter test app %s\n", __DATE__);

    pid_t child_pid;

#ifdef PERF_REMOTE
    // child_pid = fork();
    // if(child_pid == 0) {
    //     /* This is done by the child process. */

    //     const char* command     = "./build/perfservice";
    //     const char* args[]      = { "./build/perfservice", NULL };
    //     const char* env[]       = { "LD_LIBRARY_PATH=./build", NULL };

    //     execvpe(command, args, env);

    //     /* If execv returns, it must have failed. */

    //     printf("Unknown command %s\n", command);
    //     exit(0);
    // }
    // sleep(1);
#endif
    pthread_t threadId1;
    pthread_t threadId2;

    LOG(eWarning, "Creating Test threads\n");

    pthread_create(&threadId1, NULL, &task1, NULL);
    pthread_create(&threadId2, NULL, &task2, NULL);
 
    pthread_join(threadId1, NULL);
    pthread_join(threadId2, NULL);

    for(int idx = 0; idx < 1000; idx++) {
        test_inline();
    }
    // Don't need to make this call as the process terminate handler will 
    // call the RDKPerf_ReportProcess() function
    // RDKPerf_ReportProcess(getpid());
}

