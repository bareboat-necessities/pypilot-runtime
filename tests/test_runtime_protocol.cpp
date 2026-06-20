#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::AutopilotValues autopilot;
    pypilot_runtime::BoatImuValues boatimu;
    pypilot_runtime::ServoValues servo;
    pypilot_runtime::SensorValues sensors;
    pypilot_runtime::PilotValues pilots;
    pypilot_runtime::GpsValues gps;
    pypilot_runtime::WindValues wind;

    pypilot_runtime::PypilotRuntimeState state{autopilot, boatimu, sensors, servo, pilots, gps, wind};
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);

    char error[128]{};
    assert(protocol.apply_set("ap.enabled", "true", error, sizeof(error)));
    assert(autopilot.enabled.get());

    assert(protocol.apply_set("servo.command", "-0.25", error, sizeof(error)));
    assert(servo.command.get() < -0.249 && servo.command.get() > -0.251);

    assert(!protocol.apply_set("imu.heading", "22.0", error, sizeof(error)));
    assert(std::strcmp(error, "error=imu.heading is not writable\n") == 0);

    assert(!protocol.apply_set("servo.command", "2.0", error, sizeof(error)));
    assert(std::strcmp(error, "error=invalid value for servo.command\n") == 0);

    char out[128]{};
    assert(protocol.format_named_value("ap.enabled", out, sizeof(out)));
    assert(std::strcmp(out, "ap.enabled=true\n") == 0);

    assert(protocol.format_named_value("servo.command", out, sizeof(out)));
    assert(std::strcmp(out, "servo.command=-0.2500\n") == 0);

    boatimu.heading.set(123.4);
    assert(protocol.format_named_value("imu.heading", out, sizeof(out)));
    assert(std::strcmp(out, "imu.heading=123.4000\n") == 0);

    return 0;
}
