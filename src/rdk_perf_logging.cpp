/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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
*/

#include "rdk_perf_logging.h"

#include <unistd.h> // for getipd()

static bool s_VerboseLog = true;
void RDKPerfLogging(eLogLevel level, const char* function, int line, const char * format, ...)
{    
#define LOG_MESSAGE_SIZE 4096
    char logMessage[LOG_MESSAGE_SIZE];

    if(s_VerboseLog == false && level < eWarning) {
        // No logging
        return;
    }

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
    fprintf(fpOut, "Process ID %X : %s(%d) : %s", getpid(), function, line, logMessage);
    // fprintf(fpOut, "%s(%d) : %s", function, line, logMessage);
    fflush(fpOut);
    return;
}

#define BUF_SIZE 8192
void DebugBinaryData(char* szName, uint8_t* pData, size_t nSize)
{
    size_t     idx     = 0;
    uint8_t    buffer[BUF_SIZE];
    uint8_t*   ptr     = buffer;

    size_t bufAvail = (BUF_SIZE/4 - strlen(szName));
    if(nSize > bufAvail) {
        nSize = bufAvail;
        LOG(eTrace, "Size truncated to %d\n", nSize);
    }

    sprintf((char*)ptr, "%s", (char*)szName);
    ptr += strlen(szName);
    while(idx < nSize) {
        if(idx % 16 == 0) sprintf((char*)(ptr++), "\n");
        sprintf((char*)ptr, "%02X ", pData[idx]);
        ptr +=3;
        idx++;
    }
    sprintf((char*)(ptr++), "\n");

    LOG(eTrace, "%s", buffer);
}

static void __attribute__((constructor)) LogModuleInit();
static void __attribute__((destructor)) LogModuleTerminate();

// This function is assigned to execute as a library init
//  using __attribute__((constructor))
static void LogModuleInit()
{
    LOG(eWarning, "RDK Perf Logging initialize extending logging set to %d\n", s_VerboseLog);
    const char *env_log_level = getenv("RDKPER_EXTENDED_LOGGING");
    if(env_log_level != NULL &&
       strncasecmp(env_log_level, "true", strlen("true")) == 0) {
      s_VerboseLog = true;
      LOG(eWarning, "Enabling RDKPERF extended logging %d", s_VerboseLog);
    }

}
// This function is assigned to execute as library unload
// using __attribute__((destructor))
static void LogModuleTerminate()
{
    LOG(eWarning, "RDK Perf Logging terminate\n");
}
