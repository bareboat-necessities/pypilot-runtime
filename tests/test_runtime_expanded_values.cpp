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

    assert(protocol.apply_set("ap.preferred_mode", "wind", error, sizeof(error)));
    assert(std::strcmp(autopilot.preferred_mode.get(), "wind") == 0);

    assert(protocol.apply_set("servo.engaged", "true", error, sizeof(error)));
    assert(servo.engaged.get());

    assert(protocol.apply_set("ap.tack.state", "3", error, sizeof(error)));
    assert(pilots.tack_state.get() == 3.0);
    assert(protocol.apply_set("ap.tack.direction", "2", error, sizeof(error)));
    assert(pilots.tack_direction.get() == 2.0);
    assert(protocol.apply_set("profile.name", "coastal", error, sizeof(error)));
    assert(std::strcmp(sensors.profile_name.get(), "coastal") == 0);

    autopilot.heading.set(120.0);
    autopilot.heading_error.set(-3.5);
    boatimu.heading_lowpass.set(45.0);
    boatimu.heel.set(12.0);
    boatimu.headingrate.set(1.25);
    boatimu.alignment_counter.set(7.0);
    boatimu.uptime.set(12.0);
    servo.voltage.set(12.7);
    servo.current.set(1.5);
    servo.position.set(-2.0);
    servo.controller_temp.set(31.0);
    servo.motor_temp.set(29.0);
    servo.flags.set("ok");
    servo.controller.set("pwm");
    servo.amp_hours.set(0.25);
    gps.track.set(180.0);
    gps.timestamp.set(10.5);
    gps.source.set("nmea0183");
    wind.speed.set(8.2);
    wind.source.set("apparent");
    sensors.truewind_speed.set(9.5);
    sensors.truewind_direction.set(44.0);
    sensors.truewind_source.set("calculated");
    sensors.water_speed.set(4.2);
    sensors.rudder_angle.set(-1.5);
    sensors.server_version.set("0.70-cpp");
    sensors.server_uptime.set(100.0);
    sensors.status_faults.set(2.0);
    sensors.status_warnings.set(4.0);
    sensors.runtime_published_value_count.set(42.0);

    assert(protocol.format_named_value("ap.heading", out, sizeof(out)));
    assert(std::strcmp(out, "ap.heading=120.0000\n") == 0);
    assert(protocol.format_named_value("servo.voltage", out, sizeof(out)));
    assert(std::strcmp(out, "servo.voltage=12.7000\n") == 0);
    assert(protocol.format_named_value("gps.source", out, sizeof(out)));
    assert(std::strcmp(out, "gps.source=\"nmea0183\"\n") == 0);
    assert(protocol.format_named_value("wind.speed", out, sizeof(out)));
    assert(std::strcmp(out, "wind.speed=8.2000\n") == 0);
    assert(protocol.format_named_value("truewind.speed", out, sizeof(out)));
    assert(std::strcmp(out, "truewind.speed=9.5000\n") == 0);
    assert(protocol.format_named_value("water.speed", out, sizeof(out)));
    assert(std::strcmp(out, "water.speed=4.2000\n") == 0);
    assert(protocol.format_named_value("rudder.angle", out, sizeof(out)));
    assert(std::strcmp(out, "rudder.angle=-1.5000\n") == 0);
    assert(protocol.format_named_value("server.version", out, sizeof(out)));
    assert(std::strcmp(out, "server.version=\"0.70-cpp\"\n") == 0);
    assert(protocol.format_named_value("status.faults", out, sizeof(out)));
    assert(std::strcmp(out, "status.faults=2.0000\n") == 0);

    return 0;
}
