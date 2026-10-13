#!/bin/sh

set -eu

build_dir=$1
iterations=${2:-100}
library="$build_dir/librdkperf.so"

if [ ! -f "$library" ]; then
    echo "Missing test library: $library" >&2
    exit 1
fi

iteration=1
while [ "$iteration" -le "$iterations" ]; do
    if ! timeout 3s env \
        LD_LIBRARY_PATH="$build_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        LD_PRELOAD="$library" \
        /bin/true >/dev/null 2>&1; then
        echo "Timer quick-exit test failed on iteration $iteration" >&2
        exit 1
    fi
    iteration=$((iteration + 1))
done

echo "Timer quick-exit test passed ($iterations iterations)"