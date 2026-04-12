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

#include "rdk_perf_logging.h"
#include "rdk_perf_location.h"
#include "rdk_perf_sequence.h"

#ifdef USE_RDK_PERF
#include "rdk_perf.h"
#define PERF_FUNC(a) RDKPerf perf(a);
#else
#define PERF_FUNC(a)
#endif // USE_RDK_PERF


static PerfSequence* _perf_sequence = nullptr;

// Module constructor/destructor functions
static void __attribute__((constructor)) PerfSequenceModuleInit();
static void __attribute__((destructor)) PerfSequenceModuleTerminate();

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
void PerfSequenceModuleInit()
{
    LOG(eWarning, "Initialize\n");
    if(_perf_sequence != nullptr) {
        LOG(eWarning, "_perf_sequence already initialized\n");
        delete _perf_sequence;
    }
    _perf_sequence = new PerfSequence();
}

// This function is assigned to execute as a library termination
//  using __attribute__((destructor))
void PerfSequenceModuleTerminate()
{
    LOG(eWarning, "Terminate\n");
    if(_perf_sequence != nullptr) {
        delete _perf_sequence;
        _perf_sequence = nullptr;
    }
}

PerfSequence::PerfSequence()
: _shared_memory_block(nullptr)
, _memory_block_size(0)
, _sequences(nullptr)
, _locations(nullptr)
, _current_sequence(nullptr)
, _current_location(nullptr)
{
    // Caclulate the size of the memory block
    _memory_block_size = PerfLocation::GetLocationDataSize() * PerfLocation::GetMaxLocations();
    _memory_block_size += sizeof(Sequence) * MAX_SEQUENCE;

    // Attach to the Shared Memory Block
    _shared_memory_block = SharedMemoryBlock::get_instance(_memory_block_size);
    if(_shared_memory_block == nullptr) {
        LOG(eError, "Failed to attach to shared memory block\n");
        return;
    }

    // Get the pointer to the sequence array
    _sequences = (Sequence*) _shared_memory_block->get_data();
    LOG(eError, "Sequence array at %p\n", _sequences);
    // Get the pointer to the location array
    _locations = (Location*) ((uint8_t*) _sequences + sizeof(Sequence) * MAX_SEQUENCE);
    LOG(eError, "Location array at %p\n", _locations);
    PerfLocation::SetLocationsArray(_locations);

    return;
}

PerfSequence::~PerfSequence()
{
    // Clean up the shared memory block
    if(_shared_memory_block != nullptr) {
        delete _shared_memory_block;
    }
    return;
}

std::vector<std::string> PerfSequence::GetAllSequenceNames()
{
    PERF_FUNC(__FUNCTION__);

    std::vector<std::string> sequenceNames;

    for(uint32_t i = 0; i < MAX_SEQUENCE; i++) {
        if(_sequences[i].name[0] != '\0') {
            sequenceNames.push_back(std::string(_sequences[i].name));
        }
    }

    return sequenceNames;
}

bool PerfSequence::GetSequence(const char* sequenceName)
{
    PERF_FUNC(__FUNCTION__);

    bool retVal = false;

    // Check if we are already in the sequence
    if(_current_sequence != nullptr) {
        if(strncmp(_current_sequence->name, sequenceName, MAX_NAME_LEN) == 0) {
            return true;
        }
    }

    // New sequence, reset the current location
    SetCurrentLocation(nullptr);

    // Find the sequence
    for(uint32_t i = 0; i < MAX_SEQUENCE; i++) {
        if(strncmp(_sequences[i].name, sequenceName, MAX_NAME_LEN) == 0) {
            _current_sequence = &_sequences[i];
            LOG(eTrace, "Found sequence %s\n", sequenceName);
            retVal = true;
        }
    }

    if(!retVal) {
        // Sequence not found
        LOG(eTrace, "Failed to find sequence %s\n", sequenceName);
        // Add the sequence
        uint32_t i = 0;
        for(i = 0; i < MAX_SEQUENCE; i++) {
            if(_sequences[i].name[0] == '\0') {
                strncpy(_sequences[i].name, sequenceName, MAX_NAME_LEN);
                LOG(eTrace, "Added sequence %s\n", _sequences[i].name);
                _sequences[i].location_offset = INVALID_OFFSET;
                // Initialize the timestamp circular buffer
                CircularBuffer rootTimeStamps(&_sequences[i].timeStampCirBuffer[0], MAX_TIME_STAMPS);
                rootTimeStamps.set_name(_sequences[i].name);
                _current_sequence = &_sequences[i];
                retVal = true;
                break;
            }
            LOG(eTrace, "Sequence %d %s\n", i, _sequences[i].name);
        }
        if(!retVal) {
            // No more room for sequences
            LOG(eError, "No more room for sequences, already allocated %d\n", i + 1);
        }
    }

    // Failed to find or add the sequence
    if(!retVal) {
        LOG(eError, "Failed to find or add sequence %s\n", sequenceName);
    }
    return retVal;
}

bool PerfSequence::RemoveSequence(const char* sequenceName)
{
    PERF_FUNC(__FUNCTION__);

    bool retVal = false;
    // Lock the shared memory block
    _shared_memory_block->lock();

    // Find the sequence
    for(uint32_t i = 0; i < MAX_SEQUENCE; i++) {
        if(strncmp(_sequences[i].name, sequenceName, MAX_NAME_LEN) == 0) {
            _sequences[i].name[0] = '\0';

            // Remove all locations
            Location* location = _locations + _sequences[i].location_offset;
            while(location != nullptr) {
                Location* next = _locations + location->child_offset;
                LOG(eTrace, "Removing location %s", location->name);
                location->name[0] = '\0';
                location = next;
            }

            _sequences[i].location_offset = INVALID_OFFSET;

            LOG(eTrace, "Removed sequence %s", sequenceName);
            retVal = true;
        }
    }

    // Unlock the shared memory block
    _shared_memory_block->unlock();

    LOG(eError, "Failed to find sequence %s ro remove\n", sequenceName);
    return retVal;
}

bool PerfSequence::FindLocation(const char* locationName)
{
    PERF_FUNC(__FUNCTION__);

    bool retVal = false;
    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to find location %s\n", locationName);
        return retVal;
    }

    // Is this the child of the current location
    if(_current_location != nullptr) {
        if (strncmp(_current_location->GetChildName(), locationName, MAX_NAME_LEN) == 0) {
            LOG(eTrace, "Found child location %s\n", locationName);
            SetCurrentLocation(_current_location->GetLocationChildOffset());

            retVal = true;
        }
    }

    if(!retVal) {
        // Find the location in the sequence location chain
        if(_current_sequence->location_offset != INVALID_OFFSET) {
            Location* location = _locations + _current_sequence->location_offset;
            while(location->name[0] != '\0') {
                LOG(eTrace, "Comparing location %s with %s\n", location->name, locationName);
                if(strncmp(location->name, locationName, MAX_NAME_LEN) == 0) {
                    LOG(eTrace, "Found location %s\n", locationName);
                    SetCurrentLocation(location);
                    retVal = true;
                }
                if(location->child_offset == INVALID_OFFSET) {
                    break;
                }
                location = _locations + location->child_offset;
            }
        }
    }

    if(!retVal) {
        LOG(eTrace, "Failed to find location %s\n", locationName);
    }
    return retVal;
}

void PerfSequence::SetCurrentLocation(Location* location)
{
    if(_current_location != nullptr) {
        delete _current_location;
        _current_location = nullptr;
    }
    if(location != nullptr) {
        _current_location = new PerfLocation(location);
    }
}

void PerfSequence::SetCurrentLocation(uint32_t locationOffset)
{
    if(_current_location != nullptr) {
        delete _current_location;
        _current_location = nullptr;
    }
    if(locationOffset != (uint32_t)INVALID_OFFSET) {
        _current_location = new PerfLocation(locationOffset);
    }
}

bool PerfSequence::AddLocation(const char* locationName)
{
    PERF_FUNC(__FUNCTION__);

    bool retVal = false;

    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to add location %s\n", locationName);
        return retVal;
    }

    // Add a new location
    _shared_memory_block->lock();
    uint32_t i = 0;
    for(i = 0; i < MAX_LOCATIONS; i++) {
        if(_locations[i].name[0] == '\0') {
            strncpy(_locations[i].name, locationName, MAX_NAME_LEN);
            LOG(eWarning, "Adding location %s at index %ld\n", locationName, i);
            _locations[i].sequence = _current_sequence;
            _locations[i].count = 0;
            _locations[i].parent_offset = INVALID_OFFSET;
            _locations[i].child_offset = INVALID_OFFSET;
            _locations[i].total = 0;
            _locations[i].total_elapsed = 0;
            _locations[i].min = 0;
            _locations[i].max = 0;

            CircularBuffer circBuffer(&_locations[i].timeStampCirBuffer[0], MAX_TIME_STAMPS);
            circBuffer.set_name(_locations[i].name);
            // If this is the first location in the sequence set the starting location offset
            if(_current_sequence->location_offset == INVALID_OFFSET) {
                _current_sequence->location_offset = i;
                LOG(eTrace, "Adding first location %s at offset %lu\n", locationName, _current_sequence->location_offset);
            }
            else {
                // Find the last location in the sequence
                Location* endLocation = _locations + _current_sequence->location_offset;
                while(endLocation->child_offset != INVALID_OFFSET) {
                    endLocation = _locations + endLocation->child_offset;
                }
                _locations[i].parent_offset = (uint32_t)(((uint8_t*)endLocation - (uint8_t*)_locations)/sizeof(Location));
                endLocation->child_offset = i;
            }

            SetCurrentLocation(&_locations[i]);

            retVal = true;
            break;
        }
    }
    _shared_memory_block->unlock();

    if(!retVal) {
        // No more room for locations
        LOG(eError, "No more room for locations, already allocated %d\n", i + 1);
    }
    return retVal;
}

uint64_t PerfSequence::GetTimeStampUS()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

bool PerfSequence::RecordLocation(const char* locationName)
{
    PERF_FUNC(__FUNCTION__);

    bool retVal = false;
    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to record location %s\n", locationName);
        return retVal;
    }

    // Find the location
    uint64_t timeStampUS = GetTimeStampUS();
    if(FindLocation(locationName)) {
        retVal = true;
    }
    else {
        // Add a new location
        retVal = AddLocation(locationName);
    }

    if(retVal) {
        _shared_memory_block->lock();
        _current_location->AddTimeStamp(timeStampUS);
        _shared_memory_block->unlock();

        LOG(eTrace, "Recorded location %s\n", locationName);
    }
    else {
        LOG(eError, "Failed to record location %s\n", locationName);
    }

    return retVal;
}

uint32_t PerfSequence::GetLocationsDepth()
{
    PERF_FUNC(__FUNCTION__);

    uint32_t count = 0;
    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to get location count\n");
        return count;
    }

    // Count the locations
    Location* location = _locations + _current_sequence->location_offset;
    while(location != nullptr) {
        count++;
        if(location->child_offset == INVALID_OFFSET) {
            break;
        }   
        location = _locations + location->child_offset;
    }

    return count;
}

DataRecord* PerfSequence::GetDataRecord()
{
    PERF_FUNC(__FUNCTION__);

    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to get data record\n");
        return nullptr;
    }

    PerfLocation location(_current_sequence->location_offset);
    return location.GetDataRecord();
}

uint32_t PerfSequence::GetRecordCount()
{
    PERF_FUNC(__FUNCTION__);

    uint32_t count = 0;
    // Check if we have a sequence
    if(_current_sequence == nullptr) {
        LOG(eError, "No current sequence to get record count\n");
        return count;
    }

    // Count the number of records in the sequence
    if(_current_sequence->location_offset == INVALID_OFFSET) {
        return count;
    }


    Location* location = _locations + _current_sequence->location_offset;
    // Find the tail of the location chain
    while(location->child_offset != INVALID_OFFSET) {
        location = _locations + location->child_offset;
    }

    count = location->count;

    return count;
}

PerfSequence* PerfSequence::GetInstance()
{
    if(_perf_sequence == nullptr) {
        LOG(eWarning, "Creating new PerfSequence instance\n");
        _perf_sequence = new PerfSequence();
    }

    if(_perf_sequence == nullptr) {
        LOG(eError, "Failed to create PerfSequence instance\n");
    }

    return _perf_sequence;
}