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
#include "rdk_perf_logging.h"
#include <sys/wait.h>

//#define DO_UNIT_TESTS
//#define DO_THREAD_TESTS
#define DO_LATENCY_TESTS

// Unit Tests prototype
void unit_tests();
void unit_tests_c();

uint32_t Func3(uint32_t nCount)
{
    RDKPerf perf(__FUNCTION__);

    nCount = 1000000000;
    while(nCount >= 1) {
        nCount--;
        if(nCount == 1) {
            break;
        }
    }
    //usleep(100);
    nCount++;
    return nCount;
}

void Func2()
{
    RDKPerf perf(__FUNCTION__);
    for(int nIdx = 0; nIdx < 5; nIdx++) {
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
#define MAX_LOOP 1
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

// Declared in unit_tests.cpp
void do_work(uint32_t timeoutMS);

void test_latency_4()
{
    LOG(eTrace, "END\n");
    RDKLatency("test_sequence", __FUNCTION__);
}

void test_latency_3()
{
    LOG(eTrace, "\n");
    RDKLatency("test_sequence", __FUNCTION__);
    do_work(1); 
    test_latency_4();
}

void test_latency_2()
{
    LOG(eTrace, "\n");
    // fork the process and wait for child to complete
    RDKLatency("test_sequence", __FUNCTION__);

#if 1
    LOG(eTrace, "Forking child process\n");
    pid_t child_pid = fork();
    if(child_pid == 0) {
        RDKLatency("test_sequence", "child_start");

        LOG(eTrace, "Child process %d created\n", getpid());
        /* This is done by the child process. */
        do_work(1); 
        RDKLatency("test_sequence", "child_exit");
        LOG(eTrace, "Child process %d exiting\n", getpid());
        kill(getpid(), SIGKILL);        // SIGQUIT takes 180ms, SIGKILL takes 0.5ms
    }
    else {
        // Parent process
        int status;
        LOG(eTrace, "Parent process %d waiting for child %d\n", getpid(), child_pid);
        pid_t killed = waitpid(child_pid, &status, 0);
        if(killed == -1) {
            LOG(eError, "Failed to wait for child process\n");
        }
        LOG(eTrace, "Child process %d <-> %d completed\n", killed, child_pid);
    }

    test_latency_3();
#endif
}

void test_latency_1()
{
    LOG(eTrace, "\n");
    RDKLatency("test_sequence", __FUNCTION__);
    do_work(1);  
    test_latency_2();
}

void test_latency()
{
    LOG(eTrace, "\n");
    RDKLatency("test_sequence", __FUNCTION__);
    do_work(1); 
    test_latency_1();
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
#ifdef DO_UNIT_TESTS
    // Perform Unit tests
    unit_tests();
    //unit_tests_c();
#endif // DO_UINT_TESTS

#ifdef DO_THREAD_TESTS
    pthread_t threadId1;
    pthread_t threadId2;

    LOG(eWarning, "Creating Test threads\n");

    pthread_create(&threadId1, NULL, &task1, NULL);
    pthread_create(&threadId2, NULL, &task2, NULL);
 
    pthread_join(threadId1, NULL);
    pthread_join(threadId2, NULL);
#endif

#ifdef DO_INLINE_TESTS
    for(int idx = 0; idx < 10; idx++) {
        test_inline();
    }
#endif

#ifdef DO_LATENCY_TESTS
    for(int idx = 0; idx < 499; idx++) {
        // if(idx % 250 == 0) {
        //     LOG(eWarning, "Running latency test %d\n", idx);
        // }
        test_latency();
    }
    RDKLatencyReport("test_sequence");
#endif // DO_LATENCY_TESTS

    // Don't need to make this call as the process terminate handler will 
    // call the RDKPerf_ReportProcess() function
    // RDKPerf_ReportProcess(getpid());
}

