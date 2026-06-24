#pragma once

#include <pypilot_data_model.hpp>
#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

template<typename Real>
inline bool publish_number_if_valid(const pypilot_data_model::Stamped<Real>& src, RuntimeNumber& dst) {
    if (!src.valid) return false;
    dst.set(static_cast<double>(src.value));
    return true;
}

template<typename Real>
inline size_t apply_runtime_commands_to_data_model(const PypilotRuntimeState& state,
                                                   pypilot_data_model::DataModel<Real>& model,
                                                   uint64_t now_us) {
    size_t applied = 0;

    model.ap.enabled.value = state.autopilot.enabled.get(); ++applied;

    pypilot_data_model::AutopilotMode mode{};
    if (pypilot_data_model::autopilot_mode_from_name(state.autopilot.mode.get(), mode)) {
        model.ap.mode.value = mode;
        ++applied;
    }
    if (pypilot_data_model::autopilot_mode_from_name(state.autopilot.preferred_mode.get(), mode)) {
        model.ap.preferred_mode.value = mode;
        ++applied;
    }

    pypilot_data_model::PilotName pilot{};
    if (pypilot_data_model::pilot_from_name(state.autopilot.pilot.get(), pilot)) {
        model.ap.pilot.value = pilot;
        ++applied;
    }

    model.ap.heading_command_deg.set(static_cast<Real>(state.autopilot.heading_command.get()), now_us); ++applied;
    model.servo.engaged.value = state.servo.engaged.get(); ++applied;
    model.servo.command_norm.set_external(static_cast<Real>(state.servo.command.get()), now_us); ++applied;

    return applied;
}

template<typename Real>
inline size_t publish_data_model_to_runtime(PypilotRuntimeState& state,
                                            const pypilot_data_model::DataModel<Real>& model) {
    size_t published = 0;

    state.autopilot.enabled.set(model.ap.enabled.value); ++published;
    state.autopilot.mode.set(pypilot_data_model::autopilot_mode_name(model.ap.mode.value)); ++published;
    state.autopilot.preferred_mode.set(pypilot_data_model::autopilot_mode_name(model.ap.preferred_mode.value)); ++published;
    state.autopilot.pilot.set(pypilot_data_model::pilot_name(model.ap.pilot.value)); ++published;

    if (publish_number_if_valid(model.ap.heading_command_deg, state.autopilot.heading_command)) ++published;
    if (publish_number_if_valid(model.ap.heading_deg, state.autopilot.heading)) ++published;
    if (publish_number_if_valid(model.ap.heading_error_deg, state.autopilot.heading_error)) ++published;

    if (publish_number_if_valid(model.imu.heading_deg, state.boatimu.heading)) ++published;
    if (publish_number_if_valid(model.imu.roll_deg, state.boatimu.roll)) ++published;
    if (publish_number_if_valid(model.imu.pitch_deg, state.boatimu.pitch)) ++published;
    if (publish_number_if_valid(model.imu.heel_deg, state.boatimu.heel)) ++published;
    if (publish_number_if_valid(model.imu.heading_rate_deg_s, state.boatimu.headingrate)) ++published;
    if (publish_number_if_valid(model.imu.heading_lowpass_deg, state.boatimu.heading_lowpass)) ++published;
    if (model.imu.alignment_counter.value >= 0) { state.boatimu.alignment_counter.set(static_cast<double>(model.imu.alignment_counter.value)); ++published; }
    if (publish_number_if_valid(model.imu.uptime_s, state.boatimu.uptime)) ++published;

    state.servo.engaged.set(model.servo.engaged.value); ++published;
    if (model.servo.command_norm.valid) { state.servo.command.set(model.servo.command_norm.value); ++published; }
    if (publish_number_if_valid(model.servo.voltage_v, state.servo.voltage)) ++published;
    if (publish_number_if_valid(model.servo.current_a, state.servo.current)) ++published;
    if (publish_number_if_valid(model.servo.position_deg, state.servo.position)) ++published;
    if (publish_number_if_valid(model.servo.controller_temp_c, state.servo.controller_temp)) ++published;
    if (publish_number_if_valid(model.servo.motor_temp_c, state.servo.motor_temp)) ++published;
    if (model.servo.amp_hours_ah.value != 0.0f) { state.servo.amp_hours.set(model.servo.amp_hours_ah.value); ++published; }

    if (publish_number_if_valid(model.navigation.gps.speed_kn, state.gps.speed)) ++published;
    if (publish_number_if_valid(model.navigation.gps.track_deg, state.gps.track)) ++published;
    if (publish_number_if_valid(model.navigation.gps.timestamp_s, state.gps.timestamp)) ++published;
    state.gps.source.set(pypilot_data_model::sensor_source_name(model.navigation.gps.source.value)); ++published;

    if (publish_number_if_valid(model.wind.apparent.direction_deg, state.wind.direction)) ++published;
    if (publish_number_if_valid(model.wind.apparent.speed_kn, state.wind.speed)) ++published;
    state.wind.source.set(pypilot_data_model::sensor_source_name(model.wind.apparent.source.value)); ++published;

    if (publish_number_if_valid(model.wind.truewind.direction_deg, state.sensors.truewind_direction)) ++published;
    if (publish_number_if_valid(model.wind.truewind.speed_kn, state.sensors.truewind_speed)) ++published;
    state.sensors.truewind_source.set(pypilot_data_model::sensor_source_name(model.wind.truewind.source.value)); ++published;

    if (publish_number_if_valid(model.water.speed_kn, state.sensors.water_speed)) ++published;
    if (publish_number_if_valid(model.rudder.angle_deg, state.sensors.rudder_angle)) ++published;

    state.pilots.tack_state.set(static_cast<double>(model.tack.state.value)); ++published;
    state.pilots.tack_direction.set(static_cast<double>(model.tack.direction.value)); ++published;

    state.sensors.profile_name.set(model.server.profile_name); ++published;
    state.sensors.server_version.set(model.server.version); ++published;
    if (publish_number_if_valid(model.server.uptime_s, state.sensors.server_uptime)) ++published;
    state.sensors.status_faults.set(static_cast<double>(model.status.faults.value)); ++published;
    state.sensors.status_warnings.set(static_cast<double>(model.status.warnings.value)); ++published;
    if (model.runtime_publication.published_value_count.valid) {
        state.sensors.runtime_published_value_count.set(static_cast<double>(model.runtime_publication.published_value_count.value));
        ++published;
    }

    return published;
}

} // namespace pypilot_runtime
