#include <cassert>
#include <cstring>
#include <pypilot_runtime_data_model_publication.hpp>

int main() {
    using namespace pypilot_runtime;
    using namespace pypilot_data_model;

    AutopilotValues ap;
    BoatImuValues imu;
    SensorValues sensors;
    ServoValues servo;
    PilotValues pilots;
    GpsValues gps;
    WindValues wind;
    PypilotRuntimeState state{ap, imu, sensors, servo, pilots, gps, wind};

    DataModel<> model;
    model.ap.enabled.value = true;
    model.ap.mode.value = AutopilotMode::wind;
    model.ap.pilot.value = PilotName::basic;
    model.ap.heading_deg.set(123.0f, 100);
    model.imu.heading_lowpass_deg.set(122.5f, 100);
    model.servo.current_a.set(1.5f, 100);
    model.navigation.gps.speed_kn.set(5.0f, 100);
    model.wind.apparent.speed_kn.set(10.0f, 100);

    const size_t published = publish_data_model_to_runtime(state, model);
    assert(published >= 6);
    assert(ap.enabled.get());
    assert(std::strcmp(ap.mode.get(), "wind") == 0);
    assert(ap.heading.get() == 123.0);
    assert(imu.heading_lowpass.get() == 122.5);
    assert(servo.current.get() == 1.5);
    assert(gps.speed.get() == 5.0);
    assert(wind.speed.get() == 10.0);

    return 0;
}
