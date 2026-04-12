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

// Aided by CoPilot :-) 

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

#include "rdk_perf_shm_block.h"
#include "rdk_perf_logging.h" // for LOG macro

#define ERR_LOG(msg) LOG(eError, "%s - %d: %s\n", msg, errno, strerror(errno));

#define INITIALIZATION_UUID      "22b90a50-fccc-4fec-8314-fabd059a27e6"
#define INITIALIZATION_UUID_LEN  37
static uint8_t initialization_uuid_binary[] = {0x22, 0xb9, 0x0a, 0x50, 0xfc, 0xcc, 0x4f, 0xec, 0x83, 0x14, 0xfa, 0xbd, 0x05, 0x9a, 0x27, 0xe6};
#define INITIALIZATION_UUID_BINARY_LEN sizeof(initialization_uuid_binary)


static SharedMemoryBlock* _shared_memory_block = nullptr;

SharedMemoryBlock::SharedMemoryBlock(uint64_t maxDataSize) 
: _initialized(false)
, _shm_fd(-1)
, _shared_block(nullptr)
{
    _data_size = sizeof(GlobalStorage) + maxDataSize;
    _initialized = initialize();
}


bool SharedMemoryBlock::initialize() 
{
    bool bInitialize_memory = false;

    // Create the shared memory segment exclusively. If it already exists, return an error
    _shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (_shm_fd == -1) {
        ERR_LOG("shm already created");
        // Try to open the existing shared memory segment
        _shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
        if (_shm_fd == -1) {
            ERR_LOG("shm_open");
            return false;
        }
    }
    else {
        // We are the first to create the shared memory segment and can now initialize the memory
        bInitialize_memory = true;

        // Set the size of the shared memory segment
        if (ftruncate(_shm_fd, _data_size) == -1) {
            ERR_LOG("ftruncate");
            return false;
        }
    }

    // Map the shared memory segment into the process's address space
    _shared_block = static_cast<GlobalStorage*>(mmap(nullptr, _data_size, PROT_READ | PROT_WRITE, MAP_SHARED, _shm_fd, 0));
    if (_shared_block == MAP_FAILED) {
        ERR_LOG("mmap");
        return false;
    }

    if(bInitialize_memory) {
        // Initialize the shared memory segment if it's the first time
        memset(_shared_block, 0, _data_size);
        if (sem_init(&_shared_block->semaphore, 1, 1) == -1) {
            ERR_LOG("sem_init");
            return false;
        }
        memcpy(_shared_block->initialization_uuid, initialization_uuid_binary, INITIALIZATION_UUID_BINARY_LEN);
    }
    else {
        // Not the first proces in the shared memory segment
        while(memcmp(&_shared_block->initialization_uuid[0], initialization_uuid_binary, INITIALIZATION_UUID_BINARY_LEN) != 0) {
            ERR_LOG("Shared memory segment not initialized");
            // wait a bit for the shared memory segment to be initialized
            usleep(1000);

            static int nWaitCount = 0;
            if(nWaitCount++ > 1000) {
                // Waited too long for the shared memory segment to be initialized
                LOG(eError, "Shared memory segment not initialized and waited too long\n");
                return false;
            }
        }
    }

    LOG(eWarning, "Shared memory segment initialized data size %lu, address %p\n", _data_size, _shared_block);
    lock();
    _shared_block->attached_intances++;
    unlock();

    return true;
}

SharedMemoryBlock::~SharedMemoryBlock() 
{
    bool bDelete = false;

    // Decrement the number of attached instances
    lock();
    _shared_block->attached_intances--;
    if(_shared_block->attached_intances == 0) {
        bDelete = true;
    }
    unlock();

    if(bDelete) {
        // Destroy the semaphore
        sem_destroy(&_shared_block->semaphore);
        // Last process in the shared memory segment
        // Unlink the shared memory segment
        shm_unlink(SHARED_MEMORY_NAME);

        _shared_memory_block = nullptr;
    }

    // Unmap the shared memory segment
    munmap(_shared_block, _data_size);

    // Close the shared memory segment
    close(_shm_fd);
}

bool SharedMemoryBlock::lock() 
{
    return sem_wait(&_shared_block->semaphore) == 0;
}

bool SharedMemoryBlock::unlock() 
{
    return sem_post(&_shared_block->semaphore) == 0;
}

SharedMemoryBlock* SharedMemoryBlock::get_instance(uint64_t maxDataSize) 
{
    if(_shared_memory_block == nullptr) {
        LOG(eWarning, "Creating new SharedMemoryBlock instance\n");
        _shared_memory_block = new SharedMemoryBlock(maxDataSize);
    }

    return _shared_memory_block;
}
