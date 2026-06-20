# pypilot-runtime

Typed, hardcoded pypilot runtime protocol layer for Linux and ESP32-S3.

This module intentionally does not use a dynamic value registry. Internal code uses typed C++ values directly. The runtime protocol server only performs hardcoded string dispatch at the external pypilot TCP protocol boundary.

Initial milestone:

- TCP runtime server on port 23322
- hardcoded typed values for autopilot, IMU, servo, GPS, and wind basics
- `name=value` command parsing
- `watch={"name":0}` continuous watch support
- watched value publishing
- read-only rejection
- one shared server example compiled by Linux CMake and Arduino ESP32-S3

## Build on Linux

This repository expects `pypilot-event-loop` as a sibling checkout:

```sh
cmake -S . -B build -DPYPILOT_EVENT_LOOP_DIR=../pypilot-event-loop
cmake --build build
ctest --test-dir build --output-on-failure
```

## Shared server example

The real example implementation is:

```text
examples/RuntimeServerExample/RuntimeServerExample.ino
```

Linux builds this wrapper:

```text
examples/RuntimeServerExample/RuntimeServerExample.cpp
```

Arduino ESP32-S3 builds the same example directory with `arduino-cli`.
