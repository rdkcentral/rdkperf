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

#include "rdk_perf_location.h"
#include "rdk_perf_logging.h"
#include <algorithm>
#include <cstring>

#ifdef USE_RDK_PERF
#include "rdk_perf.h"
#define PERF_FUNC(a) RDKPerf perf(a);
#else
#define PERF_FUNC(a)
#endif // USE_RDK_PERF

void* _locations = nullptr;


// Circular Buffer (CoPilot Implementation)
CircularBuffer::CircularBuffer()
: circBuffer(nullptr)
{
}

CircularBuffer::CircularBuffer(void* preallocatedMemory, size_t maxRecords)

{
    circBuffer = static_cast<CircBufferObject*>(preallocatedMemory);
    circBuffer->maxRecords = maxRecords;
    circBuffer->head = 0;
    circBuffer->tail = 0;
    circBuffer->currentSize = 0;
    std::memset(circBuffer->records, 0, circBuffer->maxRecords * sizeof(CircBufferRecord));}

CircularBuffer::~CircularBuffer() {
    // No need to explicitly destruct records 
    // as they are part of the location array
}

void CircularBuffer::initialize(void* preallocatedMemory, size_t maxRecords) 
{
    circBuffer = static_cast<CircBufferObject*>(preallocatedMemory);
    circBuffer->maxRecords = maxRecords;
    circBuffer->head = 0;
    circBuffer->tail = 0;
    circBuffer->currentSize = 0;
    std::memset(circBuffer->records, 0, circBuffer->maxRecords * sizeof(CircBufferRecord));
}

void CircularBuffer::initialize_with_exiting_memory(void* preallocatedMemory, size_t maxRecords) 
{
    circBuffer = static_cast<CircBufferObject*>(preallocatedMemory);
}

bool CircularBuffer::push(uint32_t key, uint64_t value) 
{
    if(circBuffer->records == nullptr) {
        LOG(eError, "Records array not set\n");
        return false;
    }

    if (full()) {
        // Remove the oldest record
        circBuffer->tail = (circBuffer->tail + 1) % circBuffer->maxRecords;
        --circBuffer->currentSize;
    }
    circBuffer->records[circBuffer->head].key = key;
    circBuffer->records[circBuffer->head].value = value;
    circBuffer->head = (circBuffer->head + 1) % circBuffer->maxRecords;
    ++circBuffer->currentSize;
    return true;
}

bool CircularBuffer::pop(uint32_t& key, uint64_t& value) 
{
    if(circBuffer->records == nullptr) {
        LOG(eError, "Records array not set\n");
        return false;
    }

    if (empty()) {
        return false; // Buffer is empty
    }
    key = circBuffer->records[circBuffer->tail].key;
    value = circBuffer->records[circBuffer->tail].value;
    circBuffer->tail = (circBuffer->tail + 1) % circBuffer->maxRecords;
    --circBuffer->currentSize;
    return true;
}

bool CircularBuffer::peek(uint32_t& key, uint64_t& value) const 
{
    if(circBuffer->records == nullptr) {
        LOG(eError, "Records array not set\n");
        return false;
    }
    
    if (empty()) {
        return false; // Buffer is empty
    }
    key = circBuffer->records[circBuffer->tail].key;
    value = circBuffer->records[circBuffer->tail].value;
    return true;
}

bool CircularBuffer::find(uint32_t key, uint64_t& value) const 
{
    if(circBuffer->records == nullptr) {
        LOG(eError, "Records array not set\n");
        return false;
    }

    if (empty()) {
        return false; // Buffer is empty
    }
    size_t index = circBuffer->tail;
    for (size_t i = 0; i < circBuffer->currentSize; ++i) {
        if (circBuffer->records[index].key == key) {
            value = circBuffer->records[index].value;
            return true;
        }
        index = (index + 1) % circBuffer->maxRecords;
    }
    return false; // Key not found
}

size_t CircularBuffer::size() const 
{
    return circBuffer->currentSize;
}

bool CircularBuffer::empty() const 
{
    return circBuffer->currentSize == 0;
}

bool CircularBuffer::full() const 
{
    return circBuffer->currentSize == circBuffer->maxRecords;
}

size_t CircularBuffer::recordSize() 
{
    return sizeof(CircBufferRecord);
}

#ifdef NEED_PREALLOCATED_MAP
// PreAllocated Map
PreallocatedMap::PreallocatedMap(void* preallocatedMemory, size_t maxEntries)
: _entries(static_cast<Entry*>(preallocatedMemory))
, _maxEntries(maxEntries)
, _currentSize(0)
{
    // Do not clear the memory as it is expected to be pre-allocated 
    // and might contain existing data

    // Calculate the current size
    for (size_t i = 0; i < _maxEntries; ++i) {
        if (_entries[i].inUse) {
            _currentSize++;
        }
    }
}

PreallocatedMap::~PreallocatedMap() 
{
    // No need to explicitly destruct entries as they are part of the location array
}

bool PreallocatedMap::insert(uint32_t key, uint64_t value) 
{
    for (size_t i = 0; i < _maxEntries; ++i) {
        if (!_entries[i].inUse) {
            _entries[i].key = key;
            _entries[i].value = value;
            _entries[i].inUse = true;
            _currentSize++;
            return true;
        }
    }
    return false; // No available slot
}

bool PreallocatedMap::erase(uint32_t key) 
{
    for (size_t i = 0; i < _maxEntries; ++i) {
        if (_entries[i].inUse && _entries[i].key == key) {
            _entries[i].inUse = false;
            _currentSize--;
            return true;
        }
    }
    return false; // Key not found
}

bool PreallocatedMap::find(uint32_t key, uint64_t& value) 
{
    for (size_t i = 0; i < _maxEntries; ++i) {
        if (_entries[i].inUse && _entries[i].key == key) {
            value = _entries[i].value;
            return true;
        }
    }
    value = INVALID_TIMESTAMP;
    return false; // Key not found
}

bool PreallocatedMap::sort() 
{
    std::sort(_entries, _entries + _maxEntries, [](const Entry& a, const Entry& b) {
        if (!a.inUse) return false;
        if (!b.inUse) return true;
        return a.key < b.key;
    });
    return true;
}

size_t PreallocatedMap::size() const 
{
    return _currentSize;
}

bool PreallocatedMap::empty() const 
{
    return _currentSize == 0;
}
#endif // NEED_PREALLOCATED_MAP

// Static method to set the locations array
void PerfLocation::SetLocationsArray(Location* locations)
{
    _locations = locations;
}

PerfLocation::PerfLocation(Location * location)
: _location(location)
, _location_offset(0)
, _timeStamps(&location->timeStampCirBuffer[0], MAX_TIME_STAMPS)
{
    // Set Location offset
    if(_locations != nullptr) {
        _location_offset = (uint32_t)(((uint8_t*)location - (uint8_t*)_locations)/sizeof(Location));
    }
    else {
        _location_offset = INVALID_OFFSET;
        LOG(eError, "Locations array not set\n");
    }
}

PerfLocation::PerfLocation(uint32_t locationOffset)
: _location(nullptr)
, _location_offset(locationOffset)
{
    if(_locations != nullptr) {
        _location = ((Location*)_locations) + locationOffset;
        _timeStamps.initialize_with_exiting_memory(&_location->timeStampCirBuffer[0], MAX_TIME_STAMPS);
    }
    else {
        LOG(eError, "Locations array not set\n");
    }
}

PerfLocation::~PerfLocation()
{
}

const char* PerfLocation::GetName()
{
    return _location->name;
}

const char* PerfLocation::GetChildName()
{
    const char* retVal = "\n";

    if(_location->child_offset == 0) {  
        return retVal;
    }

    if(_locations == nullptr) {
        return retVal;
    }

    if(_location->child_offset != 0) {
        Location* child = (Location*)((uint8_t*)_locations + _location->child_offset);
        retVal = child->name;
    }
    return retVal;
}

DataRecord* PerfLocation::GetDataRecord()
{
    DataRecord* retVal = new DataRecord();
    std::memset(retVal, 0, sizeof(DataRecord));

    // Copy the name
    std::strncpy(retVal->name, _location->name, MAX_NAME_LEN);
    // Copy the total time
    retVal->average = (double)(_location->total / (double)(_location->count));
    // Copy the total elapsed time
    retVal->average_elapsed = (double)(_location->total_elapsed / (double)(_location->count));
    // Copy the min time
    retVal->min = (double)_location->min;
    // Copy the max time
    retVal->max = (double)_location->max;
    // Copy the count
    retVal->count = _location->count;

    // If there is a child, get its data record
    if(_location->child_offset != INVALID_OFFSET) {
        PerfLocation child(_location->child_offset);
        retVal->child = child.GetDataRecord();
    }

    return retVal;
}

bool PerfLocation::AddTimeStamp(uint64_t timeStamp)
{
    bool retVal = false;

    LOG(eTrace, "Inserted timestamp %lu for count %ld\n", timeStamp, _location->count);
    // Insert the timestamp
    retVal = _timeStamps.push(_location->count, timeStamp);

    // Get the root time stamp
    uint64_t rootTimeStamp = GetRootTimeStamp(_location->count);

    // Get the matching parent time stamp
    if(_location->parent_offset != INVALID_OFFSET) {
        PerfLocation parent(_location->parent_offset);
        uint64_t parentTimeStamp = parent.GetTimeStamp(_location->count);
        if(parentTimeStamp != INVALID_TIMESTAMP) {
            // Calculate the difference
            uint64_t diff = (uint64_t)(timeStamp - parentTimeStamp);
            LOG(eTrace, "Parent timestamp %lu, diff %lf\n", parentTimeStamp, diff);
            // Update the total elapsed time
            _location->total +=  diff;

            uint64_t rootDiff = (uint64_t)(timeStamp - rootTimeStamp);
            // Update the total elapsed time
            _location->total_elapsed +=  rootDiff;

            // Update the min
            if(diff < _location->min || _location->min == 0) {
                _location->min = diff;
            }
            // Update the max
            if(diff > _location->max) {
                _location->max = diff;
            }

            LOG(eTrace, "Total %lu, Avg %lf Min %lu, Max %lu\n", 
                        _location->total, (double)(_location->total / (_location->count + 1)), 
                        _location->min, _location->max);

            // Since there is a match remove the parent timestamp from the map
            parent.RemoveTimeStamp(_location->count);
        }
        else {
            LOG(eError, "Parent timestamp not found\n");
        }
    }

    // Increment the count
    _location->count++;

    return retVal;
}

uint64_t PerfLocation::GetTimeStamp(uint32_t count)
{
    uint64_t timeStamp = 0;
    uint32_t index = 0;
    _timeStamps.peek(index, timeStamp);
    if(index != count) {
        LOG(eError, "Invalid index %u, expected %u\n", index, count);
        timeStamp = INVALID_TIMESTAMP;
    }
    else {
        LOG(eTrace, "Found timestamp %lu for count %u\n", timeStamp, count);
    }
    return timeStamp;
}

uint64_t PerfLocation::GetRootTimeStamp(uint32_t count)
{
    if(_location->sequence == nullptr) {
        return INVALID_TIMESTAMP;
    }

    PerfLocation location(_location->sequence->location_offset);
    return location.GetTimeStamp(count);

}

bool PerfLocation::RemoveTimeStamp(uint32_t count)
{
    uint64_t timeStamp = 0;
    return _timeStamps.pop(count, timeStamp);
}
