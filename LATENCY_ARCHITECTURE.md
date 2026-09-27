# RDKPerf Latency Feature Architecture

## Executive Summary

The RDKPerf latency feature provides **cross-component, cross-process timing measurement** for tracking execution flow through distributed systems. Unlike traditional profiling that measures individual function execution times, this feature measures **elapsed time between named marker points** that can span multiple components, processes, and even different codebases.

---

## Problem Statement

Traditional performance profiling tools measure function execution time within a single process. However, modern applications often involve:
- Multiple processes communicating via IPC
- Data flowing through different software components
- Asynchronous operations across system boundaries
- Complex pipelines where understanding end-to-end latency is critical

**The latency feature solves this** by allowing developers to instrument marker points across the entire system and automatically correlate timing measurements.

---

## API Usage

### Basic C API

```c
#include "rdk_perf_latency.h"

// Record a marker point
void RDKLatency(const char* sequence, const char* location);

// Generate report for specific sequence
bool RDKLatencyReport(const char* sequence);

// Generate report for all sequences
bool RDKLatencyReportAll();
```

### Example Usage - Complete Video Pipeline

```c
// Process A - Media Application (AppSrc data injection)
void AppSrc_PushBuffer(Buffer* buffer) {
    RDKLatency("video_pipeline", "appsrc_push");
    
    // Push encrypted buffer to GStreamer pipeline
    gst_app_src_push_buffer(appsrc, buffer);
}

// Process A - GStreamer Pipeline (Decryption element)
void DecryptElement_Transform(Buffer* encrypted_buffer) {
    RDKLatency("video_pipeline", "decrypt_request");
    
    // Send encrypted buffer to DRM process via IPC
    send_to_drm_service(encrypted_buffer);
}

// Process B - DRM Service (Separate process for content protection)
void DRMService_DecryptBuffer(Buffer* encrypted_buffer) {
    RDKLatency("video_pipeline", "decrypt_start");
    
    // Perform DRM decryption (license validation, key extraction, etc.)
    Buffer* clear_buffer = drm_decrypt_with_license(encrypted_buffer);
    
    RDKLatency("video_pipeline", "decrypt_complete");
    
    // Return clear buffer to GStreamer via IPC
    send_to_gstreamer(clear_buffer);
}

// Process C - Video Decoder Service (Hardware decoder)
void VideoDecoder_DecodeFrame(Buffer* clear_buffer) {
    RDKLatency("video_pipeline", "decode_start");
    
    // Submit frame to hardware decoder
    hw_decoder_submit(clear_buffer);
    
    // Wait for decode completion
    DecodedFrame* frame = hw_decoder_get_output();
    
    RDKLatency("video_pipeline", "decode_complete");
    
    // Send to renderer via IPC
    send_to_renderer(frame);
}

// Process D - Video Renderer (Compositor/Display)
void VideoRenderer_RenderFrame(DecodedFrame* frame) {
    RDKLatency("video_pipeline", "render_start");
    
    // Composite and display frame
    compositor_blend(frame);
    display_present(frame);
    
    RDKLatency("video_pipeline", "render_complete");
}

// Generate report (can be called from any process)
// This will show the complete flow across 4 processes: AppSrc → DRM → Decoder → Display
RDKLatencyReport("video_pipeline");
```

### Automatic Reporting

The implementation includes automatic periodic reporting:
- Reports every 500 iterations (configurable via `REPORT_FREQUENCY`)
- Only reports when last location in sequence is reached
- Prevents duplicate reports for same iteration

---

## Report Format

### Sample Output

```
Sequence video_pipeline Depth 8
--| appsrc_push >> Elapsed 0.000 Avg 0.000 Min 0.000 Max 0.000 Count 1000
----| decrypt_request >> Elapsed 0.085 Avg 0.085 Min 0.050 Max 0.150 Count 1000
------| decrypt_start >> Elapsed 0.320 Avg 0.235 Min 0.180 Max 0.420 Count 1000
--------| decrypt_complete >> Elapsed 8.650 Avg 8.330 Min 7.200 Max 12.800 Count 1000
----------| decode_start >> Elapsed 9.120 Avg 0.470 Min 0.220 Max 1.200 Count 1000
------------| decode_complete >> Elapsed 24.380 Avg 15.260 Min 12.600 Max 23.100 Count 1000
--------------| render_start >> Elapsed 25.250 Avg 0.870 Min 0.410 Max 2.150 Count 1000
----------------| render_complete >> Elapsed 37.620 Avg 12.370 Min 10.150 Max 19.200 Count 1000
```

### Field Descriptions

- **Indentation** (`--`, `----`, etc.): Shows hierarchy depth
- **Location name**: Marker point identifier
- **Elapsed**: Average total time from sequence start (milliseconds)
- **Avg**: Average time from parent location (milliseconds)
- **Min/Max**: Minimum/maximum time from parent (milliseconds)
- **Count**: Number of times this location was recorded

---

## Use Cases

### 1. Video Streaming Pipeline
```c
RDKLatency("video_frame", "demux_complete");
RDKLatency("video_frame", "decrypt_complete");
RDKLatency("video_frame", "decode_complete");
RDKLatency("video_frame", "render_complete");
```
**Benefit**: Identify bottlenecks in multi-process video pipeline

### 2. DRM Content Protection
```c
RDKLatency("drm_flow", "license_request_start");
RDKLatency("drm_flow", "license_received");
RDKLatency("drm_flow", "key_extraction_complete");
RDKLatency("drm_flow", "decrypt_complete");
```
**Benefit**: Measure DRM overhead and identify slow operations

### 3. Web Request Handling
```c
RDKLatency("http_request", "request_received");
RDKLatency("http_request", "auth_complete");
RDKLatency("http_request", "database_query_complete");
RDKLatency("http_request", "response_sent");
```
**Benefit**: Track request latency across service layers

### 4. IPC Communication
```c
RDKLatency("ipc_message", "message_sent");
RDKLatency("ipc_message", "message_received");
RDKLatency("ipc_message", "processing_complete");
RDKLatency("ipc_message", "response_sent");
```
**Benefit**: Measure inter-process communication overhead

---

## Architecture Overview

### High-Level Design

```
┌───────────────────────────────────────────────────────────────────────────┐
│                    RDK Video Pipeline Application                         │
│                                                                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │  Process A   │  │  Process B   │  │  Process C   │  │  Process D   │   │
│  │  Media App + │  │     DRM      │  │   Decoder    │  │   Renderer   │   │
│  │  GStreamer   │  │   Service    │  │   Service    │  │   Service    │   │
│  │              │  │              │  │              │  │              │   │
│  │ AppSrc Push  │  │              │  │              │  │              │   │
│  │      ↓       │  │              │  │              │  │              │   │
│  │ Decrypt Req  │──┼─►Decrypt     │  │              │  │              │   │
│  │              │  │   Start      │  │              │  │              │   │
│  │              │  │      ↓       │  │              │  │              │   │
│  │              │  │   Decrypt    │──┼─►Decode      │  │              │   │
│  │              │  │   Complete   │  │   Start      │  │              │   │
│  │              │  │              │  │      ↓       │  │              │   │
│  │              │  │              │  │   Decode     │──┼─►Render      │   │
│  │              │  │              │  │   Complete   │  │   Start      │   │
│  │              │  │              │  │              │  │      ↓       │   │
│  │              │  │              │  │              │  │   Render     │   │
│  │              │  │              │  │              │  │   Complete   │   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
│         │                 │                 │                 │           │
│         │ RDKLatency()    │ RDKLatency()    │ RDKLatency()    │           │
│         ▼                 ▼                 ▼                 ▼           │
└─────────┼─────────────────┼─────────────────┼─────────────────┼───────────┘
          │                 │                 │                 │
          └─────────────────┴─────────────────┴─────────────────┘
                                      │
                                      ▼
        ┌──────────────────────────────────────────────────────────┐
        │         Shared Memory Block (POSIX SHM)                  │
        │                                                          │
        │  ┌────────────────────────────────────────────────┐      │
        │  │  Sequence: "video_pipeline"                    │      │
        │  │  - root_timestamp_buffer                       │      │
        │  │  - location chain (8 locations):               │      │
        │  │    appsrc_push → decrypt_request →             │      │
        │  │    decrypt_start → decrypt_complete →          │      │
        │  │    decode_start → decode_complete →            │      │
        │  │    render_start → render_complete              │      │
        │  └────────────────────────────────────────────────┘      │
        │                                                          │
        │  ┌────────────────────────────────────────────────┐      │
        │  │  Each Location stores:                         │      │
        │  │  - timestamp_circular_buffer (25 slots)        │      │
        │  │  - statistics (min/max/avg/count)              │      │
        │  │  - parent/child offsets                        │      │
        │  └────────────────────────────────────────────────┘      │
        └──────────────────────────────────────────────────────────┘
                                      │
                                      ▼
        ┌──────────────────────────────────────────────────────────┐
        │      RDKLatencyReport("video_pipeline")                  │
        │   Shows complete flow across 4 processes:                │
        │   AppSrc → DRM Service → Decoder → Display               │
        │   Total latency: ~37ms average                           │
        │   IPC overhead visible between process boundaries        │
        └──────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Shared Memory Block (SharedMemoryBlock)

**Purpose**: Provides inter-process communication for latency data.

**Key Features**:
- POSIX shared memory (`/rdkperf_sharedmemory_block`)
- Semaphore-based locking for thread-safe access
- Process-lifetime singleton - survives across multiple processes
- Automatic initialization - first process creates, others attach

**Memory Layout**:
```
[Sequence Array: 10 entries] → [Location Array: 100 entries]
Total: ~50KB shared memory
```

---

### 2. Sequences (PerfSequence)

**Purpose**: Represents a named workflow or execution path through the system.

**Concept**: A sequence is a logical grouping of related marker points. Examples:
- `"video_decode_pipeline"` - tracks video frame processing
- `"http_request_flow"` - tracks web request handling
- `"drm_decrypt_path"` - tracks DRM decryption operations

**Key Operations**:
- `GetSequence(name)` - Find or create a sequence
- `RemoveSequence(name)` - Clean up sequence and all locations
- `GetAllSequenceNames()` - List active sequences

**Limits**: 10 concurrent sequences (configurable via `MAX_SEQUENCE`)

---

### 3. Locations (PerfLocation)

**Purpose**: Represents a specific marker point in code where timing is recorded.

**Concept**: Locations are ordered checkpoints within a sequence. Each location tracks:
- When it was reached (timestamp)
- Time elapsed since parent location
- Time elapsed since sequence start (root)

**Hierarchy**: Locations form a linked list within each sequence:
```
Sequence → Location1 → Location2 → Location3 → ...
           (parent)    (child)     (child)
```

**Limits**: 100 total locations across all sequences (configurable via `MAX_LOCATIONS`)

---

### 4. Circular Buffers (CircularBuffer)

**Purpose**: Store recent timestamp history for correlation across processes.

**Why Circular Buffers?**
- Fixed memory footprint - no dynamic allocation in shared memory
- Automatic overflow handling - oldest entries discarded when full
- Key-value storage - maps iteration count to timestamp

**Operations**:
- `push(count, timestamp)` - Add new timestamp
- `find(count, timestamp)` - Retrieve timestamp by iteration count
- `pop(count, timestamp)` - Remove oldest timestamp

**Buffer Size**: 25 timestamps per location (configurable via `MAX_TIME_STAMPS`)

---

## Data Structures

### Sequence Structure
```c
typedef struct Sequence_s {
    char        name[MAX_NAME_LEN];           // Sequence identifier
    int32_t     location_offset;              // First location in chain
    uint8_t     timeStampCirBuffer[...];      // Root timestamps
} Sequence;
```

### Location Structure
```c
typedef struct Location_s {
    char        name[MAX_NAME_LEN];
    Sequence*   sequence;                     // Parent sequence
    uint8_t     timeStampCirBuffer[...];      // Timestamp history
    uint32_t    count;                        // Number of recordings
    int32_t     parent_offset;                // Previous location
    int32_t     child_offset;                 // Next location
    uint64_t    total;                        // Total time from parent
    uint64_t    total_elapsed;                // Total time from root
    uint64_t    min;                          // Minimum delta
    uint64_t    max;                          // Maximum delta
} Location;
```

### Circular Buffer Structure
```c
typedef struct CircBufferObject_s {
    CircBufferRecord    records[MAX_TIME_STAMPS];
    uint32_t            maxRecords;
    uint32_t            head;
    uint32_t            tail;
    uint32_t            currentSize;
    char*               name;
} CircBufferObject;
```

---

## How It Works

### Recording a Location

1. **Application calls** `RDKLatency("sequence", "location")`
2. **Find or create sequence** in shared memory
3. **Capture current timestamp** using `gettimeofday()`
4. **Find or create location** in sequence chain
5. **Store timestamp** in circular buffer with iteration count as key
6. **If first location**: Store as root timestamp in sequence
7. **Retrieve parent timestamp** from circular buffer using same count
8. **Calculate deltas**:
   - Delta from parent: `current - parent`
   - Elapsed from root: `current - root`
9. **Update statistics**: min, max, average, total
10. **Remove parent timestamp** (consumed, prevents memory leak)
11. **Increment location count**

### Timestamp Correlation Algorithm

**Key Insight**: Each iteration through a sequence has a unique count number. Timestamps are stored with their count as the key, enabling correlation across processes.

**Example**:
```
Iteration 1:
  Location1: count=0, timestamp=1000 (stored in buffer)
  Location2: count=0, timestamp=1050 (finds parent timestamp 1000, delta=50)
  Location3: count=0, timestamp=1100 (finds parent timestamp 1050, delta=50)

Iteration 2:
  Location1: count=1, timestamp=2000 (stored in buffer)
  Location2: count=1, timestamp=2040 (finds parent timestamp 2000, delta=40)
  Location3: count=1, timestamp=2090 (finds parent timestamp 2040, delta=50)
```

---

## Design Advantages

### 1. Cross-Process Capability

**Traditional Approach**: Each process maintains separate profiling data with no correlation.

**Latency Feature**: Shared memory enables automatic correlation across processes.

### 2. Zero Configuration

- No setup required - shared memory created automatically
- No process registration - processes attach on first use
- No cleanup needed - OS handles shared memory lifecycle

### 3. Minimal Performance Impact

- Lock-free reads from circular buffers (when possible)
- Short critical sections - only lock during timestamp storage
- No dynamic allocation - all memory pre-allocated in shared memory
- Efficient timestamp storage - microsecond resolution via `gettimeofday()`

### 4. Flexible Granularity

Can track at any level:
- **Coarse-grained**: Major pipeline stages
- **Fine-grained**: Individual function calls
- **Mixed**: Different granularity in different components

### 5. Automatic Statistics

No manual calculation needed:
- Min/Max tracking - identifies outliers
- Average calculation - shows typical performance
- Count tracking - validates execution frequency
- Elapsed time - shows end-to-end latency

### 6. Bounded Memory Usage

- Fixed-size arrays - no memory growth over time
- Circular buffers - automatic overflow handling
- Configurable limits:
  - 10 sequences
  - 100 locations
  - 25 timestamps per location
  - Total: ~50KB shared memory

### 7. Non-Intrusive Instrumentation

```c
// Single line addition
RDKLatency("sequence", "location");

// No cleanup required
// No object lifecycle management
// No error handling needed
```

### 8. Hierarchical Visualization

Report shows:
- Call flow - order of execution
- Nesting depth - component relationships
- Timing breakdown - where time is spent

### 9. Multi-Iteration Correlation

Circular buffers enable:
- Concurrent iterations - multiple flows in progress
- Out-of-order execution - async operations supported
- Buffered matching - handles timing variations

### 10. Process-Agnostic

Works across:
- Different languages - C/C++ API
- Different executables - shared library linkage
- Different architectures - same memory layout

---

## Implementation Details

### Thread Safety

- Shared memory locking via POSIX semaphores
- Critical sections protected during timestamp storage
- Lock-free reads from circular buffers where possible

### Memory Management

- All data structures use fixed-size arrays
- No dynamic allocation in shared memory
- Circular buffers prevent unbounded growth
- Automatic cleanup when processes exit

### Timestamp Resolution

- Microsecond precision using `gettimeofday()`
- 64-bit timestamps support long-running processes
- Relative timing avoids clock synchronization issues

### Error Handling

- Graceful degradation when limits reached
- Logging via `rdk_perf_logging.h`
- No crashes on invalid input

---

## Configuration

### Compile-Time Limits

```c
#define MAX_SEQUENCE 10          // Maximum number of sequences
#define MAX_LOCATIONS 100        // Maximum total locations
#define MAX_TIME_STAMPS 25       // Timestamps per location
#define MAX_NAME_LEN 64          // Maximum name length
```

### Shared Memory Configuration

```c
#define SHARED_MEMORY_NAME "/rdkperf_sharedmemory_block"
#define SEMAPHORE_NAME "/rdkperf_shmblock_sem"
```

### Reporting Configuration

```c
#define REPORT_FREQUENCY 500     // Auto-report every N iterations
```

---

## Comparison with Traditional Profiling

| Feature | Traditional Profiling | Latency Feature |
|---------|----------------------|-----------------|
| **Scope** | Single process | Multi-process |
| **Granularity** | Function-level | Arbitrary marker points |
| **Correlation** | Manual | Automatic |
| **Setup** | Configuration files | Zero config |
| **Memory** | Unbounded | Fixed (50KB) |
| **Performance** | High overhead | Minimal overhead |
| **Cross-component** | No | Yes |
| **Real-time** | No | Yes |

---

## Best Practices

### 1. Naming Conventions

Use descriptive, hierarchical names:
```c
// Good
RDKLatency("video_decode", "h264_parser_start");
RDKLatency("video_decode", "h264_parser_complete");

// Avoid
RDKLatency("seq1", "loc1");
```

### 2. Sequence Granularity

Keep sequences focused on a single workflow:
```c
// Good - separate sequences for different workflows
RDKLatency("video_decode", "start");
RDKLatency("audio_decode", "start");

// Avoid - mixing unrelated operations
RDKLatency("media", "video_start");
RDKLatency("media", "audio_start");
```

### 3. Location Ordering

Maintain consistent ordering across processes:
```c
// Process A
RDKLatency("pipeline", "step1");
RDKLatency("pipeline", "step2");

// Process B (continues same sequence)
RDKLatency("pipeline", "step3");
RDKLatency("pipeline", "step4");
```

### 4. Report Frequency

Balance between visibility and overhead:
```c
// High-frequency operations: report less often
#define REPORT_FREQUENCY 1000

// Low-frequency operations: report more often
#define REPORT_FREQUENCY 10
```

---

## Troubleshooting

### Issue: No data in reports

**Cause**: Sequence not found or locations not recorded
**Solution**: Verify sequence name spelling is consistent

### Issue: Missing timestamps

**Cause**: Circular buffer overflow
**Solution**: Increase `MAX_TIME_STAMPS` or reduce concurrent iterations

### Issue: Incorrect timing

**Cause**: Out-of-order location recording
**Solution**: Ensure locations are recorded in correct sequence order

### Issue: Shared memory errors

**Cause**: Insufficient permissions or memory
**Solution**: Check `/dev/shm` permissions and available space

---

## Future Enhancements

### Potential Improvements

1. **Dynamic buffer sizing** - adjust based on workload
2. **Network support** - extend to distributed systems
3. **Visualization tools** - graphical timeline view
4. **Export formats** - JSON, CSV output
5. **Filtering** - selective reporting by threshold
6. **Aggregation** - combine data across multiple runs

---

## Conclusion

The RDKPerf latency feature provides a powerful, lightweight solution for tracking execution flow across complex, multi-process systems. Its key advantages are:

- **Zero-configuration** cross-process timing
- **Minimal performance impact** with bounded memory
- **Automatic correlation** of distributed operations
- **Hierarchical reporting** for easy analysis

This makes it ideal for profiling modern media pipelines, distributed services, and any system where understanding end-to-end latency is critical.

---

## References

- Main API: `rdkperf/rdk_perf_latency.h`
- Implementation: `rdkperf/rdk_latency.cpp`
- Data structures: `src/rdk_perf_latency_data.h`
- Sequence management: `src/rdk_perf_sequence.cpp`
- Location tracking: `src/rdk_perf_location.cpp`
- Shared memory: `src/rdk_perf_shm_block.h`
