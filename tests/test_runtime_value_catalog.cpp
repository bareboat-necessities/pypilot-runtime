#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState state;
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);

    char out[4096]{};
    assert(protocol.write_values_catalog(out, sizeof(out)));
    assert(std::strlen(out) > 100);
    assert(std::strstr(out, "values={") != nullptr);
    assert(std::strstr(out, "ap.enabled") != nullptr);
    assert(std::strstr(out, "ap.heading_command") != nullptr);
    assert(std::strstr(out, "ap.heading_error") != nullptr);
    assert(std::strstr(out, "ap.pilot") != nullptr);
    assert(std::strstr(out, "servo.command") != nullptr);
    assert(std::strstr(out, "servo.engaged") != nullptr);
    assert(std::strstr(out, "servo.voltage") != nullptr);
    assert(std::strstr(out, "imu.heading") != nullptr);
    assert(std::strstr(out, "imu.heading_lowpass") != nullptr);
    assert(std::strstr(out, "gps.track") != nullptr);
    assert(std::strstr(out, "gps.source") != nullptr);
    assert(std::strstr(out, "wind.speed") != nullptr);
    assert(std::strstr(out, "wind.source") != nullptr);
    assert(std::strstr(out, "server.version") != nullptr);
    assert(out[std::strlen(out) - 1] == '\n');
    return 0;
}
