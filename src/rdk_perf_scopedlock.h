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

#ifndef __RDK_PERF_SCOPEDLOCK_H__
#define __RDK_PERF_SCOPEDLOCK_H__

#ifdef USE_LIBC_SCOPED_LOCK
#include <mutex>
#else
#include <pthread.h>
#endif

#ifdef USE_LIBC_SCOPED_LOCK
extern std::recursive_mutex _lock;
#define SCOPED_LOCK()     std::lock_guard<std::recursive_mutex> lock(_lock)
#else

namespace RDKPERF {

class ScopedMutex
{
public:
    ScopedMutex(const char* strFN);
    ~ScopedMutex();
private:
    void InitMutex(pthread_mutex_t* pLock);

    const char* _strFN;
    static bool _bMutexInit;

    static pthread_mutex_t _lock;
};
}
#define SCOPED_LOCK()     RDKPERF::ScopedMutex lock(__FUNCTION__)

#endif
#endif //__RDK_PERF_SCOPEDLOCK_H__