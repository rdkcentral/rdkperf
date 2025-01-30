/**
* Copyright 2024 Comcast Cable Communications Management, LLC
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

#ifndef __RDK_PERF_LOCATION_H__
#define __RDK_PERF_LOCATION_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "rdk_perf_latency_data.h"

// Circular Buffer (CoPilot Implementation)
class CircularBuffer {
public:
    CircularBuffer();
    CircularBuffer(void* preallocatedMemory, size_t maxRecords);
    ~CircularBuffer();

    void initialize(void* preallocatedMemory, size_t maxRecords);
    void initialize_with_exiting_memory(void* preallocatedMemory, size_t maxRecords);

    bool push(uint32_t key, uint64_t value);
    bool pop(uint32_t& key, uint64_t& value);
    bool peek(uint32_t& key, uint64_t& value) const;
    bool find(uint32_t key, uint64_t& value) const;
    size_t size() const;
    bool empty() const;
    bool full() const;
    // Static method to get the size of a record
    static size_t recordSize();

private:
    CircBufferObject* circBuffer;
};

#if NEED_PREALLOCATED_MAP
class PreallocatedMap 
{
public:
    PreallocatedMap(void* preallocatedMemory, size_t maxEntries);
    ~PreallocatedMap();

    bool insert(uint32_t key, uint64_t value);
    bool erase(uint32_t key);
    bool find(uint32_t key, uint64_t& value);
    bool sort();
    size_t size() const;
    bool empty() const;

    struct Entry 
    {
        uint32_t key;
        uint64_t value;
        bool inUse;
    };

private:
    Entry* _entries;
    size_t _maxEntries;
    size_t _currentSize;
};
#endif // NEED_PREALLOCATED_MAP

class PerfLocation
{
public:
    PerfLocation(uint32_t locationOffset);
    PerfLocation(Location* location);
    ~PerfLocation();

    uint32_t        GetLocationOffset() { return _location_offset; };
    uint32_t        GetLocationParentOffset() { return _location->parent_offset; };
    uint32_t        GetLocationChildOffset() { return _location->child_offset; };
    void            SetLocationParentOffset(uint32_t parent_offset) { _location->parent_offset = parent_offset; };
    void            SetLocationChildOffset(uint32_t child_offset) { _location->child_offset = child_offset; };
    uint32_t        GetCount() { return _location->count; };

    uint64_t        GetTimeStamp(uint32_t count);
    uint64_t        GetRootTimeStamp(uint32_t count);
    bool            RemoveTimeStamp(uint32_t count);
    bool            AddTimeStamp(uint64_t timeStampUS);

    const char*     GetName();
    const char*     GetChildName();
    DataRecord*     GetDataRecord();

    // Static Methods
    static uint32_t    GetLocationDataSize() { return sizeof(Location); };
    static uint32_t    GetMaxLocations() { return MAX_LOCATIONS; };
    static void        SetLocationsArray(Location* locations);

private:
    Location*       _location;
    uint32_t        _location_offset;
    CircularBuffer  _timeStamps;
};

#endif // __RDK_PERF_LOCATION_H__