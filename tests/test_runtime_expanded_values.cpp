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
    char out[128]{};

    assert(protocol.apply_set("ap.heading_command", "123.0", error, sizeof(error)));
    assert(autopilot.heading_command.get() == 123.0);
    assert(protocol.format_named_value("ap.heading_command", out, sizeof(out)));
    assert(std::strcmp(out, "ap.heading_command=123.0000\n") == 0);

    assert(protocol.apply_set("ap.pilot", "basic", error, sizeof(error)));
    assert(std::strcmp(autopilot.pilot.get(), "basic") == 0);

    assert(protocol.apply_set("servo.engaged", "true", error, sizeof(error)));
    assert(servo.engaged.get());

    boatimu.heading_lowpass.set(45.0);
    assert(protocol.format_named_value("imu.heading_lowpass", out, sizeof(out)));
    assert(std::strcmp(out, "imu.heading_lowpass=45.0000\n") == 0);

    return 0;
}
