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

enum class RuntimeFieldKind : uint8_t {
    SettingBool,
    SettingAutopilotMode,
    SettingPilotName,
    SettingSensorSource,
    SettingUint32,
    SettingTackState,
    SettingTackDirection,
    StampedFloat,
    TimedCommandFloat,
    CString,
    ServoStateText
};

struct RuntimeValueDescriptor {
    const char* name;
    RuntimeValueType type;
    bool writable;
    RuntimeFieldKind kind;
    size_t offset;
    size_t size;
};

#define PYPILOT_RUNTIME_FIELD_OFFSET(path) \
    (static_cast<size_t>(reinterpret_cast<const char*>(&reinterpret_cast<const PypilotRuntimeState*>(0)->path) - \
                         reinterpret_cast<const char*>(reinterpret_cast<const PypilotRuntimeState*>(0))))

#define PYPILOT_RUNTIME_VALUE(NAME, TYPE, WRITABLE, KIND, PATH) \
    {NAME, RuntimeValueType::TYPE, WRITABLE, RuntimeFieldKind::KIND, PYPILOT_RUNTIME_FIELD_OFFSET(PATH), 0}

#define PYPILOT_RUNTIME_STRING_VALUE(NAME, WRITABLE, KIND, PATH, SIZE) \
    {NAME, RuntimeValueType::String, WRITABLE, RuntimeFieldKind::KIND, PYPILOT_RUNTIME_FIELD_OFFSET(PATH), SIZE}

template<typename Field>
static inline Field& runtime_field(PypilotRuntimeState& state, const RuntimeValueDescriptor& descriptor) {
    return *reinterpret_cast<Field*>(reinterpret_cast<char*>(&state) + descriptor.offset);
}

template<typename Field>
static inline const Field& runtime_field(const PypilotRuntimeState& state, const RuntimeValueDescriptor& descriptor) {
    return *reinterpret_cast<const Field*>(reinterpret_cast<const char*>(&state) + descriptor.offset);
}

static inline bool append_json_value(char* out, size_t out_size, const RuntimeValueDescriptor& descriptor) {
    char item[160]{};
    snprintf(item, sizeof(item), "\"%s\":{\"type\":\"%s\",\"writable\":%s}",
             descriptor.name,
             runtime_value_type_name(descriptor.type),
             descriptor.writable ? "true" : "false");
    if (out[0] != '\0' && out[strlen(out) - 1] != '{') {
        if (!append_cstr(out, out_size, ",")) return false;
    }
    return append_cstr(out, out_size, item);
}

static inline const RuntimeValueDescriptor* runtime_value_descriptors() {
    static const RuntimeValueDescriptor values[] = {
        PYPILOT_RUNTIME_VALUE("ap.enabled", Bool, true, SettingBool, ap.enabled),
        PYPILOT_RUNTIME_VALUE("ap.mode", String, true, SettingAutopilotMode, ap.mode),
        PYPILOT_RUNTIME_VALUE("ap.preferred_mode", String, true, SettingAutopilotMode, ap.preferred_mode),
        PYPILOT_RUNTIME_VALUE("ap.pilot", String, true, SettingPilotName, ap.pilot),
        PYPILOT_RUNTIME_VALUE("ap.heading_command", Number, true, StampedFloat, ap.heading_command_deg),
        PYPILOT_RUNTIME_VALUE("ap.heading", Number, false, StampedFloat, ap.heading_deg),
        PYPILOT_RUNTIME_VALUE("ap.heading_error", Number, false, StampedFloat, ap.heading_error_deg),
        PYPILOT_RUNTIME_VALUE("imu.heading", Number, false, StampedFloat, imu.heading_deg),
        PYPILOT_RUNTIME_VALUE("imu.roll", Number, false, StampedFloat, imu.roll_deg),
        PYPILOT_RUNTIME_VALUE("imu.pitch", Number, false, StampedFloat, imu.pitch_deg),
        PYPILOT_RUNTIME_VALUE("imu.heel", Number, false, StampedFloat, imu.heel_deg),
        PYPILOT_RUNTIME_VALUE("imu.headingrate", Number, false, StampedFloat, imu.heading_rate_deg_s),
        PYPILOT_RUNTIME_VALUE("imu.heading_lowpass", Number, false, StampedFloat, imu.heading_lowpass_deg),
        PYPILOT_RUNTIME_VALUE("servo.command", Number, true, TimedCommandFloat, servo.command_norm),
        PYPILOT_RUNTIME_VALUE("servo.engaged", Bool, true, SettingBool, servo.engaged),
        PYPILOT_RUNTIME_VALUE("servo.state", String, false, ServoStateText, servo.engaged),
        PYPILOT_RUNTIME_VALUE("servo.voltage", Number, false, StampedFloat, servo.voltage_v),
        PYPILOT_RUNTIME_VALUE("servo.current", Number, false, StampedFloat, servo.current_a),
        PYPILOT_RUNTIME_VALUE("servo.position", Number, false, StampedFloat, servo.position_deg),
        PYPILOT_RUNTIME_VALUE("gps.speed", Number, false, StampedFloat, navigation.gps.speed_kn),
        PYPILOT_RUNTIME_VALUE("gps.track", Number, false, StampedFloat, navigation.gps.track_deg),
        PYPILOT_RUNTIME_VALUE("gps.timestamp", Number, false, StampedFloat, navigation.gps.timestamp_s),
        PYPILOT_RUNTIME_VALUE("gps.source", String, false, SettingSensorSource, navigation.gps.source),
        PYPILOT_RUNTIME_VALUE("wind.direction", Number, false, StampedFloat, wind.apparent.direction_deg),
        PYPILOT_RUNTIME_VALUE("wind.speed", Number, false, StampedFloat, wind.apparent.speed_kn),
        PYPILOT_RUNTIME_VALUE("wind.source", String, false, SettingSensorSource, wind.apparent.source),
        PYPILOT_RUNTIME_VALUE("truewind.direction", Number, false, StampedFloat, wind.truewind.direction_deg),
        PYPILOT_RUNTIME_VALUE("truewind.speed", Number, false, StampedFloat, wind.truewind.speed_kn),
        PYPILOT_RUNTIME_VALUE("truewind.source", String, false, SettingSensorSource, wind.truewind.source),
        PYPILOT_RUNTIME_VALUE("water.speed", Number, false, StampedFloat, water.speed_kn),
        PYPILOT_RUNTIME_VALUE("rudder.angle", Number, false, StampedFloat, rudder.angle_deg),
        PYPILOT_RUNTIME_VALUE("ap.tack.state", Number, true, SettingTackState, tack.state),
        PYPILOT_RUNTIME_VALUE("ap.tack.direction", Number, true, SettingTackDirection, tack.direction),
        PYPILOT_RUNTIME_STRING_VALUE("profile.name", true, CString, server.profile_name, sizeof(PypilotRuntimeState::server.profile_name)),
        PYPILOT_RUNTIME_STRING_VALUE("server.version", false, CString, server.version, sizeof(PypilotRuntimeState::server.version)),
        PYPILOT_RUNTIME_VALUE("server.uptime", Number, false, StampedFloat, server.uptime_s),
        PYPILOT_RUNTIME_VALUE("status.faults", Number, false, SettingUint32, status.faults),
        PYPILOT_RUNTIME_VALUE("status.warnings", Number, false, SettingUint32, status.warnings),
        PYPILOT_RUNTIME_VALUE("runtime.published_value_count", Number, false, StampedFloat, runtime_publication.published_value_count)
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

static inline bool format_runtime_value(const PypilotRuntimeState& state,
                                        const RuntimeValueDescriptor& descriptor,
                                        char* out,
                                        size_t out_size) {
    switch (descriptor.kind) {
    case RuntimeFieldKind::SettingBool: {
        const auto& field = runtime_field<pypilot_data_model::Setting<bool>>(state, descriptor);
        return snprintf(out, out_size, "%s=%s\n", descriptor.name, field.value ? "true" : "false") > 0;
    }
    case RuntimeFieldKind::SettingAutopilotMode: {
        const auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::AutopilotMode>>(state, descriptor);
        return snprintf(out, out_size, "%s=\"%s\"\n", descriptor.name, pypilot_data_model::autopilot_mode_name(field.value)) > 0;
    }
    case RuntimeFieldKind::SettingPilotName: {
        const auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::PilotName>>(state, descriptor);
        return snprintf(out, out_size, "%s=\"%s\"\n", descriptor.name, pypilot_data_model::pilot_name(field.value)) > 0;
    }
    case RuntimeFieldKind::SettingSensorSource: {
        const auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::SensorSource>>(state, descriptor);
        return snprintf(out, out_size, "%s=\"%s\"\n", descriptor.name, pypilot_data_model::sensor_source_name(field.value)) > 0;
    }
    case RuntimeFieldKind::SettingUint32: {
        const auto& field = runtime_field<pypilot_data_model::Setting<uint32_t>>(state, descriptor);
        return snprintf(out, out_size, "%s=%u\n", descriptor.name, static_cast<unsigned>(field.value)) > 0;
    }
    case RuntimeFieldKind::SettingTackState: {
        const auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::TackState>>(state, descriptor);
        return snprintf(out, out_size, "%s=%u\n", descriptor.name, static_cast<unsigned>(field.value)) > 0;
    }
    case RuntimeFieldKind::SettingTackDirection: {
        const auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::TackDirection>>(state, descriptor);
        return snprintf(out, out_size, "%s=%u\n", descriptor.name, static_cast<unsigned>(field.value)) > 0;
    }
    case RuntimeFieldKind::StampedFloat: {
        const auto& field = runtime_field<pypilot_data_model::Stamped<float>>(state, descriptor);
        const double value = field.valid ? static_cast<double>(field.value) : 0.0;
        return snprintf(out, out_size, "%s=%.4f\n", descriptor.name, value) > 0;
    }
    case RuntimeFieldKind::TimedCommandFloat: {
        const auto& field = runtime_field<pypilot_data_model::TimedCommand<float>>(state, descriptor);
        const double value = field.valid ? static_cast<double>(field.value) : 0.0;
        return snprintf(out, out_size, "%s=%.4f\n", descriptor.name, value) > 0;
    }
    case RuntimeFieldKind::CString: {
        const char* text = reinterpret_cast<const char*>(&state) + descriptor.offset;
        return snprintf(out, out_size, "%s=\"%s\"\n", descriptor.name, text) > 0;
    }
    case RuntimeFieldKind::ServoStateText: {
        const auto& field = runtime_field<pypilot_data_model::Setting<bool>>(state, descriptor);
        return snprintf(out, out_size, "%s=\"%s\"\n", descriptor.name, field.value ? "active" : "disabled") > 0;
    }
    }
    return false;
}

static inline bool set_runtime_value(PypilotRuntimeState& state,
                                     const RuntimeValueDescriptor& descriptor,
                                     const char* payload,
                                     char* error,
                                     size_t error_size) {
    switch (descriptor.kind) {
    case RuntimeFieldKind::SettingBool: {
        auto& field = runtime_field<pypilot_data_model::Setting<bool>>(state, descriptor);
        bool parsed = false;
        if (!parse_bool_text(payload, parsed)) { snprintf(error, error_size, "error=bad bool\n"); return false; }
        field.value = parsed;
        return true;
    }
    case RuntimeFieldKind::SettingAutopilotMode: {
        auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::AutopilotMode>>(state, descriptor);
        char text[32]{};
        pypilot_data_model::AutopilotMode mode{};
        if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::autopilot_mode_from_name(text, mode)) {
            snprintf(error, error_size, "error=bad autopilot mode\n");
            return false;
        }
        field.value = mode;
        return true;
    }
    case RuntimeFieldKind::SettingPilotName: {
        auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::PilotName>>(state, descriptor);
        char text[32]{};
        pypilot_data_model::PilotName pilot{};
        if (!strip_optional_quotes(payload, text, sizeof(text)) || !pypilot_data_model::pilot_from_name(text, pilot)) {
            snprintf(error, error_size, "error=bad pilot\n");
            return false;
        }
        field.value = pilot;
        return true;
    }
    case RuntimeFieldKind::SettingUint32: {
        auto& field = runtime_field<pypilot_data_model::Setting<uint32_t>>(state, descriptor);
        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        field.value = static_cast<uint32_t>(value);
        return true;
    }
    case RuntimeFieldKind::SettingTackState: {
        auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::TackState>>(state, descriptor);
        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        field.value = static_cast<pypilot_data_model::TackState>(static_cast<int>(value));
        return true;
    }
    case RuntimeFieldKind::SettingTackDirection: {
        auto& field = runtime_field<pypilot_data_model::Setting<pypilot_data_model::TackDirection>>(state, descriptor);
        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        field.value = static_cast<pypilot_data_model::TackDirection>(static_cast<int>(value));
        return true;
    }
    case RuntimeFieldKind::StampedFloat: {
        auto& field = runtime_field<pypilot_data_model::Stamped<float>>(state, descriptor);
        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        field.set(static_cast<float>(value), 0);
        return true;
    }
    case RuntimeFieldKind::TimedCommandFloat: {
        auto& field = runtime_field<pypilot_data_model::TimedCommand<float>>(state, descriptor);
        double value = 0.0;
        if (!parse_number_text(payload, value)) { snprintf(error, error_size, "error=bad number\n"); return false; }
        field.set_external(static_cast<float>(value), 0);
        return true;
    }
    case RuntimeFieldKind::CString: {
        char text[128]{};
        if (!strip_optional_quotes(payload, text, sizeof(text))) { snprintf(error, error_size, "error=bad string\n"); return false; }
        char* target = reinterpret_cast<char*>(&state) + descriptor.offset;
        return copy_cstr(target, descriptor.size, text);
    }
    case RuntimeFieldKind::SettingSensorSource:
    case RuntimeFieldKind::ServoStateText:
        snprintf(error, error_size, "error=value not writable %s\n", descriptor.name);
        return false;
    }
    return false;
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
        if (!descriptor->writable) {
            snprintf(error, error_size, "error=value not writable %s\n", descriptor->name);
            return false;
        }
        return set_runtime_value(state_, *descriptor, payload, error, error_size);
    }

    bool format_value(const char* name, char* out, size_t out_size) const {
        const RuntimeValueDescriptor* descriptor = find_runtime_value_descriptor(name);
        if (!descriptor) return false;
        return format_runtime_value(state_, *descriptor, out, out_size);
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!out || out_size == 0) return false;
        out[0] = '\0';
        if (!append_cstr(out, out_size, "values={")) return false;
        const RuntimeValueDescriptor* values = runtime_value_descriptors();
        for (size_t i = 0; i < runtime_value_descriptor_count(); ++i) {
            if (!append_json_value(out, out_size, values[i])) return false;
        }
        return append_cstr(out, out_size, "}\n");
    }

private:
    PypilotRuntimeState& state_;
};

#undef PYPILOT_RUNTIME_VALUE
#undef PYPILOT_RUNTIME_STRING_VALUE
#undef PYPILOT_RUNTIME_FIELD_OFFSET

} // namespace pypilot_runtime
