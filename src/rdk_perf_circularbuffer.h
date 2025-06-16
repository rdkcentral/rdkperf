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


#ifndef __RDK_PERF_CIRCULARBUFFER_H__
#define __RDK_PERF_CIRCULARBUFFER_H__

#include <vector>
#include <cstdint>
#include <cstring>

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

#endif // __RDK_PERF_CIRCULARBUFFER_H__
