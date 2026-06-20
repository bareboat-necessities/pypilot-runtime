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

    char out[4096]{};
    assert(protocol.write_values_catalog(out, sizeof(out)));
    assert(std::strlen(out) > 100);
    assert(std::strstr(out, "ap.enabled") != nullptr);
    assert(std::strstr(out, "ap.heading_command") != nullptr);
    assert(std::strstr(out, "ap.heading_error") != nullptr);
    assert(std::strstr(out, "ap.pilot") != nullptr);
    assert(std::strstr(out, "servo.command") != nullptr);
    assert(std::strstr(out, "servo.engaged") != nullptr);
    assert(std::strstr(out, "servo.flags") != nullptr);
    assert(std::strstr(out, "servo.voltage") != nullptr);
    assert(std::strstr(out, "servo.amp_hours") != nullptr);
    assert(std::strstr(out, "imu.heading") != nullptr);
    assert(std::strstr(out, "imu.heading_lowpass") != nullptr);
    assert(std::strstr(out, "imu.alignmentCounter") != nullptr);
    assert(std::strstr(out, "gps.track") != nullptr);
    assert(std::strstr(out, "gps.source") != nullptr);
    assert(std::strstr(out, "wind.speed") != nullptr);
    assert(std::strstr(out, "wind.source") != nullptr);
    assert(out[std::strlen(out) - 1] == '\n');
    return 0;
}
