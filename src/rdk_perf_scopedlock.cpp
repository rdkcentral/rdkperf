/* Copyright (C) 2021 RDK Management.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS. OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
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


