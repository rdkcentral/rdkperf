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

void Func3()
{
    RDKPerf perf(__FUNCTION__);
    sleep(1);
}

void Func2()
{
    RDKPerf perf(__FUNCTION__);
    for(int nIdx = 0; nIdx < 2; nIdx++) {
        Func3();
    }
    sleep(1);
}

void Func1()
{
    RDKPerf pref(__FUNCTION__);
    sleep(2);
    Func2();
}

void* task1(void* pData)
{
    pthread_setname_np(pthread_self(), __FUNCTION__);
    RDKPerf perf(__FUNCTION__);
    Func1();

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

int main(int argc, char *argv[])
{    
    LOG(eWarning, "Enter test app %s\n", __DATE__);

    pthread_t threadId1;
    pthread_t threadId2;

    pthread_create(&threadId1, NULL, &task1, NULL);
    pthread_create(&threadId2, NULL, &task2, NULL);
 
    pthread_join(threadId1, NULL);
    pthread_join(threadId2, NULL);

   RDKPerf_ReportProcess(getpid());
}

