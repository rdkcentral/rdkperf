/**
* Copyright 2025 Comcast Cable Communications Management, LLC
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
#include "rdk_perf_circularbuffer.h"
#include "rdk_perf_logging.h"
#include <cstring>
#include <algorithm>
#include "rdk_perf_latency_data.h"

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
        LOG(eWarning, "Buffer is empty\n");
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
