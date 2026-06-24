#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState state;
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);

    char error[128]{};
    assert(protocol.apply_set("ap.enabled", "true", error, sizeof(error)));
    assert(state.ap.enabled.value);

    assert(protocol.apply_set("servo.command", "-0.25", error, sizeof(error)));
    assert(state.servo.command_norm.valid);
    assert(state.servo.command_norm.value < -0.249f && state.servo.command_norm.value > -0.251f);

    assert(!protocol.apply_set("imu.heading", "22.0", error, sizeof(error)));
    assert(std::strcmp(error, "error=value not writable imu.heading\n") == 0);

    char out[128]{};
    assert(protocol.format_value("ap.enabled", out, sizeof(out)));
    assert(std::strcmp(out, "ap.enabled=true\n") == 0);

    assert(protocol.format_value("servo.command", out, sizeof(out)));
    assert(std::strcmp(out, "servo.command=-0.2500\n") == 0);

    state.imu.heading_deg.set(123.4f, 100);
    assert(protocol.format_value("imu.heading", out, sizeof(out)));
    assert(std::strcmp(out, "imu.heading=123.4000\n") == 0);

    return 0;
}
