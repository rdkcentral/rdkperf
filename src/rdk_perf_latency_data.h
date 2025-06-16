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

#ifndef __RDK_PERF_LATENCY_DATA_H__
#define __RDK_PERF_LATENCY_DATA_H__


#include <stdint.h>
#include <string>

#define MAX_SEQUENCE 10
#define MAX_LOCATIONS 100
#define MAX_TIME_STAMPS 25
#define MAX_NAME_LEN 64
#define INVALID_TIMESTAMP (uint64_t)0
#define INVALID_OFFSET (int32_t)-1

typedef struct CircBufferRecord_s
{
    uint32_t key;
    uint64_t value;
} CircBufferRecord;

typedef struct CircBufferObject_s
{
    CircBufferRecord    records[sizeof(CircBufferRecord) * MAX_TIME_STAMPS];
    uint32_t            maxRecords;
    uint32_t            head;
    uint32_t            tail;
    uint32_t            currentSize;
} CircBufferObject;


typedef struct Sequence_s
{
    char        name[MAX_NAME_LEN];
    int32_t     location_offset;
    uint8_t     timeStampCirBuffer[sizeof(CircBufferObject)];   // Array of root timestamps for the sequence
    //uint64_t    rootTimeStamp; // root timestamp in microseconds, updated each time the first location is recorded
} Sequence;

typedef struct Location_s
{
    char        name[MAX_NAME_LEN];
    Sequence*   sequence;
    uint8_t     timeStampCirBuffer[sizeof(CircBufferObject)];
    uint32_t    count;
    int32_t     parent_offset;
    int32_t     child_offset;
    uint64_t    total;          // total time in microseconds from parent location
    uint64_t    total_elapsed;  // total elapsed time in microseconds from root location
    uint64_t    min;            // minimum time in microseconds
    uint64_t    max;            // maximum time in microseconds
} Location;

typedef struct DataRecord_s
{
    char            name[MAX_NAME_LEN];
    double          average;            // average time in microseconds
    double          average_elapsed;    // average time in microseconds from root location
    double          min;                // minimum time in microseconds
    double          max;                // maximum time in microseconds
    uint32_t        count;
    DataRecord_s*   child;
} DataRecord;


#endif // __RDK_PERF_LATENCY_DATA_H__
