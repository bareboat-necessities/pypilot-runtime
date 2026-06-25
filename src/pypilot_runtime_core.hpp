#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pypilot_data_model.hpp>

namespace pypilot_runtime {

using PypilotRuntimeState = pypilot_data_model::DataModel<float>;
using pypilot_data_model::FieldId;

enum class RuntimeProtocolValueType : uint8_t {
    Boolean,
    Number,
    Integer,
    String,
    Mode,
    PilotName
};

enum class RuntimeProtocolValueScope : uint8_t {
    Telemetry,
    Command,
    Setting,
    Status,
    Fault
};

struct RuntimeProtocolValue {
    FieldId field_id;
    const char* name;
    RuntimeProtocolValueType type;
    RuntimeProtocolValueScope scope;
    bool writable;
    bool persistent;
};

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

static inline const char* runtime_protocol_type_name(RuntimeProtocolValueType type) {
    switch (type) {
    case RuntimeProtocolValueType::Boolean: return "bool";
    case RuntimeProtocolValueType::Number: return "number";
    case RuntimeProtocolValueType::Integer: return "number";
    case RuntimeProtocolValueType::String: return "string";
    case RuntimeProtocolValueType::Mode: return "string";
    case RuntimeProtocolValueType::PilotName: return "string";
    }
    return "unknown";
}

#define PYPILOT_RUNTIME_PROTOCOL_VALUES(X) \
    X(FieldId::ap_enabled, "ap.enabled", Boolean, Command, true, true) \
    X(FieldId::ap_mode, "ap.mode", Mode, Command, true, true) \
    X(FieldId::ap_preferred_mode, "ap.preferred_mode", Mode, Setting, true, true) \
    X(FieldId::ap_pilot, "ap.pilot", PilotName, Setting, true, true) \
    X(FieldId::ap_heading_command_deg, "ap.heading_command", Number, Command, true, false) \
    X(FieldId::ap_heading_deg, "ap.heading", Number, Telemetry, false, false) \
    X(FieldId::ap_heading_error_deg, "ap.heading_error", Number, Telemetry, false, false) \
    X(FieldId::imu_heading_deg, "imu.heading", Number, Telemetry, false, false) \
    X(FieldId::imu_heading_lowpass_deg, "imu.heading_lowpass", Number, Telemetry, false, false) \
    X(FieldId::imu_pitch_deg, "imu.pitch", Number, Telemetry, false, false) \
    X(FieldId::imu_roll_deg, "imu.roll", Number, Telemetry, false, false) \
    X(FieldId::imu_heel_deg, "imu.heel", Number, Telemetry, false, false) \
    X(FieldId::imu_headingrate_deg_s, "imu.headingrate", Number, Telemetry, false, false) \
    X(FieldId::imu_headingrate_lowpass_deg_s, "imu.headingrate_lowpass", Number, Telemetry, false, false) \
    X(FieldId::gps_source, "gps.source", String, Telemetry, false, false) \
    X(FieldId::gps_speed_kn, "gps.speed", Number, Telemetry, false, false) \
    X(FieldId::gps_track_deg, "gps.track", Number, Telemetry, false, false) \
    X(FieldId::gps_timestamp_s, "gps.timestamp", Number, Telemetry, false, false) \
    X(FieldId::wind_source, "wind.source", String, Telemetry, false, false) \
    X(FieldId::wind_speed_kn, "wind.speed", Number, Telemetry, false, false) \
    X(FieldId::wind_direction_deg, "wind.direction", Number, Telemetry, false, false) \
    X(FieldId::truewind_source, "truewind.source", String, Telemetry, false, false) \
    X(FieldId::truewind_speed_kn, "truewind.speed", Number, Telemetry, false, false) \
    X(FieldId::truewind_direction_deg, "truewind.direction", Number, Telemetry, false, false) \
    X(FieldId::water_speed_kn, "water.speed", Number, Telemetry, false, false) \
    X(FieldId::rudder_angle_deg, "rudder.angle", Number, Telemetry, false, false) \
    X(FieldId::servo_engaged, "servo.engaged", Boolean, Command, true, true) \
    X(FieldId::servo_command_norm, "servo.command", Number, Command, true, false) \
    X(FieldId::servo_position_deg, "servo.position", Number, Telemetry, false, false) \
    X(FieldId::servo_current_a, "servo.current", Number, Telemetry, false, false) \
    X(FieldId::servo_voltage_v, "servo.voltage", Number, Telemetry, false, false) \
    X(FieldId::servo_controller_temp_c, "servo.controller_temp", Number, Telemetry, false, false) \
    X(FieldId::servo_motor_temp_c, "servo.motor_temp", Number, Telemetry, false, false) \
    X(FieldId::servo_flags, "servo.flags", Integer, Fault, false, false) \
    X(FieldId::ap_tack_state, "ap.tack.state", Integer, Command, true, false) \
    X(FieldId::ap_tack_direction, "ap.tack.direction", Integer, Command, true, false) \
    X(FieldId::server_profile_name, "profile.name", String, Setting, true, true) \
    X(FieldId::server_version, "server.version", String, Status, false, false) \
    X(FieldId::server_uptime_s, "server.uptime", Number, Status, false, false) \
    X(FieldId::status_faults, "status.faults", Integer, Fault, false, false) \
    X(FieldId::status_warnings, "status.warnings", Integer, Status, false, false) \
    X(FieldId::runtime_published_value_count, "runtime.published_value_count", Integer, Status, false, false)

#define PYPILOT_RUNTIME_PROTOCOL_ROW(FIELD, NAME, TYPE, SCOPE, WRITABLE, PERSISTENT) \
    {FIELD, NAME, RuntimeProtocolValueType::TYPE, RuntimeProtocolValueScope::SCOPE, WRITABLE, PERSISTENT},

static inline const RuntimeProtocolValue* runtime_protocol_values(size_t& count) {
    static const RuntimeProtocolValue values[] = {
        PYPILOT_RUNTIME_PROTOCOL_VALUES(PYPILOT_RUNTIME_PROTOCOL_ROW)
    };
    count = sizeof(values) / sizeof(values[0]);
    return values;
}

#undef PYPILOT_RUNTIME_PROTOCOL_ROW

static inline const RuntimeProtocolValue* find_runtime_protocol_value(const char* name) {
    if (!name) return nullptr;
    size_t count = 0;
    const RuntimeProtocolValue* values = runtime_protocol_values(count);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(values[i].name, name) == 0) return &values[i];
    }
    return nullptr;
}

static inline bool append_json_value(char* out, size_t out_size, const RuntimeProtocolValue& value) {
    char item[192]{};
    snprintf(item, sizeof(item), "\"%s\":{\"type\":\"%s\",\"writable\":%s}",
             value.name,
             runtime_protocol_type_name(value.type),
             value.writable ? "true" : "false");
    if (out[0] != '\0' && out[strlen(out) - 1] != '{') {
        if (!append_cstr(out, out_size, ",")) return false;
    }
    return append_cstr(out, out_size, item);
}

static inline bool format_runtime_value(const PypilotRuntimeState& state,
                                        const RuntimeProtocolValue& value,
                                        char* out,
                                        size_t out_size) {
    switch (value.type) {
    case RuntimeProtocolValueType::Boolean: {
        bool b = false;
        pypilot_data_model::read_bool(state, value.field_id, b);
        return snprintf(out, out_size, "%s=%s\n", value.name, b ? "true" : "false") > 0;
    }
    case RuntimeProtocolValueType::Number:
    case RuntimeProtocolValueType::Integer: {
        float n = 0.0f;
        pypilot_data_model::read_number(state, value.field_id, n);
        return snprintf(out, out_size, "%s=%.4f\n", value.name, static_cast<double>(n)) > 0;
    }
    case RuntimeProtocolValueType::String: {
        const char* text = "";
        if (pypilot_data_model::read_string(state, value.field_id, text)) {
            return snprintf(out, out_size, "%s=\"%s\"\n", value.name, text) > 0;
        }
        pypilot_data_model::SensorSource source{};
        if (pypilot_data_model::read_source(state, value.field_id, source)) {
            return snprintf(out, out_size, "%s=\"%s\"\n", value.name, pypilot_data_model::sensor_source_name(source)) > 0;
        }
        return snprintf(out, out_size, "%s=\"\"\n", value.name) > 0;
    }
    case RuntimeProtocolValueType::Mode: {
        pypilot_data_model::AutopilotMode mode{};
        pypilot_data_model::read_mode(state, value.field_id, mode);
        return snprintf(out, out_size, "%s=\"%s\"\n", value.name, pypilot_data_model::autopilot_mode_name(mode)) > 0;
    }
    case RuntimeProtocolValueType::PilotName: {
        pypilot_data_model::PilotName pilot{};
        pypilot_data_model::read_pilot(state, value.field_id, pilot);
        return snprintf(out, out_size, "%s=\"%s\"\n", value.name, pypilot_data_model::pilot_name(pilot)) > 0;
    }
    }
    return false;
}

static inline bool apply_runtime_set(PypilotRuntimeState& state,
                                     const RuntimeProtocolValue& value,
                                     const char* payload,
                                     char* error,
                                     size_t error_size) {
    switch (value.type) {
    case RuntimeProtocolValueType::Boolean: {
        bool parsed = false;
        if (!parse_bool_text(payload, parsed)) { snprintf(error, error_size, "error=bad bool\n"); return false; }
        if (!pypilot_data_model::apply_bool(state, value.field_id, parsed, 0)) { snprintf(error, error_size, "error=unsupported bool %s\n", value.name); return false; }
        return true;
    }
    case RuntimeProtocolValueType::Number:
    case RuntimeProtocolValueType::Integer: {
        double parsed = 0.0;
        if (!parse_number_text(payload, parsed)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        if (!pypilot_data_model::apply_number(state, value.field_id, static_cast<float>(parsed), 0)) { snprintf(error, error_size, "error=unsupported number %s\n", value.name); return false; }
        return true;
    }
    case RuntimeProtocolValueType::String: {
        char text[128]{};
        if (!strip_optional_quotes(payload, text, sizeof(text))) { snprintf(error, error_size, "error=bad string\n"); return false; }
        if (!pypilot_data_model::apply_string(state, value.field_id, text, 0)) { snprintf(error, error_size, "error=unsupported string %s\n", value.name); return false; }
        return true;
    }
    case RuntimeProtocolValueType::Mode: {
        char text[32]{};
        pypilot_data_model::AutopilotMode mode{};
        if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::autopilot_mode_from_name(text, mode)) { snprintf(error, error_size, "error=bad autopilot mode\n"); return false; }
        if (!pypilot_data_model::apply_mode(state, value.field_id, mode, 0)) { snprintf(error, error_size, "error=unsupported mode %s\n", value.name); return false; }
        return true;
    }
    case RuntimeProtocolValueType::PilotName: {
        char text[32]{};
        pypilot_data_model::PilotName pilot{};
        if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::pilot_from_name(text, pilot)) { snprintf(error, error_size, "error=bad pilot\n"); return false; }
        if (!pypilot_data_model::apply_pilot(state, value.field_id, pilot, 0)) { snprintf(error, error_size, "error=unsupported pilot %s\n", value.name); return false; }
        return true;
    }
    }
    return false;
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool value_exists(const char* name) const {
        return find_runtime_protocol_value(name) != nullptr;
    }

    bool writable(const char* name) const {
        const RuntimeProtocolValue* value = find_runtime_protocol_value(name);
        return value && value->writable;
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const RuntimeProtocolValue* value = find_runtime_protocol_value(name);
        if (!value) { snprintf(error, error_size, "error=unknown value %s\n", name ? name : ""); return false; }
        if (!value->writable) { snprintf(error, error_size, "error=value not writable %s\n", value->name); return false; }
        return apply_runtime_set(state_, *value, payload, error, error_size);
    }

    bool format_value(const char* name, char* out, size_t out_size) const {
        const RuntimeProtocolValue* value = find_runtime_protocol_value(name);
        if (!value) return false;
        return format_runtime_value(state_, *value, out, out_size);
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!out || out_size == 0) return false;
        out[0] = '\0';
        if (!append_cstr(out, out_size, "values={")) return false;
        size_t count = 0;
        const RuntimeProtocolValue* values = runtime_protocol_values(count);
        for (size_t i = 0; i < count; ++i) {
            if (!append_json_value(out, out_size, values[i])) return false;
        }
        return append_cstr(out, out_size, "}\n");
    }

private:
    PypilotRuntimeState& state_;
};

#undef PYPILOT_RUNTIME_PROTOCOL_VALUES

} // namespace pypilot_runtime
