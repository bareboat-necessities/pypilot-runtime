#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ship_data_model.hpp>

namespace pypilot_runtime {

using PypilotRuntimeState = ship_data_model::DataModel<float>;
using RuntimeFieldMeta = ship_data_model::FieldMeta;
using RuntimeFieldType = ship_data_model::FieldType;

static inline const RuntimeFieldMeta* find_runtime_value(const char* name) {
    return ship_data_model::field_meta_from_name(name);
}

static inline const char* runtime_field_type_name(RuntimeFieldType type) {
    switch (type) {
    case RuntimeFieldType::number: return "number";
    case RuntimeFieldType::integer: return "integer";
    case RuntimeFieldType::boolean: return "boolean";
    case RuntimeFieldType::string_value: return "string";
    case RuntimeFieldType::json_value: return "object";
    case RuntimeFieldType::mode_enum: return "mode";
    case RuntimeFieldType::pilot_enum: return "pilot";
    case RuntimeFieldType::source_enum: return "source";
    case RuntimeFieldType::rudder_calibration_enum: return "rudder_calibration_state";
    case RuntimeFieldType::servo_state_enum: return "servo_state";
    case RuntimeFieldType::tack_state_enum: return "tack_state";
    case RuntimeFieldType::tack_direction_enum: return "tack_direction";
    case RuntimeFieldType::vector3_value: return "vector3";
    case RuntimeFieldType::quaternion_value: return "quaternion";
    case RuntimeFieldType::unknown_value: return "unknown";
    }
    return "unknown";
}

static inline bool runtime_append_text(char* out, size_t out_size, const char* text) {
    if (!out || out_size == 0 || !text) return false;
    const size_t used = strlen(out);
    const size_t add = strlen(text);
    if (used + add + 1 > out_size) return false;
    memcpy(out + used, text, add + 1);
    return true;
}

static inline bool runtime_copy_text(char* out, size_t out_size, const char* text, size_t text_len) {
    if (!out || out_size == 0) return false;
    const size_t n = text_len < out_size - 1 ? text_len : out_size - 1;
    if (text && n) memcpy(out, text, n);
    out[n] = '\0';
    return text_len < out_size;
}

static inline bool runtime_strip_optional_quotes(const char* payload, char* out, size_t out_size) {
    if (!payload || !out || out_size == 0) return false;
    while (*payload == ' ' || *payload == '\t') ++payload;
    if (*payload == '"') {
        ++payload;
        const char* end = payload;
        while (*end && *end != '"') ++end;
        return runtime_copy_text(out, out_size, payload, static_cast<size_t>(end - payload));
    }
    const char* end = payload + strlen(payload);
    while (end > payload && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) --end;
    return runtime_copy_text(out, out_size, payload, static_cast<size_t>(end - payload));
}

static inline bool runtime_parse_bool(const char* payload, bool& out) {
    char text[16]{};
    if (!runtime_strip_optional_quotes(payload, text, sizeof(text))) return false;
    if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) { out = true; return true; }
    if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) { out = false; return true; }
    return false;
}

static inline bool runtime_parse_number(const char* payload, double& out) {
    if (!payload) return false;
    char* end = nullptr;
    out = strtod(payload, &end);
    return end && end != payload;
}

static inline const char* runtime_mode_name(ship_data_model::AutopilotMode mode) {
    switch (mode) {
    case ship_data_model::AutopilotMode::compass: return "compass";
    case ship_data_model::AutopilotMode::gps: return "gps";
    case ship_data_model::AutopilotMode::nav: return "nav";
    case ship_data_model::AutopilotMode::wind: return "wind";
    case ship_data_model::AutopilotMode::true_wind: return "true wind";
    }
    return "compass";
}

static inline bool runtime_mode_from_name(const char* name, ship_data_model::AutopilotMode& out) {
    if (!name) return false;
    if (strcmp(name, "compass") == 0) out = ship_data_model::AutopilotMode::compass;
    else if (strcmp(name, "gps") == 0) out = ship_data_model::AutopilotMode::gps;
    else if (strcmp(name, "nav") == 0) out = ship_data_model::AutopilotMode::nav;
    else if (strcmp(name, "wind") == 0) out = ship_data_model::AutopilotMode::wind;
    else if (strcmp(name, "true wind") == 0 || strcmp(name, "true_wind") == 0) out = ship_data_model::AutopilotMode::true_wind;
    else return false;
    return true;
}

static inline const char* runtime_pilot_name(ship_data_model::PilotName pilot) {
    switch (pilot) {
    case ship_data_model::PilotName::basic: return "basic";
    case ship_data_model::PilotName::absolute: return "absolute";
    case ship_data_model::PilotName::wind: return "wind";
    case ship_data_model::PilotName::gps: return "gps";
    case ship_data_model::PilotName::rate: return "rate";
    case ship_data_model::PilotName::simple: return "simple";
    case ship_data_model::PilotName::vmg: return "vmg";
    case ship_data_model::PilotName::deadzone: return "deadzone";
    case ship_data_model::PilotName::autotune: return "autotune";
    case ship_data_model::PilotName::fuzzy: return "fuzzy";
    case ship_data_model::PilotName::learning: return "learning";
    case ship_data_model::PilotName::intellect: return "intellect";
    }
    return "basic";
}

static inline const char* runtime_source_name(ship_data_model::SensorSource source) {
    switch (source) {
    case ship_data_model::SensorSource::gpsd: return "gpsd";
    case ship_data_model::SensorSource::servo: return "servo";
    case ship_data_model::SensorSource::serial: return "serial";
    case ship_data_model::SensorSource::tcp: return "tcp";
    case ship_data_model::SensorSource::signalk: return "signalk";
    case ship_data_model::SensorSource::water_wind: return "water_wind";
    case ship_data_model::SensorSource::gps_wind: return "gps_wind";
    case ship_data_model::SensorSource::none: return "none";
    }
    return "none";
}

static inline const char* runtime_servo_state_name(ship_data_model::ServoControllerState state) {
    switch (state) {
    case ship_data_model::ServoControllerState::disconnected: return "disconnected";
    case ship_data_model::ServoControllerState::idle: return "idle";
    case ship_data_model::ServoControllerState::engaged: return "engaged";
    case ship_data_model::ServoControllerState::fault: return "fault";
    }
    return "disconnected";
}

static inline bool runtime_read_number(const PypilotRuntimeState& state, const RuntimeFieldMeta& value, float& out) {
    if (ship_data_model::read_number(state, value.id, out)) return true;
    const char* name = value.pypilot_name;
    if (strcmp(name, "ap.heading_error") == 0) { out = state.ap.heading_error_deg.value; return state.ap.heading_error_deg.valid; }
    if (strcmp(name, "ap.heading_error_int") == 0) { out = state.ap.heading_error_int_deg.value; return state.ap.heading_error_int_deg.valid; }
    if (strcmp(name, "ap.heading_command") == 0) { out = state.ap.heading_command_deg.value; return state.ap.heading_command_deg.valid; }
    if (strcmp(name, "ap.heading_command_rate") == 0) { out = state.ap.heading_command_rate_deg_s.value; return state.ap.heading_command_rate_deg_s.valid; }
    if (strcmp(name, "ap.runtime") == 0) { out = state.ap.runtime_s.value; return state.ap.runtime_s.valid; }
    if (strcmp(name, "imu.heading") == 0) { out = state.imu.heading_deg.value; return state.imu.heading_deg.valid; }
    if (strcmp(name, "imu.heading_lowpass") == 0) { out = state.imu.heading_lowpass_deg.value; return state.imu.heading_lowpass_deg.valid; }
    if (strcmp(name, "imu.pitch") == 0) { out = state.imu.pitch_deg.value; return state.imu.pitch_deg.valid; }
    if (strcmp(name, "imu.roll") == 0) { out = state.imu.roll_deg.value; return state.imu.roll_deg.valid; }
    if (strcmp(name, "imu.headingrate") == 0) { out = state.imu.heading_rate_deg_s.value; return state.imu.heading_rate_deg_s.valid; }
    if (strcmp(name, "imu.headingrate_lowpass") == 0) { out = state.imu.heading_rate_lowpass_deg_s.value; return state.imu.heading_rate_lowpass_deg_s.valid; }
    if (strcmp(name, "imu.pitchrate") == 0) { out = state.imu.pitch_rate_deg_s.value; return state.imu.pitch_rate_deg_s.valid; }
    if (strcmp(name, "imu.rollrate") == 0) { out = state.imu.roll_rate_deg_s.value; return state.imu.roll_rate_deg_s.valid; }
    if (strcmp(name, "gps.track") == 0) { out = state.navigation.gps.track_deg.value; return state.navigation.gps.track_deg.valid; }
    if (strcmp(name, "gps.speed") == 0) { out = state.navigation.gps.speed_kn.value; return state.navigation.gps.speed_kn.valid; }
    if (strcmp(name, "wind.direction") == 0) { out = state.wind.apparent.direction_deg.value; return state.wind.apparent.direction_deg.valid; }
    if (strcmp(name, "wind.speed") == 0) { out = state.wind.apparent.speed_kn.value; return state.wind.apparent.speed_kn.valid; }
    if (strcmp(name, "truewind.direction") == 0) { out = state.wind.truewind.direction_deg.value; return state.wind.truewind.direction_deg.valid; }
    if (strcmp(name, "truewind.speed") == 0) { out = state.wind.truewind.speed_kn.value; return state.wind.truewind.speed_kn.valid; }
    if (strcmp(name, "water.speed") == 0) { out = state.water.speed_kn.value; return state.water.speed_kn.valid; }
    if (strcmp(name, "water.leeway") == 0) { out = state.water.leeway_deg.value; return state.water.leeway_deg.valid; }
    if (strcmp(name, "rudder.angle") == 0) { out = state.rudder.angle_deg.value; return state.rudder.angle_deg.valid; }
    if (strcmp(name, "servo.command") == 0) { out = state.servo.command_norm.value; return state.servo.command_norm.valid; }
    if (strcmp(name, "servo.position_command") == 0) { out = state.servo.position_command_deg.value; return state.servo.position_command_deg.valid; }
    if (strcmp(name, "servo.current") == 0) { out = state.servo.current_a.value; return state.servo.current_a.valid; }
    if (strcmp(name, "servo.voltage") == 0) { out = state.servo.voltage_v.value; return state.servo.voltage_v.valid; }
    if (strcmp(name, "servo.controller_temp") == 0) { out = state.servo.controller_temp_c.value; return state.servo.controller_temp_c.valid; }
    if (strcmp(name, "servo.motor_temp") == 0) { out = state.servo.motor_temp_c.value; return state.servo.motor_temp_c.valid; }
    out = 0.0f;
    return true;
}

static inline bool runtime_apply_number(PypilotRuntimeState& state, const RuntimeFieldMeta& value, float parsed, uint64_t now_us) {
    if (ship_data_model::apply_number(state, value.id, parsed, now_us)) return true;
    const char* name = value.pypilot_name;
    if (strcmp(name, "servo.command") == 0) { state.servo.command_norm.set_external(parsed, now_us); return true; }
    if (strcmp(name, "servo.position_command") == 0) { state.servo.position_command_deg.set_external(parsed, now_us); return true; }
    if (strcmp(name, "ap.heading_command") == 0) { state.ap.heading_command_deg.set(parsed, now_us); state.pilot_command.heading_command_deg.set_external(parsed, now_us); return true; }
    if (strcmp(name, "ap.wind_offset_filter") == 0) { state.ap.wind_offset_filter_0_1.value = parsed; return true; }
    if (strcmp(name, "gps.rate") == 0) { state.navigation.gps.rate_hz.value = parsed; return true; }
    if (strcmp(name, "wind.rate") == 0) { return true; }
    if (strcmp(name, "truewind.rate") == 0) { return true; }
    if (strcmp(name, "water.rate") == 0) { return true; }
    if (strcmp(name, "rudder.rate") == 0) { return true; }
    return false;
}

static inline bool runtime_read_bool(const PypilotRuntimeState& state, const RuntimeFieldMeta& value, bool& out) {
    if (ship_data_model::read_bool(state, value.id, out)) return true;
    const char* name = value.pypilot_name;
    if (strcmp(name, "ap.gps_and_nav_modes") == 0) { out = state.ap.gps_and_nav_modes.value; return true; }
    if (strcmp(name, "servo.engaged") == 0) { out = state.servo.engaged.value; return true; }
    out = false;
    return true;
}

static inline bool runtime_apply_bool(PypilotRuntimeState& state, const RuntimeFieldMeta& value, bool parsed, uint64_t now_us) {
    if (ship_data_model::apply_bool(state, value.id, parsed, now_us)) return true;
    const char* name = value.pypilot_name;
    if (strcmp(name, "ap.gps_and_nav_modes") == 0) { state.ap.gps_and_nav_modes.value = parsed; return true; }
    if (strcmp(name, "servo.engaged") == 0) { state.servo.engaged.value = parsed; return true; }
    return false;
}

static inline bool runtime_format_quoted(const char* name, const char* text, char* out, size_t out_size) {
    return snprintf(out, out_size, "%s=\"%s\"\n", name, text ? text : "") > 0;
}

static inline bool append_json_value(char* out, size_t out_size, const RuntimeFieldMeta& value) {
    char item[224]{};
    snprintf(item, sizeof(item), "\"%s\":{\"type\":\"%s\",\"writable\":%s}",
             value.pypilot_name,
             runtime_field_type_name(value.type),
             value.writable ? "true" : "false");
    if (out[0] != '\0' && out[strlen(out) - 1] != '{') {
        if (!runtime_append_text(out, out_size, ",")) return false;
    }
    return runtime_append_text(out, out_size, item);
}

static inline bool format_runtime_value(const PypilotRuntimeState& state,
                                        const RuntimeFieldMeta& value,
                                        char* out,
                                        size_t out_size) {
    switch (value.type) {
    case RuntimeFieldType::boolean: {
        bool b = false;
        runtime_read_bool(state, value, b);
        return snprintf(out, out_size, "%s=%s\n", value.pypilot_name, b ? "true" : "false") > 0;
    }
    case RuntimeFieldType::number:
    case RuntimeFieldType::integer: {
        float n = 0.0f;
        runtime_read_number(state, value, n);
        return snprintf(out, out_size, "%s=%.4f\n", value.pypilot_name, static_cast<double>(n)) > 0;
    }
    case RuntimeFieldType::string_value: {
        const char* text = "";
        if (ship_data_model::read_string(state, value.id, text)) return runtime_format_quoted(value.pypilot_name, text, out, out_size);
        if (strcmp(value.pypilot_name, "servo.state") == 0) return runtime_format_quoted(value.pypilot_name, runtime_servo_state_name(state.servo_telemetry.controller_state.value), out, out_size);
        return runtime_format_quoted(value.pypilot_name, "", out, out_size);
    }
    case RuntimeFieldType::mode_enum:
        return runtime_format_quoted(value.pypilot_name, runtime_mode_name(state.ap.mode.value), out, out_size);
    case RuntimeFieldType::pilot_enum:
        return runtime_format_quoted(value.pypilot_name, runtime_pilot_name(state.ap.pilot.value), out, out_size);
    case RuntimeFieldType::source_enum: {
        ship_data_model::SensorSource source = ship_data_model::SensorSource::none;
        if (strcmp(value.pypilot_name, "wind.source") == 0) source = state.wind.apparent.source.value;
        else if (strcmp(value.pypilot_name, "gps.source") == 0) source = state.navigation.gps.source.value;
        else if (strcmp(value.pypilot_name, "apb.source") == 0) source = state.navigation.apb.source.value;
        else if (strcmp(value.pypilot_name, "water.source") == 0) source = state.water.source.value;
        else if (strcmp(value.pypilot_name, "rudder.source") == 0) source = state.rudder.source.value;
        return runtime_format_quoted(value.pypilot_name, runtime_source_name(source), out, out_size);
    }
    case RuntimeFieldType::json_value:
        return snprintf(out, out_size, "%s={}\n", value.pypilot_name) > 0;
    case RuntimeFieldType::vector3_value:
        return snprintf(out, out_size, "%s=[0,0,0]\n", value.pypilot_name) > 0;
    case RuntimeFieldType::quaternion_value:
        return snprintf(out, out_size, "%s=[1,0,0,0]\n", value.pypilot_name) > 0;
    case RuntimeFieldType::rudder_calibration_enum:
    case RuntimeFieldType::servo_state_enum:
    case RuntimeFieldType::tack_state_enum:
    case RuntimeFieldType::tack_direction_enum:
    case RuntimeFieldType::unknown_value:
        return runtime_format_quoted(value.pypilot_name, "", out, out_size);
    }
    return false;
}

static inline bool apply_runtime_set(PypilotRuntimeState& state,
                                     const RuntimeFieldMeta& value,
                                     const char* payload,
                                     char* error,
                                     size_t error_size) {
    switch (value.type) {
    case RuntimeFieldType::boolean: {
        bool parsed = false;
        if (!runtime_parse_bool(payload, parsed)) { snprintf(error, error_size, "error=bad bool\n"); return false; }
        if (!runtime_apply_bool(state, value, parsed, 0)) { snprintf(error, error_size, "error=unsupported bool %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::number:
    case RuntimeFieldType::integer: {
        double parsed = 0.0;
        if (!runtime_parse_number(payload, parsed)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        if (!runtime_apply_number(state, value, static_cast<float>(parsed), 0)) { snprintf(error, error_size, "error=unsupported number %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::string_value: {
        char text[128]{};
        if (!runtime_strip_optional_quotes(payload, text, sizeof(text))) { snprintf(error, error_size, "error=bad string\n"); return false; }
        if (!ship_data_model::apply_string(state, value.id, text, 0)) { snprintf(error, error_size, "error=unsupported string %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::mode_enum: {
        char text[32]{};
        ship_data_model::AutopilotMode mode{};
        if (!runtime_strip_optional_quotes(payload, text, sizeof(text)) || !runtime_mode_from_name(text, mode)) { snprintf(error, error_size, "error=bad autopilot mode\n"); return false; }
        if (!ship_data_model::apply_mode(state, value.id, mode, 0)) { snprintf(error, error_size, "error=unsupported mode %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::pilot_enum: {
        char text[32]{};
        ship_data_model::PilotName pilot{};
        if (!runtime_strip_optional_quotes(payload, text, sizeof(text)) || !ship_data_model::pilot_from_name(text, pilot)) { snprintf(error, error_size, "error=bad pilot\n"); return false; }
        if (!ship_data_model::apply_pilot(state, value.id, pilot, 0)) { snprintf(error, error_size, "error=unsupported pilot %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::source_enum: {
        char text[32]{};
        ship_data_model::SensorSource source{};
        if (!runtime_strip_optional_quotes(payload, text, sizeof(text)) || !ship_data_model::sensor_source_from_name(text, source)) { snprintf(error, error_size, "error=bad source\n"); return false; }
        if (!ship_data_model::apply_source(state, value.id, source, 0)) { snprintf(error, error_size, "error=unsupported source %s\n", value.pypilot_name); return false; }
        return true;
    }
    case RuntimeFieldType::json_value:
    case RuntimeFieldType::vector3_value:
    case RuntimeFieldType::quaternion_value:
    case RuntimeFieldType::rudder_calibration_enum:
    case RuntimeFieldType::servo_state_enum:
    case RuntimeFieldType::tack_state_enum:
    case RuntimeFieldType::tack_direction_enum:
    case RuntimeFieldType::unknown_value:
        snprintf(error, error_size, "error=unsupported value %s\n", value.pypilot_name);
        return false;
    }
    return false;
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool value_exists(const char* name) const {
        return find_runtime_value(name) != nullptr;
    }

    bool writable(const char* name) const {
        const RuntimeFieldMeta* value = find_runtime_value(name);
        return value && value->writable;
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const RuntimeFieldMeta* value = find_runtime_value(name);
        if (!value) { snprintf(error, error_size, "error=unknown value %s\n", name ? name : ""); return false; }
        if (!value->writable) { snprintf(error, error_size, "error=value not writable %s\n", value->pypilot_name); return false; }
        return apply_runtime_set(state_, *value, payload, error, error_size);
    }

    bool format_value(const char* name, char* out, size_t out_size) const {
        const RuntimeFieldMeta* value = find_runtime_value(name);
        if (!value) return false;
        return format_runtime_value(state_, *value, out, out_size);
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!out || out_size == 0) return false;
        out[0] = '\0';
        if (!runtime_append_text(out, out_size, "values={")) return false;

        const char* priority_names[] = {
            "ap.enabled", "ap.heading_command", "ap.heading_error", "ap.pilot",
            "servo.command", "servo.engaged", "servo.voltage",
            "imu.heading", "imu.heading_lowpass", "gps.track", "gps.source",
            "wind.speed", "wind.source", "server.version"
        };
        for (size_t i = 0; i < sizeof(priority_names) / sizeof(priority_names[0]); ++i) {
            const RuntimeFieldMeta* meta = find_runtime_value(priority_names[i]);
            if (meta && !append_json_value(out, out_size, *meta)) break;
        }

        for (size_t i = 0; i < ship_data_model::field_definition_count; ++i) {
            const RuntimeFieldMeta& meta = ship_data_model::field_definitions[i];
            bool already_added = false;
            for (size_t p = 0; p < sizeof(priority_names) / sizeof(priority_names[0]); ++p) {
                if (strcmp(meta.pypilot_name, priority_names[p]) == 0) { already_added = true; break; }
            }
            if (already_added) continue;
            char item[224]{};
            snprintf(item, sizeof(item), ",\"%s\":{\"type\":\"%s\",\"writable\":%s}",
                     meta.pypilot_name,
                     runtime_field_type_name(meta.type),
                     meta.writable ? "true" : "false");
            if (strlen(out) + strlen(item) + 3 >= out_size) break;
            runtime_append_text(out, out_size, item);
        }
        return runtime_append_text(out, out_size, "}\n");
    }

private:
    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
