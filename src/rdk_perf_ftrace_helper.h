/**
* Copyright 2023 Comcast Cable Communications Management, LLC
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

#ifndef __RDK_PERF_FTRACE_HELPER_H__
#define __RDK_PERF_FTRACE_HELPER_H__

#include <stdio.h>
#include <stdarg.h>

#define RDK_PERF_TRACE_MARKER "/sys/kernel/debug/tracing/trace_marker"

void ftrace_print(const char* msg, ...)
{
    va_list args;
    va_start (args, msg);
    FILE* tracefd = fopen(RDK_PERF_TRACE_MARKER, "wt");
    if (tracefd != NULL) {
        vfprintf(tracefd, msg, args);
        fclose(tracefd);
    }
    va_end(args);
}

#endif // __RDK_PERF_FTRACE_HELPER_H__
