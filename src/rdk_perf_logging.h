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

#ifndef __RDK_PERF_LOGGING_H__
#define __RDK_PERF_LOGGING_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#define LOG(level, ...) RDKPerfLogging(level, __FUNCTION__, __LINE__, __VA_ARGS__)
typedef enum eLogLevel_
{
    eTrace,
    eWarning,
    eError
} eLogLevel;

#define LOG_MESSAGE_SIZE 4096

void RDKPerfLogging(eLogLevel level, const char* function, int line, const char * format, ...);
void DebugBinaryData(char* szName, uint8_t* pData, size_t nSize);

#endif // __RDK_PERF_LOGGING_H__

