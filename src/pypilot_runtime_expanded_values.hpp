#pragma once

#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

struct ExpandedAutopilotValues {
    RuntimeBool enabled{"ap.enabled", false, true};
    RuntimeString<24> mode{"ap.mode", "compass", true};
    RuntimeString<24> pilot{"ap.pilot", "basic", true};
    RuntimeNumber heading_command{"ap.heading_command", 0.0, 0.0, 360.0, true};
};

struct ExpandedServoValues {
    RuntimeNumber command{"servo.command", 0.0, -1.0, 1.0, true};
    RuntimeBool engaged{"servo.engaged", false, true};
    RuntimeString<24> state{"servo.state", "idle", false};
};

struct ExpandedBoatImuValues {
    RuntimeNumber heading{"imu.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber roll{"imu.roll", 0.0, -180.0, 180.0, false};
    RuntimeNumber pitch{"imu.pitch", 0.0, -180.0, 180.0, false};
    RuntimeNumber heading_lowpass{"imu.heading_lowpass", 0.0, 0.0, 360.0, false};
};

} // namespace pypilot_runtime
