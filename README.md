# pypilot-runtime

Typed, hardcoded pypilot runtime protocol layer for Linux and ESP32-S3.

This module intentionally does not use a dynamic value registry. Internal code uses typed C++ values directly. The runtime protocol server only performs hardcoded string dispatch at the external pypilot TCP protocol boundary.

## Implemented pieces

- TCP runtime server on port 23322
- NMEA 0183 UDP broadcast receiver on port 10110 by default
- hardcoded typed values for autopilot, IMU, servo, GPS, and wind basics
- `name=value` command parsing
- continuous and periodic watch support
- watched value publishing
- read-only rejection
- client helper
- required settings bridge through `pypilot-settings`
- required mDNS helper through `pypilot-mdns` for `_pypilot._tcp.local.` advertisement and Signal K discovery
- required NMEA 0183 connector dependency through `pypilot-nmea0183-connector`
- required Signal K connector dependency through `pypilot-signalk-connector`
- one shared server example compiled by Linux CMake and Arduino ESP32-S3

## Dependencies

Required sibling checkouts:

```text
pypilot-event-loop
pypilot-settings
pypilot-mdns
pypilot-data-model
pypilot-nmea0183-connector
pypilot-signalk-connector
```

CMake treats these as real dependencies. Missing checkouts are fatal configuration errors, not optional feature drops.

The `pypilot_runtime` target links:

```text
pypilot_event_loop
pypilot_settings
pypilot_mdns
pypilot_nmea0183_connector
pypilot_signalk_connector
```

and exports these compile definitions:

```text
PYPILOT_RUNTIME_WITH_DATA_MODEL=1
PYPILOT_RUNTIME_WITH_SETTINGS=1
PYPILOT_RUNTIME_WITH_MDNS=1
PYPILOT_RUNTIME_WITH_NMEA0183_CONNECTOR=1
PYPILOT_RUNTIME_WITH_SIGNALK_CONNECTOR=1
```

## NMEA 0183 UDP input

`PypilotRuntimeService` binds a UDP datagram listener on port `10110` by default and processes NMEA 0183 broadcast sentences with `pypilot-nmea0183-connector`. Parsed GPS, heading, wind, water-speed, leeway, rudder, APB, and XTE values are applied directly into the shared `pypilot-data-model` state with `SensorSource::serial`.

Runtime settings:

```text
runtime.nmea0183.udp.port      default 10110
runtime.nmea0183.udp.enabled   default true
```

Environment overrides:

```text
PYPILOT_RUNTIME_NMEA0183_UDP_PORT
PYPILOT_NMEA0183_UDP_PORT
PYPILOT_RUNTIME_NMEA0183_UDP_ENABLED
PYPILOT_NMEA0183_UDP_ENABLED
```

Set the port to `0` or set enabled to `false` to disable the listener.

## Runtime mDNS helper

```cpp
#include <pypilot_runtime_mdns.hpp>

pypilot_runtime::RuntimeMdnsService mdns;
mdns.begin("pypilot");
mdns.advertise_pypilot(23322);
```

The helper keeps mDNS mechanics outside the core runtime value dispatch. Runtime owns values and TCP protocol; `pypilot-mdns` owns DNS-SD advertisement/discovery.

## Build on Linux

This repository expects all required modules as sibling checkouts:

```sh
cmake -S . -B build \
  -DPYPILOT_EVENT_LOOP_DIR=../pypilot-event-loop \
  -DPYPILOT_SETTINGS_DIR=../pypilot-settings \
  -DPYPILOT_MDNS_DIR=../pypilot-mdns \
  -DPYPILOT_NMEA0183_CONNECTOR_DIR=../pypilot-nmea0183-connector \
  -DPYPILOT_SIGNALK_CONNECTOR_DIR=../pypilot-signalk-connector
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
