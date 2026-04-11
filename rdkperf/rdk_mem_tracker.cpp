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
#include <dlfcn.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string>
#include <map>

#include "rdk_mem_tracker.h"

RDKMemTracker::RDKMemTracker(LogFunction pLog) 
: _sequence(0)
, _lastFreed(0)
, _totalAllocatedSize(0)
, _totalAllocatedElements(0)
, _logFunc(NULL)
{
    if(pLog != NULL) {
        _logFunc = pLog;
    }
    return;
}

RDKMemTracker::~RDKMemTracker()
{
    Report();

    auto it = _map.begin();
    while(it != _map.end()) {
        delete it->second;
        it++;
    }
    _map.clear();

    return;
}

void RDKMemTracker::Allocation(void* memory, uint32_t size, const char* szFunction, const uint32_t nLine)
{
    _sequence++;
    AllocationData* pAlloc = (AllocationData*)calloc(1, sizeof(AllocationData));
    pAlloc->size        = size;
    pAlloc->szFunction  = szFunction;
    pAlloc->nLine       = nLine;
    pAlloc->sequence    = _sequence;

    _map[memory] = pAlloc;
    _totalAllocatedSize        += size;
    _totalAllocatedElements    += 1;

    return;
}

void RDKMemTracker::Free(void* memory)
{
    auto it = _map.find(memory);
    if(it != _map.end()) {
        AllocationData* pAlloc = it->second;
        if(_lastFreed != pAlloc->sequence - 1) {
            // Free out of sequence
            Log("Freeing memory %p of size %u out of sequence %u, last freed sequence %u\n",
                 memory, pAlloc->size, pAlloc->sequence, _lastFreed);
        }
        _lastFreed                  = pAlloc->sequence;
        _totalAllocatedSize        -= pAlloc->size;
        _totalAllocatedElements    -= 1;
        ::free(pAlloc);
        _map.erase(it);
    }
    else {
        // Error, allocation not in list
        Log("ERROR: Could not find memory item %p in the allocation list\n", memory);
    }

    return;
}

void RDKMemTracker::Log(std::string& buffer)
{
    if(_logFunc != NULL) {
        _logFunc(buffer.c_str());
    }
    else {
        fprintf(stdout, "%s", buffer.c_str());
    }
}

void RDKMemTracker::Log(const char * format, ...)
{
    char logMessage[LOG_MESSAGE_SIZE];

    // Generate the log string
    va_list ap;
    va_start(ap, format);
    vsnprintf(logMessage, LOG_MESSAGE_SIZE, format, ap);
    va_end(ap);

    std::string buffer(logMessage);
    Log(buffer);
}

void RDKMemTracker::Report(bool bForce)
{
    if(bForce == true || _map.size() > 0) {
        // How many allocations in the map
        Log("Total active allocations %u (%u)\n", _totalAllocatedElements, _map.size());

        auto it = _map.begin();
        while(it != _map.end()) {
            AllocationData* pAlloc = it->second;
            Log("Element sequence %u size %u, allocated here %s, %u\n",
                pAlloc->sequence, pAlloc->size, pAlloc->szFunction, pAlloc->nLine);
            it++;
        }
    }
}


// C interface
extern "C" {

RDKMemTrackerHandle CreateMemTrackerWithLogCallback(LogFunction pLog)
{
    return static_cast<RDKMemTrackerHandle>(new RDKMemTracker(pLog));
}

RDKMemTrackerHandle CreateMemTracker()
{
    return static_cast<RDKMemTrackerHandle>(new RDKMemTracker());
}

void TerminateMemTracker(RDKMemTrackerHandle hTracker)
{
    delete static_cast<RDKMemTracker*>(hTracker);
    return;
}

// Memory tracking
void TrackAllocation(RDKMemTrackerHandle hTracker, void* memory, uint32_t size, const char* szFunction, const uint32_t nLine)
{
    static_cast<RDKMemTracker*>(hTracker)->Allocation(memory, size, szFunction, nLine);
    return;
}

void TrackFree(RDKMemTrackerHandle hTracker, void* memory)
{
    static_cast<RDKMemTracker*>(hTracker)->Free(memory);
    return;
}

// Reporting
void MemTrackerReport(RDKMemTrackerHandle hTracker)
{
    static_cast<RDKMemTracker*>(hTracker)->Report();
    return;
}


}   // extern "C"
