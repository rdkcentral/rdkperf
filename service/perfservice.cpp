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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "rdk_perf.h"
#include "rdk_perf_logging.h"

int main(int argc, char *argv[])
{    
    LOG(eWarning, "Enter perfservice app %s\n", __DATE__);

    sleep(120);

    LOG(eWarning, "Exit perfservice app %s\n", __DATE__);

    return 0;
}

