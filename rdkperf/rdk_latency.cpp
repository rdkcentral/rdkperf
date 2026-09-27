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

#include "rdk_perf_sequence.h"
#include "rdk_perf_location.h"
#include "rdk_perf_latency.h"
#include "rdk_perf_scopedlock.h"

#define REPORT_FREQUENCY 500

#ifdef USE_RDK_PERF
#include "rdk_perf.h"
#define PERF_FUNC(a) RDKPerf perf(a);
#else
#define PERF_FUNC(a)
#endif // USE_RDK_PERF

// Prototypes
bool RDKCheckSequenceCount(PerfSequence* pSeq);

void RDKLatency(const char* sequence, const char* location) 
{
    PERF_FUNC(__FUNCTION__);
    
    SCOPED_LOCK();

    PerfSequence* pSeq = PerfSequence::GetInstance();
    if(pSeq->GetSequence(sequence)) {
        pSeq->RecordLocation(location);

        if(RDKCheckSequenceCount(pSeq)) {
            RDKLatencyReport(sequence);
        }
    }
}

bool RDKCheckSequenceCount(PerfSequence* pSeq)
{
    PERF_FUNC(__FUNCTION__);

    static uint32_t lastCount = 0;
    bool retVal = false;
    
    uint32_t count = pSeq->GetRecordCount();
    // Report every n-th location
    if(count != lastCount && count % REPORT_FREQUENCY == 0) {
        // Count will be the same for all the locations
        // in the sequence, so only print the report when the 
        // last location in the sequence is recorded
        retVal = true;
    }
    lastCount = count;

    return retVal;
}
bool RDKLatencyReportAll()
{
    // For all sequences in the PerfSequence instance
    PERF_FUNC(__FUNCTION__);
    SCOPED_LOCK();
    PerfSequence* pSeq = PerfSequence::GetInstance();
    // Iterate through all sequences
    // Note: This requires adding an iterator method to PerfSequence class
    // For simplicity, assuming we have a method GetAllSequenceNames that returns a vector of names
    std::vector<std::string> sequenceNames = pSeq->GetAllSequenceNames();
    for(const auto& seqName : sequenceNames) {
        RDKLatencyReport(seqName.c_str());
    }
    return true;
}
bool RDKLatencyReport(const char* sequence) 
{
    PERF_FUNC(__FUNCTION__);

    SCOPED_LOCK();
    FILE* fp = stderr;
    //fp = fopen("/tmp/rdkperf_latency.log", "a");

    PerfSequence* pSeq = PerfSequence::GetInstance();
    if(pSeq->GetSequence(sequence)) {
        DataRecord* pData = pSeq->GetDataRecord();
        if(pData != nullptr) {
            fprintf(fp, "Sequence %s Depth %d\n", pSeq->GetName(), pSeq->GetLocationsDepth());
            uint8_t depth = 1;
            while(pData != nullptr) {
                for(uint8_t i = 0; i <= depth; i++) {
                    fprintf(fp, "--");
                }
                fprintf(fp, "| %s >> Elapsed %0.3lf Avg %0.3lf Min %0.3lf Max %0.3lf Count %u \n", 
                            pData->name, 
                            (double)pData->average_elapsed / 1000.0,
                            (double)pData->average / 1000.0, 
                            (double)pData->min / 1000.0, 
                            (double)pData->max / 1000.0, 
                            pData->count);
                pData = pData->child;
                depth++;
            }
            return true;
        }
    }
    return false;
}