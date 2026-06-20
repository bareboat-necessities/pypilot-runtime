#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace pypilot_runtime {

static inline bool copy_cstr(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) return false;
    if (!src) {
        dst[0] = '\0';
        return true;
    }
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
    if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) {
        out = true;
        return true;
    }
    if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) {
        out = false;
        return true;
    }
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

class RuntimeBool final {
public:
    RuntimeBool(const char* name, bool initial, bool writable = false)
        : name_(name), value_(initial), writable_(writable) {}
    const char* name() const { return name_; }
    bool get() const { return value_; }
    bool writable() const { return writable_; }
    bool changed() const { return changed_; }
    void clear_changed() { changed_ = false; }
    bool set(bool value) {
        if (value_ != value) {
            value_ = value;
            changed_ = true;
        }
        return true;
    }
    bool parse_set(const char* text) {
        bool parsed = false;
        return parse_bool_text(text, parsed) && set(parsed);
    }
    bool format(char* out, size_t out_size) const {
        return snprintf(out, out_size, "%s=%s\n", name_, value_ ? "true" : "false") > 0;
    }
private:
    const char* name_ = "";
    bool value_ = false;
    bool writable_ = false;
    bool changed_ = true;
};

class RuntimeNumber final {
public:
    RuntimeNumber(const char* name, double initial, double min_value, double max_value, bool writable = false)
        : name_(name), value_(initial), min_(min_value), max_(max_value), writable_(writable) {}
    const char* name() const { return name_; }
    double get() const { return value_; }
    bool writable() const { return writable_; }
    bool changed() const { return changed_; }
    void clear_changed() { changed_ = false; }
    bool set(double value) {
        if (value < min_ || value > max_) return false;
        if (value_ != value) {
            value_ = value;
            changed_ = true;
        }
        return true;
    }
    bool parse_set(const char* text) {
        double parsed = 0.0;
        return parse_number_text(text, parsed) && set(parsed);
    }
    bool format(char* out, size_t out_size) const {
        return snprintf(out, out_size, "%s=%.4f\n", name_, value_) > 0;
    }
private:
    const char* name_ = "";
    double value_ = 0.0;
    double min_ = 0.0;
    double max_ = 0.0;
    bool writable_ = false;
    bool changed_ = true;
};

template<size_t Capacity>
class RuntimeString final {
public:
    RuntimeString(const char* name, const char* initial, bool writable = false)
        : name_(name), writable_(writable) {
        copy_cstr(value_, sizeof(value_), initial);
    }
    const char* name() const { return name_; }
    const char* get() const { return value_; }
    bool writable() const { return writable_; }
    bool changed() const { return changed_; }
    void clear_changed() { changed_ = false; }
    bool set(const char* value) {
        char stripped[Capacity]{};
        if (!strip_optional_quotes(value, stripped, sizeof(stripped))) return false;
        if (strcmp(value_, stripped) != 0) {
            copy_cstr(value_, sizeof(value_), stripped);
            changed_ = true;
        }
        return true;
    }
    bool parse_set(const char* text) { return set(text); }
    bool format(char* out, size_t out_size) const {
        return snprintf(out, out_size, "%s=\"%s\"\n", name_, value_) > 0;
    }
private:
    const char* name_ = "";
    char value_[Capacity]{};
    bool writable_ = false;
    bool changed_ = true;
};

struct AutopilotValues {
    RuntimeBool enabled{"ap.enabled", false, true};
    RuntimeString<24> mode{"ap.mode", "compass", true};
    RuntimeString<24> pilot{"ap.pilot", "basic", true};
    RuntimeNumber heading_command{"ap.heading_command", 0.0, 0.0, 360.0, true};
};

struct BoatImuValues {
    RuntimeNumber heading{"imu.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber roll{"imu.roll", 0.0, -180.0, 180.0, false};
    RuntimeNumber pitch{"imu.pitch", 0.0, -180.0, 180.0, false};
    RuntimeNumber heading_lowpass{"imu.heading_lowpass", 0.0, 0.0, 360.0, false};
};

struct ServoValues {
    RuntimeNumber command{"servo.command", 0.0, -1.0, 1.0, true};
    RuntimeBool engaged{"servo.engaged", false, true};
    RuntimeString<24> state{"servo.state", "idle", false};
};

struct SensorValues {};
struct PilotValues {};

struct GpsValues {
    RuntimeNumber speed{"gps.speed", 0.0, 0.0, 200.0, false};
};

struct WindValues {
    RuntimeNumber direction{"wind.direction", 0.0, 0.0, 360.0, false};
};

struct PypilotRuntimeState {
    AutopilotValues& autopilot;
    BoatImuValues& boatimu;
    SensorValues& sensors;
    ServoValues& servo;
    PilotValues& pilots;
    GpsValues& gps;
    WindValues& wind;
};

enum class PypilotValueId : uint16_t {
    ApEnabled,
    ApMode,
    ApPilot,
    ApHeadingCommand,
    ImuHeading,
    ImuRoll,
    ImuPitch,
    ImuHeadingLowpass,
    ServoCommand,
    ServoEngaged,
    ServoState,
    GpsSpeed,
    WindDirection,
    Unknown
};

static inline PypilotValueId parse_value_name(const char* name) {
    if (!name) return PypilotValueId::Unknown;
    if (strcmp(name, "ap.enabled") == 0) return PypilotValueId::ApEnabled;
    if (strcmp(name, "ap.mode") == 0) return PypilotValueId::ApMode;
    if (strcmp(name, "ap.pilot") == 0) return PypilotValueId::ApPilot;
    if (strcmp(name, "ap.heading_command") == 0) return PypilotValueId::ApHeadingCommand;
    if (strcmp(name, "imu.heading") == 0) return PypilotValueId::ImuHeading;
    if (strcmp(name, "imu.roll") == 0) return PypilotValueId::ImuRoll;
    if (strcmp(name, "imu.pitch") == 0) return PypilotValueId::ImuPitch;
    if (strcmp(name, "imu.heading_lowpass") == 0) return PypilotValueId::ImuHeadingLowpass;
    if (strcmp(name, "servo.command") == 0) return PypilotValueId::ServoCommand;
    if (strcmp(name, "servo.engaged") == 0) return PypilotValueId::ServoEngaged;
    if (strcmp(name, "servo.state") == 0) return PypilotValueId::ServoState;
    if (strcmp(name, "gps.speed") == 0) return PypilotValueId::GpsSpeed;
    if (strcmp(name, "wind.direction") == 0) return PypilotValueId::WindDirection;
    return PypilotValueId::Unknown;
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool writable(PypilotValueId id) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.writable();
        case PypilotValueId::ApMode: return state_.autopilot.mode.writable();
        case PypilotValueId::ApPilot: return state_.autopilot.pilot.writable();
        case PypilotValueId::ApHeadingCommand: return state_.autopilot.heading_command.writable();
        case PypilotValueId::ServoCommand: return state_.servo.command.writable();
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.writable();
        default: return false;
        }
    }

    bool changed(PypilotValueId id) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.changed();
        case PypilotValueId::ApMode: return state_.autopilot.mode.changed();
        case PypilotValueId::ApPilot: return state_.autopilot.pilot.changed();
        case PypilotValueId::ApHeadingCommand: return state_.autopilot.heading_command.changed();
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.changed();
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.changed();
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.changed();
        case PypilotValueId::ImuHeadingLowpass: return state_.boatimu.heading_lowpass.changed();
        case PypilotValueId::ServoCommand: return state_.servo.command.changed();
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.changed();
        case PypilotValueId::ServoState: return state_.servo.state.changed();
        case PypilotValueId::GpsSpeed: return state_.gps.speed.changed();
        case PypilotValueId::WindDirection: return state_.wind.direction.changed();
        default: return false;
        }
    }

    void clear_changed(PypilotValueId id) {
        switch (id) {
        case PypilotValueId::ApEnabled: state_.autopilot.enabled.clear_changed(); break;
        case PypilotValueId::ApMode: state_.autopilot.mode.clear_changed(); break;
        case PypilotValueId::ApPilot: state_.autopilot.pilot.clear_changed(); break;
        case PypilotValueId::ApHeadingCommand: state_.autopilot.heading_command.clear_changed(); break;
        case PypilotValueId::ImuHeading: state_.boatimu.heading.clear_changed(); break;
        case PypilotValueId::ImuRoll: state_.boatimu.roll.clear_changed(); break;
        case PypilotValueId::ImuPitch: state_.boatimu.pitch.clear_changed(); break;
        case PypilotValueId::ImuHeadingLowpass: state_.boatimu.heading_lowpass.clear_changed(); break;
        case PypilotValueId::ServoCommand: state_.servo.command.clear_changed(); break;
        case PypilotValueId::ServoEngaged: state_.servo.engaged.clear_changed(); break;
        case PypilotValueId::ServoState: state_.servo.state.clear_changed(); break;
        case PypilotValueId::GpsSpeed: state_.gps.speed.clear_changed(); break;
        case PypilotValueId::WindDirection: state_.wind.direction.clear_changed(); break;
        default: break;
        }
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const PypilotValueId id = parse_value_name(name);
        if (id == PypilotValueId::Unknown) {
            snprintf(error, error_size, "error=unknown value %s\n", name ? name : "");
            return false;
        }
        if (!writable(id)) {
            snprintf(error, error_size, "error=%s is not writable\n", name ? name : "");
            return false;
        }
        bool ok = false;
        switch (id) {
        case PypilotValueId::ApEnabled: ok = state_.autopilot.enabled.parse_set(payload); break;
        case PypilotValueId::ApMode: ok = state_.autopilot.mode.parse_set(payload); break;
        case PypilotValueId::ApPilot: ok = state_.autopilot.pilot.parse_set(payload); break;
        case PypilotValueId::ApHeadingCommand: ok = state_.autopilot.heading_command.parse_set(payload); break;
        case PypilotValueId::ServoCommand: ok = state_.servo.command.parse_set(payload); break;
        case PypilotValueId::ServoEngaged: ok = state_.servo.engaged.parse_set(payload); break;
        default: ok = false; break;
        }
        if (!ok) snprintf(error, error_size, "error=invalid value for %s\n", name ? name : "");
        return ok;
    }

    bool format_value(PypilotValueId id, char* out, size_t out_size) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.format(out, out_size);
        case PypilotValueId::ApMode: return state_.autopilot.mode.format(out, out_size);
        case PypilotValueId::ApPilot: return state_.autopilot.pilot.format(out, out_size);
        case PypilotValueId::ApHeadingCommand: return state_.autopilot.heading_command.format(out, out_size);
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.format(out, out_size);
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.format(out, out_size);
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.format(out, out_size);
        case PypilotValueId::ImuHeadingLowpass: return state_.boatimu.heading_lowpass.format(out, out_size);
        case PypilotValueId::ServoCommand: return state_.servo.command.format(out, out_size);
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.format(out, out_size);
        case PypilotValueId::ServoState: return state_.servo.state.format(out, out_size);
        case PypilotValueId::GpsSpeed: return state_.gps.speed.format(out, out_size);
        case PypilotValueId::WindDirection: return state_.wind.direction.format(out, out_size);
        default: return false;
        }
    }

    bool format_named_value(const char* name, char* out, size_t out_size) const {
        return format_value(parse_value_name(name), out, out_size);
    }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!copy_cstr(out, out_size, "values={\"ap.enabled\":{\"type\":\"BooleanValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.mode\":{\"type\":\"StringValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.pilot\":{\"type\":\"StringValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.heading_command\":{\"type\":\"RangeProperty\",\"min\":0,\"max\":360,\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"imu.heading\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.roll\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.pitch\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.heading_lowpass\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"servo.command\":{\"type\":\"RangeProperty\",\"min\":-1,\"max\":1,\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"servo.engaged\":{\"type\":\"BooleanValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"servo.state\":{\"type\":\"StringValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"gps.speed\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"wind.direction\":{\"type\":\"SensorValue\"}}\n")) return false;
        return true;
    }
private:
    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
