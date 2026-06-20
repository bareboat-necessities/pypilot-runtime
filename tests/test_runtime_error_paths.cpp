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

    assert(!protocol.apply_set("unknown.name", "1", error, sizeof(error)));
    assert(std::strcmp(error, "error=unknown value unknown.name\n") == 0);

    assert(!protocol.apply_set("imu.heading", "12", error, sizeof(error)));
    assert(std::strcmp(error, "error=imu.heading is not writable\n") == 0);

    assert(!protocol.apply_set("servo.command", "2.0", error, sizeof(error)));
    assert(std::strcmp(error, "error=invalid value for servo.command\n") == 0);

    assert(!protocol.apply_set("ap.enabled", "maybe", error, sizeof(error)));
    assert(std::strcmp(error, "error=invalid value for ap.enabled\n") == 0);

    assert(pypilot_runtime::parse_value_name("unknown.name") == pypilot_runtime::PypilotValueId::Unknown);

    return 0;
}
