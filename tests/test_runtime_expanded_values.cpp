#include <cassert>

#include <pypilot_runtime_expanded_values.hpp>

int main() {
    pypilot_runtime::ExpandedAutopilotValues autopilot;
    pypilot_runtime::ExpandedServoValues servo;
    pypilot_runtime::ExpandedBoatImuValues imu;

    assert(autopilot.heading_command.set(123.0));
    assert(autopilot.heading_command.get() == 123.0);
    assert(servo.engaged.set(true));
    assert(servo.engaged.get());
    assert(imu.heading_lowpass.set(45.0));
    assert(imu.heading_lowpass.get() == 45.0);
    return 0;
}
