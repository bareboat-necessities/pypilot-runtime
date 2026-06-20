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

    char out[1024]{};
    assert(protocol.write_values_catalog(out, sizeof(out)));
    assert(std::strlen(out) > 100);
    assert(std::strstr(out, "ap.enabled") != nullptr);
    assert(std::strstr(out, "servo.command") != nullptr);
    assert(std::strstr(out, "imu.heading") != nullptr);
    assert(out[std::strlen(out) - 1] == '\n');
    return 0;
}
