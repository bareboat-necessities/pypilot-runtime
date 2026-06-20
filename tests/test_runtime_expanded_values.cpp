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

    autopilot.heading.set(120.0);
    autopilot.heading_error.set(-3.5);
    boatimu.heading_lowpass.set(45.0);
    boatimu.alignment_counter.set(7.0);
    boatimu.uptime.set(12.0);
    servo.voltage.set(12.7);
    servo.current.set(1.5);
    servo.flags.set("ok");
    servo.controller.set("pwm");
    servo.amp_hours.set(0.25);
    gps.track.set(180.0);
    gps.source.set("nmea0183");
    wind.speed.set(8.2);
    wind.source.set("apparent");

    assert(protocol.format_named_value("ap.heading", out, sizeof(out)));
    assert(std::strcmp(out, "ap.heading=120.0000\n") == 0);
    assert(protocol.format_named_value("servo.voltage", out, sizeof(out)));
    assert(std::strcmp(out, "servo.voltage=12.7000\n") == 0);
    assert(protocol.format_named_value("gps.source", out, sizeof(out)));
    assert(std::strcmp(out, "gps.source=\"nmea0183\"\n") == 0);
    assert(protocol.format_named_value("wind.speed", out, sizeof(out)));
    assert(std::strcmp(out, "wind.speed=8.2000\n") == 0);

    return 0;
}
