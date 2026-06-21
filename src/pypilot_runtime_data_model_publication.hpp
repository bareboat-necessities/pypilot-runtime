#pragma once

#include <stdio.h>

#include <pypilot_data_model.hpp>
#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

template<typename Real>
inline size_t publish_data_model_to_runtime(PypilotRuntimeState& state,
                                            const pypilot_data_model::DataModel<Real>& model) {
    size_t count = 0;
    const pypilot_data_model::RuntimeValueMetadata* values = pypilot_data_model::core_runtime_values(count);
    size_t published = 0;

    for (size_t i = 0; i < count; ++i) {
        const pypilot_data_model::RuntimeValueMetadata& meta = values[i];
        Real number = Real(0);
        bool boolean = false;
        const char* text = 0;
        char scratch[32]{};

        switch (meta.type) {
        case pypilot_data_model::RuntimeValueType::boolean:
            if (!pypilot_data_model::read_bool(model, meta.field_id, boolean)) break;
            if (meta.field_id == pypilot_data_model::FieldId::ap_enabled) { state.autopilot.enabled.set(boolean); ++published; }
            else if (meta.field_id == pypilot_data_model::FieldId::servo_engaged) { state.servo.engaged.set(boolean); ++published; }
            break;

        case pypilot_data_model::RuntimeValueType::number:
        case pypilot_data_model::RuntimeValueType::integer:
            if (!pypilot_data_model::read_number(model, meta.field_id, number)) break;
            switch (meta.field_id) {
            case pypilot_data_model::FieldId::ap_heading_command_deg: state.autopilot.heading_command.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::ap_heading_deg: state.autopilot.heading.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::ap_heading_error_deg: state.autopilot.heading_error.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::imu_heading_deg: state.boatimu.heading.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::imu_roll_deg: state.boatimu.roll.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::imu_pitch_deg: state.boatimu.pitch.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::imu_heading_lowpass_deg: state.boatimu.heading_lowpass.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::servo_command_norm: state.servo.command.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::servo_voltage_v: state.servo.voltage.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::servo_current_a: state.servo.current.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::servo_amp_hours_ah: state.servo.amp_hours.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::servo_flags:
                snprintf(scratch, sizeof(scratch), "%lu", static_cast<unsigned long>(number));
                state.servo.flags.set(scratch);
                ++published;
                break;
            case pypilot_data_model::FieldId::gps_speed_kn: state.gps.speed.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::gps_track_deg: state.gps.track.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::wind_direction_deg: state.wind.direction.set(static_cast<double>(number)); ++published; break;
            case pypilot_data_model::FieldId::wind_speed_kn: state.wind.speed.set(static_cast<double>(number)); ++published; break;
            default: break;
            }
            break;

        case pypilot_data_model::RuntimeValueType::mode:
            if (meta.field_id == pypilot_data_model::FieldId::ap_mode) {
                state.autopilot.mode.set(pypilot_data_model::autopilot_mode_name(model.ap.mode.value));
                ++published;
            }
            break;

        case pypilot_data_model::RuntimeValueType::pilot_name:
            if (meta.field_id == pypilot_data_model::FieldId::ap_pilot) {
                state.autopilot.pilot.set(pypilot_data_model::pilot_name(model.ap.pilot.value));
                ++published;
            }
            break;

        case pypilot_data_model::RuntimeValueType::string_value:
            if (!pypilot_data_model::read_string(model, meta.field_id, text)) break;
            if (meta.field_id == pypilot_data_model::FieldId::server_version) {
                (void)text;
            }
            break;
        }
    }

    return published;
}

} // namespace pypilot_runtime
