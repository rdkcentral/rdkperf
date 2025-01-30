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
#ifndef __RDK_PERF_SEQUENCE_H__
#define __RDK_PERF_SEQUENCE_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "rdk_perf_shm_block.h"
#include "rdk_perf_latency_data.h"




// Forward declarations
class PerfLocation;

class PerfSequence
{
public:
    PerfSequence();
    ~PerfSequence();

    bool            GetSequence(const char* sequenceName);
    const char*     GetName() { return _current_sequence != NULL ? _current_sequence->name : ""; };
    bool            RemoveSequence(const char* sequenceName);
    bool            RecordLocation(const char* locationName);
    uint32_t        GetLocationsDepth();

    DataRecord*     GetDataRecord();
    uint32_t        GetRecordCount();

    static PerfSequence* GetInstance();

private:
    bool            FindLocation(const char* locationName);
    bool            AddLocation(const char* locationName);
    void            SetCurrentLocation(Location* location);
    void            SetCurrentLocation(uint32_t locationOffset);
    uint64_t        GetTimeStampUS();


private:
    SharedMemoryBlock*      _shared_memory_block;
    uint64_t                _memory_block_size;
    Sequence*               _sequences;
    Location*               _locations;
    Sequence*               _current_sequence;
    PerfLocation*           _current_location;
};

#endif // __RDK_PERF_SEQUENCE_H__