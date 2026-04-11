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

#ifndef __RDK_MEM_TRACKER_H__
#define __RDK_MEM_TRACKER_H__

#ifdef __cplusplus
extern "C" {
#endif
typedef void (*LogFunction)(const char * ptr);
#ifdef __cplusplus
}   // extern "C"
#endif

#ifdef __cplusplus

#include <string>
#include <list>
#include <map>
#include <stack>

#define LOG_MESSAGE_SIZE 4096

class RDKMemTracker
{
public:

    RDKMemTracker(LogFunction pLog = NULL);
    ~RDKMemTracker();

    void Allocation(void* memory, uint32_t size, const char* szFunction, const uint32_t nLine);
    void Free(void* memory);

    void Report(bool bForce = false);

    void Log(std::string& buffer);
    void Log(const char * format, ...);

private:
    typedef struct AllocationData_ 
    {
        uint32_t        size;
        const char*     szFunction;
        uint32_t        nLine;
        uint32_t        sequence;
    } AllocationData;

    std::map<void*, AllocationData*>    _map;
    uint32_t                            _sequence;
    uint32_t                            _lastFreed;
    uint32_t                            _totalAllocatedSize;
    uint32_t                            _totalAllocatedElements;

    LogFunction                         _logFunc;

};

#endif // #ifdef __cplusplus

// C interface
#ifdef __cplusplus
extern "C" {
#endif

typedef void* RDKMemTrackerHandle;

// Lifecycle
RDKMemTrackerHandle CreateMemTracker();
RDKMemTrackerHandle CreateMemTrackerWithLogCallback(LogFunction pLog);
void TerminateMemTracker(RDKMemTrackerHandle hTracker);

// Memory tracking
void TrackAllocation(RDKMemTrackerHandle hTracker, void* memory, uint32_t size, const char* szFunction, const uint32_t nLine);
void TrackFree(RDKMemTrackerHandle hTracker, void* memory);

// Reporting
void MemTrackerReport(RDKMemTrackerHandle hTracker);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif // __RDK_MEM_TRACKER_H__
