#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pypilot_data_model.hpp>

namespace pypilot_runtime {

using PypilotRuntimeState = pypilot_data_model::DataModel<float>;

static inline bool copy_cstr(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) return false;
    if (!src) { dst[0] = '\0'; return true; }
    size_t i = 0;
    for (; i + 1 < dst_size && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
    return src[i] == '\0';
}

static inline bool append_cstr(char* dst, size_t dst_size, const char* src) {
    if (!dst || !src || dst_size == 0) return false;
    const size_t used = strlen(dst);
    if (used >= dst_size) return false;
    return copy_cstr(dst + used, dst_size - used, src);
}

static inline bool parse_bool_text(const char* text, bool& out) {
    if (!text) return false;
    if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) { out = true; return true; }
    if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) { out = false; return true; }
    return false;
}

static inline bool parse_number_text(const char* text, double& out) {
    if (!text || !*text) return false;
    char* end = nullptr;
    const double value = strtod(text, &end);
    if (!end || (*end != '\0' && *end != '}' && *end != ',')) return false;
    out = value;
    return true;
}

static inline bool strip_optional_quotes(const char* text, char* out, size_t out_size) {
    if (!text || !out || out_size == 0) return false;
    const size_t len = strlen(text);
    if (len >= 2 && text[0] == '"' && text[len - 1] == '"') {
        const size_t copy_len = len - 2;
        if (copy_len + 1 > out_size) return false;
        memcpy(out, text + 1, copy_len);
        out[copy_len] = '\0';
        return true;
    }
    return copy_cstr(out, out_size, text);
}

enum class RuntimeValueType : uint8_t {
    Bool,
    Number,
    String
};

static inline const char* runtime_value_type_name(RuntimeValueType type) {
    switch (type) {
    case RuntimeValueType::Bool: return "bool";
    case RuntimeValueType::Number: return "number";
    case RuntimeValueType::String: return "string";
    }
    return "unknown";
}

using RuntimeValueFormatter = bool (*)(const PypilotRuntimeState& state, char* out, size_t out_size);
using RuntimeValueSetter = bool (*)(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size);

struct RuntimeValueDescriptor {
    const char* name;
    RuntimeValueType type;
    bool writable;
    RuntimeValueFormatter format;
    RuntimeValueSetter set;
};

static inline bool format_bool_value(const char* name, bool value, char* out, size_t out_size) {
    return snprintf(out, out_size, "%s=%s\n", name, value ? "true" : "false") > 0;
}

static inline bool format_string_value(const char* name, const char* value, char* out, size_t out_size) {
    return snprintf(out, out_size, "%s=\"%s\"\n", name, value ? value : "") > 0;
}

template<typename T>
static inline bool format_stamped_value(const char* name,
                                        const pypilot_data_model::Stamped<T>& stamped,
                                        char* out,
                                        size_t out_size) {
    const double value = stamped.valid ? static_cast<double>(stamped.value) : 0.0;
    return snprintf(out, out_size, "%s=%.4f\n", name, value) > 0;
}

template<typename T>
static inline bool format_command_value(const char* name,
                                        const pypilot_data_model::TimedCommand<T>& command,
                                        char* out,
                                        size_t out_size) {
    const double value = command.valid ? static_cast<double>(command.value) : 0.0;
    return snprintf(out, out_size, "%s=%.4f\n", name, value) > 0;
}

static inline bool set_bool_value(bool& target, const char* payload, char* error, size_t error_size) {
    bool parsed = false;
    if (!parse_bool_text(payload, parsed)) {
        snprintf(error, error_size, "error=bad bool\n");
        return false;
    }
    target = parsed;
    return true;
}

static inline bool set_ap_enabled(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    return set_bool_value(state.ap.enabled.value, payload, error, error_size);
}

static inline bool set_servo_engaged(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    return set_bool_value(state.servo.engaged.value, payload, error, error_size);
}

static inline bool set_ap_mode_value(pypilot_data_model::Setting<pypilot_data_model::AutopilotMode>& target,
                                     const char* payload,
                                     char* error,
                                     size_t error_size) {
    char text[32]{};
    pypilot_data_model::AutopilotMode mode{};
    if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::autopilot_mode_from_name(text, mode)) {
        snprintf(error, error_size, "error=bad autopilot mode\n");
        return false;
    }
    target.value = mode;
    return true;
}

static inline bool set_ap_mode(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    return set_ap_mode_value(state.ap.mode, payload, error, error_size);
}

static inline bool set_ap_preferred_mode(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    return set_ap_mode_value(state.ap.preferred_mode, payload, error, error_size);
}

static inline bool set_ap_pilot(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    char text[32]{};
    pypilot_data_model::PilotName pilot{};
    if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::pilot_from_name(text, pilot)) {
        snprintf(error, error_size, "error=bad pilot\n");
        return false;
    }
    state.ap.pilot.value = pilot;
    return true;
}

static inline bool set_profile_name(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    char text[64]{};
    if (!strip_optional_quotes(payload, text, sizeof(text))) {
        snprintf(error, error_size, "error=bad profile name\n");
        return false;
    }
    return copy_cstr(state.server.profile_name, sizeof(state.server.profile_name), text);
}

static inline bool parse_runtime_number(const char* payload, double& value, char* error, size_t error_size) {
    if (!parse_number_text(payload, value)) {
        snprintf(error, error_size, "error=bad number\n");
        return false;
    }
    return true;
}

static inline bool set_ap_heading_command(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    double value = 0.0;
    if (!parse_runtime_number(payload, value, error, error_size)) return false;
    state.ap.heading_command_deg.set(static_cast<float>(value), 0);
    return true;
}

static inline bool set_servo_command(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    double value = 0.0;
    if (!parse_runtime_number(payload, value, error, error_size)) return false;
    state.servo.command_norm.set_external(static_cast<float>(value), 0);
    return true;
}

static inline bool set_tack_state(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    double value = 0.0;
    if (!parse_runtime_number(payload, value, error, error_size)) return false;
    state.tack.state.value = static_cast<pypilot_data_model::TackState>(static_cast<int>(value));
    return true;
}

static inline bool set_tack_direction(PypilotRuntimeState& state, const char* payload, char* error, size_t error_size) {
    double value = 0.0;
    if (!parse_runtime_number(payload, value, error, error_size)) return false;
    state.tack.direction.value = static_cast<pypilot_data_model::TackDirection>(static_cast<int>(value));
    return true;
}

static inline bool format_ap_enabled(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_bool_value("ap.enabled", state.ap.enabled.value, out, out_size); }
static inline bool format_ap_mode(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("ap.mode", pypilot_data_model::autopilot_mode_name(state.ap.mode.value), out, out_size); }
static inline bool format_ap_preferred_mode(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("ap.preferred_mode", pypilot_data_model::autopilot_mode_name(state.ap.preferred_mode.value), out, out_size); }
static inline bool format_ap_pilot(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("ap.pilot", pypilot_data_model::pilot_name(state.ap.pilot.value), out, out_size); }
static inline bool format_ap_heading_command(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("ap.heading_command", state.ap.heading_command_deg, out, out_size); }
static inline bool format_ap_heading(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("ap.heading", state.ap.heading_deg, out, out_size); }
static inline bool format_ap_heading_error(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("ap.heading_error", state.ap.heading_error_deg, out, out_size); }
static inline bool format_imu_heading(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.heading", state.imu.heading_deg, out, out_size); }
static inline bool format_imu_roll(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.roll", state.imu.roll_deg, out, out_size); }
static inline bool format_imu_pitch(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.pitch", state.imu.pitch_deg, out, out_size); }
static inline bool format_imu_heel(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.heel", state.imu.heel_deg, out, out_size); }
static inline bool format_imu_headingrate(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.headingrate", state.imu.heading_rate_deg_s, out, out_size); }
static inline bool format_imu_heading_lowpass(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("imu.heading_lowpass", state.imu.heading_lowpass_deg, out, out_size); }
static inline bool format_servo_command(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_command_value("servo.command", state.servo.command_norm, out, out_size); }
static inline bool format_servo_engaged(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_bool_value("servo.engaged", state.servo.engaged.value, out, out_size); }
static inline bool format_servo_state(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("servo.state", state.servo.engaged.value ? "active" : "disabled", out, out_size); }
static inline bool format_servo_voltage(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("servo.voltage", state.servo.voltage_v, out, out_size); }
static inline bool format_servo_current(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("servo.current", state.servo.current_a, out, out_size); }
static inline bool format_servo_position(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("servo.position", state.servo.position_deg, out, out_size); }
static inline bool format_gps_speed(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("gps.speed", state.navigation.gps.speed_kn, out, out_size); }
static inline bool format_gps_track(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("gps.track", state.navigation.gps.track_deg, out, out_size); }
static inline bool format_gps_timestamp(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("gps.timestamp", state.navigation.gps.timestamp_s, out, out_size); }
static inline bool format_gps_source(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("gps.source", pypilot_data_model::sensor_source_name(state.navigation.gps.source.value), out, out_size); }
static inline bool format_wind_direction(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("wind.direction", state.wind.apparent.direction_deg, out, out_size); }
static inline bool format_wind_speed(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("wind.speed", state.wind.apparent.speed_kn, out, out_size); }
static inline bool format_wind_source(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("wind.source", pypilot_data_model::sensor_source_name(state.wind.apparent.source.value), out, out_size); }
static inline bool format_truewind_direction(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("truewind.direction", state.wind.truewind.direction_deg, out, out_size); }
static inline bool format_truewind_speed(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("truewind.speed", state.wind.truewind.speed_kn, out, out_size); }
static inline bool format_truewind_source(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("truewind.source", pypilot_data_model::sensor_source_name(state.wind.truewind.source.value), out, out_size); }
static inline bool format_water_speed(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("water.speed", state.water.speed_kn, out, out_size); }
static inline bool format_rudder_angle(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("rudder.angle", state.rudder.angle_deg, out, out_size); }
static inline bool format_tack_state(const PypilotRuntimeState& state, char* out, size_t out_size) { return snprintf(out, out_size, "ap.tack.state=%u\n", static_cast<unsigned>(state.tack.state.value)) > 0; }
static inline bool format_tack_direction(const PypilotRuntimeState& state, char* out, size_t out_size) { return snprintf(out, out_size, "ap.tack.direction=%u\n", static_cast<unsigned>(state.tack.direction.value)) > 0; }
static inline bool format_profile_name(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("profile.name", state.server.profile_name, out, out_size); }
static inline bool format_server_version(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_string_value("server.version", state.server.version, out, out_size); }
static inline bool format_server_uptime(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("server.uptime", state.server.uptime_s, out, out_size); }
static inline bool format_status_faults(const PypilotRuntimeState& state, char* out, size_t out_size) { return snprintf(out, out_size, "status.faults=%u\n", state.status.faults.value) > 0; }
static inline bool format_status_warnings(const PypilotRuntimeState& state, char* out, size_t out_size) { return snprintf(out, out_size, "status.warnings=%u\n", state.status.warnings.value) > 0; }
static inline bool format_runtime_published_count(const PypilotRuntimeState& state, char* out, size_t out_size) { return format_stamped_value("runtime.published_value_count", state.runtime_publication.published_value_count, out, out_size); }

static inline const RuntimeValueDescriptor* runtime_value_descriptors() {
    static const RuntimeValueDescriptor values[] = {
        {"ap.enabled", RuntimeValueType::Bool, true, format_ap_enabled, set_ap_enabled},
        {"ap.mode", RuntimeValueType::String, true, format_ap_mode, set_ap_mode},
        {"ap.preferred_mode", RuntimeValueType::String, true, format_ap_preferred_mode, set_ap_preferred_mode},
        {"ap.pilot", RuntimeValueType::String, true, format_ap_pilot, set_ap_pilot},
        {"ap.heading_command", RuntimeValueType::Number, true, format_ap_heading_command, set_ap_heading_command},
        {"ap.heading", RuntimeValueType::Number, false, format_ap_heading, nullptr},
        {"ap.heading_error", RuntimeValueType::Number, false, format_ap_heading_error, nullptr},
        {"imu.heading", RuntimeValueType::Number, false, format_imu_heading, nullptr},
        {"imu.roll", RuntimeValueType::Number, false, format_imu_roll, nullptr},
        {"imu.pitch", RuntimeValueType::Number, false, format_imu_pitch, nullptr},
        {"imu.heel", RuntimeValueType::Number, false, format_imu_heel, nullptr},
        {"imu.headingrate", RuntimeValueType::Number, false, format_imu_headingrate, nullptr},
        {"imu.heading_lowpass", RuntimeValueType::Number, false, format_imu_heading_lowpass, nullptr},
        {"servo.command", RuntimeValueType::Number, true, format_servo_command, set_servo_command},
        {"servo.engaged", RuntimeValueType::Bool, true, format_servo_engaged, set_servo_engaged},
        {"servo.state", RuntimeValueType::String, false, format_servo_state, nullptr},
        {"servo.voltage", RuntimeValueType::Number, false, format_servo_voltage, nullptr},
        {"servo.current", RuntimeValueType::Number, false, format_servo_current, nullptr},
        {"servo.position", RuntimeValueType::Number, false, format_servo_position, nullptr},
        {"gps.speed", RuntimeValueType::Number, false, format_gps_speed, nullptr},
        {"gps.track", RuntimeValueType::Number, false, format_gps_track, nullptr},
        {"gps.timestamp", RuntimeValueType::Number, false, format_gps_timestamp, nullptr},
        {"gps.source", RuntimeValueType::String, false, format_gps_source, nullptr},
        {"wind.direction", RuntimeValueType::Number, false, format_wind_direction, nullptr},
        {"wind.speed", RuntimeValueType::Number, false, format_wind_speed, nullptr},
        {"wind.source", RuntimeValueType::String, false, format_wind_source, nullptr},
        {"truewind.direction", RuntimeValueType::Number, false, format_truewind_direction, nullptr},
        {"truewind.speed", RuntimeValueType::Number, false, format_truewind_speed, nullptr},
        {"truewind.source", RuntimeValueType::String, false, format_truewind_source, nullptr},
        {"water.speed", RuntimeValueType::Number, false, format_water_speed, nullptr},
        {"rudder.angle", RuntimeValueType::Number, false, format_rudder_angle, nullptr},
        {"ap.tack.state", RuntimeValueType::Number, true, format_tack_state, set_tack_state},
        {"ap.tack.direction", RuntimeValueType::Number, true, format_tack_direction, set_tack_direction},
        {"profile.name", RuntimeValueType::String, true, format_profile_name, set_profile_name},
        {"server.version", RuntimeValueType::String, false, format_server_version, nullptr},
        {"server.uptime", RuntimeValueType::Number, false, format_server_uptime, nullptr},
        {"status.faults", RuntimeValueType::Number, false, format_status_faults, nullptr},
        {"status.warnings", RuntimeValueType::Number, false, format_status_warnings, nullptr},
        {"runtime.published_value_count", RuntimeValueType::Number, false, format_runtime_published_count, nullptr}
    };
    return values;
}

static inline size_t runtime_value_descriptor_count() {
    return 39u;
}

static inline const RuntimeValueDescriptor* find_runtime_value_descriptor(const char* name) {
    if (!name) return nullptr;
    const RuntimeValueDescriptor* values = runtime_value_descriptors();
    for (size_t i = 0; i < runtime_value_descriptor_count(); ++i) {
        if (strcmp(values[i].name, name) == 0) return &values[i];
    }
    return nullptr;
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool value_exists(const char* name) const {
        return find_runtime_value_descriptor(name) != nullptr;
    }

    bool writable(const char* name) const {
        const RuntimeValueDescriptor* descriptor = find_runtime_value_descriptor(name);
        return descriptor && descriptor->writable;
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const RuntimeValueDescriptor* descriptor = find_runtime_value_descriptor(name);
        if (!descriptor) {
            snprintf(error, error_size, "error=unknown value %s\n", name ? name : "");
            return false;
        }
        if (!descriptor->writable || !descriptor->set) {
            snprintf(error, error_size, "error=value not writable %s\n", descriptor->name);
            return false;
        }
        return descriptor->set(state_, payload, error, error_size);
    }

    bool format_value(const char* name, char* out, size_t out_size) const {
        const RuntimeValueDescriptor* descriptor = find_runtime_value_descriptor(name);
        if (!descriptor || !descriptor->format) return false;
        return descriptor->format(state_, out, out_size);
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!out || out_size == 0) return false;
        out[0] = '\0';
        if (!append_cstr(out, out_size, "values={")) return false;
        const RuntimeValueDescriptor* values = runtime_value_descriptors();
        for (size_t i = 0; i < runtime_value_descriptor_count(); ++i) {
            if (!append_json_value(out,
                                   out_size,
                                   values[i].name,
                                   runtime_value_type_name(values[i].type),
                                   values[i].writable)) {
                return false;
            }
        }
        return append_cstr(out, out_size, "}\n");
    }

private:
    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
