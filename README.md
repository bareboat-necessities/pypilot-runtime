# pypilot-runtime

Typed, hardcoded pypilot runtime protocol layer for Linux and ESP32-S3.

This module intentionally does not use a dynamic value registry. Internal code uses typed C++ values directly. The runtime protocol server only performs hardcoded string dispatch at the external pypilot TCP protocol boundary.

Implemented pieces:

- TCP runtime server on port 23322
- hardcoded typed values for autopilot, IMU, servo, GPS, and wind basics
- `name=value` command parsing
- continuous and periodic watch support
- watched value publishing
- read-only rejection
- client helper
- optional settings bridge through `pypilot-settings`
- optional mDNS helper through `pypilot-mdns` for `_pypilot._tcp.local.` advertisement and Signal K discovery
- one shared server example compiled by Linux CMake and Arduino ESP32-S3

## Dependencies

Required:

```text
pypilot-event-loop
```

Optional sibling checkouts:

```text
pypilot-settings
pypilot-mdns
```

If `pypilot-settings` is present, CMake enables `PYPILOT_RUNTIME_WITH_SETTINGS` and builds the runtime settings bridge test. If `pypilot-mdns` is present, CMake enables `PYPILOT_RUNTIME_WITH_MDNS`, links `pypilot_mdns`, and builds the runtime mDNS compile test.

## Runtime mDNS helper

```cpp
#include <pypilot_runtime_mdns.hpp>

pypilot_runtime::RuntimeMdnsService mdns;
mdns.begin("pypilot");
mdns.advertise_pypilot(23322);
```

The helper keeps mDNS integration outside the core runtime value dispatch. Runtime still owns values and TCP protocol; `pypilot-mdns` owns DNS-SD advertisement/discovery.

## Build on Linux

This repository expects `pypilot-event-loop` as a sibling checkout. `pypilot-settings` and `pypilot-mdns` are optional but enabled when present:

```sh
cmake -S . -B build \
  -DPYPILOT_EVENT_LOOP_DIR=../pypilot-event-loop \
  -DPYPILOT_SETTINGS_DIR=../pypilot-settings \
  -DPYPILOT_MDNS_DIR=../pypilot-mdns
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
