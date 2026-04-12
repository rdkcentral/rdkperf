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

#ifndef __RDK_PERF_SHARED_MEMORY_BLOCK_H__
#define __RDK_PERF_SHARED_MEMORY_BLOCK_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>

#define SHARED_MEMORY_NAME "/rdkperf_sharedmemory_block"
#define SEMAPHORE_NAME "/rdkperf_shmblock_sem"

struct GlobalStorage {
    uint8_t     initialization_uuid[16];
    uint16_t    attached_intances;
    sem_t       semaphore;
    // Data
    uint8_t     data[0];
};

class SharedMemoryBlock {
public:
    SharedMemoryBlock(uint64_t maxDataSize);
    ~SharedMemoryBlock();

    bool        lock();
    bool        unlock();

    void*       get_data()      { return &_shared_block->data[0]; }

    // Static methods
    static SharedMemoryBlock* get_instance(uint64_t maxDataSize);

private:
    bool        initialize();

    bool            _initialized;
    int             _shm_fd;
    uint64_t        _data_size;
    GlobalStorage*  _shared_block;

};


#endif // __RDK_PERF_SHARED_MEMORY_BLOCK_H__