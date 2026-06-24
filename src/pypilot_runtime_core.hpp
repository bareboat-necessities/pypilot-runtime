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

static inline bool append_json_value(char* out, size_t out_size, const char* name, const char* type, bool writable) {
    char item[160]{};
    snprintf(item, sizeof(item), "\"%s\":{\"type\":\"%s\",\"writable\":%s}",
             name, type, writable ? "true" : "false");
    if (out[0] != '\0' && out[strlen(out) - 1] != '{') {
        if (!append_cstr(out, out_size, ",")) return false;
    }
    return append_cstr(out, out_size, item);
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool value_exists(const char* name) const {
        if (!name) return false;
        if (strcmp(name, "ap.enabled") == 0) return true;
        if (strcmp(name, "ap.mode") == 0) return true;
        if (strcmp(name, "ap.preferred_mode") == 0) return true;
        if (strcmp(name, "ap.pilot") == 0) return true;
        if (strcmp(name, "ap.heading_command") == 0) return true;
        if (strcmp(name, "ap.heading") == 0) return true;
        if (strcmp(name, "ap.heading_error") == 0) return true;
        if (strcmp(name, "imu.heading") == 0) return true;
        if (strcmp(name, "imu.roll") == 0) return true;
        if (strcmp(name, "imu.pitch") == 0) return true;
        if (strcmp(name, "imu.heel") == 0) return true;
        if (strcmp(name, "imu.headingrate") == 0) return true;
        if (strcmp(name, "imu.heading_lowpass") == 0) return true;
        if (strcmp(name, "servo.command") == 0) return true;
        if (strcmp(name, "servo.engaged") == 0) return true;
        if (strcmp(name, "servo.state") == 0) return true;
        if (strcmp(name, "servo.voltage") == 0) return true;
        if (strcmp(name, "servo.current") == 0) return true;
        if (strcmp(name, "servo.position") == 0) return true;
        if (strcmp(name, "gps.speed") == 0) return true;
        if (strcmp(name, "gps.track") == 0) return true;
        if (strcmp(name, "gps.timestamp") == 0) return true;
        if (strcmp(name, "gps.source") == 0) return true;
        if (strcmp(name, "wind.direction") == 0) return true;
        if (strcmp(name, "wind.speed") == 0) return true;
        if (strcmp(name, "wind.source") == 0) return true;
        if (strcmp(name, "truewind.direction") == 0) return true;
        if (strcmp(name, "truewind.speed") == 0) return true;
        if (strcmp(name, "truewind.source") == 0) return true;
        if (strcmp(name, "water.speed") == 0) return true;
        if (strcmp(name, "rudder.angle") == 0) return true;
        if (strcmp(name, "ap.tack.state") == 0) return true;
        if (strcmp(name, "ap.tack.direction") == 0) return true;
        if (strcmp(name, "profile.name") == 0) return true;
        if (strcmp(name, "server.version") == 0) return true;
        if (strcmp(name, "server.uptime") == 0) return true;
        if (strcmp(name, "status.faults") == 0) return true;
        if (strcmp(name, "status.warnings") == 0) return true;
        if (strcmp(name, "runtime.published_value_count") == 0) return true;
        return false;
    }

    bool writable(const char* name) const {
        if (!name) return false;
        if (strcmp(name, "ap.enabled") == 0) return true;
        if (strcmp(name, "ap.mode") == 0) return true;
        if (strcmp(name, "ap.preferred_mode") == 0) return true;
        if (strcmp(name, "ap.pilot") == 0) return true;
        if (strcmp(name, "ap.heading_command") == 0) return true;
        if (strcmp(name, "servo.command") == 0) return true;
        if (strcmp(name, "servo.engaged") == 0) return true;
        if (strcmp(name, "ap.tack.state") == 0) return true;
        if (strcmp(name, "ap.tack.direction") == 0) return true;
        if (strcmp(name, "profile.name") == 0) return true;
        return false;
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        if (!value_exists(name)) {
            snprintf(error, error_size, "error=unknown value %s\n", name ? name : "");
            return false;
        }
        if (!writable(name)) {
            snprintf(error, error_size, "error=value not writable %s\n", name);
            return false;
        }

        if (strcmp(name, "ap.enabled") == 0) {
            bool parsed = false;
            if (!parse_bool_text(payload, parsed)) { snprintf(error, error_size, "error=bad bool\n"); return false; }
            state_.ap.enabled.value = parsed;
            return true;
        }
        if (strcmp(name, "ap.mode") == 0 || strcmp(name, "ap.preferred_mode") == 0) {
            char text[32]{};
            pypilot_data_model::AutopilotMode mode{};
            if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::autopilot_mode_from_name(text, mode)) {
                snprintf(error, error_size, "error=bad autopilot mode\n");
                return false;
            }
            if (strcmp(name, "ap.mode") == 0) state_.ap.mode.value = mode;
            else state_.ap.preferred_mode.value = mode;
            return true;
        }
        if (strcmp(name, "ap.pilot") == 0) {
            char text[32]{};
            pypilot_data_model::PilotName pilot{};
            if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::pilot_from_name(text, pilot)) {
                snprintf(error, error_size, "error=bad pilot\n");
                return false;
            }
            state_.ap.pilot.value = pilot;
            return true;
        }
        if (strcmp(name, "profile.name") == 0) {
            char text[64]{};
            if (!strip_optional_quotes(payload, text, sizeof(text))) { snprintf(error, error_size, "error=bad profile name\n"); return false; }
            return copy_cstr(state_.server.profile_name, sizeof(state_.server.profile_name), text);
        }

        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        if (strcmp(name, "ap.heading_command") == 0) { state_.ap.heading_command_deg.set(static_cast<float>(value), 0); return true; }
        if (strcmp(name, "servo.command") == 0) { state_.servo.command_norm.set_external(static_cast<float>(value), 0); return true; }
        if (strcmp(name, "servo.engaged") == 0) { state_.servo.engaged.value = value != 0.0; return true; }
        if (strcmp(name, "ap.tack.state") == 0) { state_.tack.state.value = static_cast<pypilot_data_model::TackState>(static_cast<int>(value)); return true; }
        if (strcmp(name, "ap.tack.direction") == 0) { state_.tack.direction.value = static_cast<pypilot_data_model::TackDirection>(static_cast<int>(value)); return true; }

        snprintf(error, error_size, "error=unhandled set %s\n", name);
        return false;
    }

    bool format_value(const char* name, char* out, size_t out_size) const {
        if (!name || !out || out_size == 0) return false;
        if (strcmp(name, "ap.enabled") == 0) return snprintf(out, out_size, "ap.enabled=%s\n", state_.ap.enabled.value ? "true" : "false") > 0;
        if (strcmp(name, "ap.mode") == 0) return snprintf(out, out_size, "ap.mode=\"%s\"\n", pypilot_data_model::autopilot_mode_name(state_.ap.mode.value)) > 0;
        if (strcmp(name, "ap.preferred_mode") == 0) return snprintf(out, out_size, "ap.preferred_mode=\"%s\"\n", pypilot_data_model::autopilot_mode_name(state_.ap.preferred_mode.value)) > 0;
        if (strcmp(name, "ap.pilot") == 0) return snprintf(out, out_size, "ap.pilot=\"%s\"\n", pypilot_data_model::pilot_name(state_.ap.pilot.value)) > 0;
        if (strcmp(name, "ap.heading_command") == 0) return format_stamped(name, state_.ap.heading_command_deg, out, out_size);
        if (strcmp(name, "ap.heading") == 0) return format_stamped(name, state_.ap.heading_deg, out, out_size);
        if (strcmp(name, "ap.heading_error") == 0) return format_stamped(name, state_.ap.heading_error_deg, out, out_size);
        if (strcmp(name, "imu.heading") == 0) return format_stamped(name, state_.imu.heading_deg, out, out_size);
        if (strcmp(name, "imu.roll") == 0) return format_stamped(name, state_.imu.roll_deg, out, out_size);
        if (strcmp(name, "imu.pitch") == 0) return format_stamped(name, state_.imu.pitch_deg, out, out_size);
        if (strcmp(name, "imu.heel") == 0) return format_stamped(name, state_.imu.heel_deg, out, out_size);
        if (strcmp(name, "imu.headingrate") == 0) return format_stamped(name, state_.imu.heading_rate_deg_s, out, out_size);
        if (strcmp(name, "imu.heading_lowpass") == 0) return format_stamped(name, state_.imu.heading_lowpass_deg, out, out_size);
        if (strcmp(name, "servo.command") == 0) return format_command(name, state_.servo.command_norm, out, out_size);
        if (strcmp(name, "servo.engaged") == 0) return snprintf(out, out_size, "servo.engaged=%s\n", state_.servo.engaged.value ? "true" : "false") > 0;
        if (strcmp(name, "servo.state") == 0) return snprintf(out, out_size, "servo.state=\"%s\"\n", servo_state_text()) > 0;
        if (strcmp(name, "servo.voltage") == 0) return format_stamped(name, state_.servo.voltage_v, out, out_size);
        if (strcmp(name, "servo.current") == 0) return format_stamped(name, state_.servo.current_a, out, out_size);
        if (strcmp(name, "servo.position") == 0) return format_stamped(name, state_.servo.position_deg, out, out_size);
        if (strcmp(name, "gps.speed") == 0) return format_stamped(name, state_.navigation.gps.speed_kn, out, out_size);
        if (strcmp(name, "gps.track") == 0) return format_stamped(name, state_.navigation.gps.track_deg, out, out_size);
        if (strcmp(name, "gps.timestamp") == 0) return format_stamped(name, state_.navigation.gps.timestamp_s, out, out_size);
        if (strcmp(name, "gps.source") == 0) return snprintf(out, out_size, "gps.source=\"%s\"\n", pypilot_data_model::sensor_source_name(state_.navigation.gps.source.value)) > 0;
        if (strcmp(name, "wind.direction") == 0) return format_stamped(name, state_.wind.apparent.direction_deg, out, out_size);
        if (strcmp(name, "wind.speed") == 0) return format_stamped(name, state_.wind.apparent.speed_kn, out, out_size);
        if (strcmp(name, "wind.source") == 0) return snprintf(out, out_size, "wind.source=\"%s\"\n", pypilot_data_model::sensor_source_name(state_.wind.apparent.source.value)) > 0;
        if (strcmp(name, "truewind.direction") == 0) return format_stamped(name, state_.wind.truewind.direction_deg, out, out_size);
        if (strcmp(name, "truewind.speed") == 0) return format_stamped(name, state_.wind.truewind.speed_kn, out, out_size);
        if (strcmp(name, "truewind.source") == 0) return snprintf(out, out_size, "truewind.source=\"%s\"\n", pypilot_data_model::sensor_source_name(state_.wind.truewind.source.value)) > 0;
        if (strcmp(name, "water.speed") == 0) return format_stamped(name, state_.water.speed_kn, out, out_size);
        if (strcmp(name, "rudder.angle") == 0) return format_stamped(name, state_.rudder.angle_deg, out, out_size);
        if (strcmp(name, "ap.tack.state") == 0) return snprintf(out, out_size, "ap.tack.state=%u\n", static_cast<unsigned>(state_.tack.state.value)) > 0;
        if (strcmp(name, "ap.tack.direction") == 0) return snprintf(out, out_size, "ap.tack.direction=%u\n", static_cast<unsigned>(state_.tack.direction.value)) > 0;
        if (strcmp(name, "profile.name") == 0) return snprintf(out, out_size, "profile.name=\"%s\"\n", state_.server.profile_name) > 0;
        if (strcmp(name, "server.version") == 0) return snprintf(out, out_size, "server.version=\"%s\"\n", state_.server.version) > 0;
        if (strcmp(name, "server.uptime") == 0) return format_stamped(name, state_.server.uptime_s, out, out_size);
        if (strcmp(name, "status.faults") == 0) return snprintf(out, out_size, "status.faults=%u\n", state_.status.faults.value) > 0;
        if (strcmp(name, "status.warnings") == 0) return snprintf(out, out_size, "status.warnings=%u\n", state_.status.warnings.value) > 0;
        if (strcmp(name, "runtime.published_value_count") == 0) return format_stamped(name, state_.runtime_publication.published_value_count, out, out_size);
        return false;
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!out || out_size == 0) return false;
        out[0] = '\0';
        if (!append_cstr(out, out_size, "values={")) return false;
        if (!append_value_catalog(out, out_size)) return false;
        if (!append_cstr(out, out_size, "}\n")) return false;
        return true;
    }

private:
    template<typename T>
    bool format_stamped(const char* name, const pypilot_data_model::Stamped<T>& stamped, char* out, size_t out_size) const {
        const double value = stamped.valid ? static_cast<double>(stamped.value) : 0.0;
        return snprintf(out, out_size, "%s=%.4f\n", name, value) > 0;
    }

    template<typename T>
    bool format_command(const char* name, const pypilot_data_model::TimedCommand<T>& command, char* out, size_t out_size) const {
        const double value = command.valid ? static_cast<double>(command.value) : 0.0;
        return snprintf(out, out_size, "%s=%.4f\n", name, value) > 0;
    }

    const char* servo_state_text() const {
        if (!state_.servo.engaged.value) return "disabled";
        return "active";
    }

    bool append_value_catalog(char* out, size_t out_size) const {
        return append_json_value(out, out_size, "ap.enabled", "bool", true) &&
               append_json_value(out, out_size, "ap.mode", "string", true) &&
               append_json_value(out, out_size, "ap.preferred_mode", "string", true) &&
               append_json_value(out, out_size, "ap.pilot", "string", true) &&
               append_json_value(out, out_size, "ap.heading_command", "number", true) &&
               append_json_value(out, out_size, "ap.heading", "number", false) &&
               append_json_value(out, out_size, "ap.heading_error", "number", false) &&
               append_json_value(out, out_size, "imu.heading", "number", false) &&
               append_json_value(out, out_size, "imu.roll", "number", false) &&
               append_json_value(out, out_size, "imu.pitch", "number", false) &&
               append_json_value(out, out_size, "imu.heel", "number", false) &&
               append_json_value(out, out_size, "imu.headingrate", "number", false) &&
               append_json_value(out, out_size, "imu.heading_lowpass", "number", false) &&
               append_json_value(out, out_size, "servo.command", "number", true) &&
               append_json_value(out, out_size, "servo.engaged", "bool", true) &&
               append_json_value(out, out_size, "servo.state", "string", false) &&
               append_json_value(out, out_size, "servo.voltage", "number", false) &&
               append_json_value(out, out_size, "servo.current", "number", false) &&
               append_json_value(out, out_size, "servo.position", "number", false) &&
               append_json_value(out, out_size, "gps.speed", "number", false) &&
               append_json_value(out, out_size, "gps.track", "number", false) &&
               append_json_value(out, out_size, "gps.source", "string", false) &&
               append_json_value(out, out_size, "wind.direction", "number", false) &&
               append_json_value(out, out_size, "wind.speed", "number", false) &&
               append_json_value(out, out_size, "wind.source", "string", false) &&
               append_json_value(out, out_size, "water.speed", "number", false) &&
               append_json_value(out, out_size, "rudder.angle", "number", false) &&
               append_json_value(out, out_size, "profile.name", "string", true) &&
               append_json_value(out, out_size, "server.version", "string", false) &&
               append_json_value(out, out_size, "server.uptime", "number", false) &&
               append_json_value(out, out_size, "status.faults", "number", false) &&
               append_json_value(out, out_size, "status.warnings", "number", false) &&
               append_json_value(out, out_size, "runtime.published_value_count", "number", false);
    }

    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
