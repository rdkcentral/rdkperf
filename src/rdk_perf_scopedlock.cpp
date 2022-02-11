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
#ifdef USE_LIBC_SCOPED_LOCK
#include <mutex>
#else
#include <pthread.h>
#endif

#include "rdk_perf_logging.h"
#include "rdk_perf_scopedlock.h"

#ifdef USE_LIBC_SCOPED_LOCK

std::recursive_mutex _lock;

#else // USE_LIBC_SCOPED_LOCK
ScopedMutex::ScopedMutex(const char* strFN) 
: _strFN(strFN)
{
    if(!_bMutexInit) {
        InitMutex(&_lock);
    }
    
    if(_bMutexInit) {
        pthread_mutex_lock(&_lock);
    }
    else {
        LOG(eError, "Mutex was not initialized for %s\n", _strFN);
    }
}
ScopedMutex::~ScopedMutex() 
{
    pthread_mutex_unlock(&_lock);
}
void ScopedMutex::InitMutex(pthread_mutex_t* pLock)
{
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(pLock, &Attr) != 0) {
        LOG(eError, "\n mutex init failed for %s\n", _strFN);
    }
    else {
        _bMutexInit = true;
    }
    return;
}

pthread_mutex_t ScopedMutex::_lock;
bool ScopedMutex::_bMutexInit = false;
#endif //USE_LIBC_SCOPED_LOCK


