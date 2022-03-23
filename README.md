

# **RDKPerf**

## Overview

RDKPerf is a lightweight profiling tool for real time performance metrics reporting.  It allows for easy instrumentation of functions or specific library calls with minimal disruption to the original code flow.  

Metrics reports can be initiated directly from the code or can happen automatically at controlled intervals.  The reporting data will contain a call stack of all instrumented functions so it is possible to see how different code modules are used in conjunction with each other.


## License
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## Instructions

Adding metrics gathering for a function is as easy as instantiating a RDKPerf object on the stack.   When it is created a timestamp is logged within the object.  When the object is destructed the elapsed time is recoded in a table.

There are two different constructors.  The simplest form just contains the name of the operation that you want to time.  This is generally the module function name, but can represent a subsection of code, or a specific library call.

    RDKPerf(const char* szName);

The second form adds a threshold value in microseconds.  If the threshold is reached then a call stack from this point down will be instantly logged.

    RDKPerf(const char* szName, uint32_t nThresholdInUS);

In C modules start and stop function are provided to abstract the constructor/destructor functionality.


    RDKPerfHandle RDKPerfStart(const char* szName);
    void RDKPerfStop(RDKPerfHandle hPerf);
    void RDKPerfSetThreshold(RDKPerfHandle hPerf, uint32_t nThresholdInUS);

### Instrumenting module functions

In C++

    void MyFunction()
    {
        RDKPerf perf(__FUNCTION__);
        
        for(int nIdx = 0; nIdx < 2; nIdx++) {
            DoTask(nIdx);
        }
    }

In C

    void MyFunction()
    {
        RDKPerfHandle hPerf = RDKPerfStart(__FUNCTION__);
        
        for(int nIdx = 0; nIdx < 2; nIdx++) {
            DoTask(nIdx);
        }

        RDKPerfStop(hPerf);
    }

### Instrumenting a section of code or library call.

In C++

    RDKPerf* decrypt_perf = new RDKPerf("session_decrypt");
    result = session_decrypt(session, encryptedData, totalEncrypted, mappedIV, mappedIVSize, 
                             mappedKeyID, mappedKeyIDSize, initWithLast15, caps_str, caps_len);
    delete decrypt_perf;

In C

    RDKPerfHandle hPerf = RDKPerfStart("session_decrypt");
    result = session_decrypt(session, encryptedData, totalEncrypted, mappedIV, mappedIVSize, 
                             mappedKeyID, mappedKeyIDSize, initWithLast15, caps_str, caps_len);
    RDKPerfStop(hPerf);


## How to build

Add the header file to any module that needs instrumentation.

     #include "rdk_perf.h"

Link to the library

    LD_FLAGS += -L$(PERF_LIBRARY_LOCATION) -lrdkperf -lperftool

## Logged data

Data will be generated with a call to ReportData().  

There are two variants for this function one to report all data for a given process, and one to report all data for a given thread.

    void RDKPerf_ReportProcess(pid_t pID);
    void RDKPerf_ReportThread(pthread_t tID);



There is also a background timer that calls ReportProcess at ever increasing intervals until it reaches steady state at 10 minute gaps.

Here is an example of collected data. 

    ReportData(346) : Found 2 threads in this process 36FF named </usr/bin/WPEWebProcess>
    ReportData(197) : Printing report on 990FE270 thread named appsrc3:src
    ReportData(551) : | root_node (Count, Max, Min, Avg) Total 0, 0.000, 1000000.000, 0.000 Interval 0, 0.000, 1000000.000, 0.000
    ReportData(551) : --| transform_from_cleardata (Count, Max, Min, Avg) Total 350, 22.577, 1.493, 2.545 Interval 0, 0.000, 1000000.000, 0.000
    ReportData(551) : --| get_context (Count, Max, Min, Avg) Total 1, 0.362, 0.362, 0.362 Interval 0, 0.000, 1000000.000, 0.000
    ReportData(551) : --| transform_caps (Count, Max, Min, Avg) Total 9, 0.269, 0.073, 0.134 Interval 0, 0.000, 1000000.000, 0.000
    ReportData(551) : --| session_decrypt_ex_video (Count, Max, Min, Avg) Total 914, 30.841, 7.232, 9.353 Interval 122, 14.978, 7.255, 9.157
    ReportData(551) : ----| decrypt_subsample (Count, Max, Min, Avg) Total 914, 20.951, 4.122, 5.504 Interval 122, 11.511, 4.122, 5.337
    ReportData(551) : ----| transform_subsample (Count, Max, Min, Avg) Total 914, 11.179, 2.831, 3.771 Interval 122, 10.232, 2.942, 3.743
    ReportData(551) : ------| svp_transform (Count, Max, Min, Avg) Total 914, 11.167, 2.819, 3.751 Interval 122, 10.219, 2.933, 3.727
    ReportData(551) : --------| Buffer_CopyByIndex (Count, Max, Min, Avg) Total 914, 3.584, 0.644, 0.874 Interval 122, 2.559, 0.674, 0.848
    ReportData(551) : --------| Buffer_Free (Count, Max, Min, Avg) Total 914, 2.757, 0.597, 0.771 Interval 122, 2.757, 0.607, 0.772
    ReportData(551) : --------| alloc_token (Count, Max, Min, Avg) Total 914, 0.244, 0.005, 0.012 Interval 122, 0.021, 0.005, 0.011
    ReportData(551) : --------| buffer_from_token (Count, Max, Min, Avg) Total 914, 0.022, 0.001, 0.002 Interval 122, 0.004, 0.001, 0.002
    ReportData(551) : ----| token_size (Count, Max, Min, Avg) Total 914, 0.050, 0.001, 0.003 Interval 121, 0.004, 0.001, 0.002
    ReportData(197) : Printing report on AF738010 thread named WPEWebProcess
    ReportData(551) : | root_node (Count, Max, Min, Avg) Total 0, 0.000, 1000000.000, 0.000 Interval 0, 0.000, 1000000.000, 0.000
    ReportData(551) : --| free_context (Count, Max, Min, Avg) Total 2, 33.302, 0.050, 16.676 Interval 2, 33.302, 0.050, 16.676
    ReportData(551) : --| get_context (Count, Max, Min, Avg) Total 1, 0.267, 0.267, 0.267 Interval 0, 0.000, 1000000.000, 0.000
    TimerCallback(71) : Next performance log in 100 seconds

The data identifies the names of the process and threads contained and indicates the name of the instrumented code, it's level in the call stack and some aggregated data about that function.

    ReportData(551) : --| session_decrypt_ex_video (Count, Max, Min, Avg) Total 914, 30.841, 7.232, 9.353 Interval 122, 14.978, 7.255, 9.157

In this example *session_decrypt_ex_video* is one level below the root node (meaning that this is the top level of instrumentation) and over the life time of the process it has been called 914 times with an average elapsed time of 9.353 milliseconds (max of 30.841 ms and min of 7.232 ms).

We can also observe the same data broken out since the last time this data was logged

    Interval 122, 14.978, 7.255, 9.157

In this case the function was called 122 times with an overall average elapsed time of 9.157ms.

Below that function call *transform_subsample* is two levels below the root node, one level below *session_decrypt_ex_video*.   

    ReportData(551) : ----| transform_subsample (Count, Max, Min, Avg) Total 914, 11.179, 2.831, 3.771 Interval 122, 10.232, 2.942, 3.743

It also is called at the same cadence of it's parent (914 times) but only accounts for on average 3.771ms of the overall 9.353ms elapsed time.

It is important to note here is that this does not mean that *session_decrypt_ex_video* called *transform_subsample* directly, only that *transform_subsample* is the next instrumented element in the call stack, there may be several levels of un-instrumented code between these functions.

If there are no new calls to a specific thread over the reporting interval then it is removed from the table.

And lastly 

    TimerCallback(71) : Next performance log in 100 seconds

Indicates that the next round of performance data will be logged in 100 seconds
