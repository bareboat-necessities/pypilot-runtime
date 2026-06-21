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
    model.ap.preferred_mode.value = AutopilotMode::gps;
    model.ap.pilot.value = PilotName::basic;
    model.ap.heading_deg.set(123.0f, 100);
    model.imu.heading_lowpass_deg.set(122.5f, 100);
    model.imu.heel_deg.set(7.0f, 100);
    model.servo.current_a.set(1.5f, 100);
    model.navigation.gps.speed_kn.set(5.0f, 100);
    model.wind.apparent.speed_kn.set(10.0f, 100);
    model.wind.truewind.speed_kn.set(11.0f, 100);
    model.wind.truewind.direction_deg.set(44.0f, 100);
    model.water.speed_kn.set(4.0f, 100);
    model.rudder.angle_deg.set(-3.0f, 100);
    model.tack.state.value = TackState::tacking;
    model.tack.direction.value = TackDirection::starboard;
    std::strcpy(model.server.version, "0.70-cpp");
    std::strcpy(model.server.profile_name, "coastal");
    model.server.uptime_s.set(100.0f, 100);
    model.status.faults.value = 2;
    model.status.warnings.value = 4;
    model.runtime_publication.published_value_count.set(42u, 100);

    const size_t published = publish_data_model_to_runtime(state, model);
    assert(published >= 18);
    assert(ap.enabled.get());
    assert(std::strcmp(ap.mode.get(), "wind") == 0);
    assert(std::strcmp(ap.preferred_mode.get(), "gps") == 0);
    assert(ap.heading.get() == 123.0);
    assert(imu.heading_lowpass.get() == 122.5);
    assert(imu.heel.get() == 7.0);
    assert(servo.current.get() == 1.5);
    assert(gps.speed.get() == 5.0);
    assert(wind.speed.get() == 10.0);
    assert(sensors.truewind_speed.get() == 11.0);
    assert(sensors.truewind_direction.get() == 44.0);
    assert(sensors.water_speed.get() == 4.0);
    assert(sensors.rudder_angle.get() == -3.0);
    assert(pilots.tack_state.get() == static_cast<double>(TackState::tacking));
    assert(pilots.tack_direction.get() == static_cast<double>(TackDirection::starboard));
    assert(std::strcmp(sensors.server_version.get(), "0.70-cpp") == 0);
    assert(std::strcmp(sensors.profile_name.get(), "coastal") == 0);
    assert(sensors.server_uptime.get() == 100.0);
    assert(sensors.status_faults.get() == 2.0);
    assert(sensors.status_warnings.get() == 4.0);
    assert(sensors.runtime_published_value_count.get() == 42.0);

    return 0;
}
