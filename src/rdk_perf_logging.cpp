/**
* Copyright 2022 Comcast Cable Communications Management, LLC
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
#include "rdk_perf_scopedlock.h"

#include <unistd.h> // for getipd()
#include <pthread.h>

static bool s_VerboseLog = false;
void RDKPerfLogging(eLogLevel level, const char* function, int line, const char * format, ...)
{    
    char logMessage[LOG_MESSAGE_SIZE];

    if(s_VerboseLog == false && level < eWarning) {
        // No logging
        return;
    }

    SCOPED_LOCK();

    // Generate the log string
    va_list ap;
    va_start(ap, format);
    vsnprintf(logMessage, LOG_MESSAGE_SIZE, format, ap);
    va_end(ap);

    FILE* fpOut = stdout;
    if(level == eError) {
        fpOut = stderr;
    }

    // printf for now.
    fprintf(fpOut, "Process ID %X : Thread ID %X : %s(%d) : %s", getpid(), (uint32_t)pthread_self(), function, line, logMessage);
    fflush(fpOut);
    return;
}

static void __attribute__((constructor)) LogModuleInit();
static void __attribute__((destructor)) LogModuleTerminate();

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
static void LogModuleInit()
{
#ifndef NO_PERF
    LOG(eWarning, "RDK Perf Logging initialize extending logging set to %d\n", s_VerboseLog);
    const char *env_log_level = getenv("RDKPER_EXTENDED_LOGGING");
    if(env_log_level != NULL &&
       strncasecmp(env_log_level, "true", strlen("true")) == 0) {
      s_VerboseLog = true;
      LOG(eWarning, "Enabling RDKPERF extended logging %d", s_VerboseLog);
    }
#endif
}
// This function is assigned to execute as library unload
// using __attribute__((destructor))
static void LogModuleTerminate()
{
#ifndef NO_PERF
    LOG(eWarning, "RDK Perf Logging terminate\n");
#endif
}
