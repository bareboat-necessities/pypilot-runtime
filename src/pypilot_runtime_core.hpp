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

class RuntimeBool final {
public:
    RuntimeBool(const char* name, bool initial, bool writable = false) : name_(name), value_(initial), writable_(writable) {}
    const char* name() const { return name_; }
    bool get() const { return value_; }
    bool writable() const { return writable_; }
    bool changed() const { return changed_; }
    void clear_changed() { changed_ = false; }
    bool set(bool value) { if (value_ != value) { value_ = value; changed_ = true; } return true; }
    bool parse_set(const char* text) { bool parsed = false; return parse_bool_text(text, parsed) && set(parsed); }
    bool format(char* out, size_t out_size) const { return snprintf(out, out_size, "%s=%s\n", name_, value_ ? "true" : "false") > 0; }
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
    bool set(double value) { if (value < min_ || value > max_) return false; if (value_ != value) { value_ = value; changed_ = true; } return true; }
    bool parse_set(const char* text) { double parsed = 0.0; return parse_number_text(text, parsed) && set(parsed); }
    bool format(char* out, size_t out_size) const { return snprintf(out, out_size, "%s=%.4f\n", name_, value_) > 0; }
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
    RuntimeString(const char* name, const char* initial, bool writable = false) : name_(name), writable_(writable) { copy_cstr(value_, sizeof(value_), initial); }
    const char* name() const { return name_; }
    const char* get() const { return value_; }
    bool writable() const { return writable_; }
    bool changed() const { return changed_; }
    void clear_changed() { changed_ = false; }
    bool set(const char* value) {
        char stripped[Capacity]{};
        if (!strip_optional_quotes(value, stripped, sizeof(stripped))) return false;
        if (strcmp(value_, stripped) != 0) { copy_cstr(value_, sizeof(value_), stripped); changed_ = true; }
        return true;
    }
    bool parse_set(const char* text) { return set(text); }
    bool format(char* out, size_t out_size) const { return snprintf(out, out_size, "%s=\"%s\"\n", name_, value_) > 0; }
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
    RuntimeNumber heading{"ap.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber heading_error{"ap.heading_error", 0.0, -180.0, 180.0, false};
};

struct BoatImuValues {
    RuntimeNumber heading{"imu.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber roll{"imu.roll", 0.0, -180.0, 180.0, false};
    RuntimeNumber pitch{"imu.pitch", 0.0, -180.0, 180.0, false};
    RuntimeNumber heading_lowpass{"imu.heading_lowpass", 0.0, 0.0, 360.0, false};
    RuntimeNumber alignment_counter{"imu.alignmentCounter", 0.0, 0.0, 1000000.0, false};
    RuntimeNumber uptime{"imu.uptime", 0.0, 0.0, 1000000000.0, false};
};

struct ServoValues {
    RuntimeNumber command{"servo.command", 0.0, -1.0, 1.0, true};
    RuntimeBool engaged{"servo.engaged", false, true};
    RuntimeString<24> state{"servo.state", "idle", false};
    RuntimeString<64> flags{"servo.flags", "", false};
    RuntimeNumber voltage{"servo.voltage", 0.0, 0.0, 100.0, false};
    RuntimeNumber current{"servo.current", 0.0, -100.0, 100.0, false};
    RuntimeString<32> controller{"servo.controller", "", false};
    RuntimeNumber amp_hours{"servo.amp_hours", 0.0, -1000000.0, 1000000.0, false};
};

struct SensorValues {};
struct PilotValues {};

struct GpsValues {
    RuntimeNumber speed{"gps.speed", 0.0, 0.0, 200.0, false};
    RuntimeNumber track{"gps.track", 0.0, 0.0, 360.0, false};
    RuntimeString<32> source{"gps.source", "", false};
};

struct WindValues {
    RuntimeNumber direction{"wind.direction", 0.0, 0.0, 360.0, false};
    RuntimeNumber speed{"wind.speed", 0.0, 0.0, 200.0, false};
    RuntimeString<32> source{"wind.source", "", false};
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
    ApEnabled, ApMode, ApPilot, ApHeadingCommand, ApHeading, ApHeadingError,
    ImuHeading, ImuRoll, ImuPitch, ImuHeadingLowpass, ImuAlignmentCounter, ImuUptime,
    ServoCommand, ServoEngaged, ServoState, ServoFlags, ServoVoltage, ServoCurrent, ServoController, ServoAmpHours,
    GpsSpeed, GpsTrack, GpsSource,
    WindDirection, WindSpeed, WindSource,
    Unknown
};

static inline PypilotValueId parse_value_name(const char* name) {
    if (!name) return PypilotValueId::Unknown;
    if (strcmp(name, "ap.enabled") == 0) return PypilotValueId::ApEnabled;
    if (strcmp(name, "ap.mode") == 0) return PypilotValueId::ApMode;
    if (strcmp(name, "ap.pilot") == 0) return PypilotValueId::ApPilot;
    if (strcmp(name, "ap.heading_command") == 0) return PypilotValueId::ApHeadingCommand;
    if (strcmp(name, "ap.heading") == 0) return PypilotValueId::ApHeading;
    if (strcmp(name, "ap.heading_error") == 0) return PypilotValueId::ApHeadingError;
    if (strcmp(name, "imu.heading") == 0) return PypilotValueId::ImuHeading;
    if (strcmp(name, "imu.roll") == 0) return PypilotValueId::ImuRoll;
    if (strcmp(name, "imu.pitch") == 0) return PypilotValueId::ImuPitch;
    if (strcmp(name, "imu.heading_lowpass") == 0) return PypilotValueId::ImuHeadingLowpass;
    if (strcmp(name, "imu.alignmentCounter") == 0) return PypilotValueId::ImuAlignmentCounter;
    if (strcmp(name, "imu.uptime") == 0) return PypilotValueId::ImuUptime;
    if (strcmp(name, "servo.command") == 0) return PypilotValueId::ServoCommand;
    if (strcmp(name, "servo.engaged") == 0) return PypilotValueId::ServoEngaged;
    if (strcmp(name, "servo.state") == 0) return PypilotValueId::ServoState;
    if (strcmp(name, "servo.flags") == 0) return PypilotValueId::ServoFlags;
    if (strcmp(name, "servo.voltage") == 0) return PypilotValueId::ServoVoltage;
    if (strcmp(name, "servo.current") == 0) return PypilotValueId::ServoCurrent;
    if (strcmp(name, "servo.controller") == 0) return PypilotValueId::ServoController;
    if (strcmp(name, "servo.amp_hours") == 0) return PypilotValueId::ServoAmpHours;
    if (strcmp(name, "gps.speed") == 0) return PypilotValueId::GpsSpeed;
    if (strcmp(name, "gps.track") == 0) return PypilotValueId::GpsTrack;
    if (strcmp(name, "gps.source") == 0) return PypilotValueId::GpsSource;
    if (strcmp(name, "wind.direction") == 0) return PypilotValueId::WindDirection;
    if (strcmp(name, "wind.speed") == 0) return PypilotValueId::WindSpeed;
    if (strcmp(name, "wind.source") == 0) return PypilotValueId::WindSource;
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
        case PypilotValueId::ApHeading: return state_.autopilot.heading.changed();
        case PypilotValueId::ApHeadingError: return state_.autopilot.heading_error.changed();
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.changed();
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.changed();
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.changed();
        case PypilotValueId::ImuHeadingLowpass: return state_.boatimu.heading_lowpass.changed();
        case PypilotValueId::ImuAlignmentCounter: return state_.boatimu.alignment_counter.changed();
        case PypilotValueId::ImuUptime: return state_.boatimu.uptime.changed();
        case PypilotValueId::ServoCommand: return state_.servo.command.changed();
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.changed();
        case PypilotValueId::ServoState: return state_.servo.state.changed();
        case PypilotValueId::ServoFlags: return state_.servo.flags.changed();
        case PypilotValueId::ServoVoltage: return state_.servo.voltage.changed();
        case PypilotValueId::ServoCurrent: return state_.servo.current.changed();
        case PypilotValueId::ServoController: return state_.servo.controller.changed();
        case PypilotValueId::ServoAmpHours: return state_.servo.amp_hours.changed();
        case PypilotValueId::GpsSpeed: return state_.gps.speed.changed();
        case PypilotValueId::GpsTrack: return state_.gps.track.changed();
        case PypilotValueId::GpsSource: return state_.gps.source.changed();
        case PypilotValueId::WindDirection: return state_.wind.direction.changed();
        case PypilotValueId::WindSpeed: return state_.wind.speed.changed();
        case PypilotValueId::WindSource: return state_.wind.source.changed();
        default: return false;
        }
    }

    void clear_changed(PypilotValueId id) {
        switch (id) {
        case PypilotValueId::ApEnabled: state_.autopilot.enabled.clear_changed(); break;
        case PypilotValueId::ApMode: state_.autopilot.mode.clear_changed(); break;
        case PypilotValueId::ApPilot: state_.autopilot.pilot.clear_changed(); break;
        case PypilotValueId::ApHeadingCommand: state_.autopilot.heading_command.clear_changed(); break;
        case PypilotValueId::ApHeading: state_.autopilot.heading.clear_changed(); break;
        case PypilotValueId::ApHeadingError: state_.autopilot.heading_error.clear_changed(); break;
        case PypilotValueId::ImuHeading: state_.boatimu.heading.clear_changed(); break;
        case PypilotValueId::ImuRoll: state_.boatimu.roll.clear_changed(); break;
        case PypilotValueId::ImuPitch: state_.boatimu.pitch.clear_changed(); break;
        case PypilotValueId::ImuHeadingLowpass: state_.boatimu.heading_lowpass.clear_changed(); break;
        case PypilotValueId::ImuAlignmentCounter: state_.boatimu.alignment_counter.clear_changed(); break;
        case PypilotValueId::ImuUptime: state_.boatimu.uptime.clear_changed(); break;
        case PypilotValueId::ServoCommand: state_.servo.command.clear_changed(); break;
        case PypilotValueId::ServoEngaged: state_.servo.engaged.clear_changed(); break;
        case PypilotValueId::ServoState: state_.servo.state.clear_changed(); break;
        case PypilotValueId::ServoFlags: state_.servo.flags.clear_changed(); break;
        case PypilotValueId::ServoVoltage: state_.servo.voltage.clear_changed(); break;
        case PypilotValueId::ServoCurrent: state_.servo.current.clear_changed(); break;
        case PypilotValueId::ServoController: state_.servo.controller.clear_changed(); break;
        case PypilotValueId::ServoAmpHours: state_.servo.amp_hours.clear_changed(); break;
        case PypilotValueId::GpsSpeed: state_.gps.speed.clear_changed(); break;
        case PypilotValueId::GpsTrack: state_.gps.track.clear_changed(); break;
        case PypilotValueId::GpsSource: state_.gps.source.clear_changed(); break;
        case PypilotValueId::WindDirection: state_.wind.direction.clear_changed(); break;
        case PypilotValueId::WindSpeed: state_.wind.speed.clear_changed(); break;
        case PypilotValueId::WindSource: state_.wind.source.clear_changed(); break;
        default: break;
        }
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const PypilotValueId id = parse_value_name(name);
        if (id == PypilotValueId::Unknown) { snprintf(error, error_size, "error=unknown value %s\n", name ? name : ""); return false; }
        if (!writable(id)) { snprintf(error, error_size, "error=%s is not writable\n", name ? name : ""); return false; }
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
        case PypilotValueId::ApHeading: return state_.autopilot.heading.format(out, out_size);
        case PypilotValueId::ApHeadingError: return state_.autopilot.heading_error.format(out, out_size);
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.format(out, out_size);
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.format(out, out_size);
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.format(out, out_size);
        case PypilotValueId::ImuHeadingLowpass: return state_.boatimu.heading_lowpass.format(out, out_size);
        case PypilotValueId::ImuAlignmentCounter: return state_.boatimu.alignment_counter.format(out, out_size);
        case PypilotValueId::ImuUptime: return state_.boatimu.uptime.format(out, out_size);
        case PypilotValueId::ServoCommand: return state_.servo.command.format(out, out_size);
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.format(out, out_size);
        case PypilotValueId::ServoState: return state_.servo.state.format(out, out_size);
        case PypilotValueId::ServoFlags: return state_.servo.flags.format(out, out_size);
        case PypilotValueId::ServoVoltage: return state_.servo.voltage.format(out, out_size);
        case PypilotValueId::ServoCurrent: return state_.servo.current.format(out, out_size);
        case PypilotValueId::ServoController: return state_.servo.controller.format(out, out_size);
        case PypilotValueId::ServoAmpHours: return state_.servo.amp_hours.format(out, out_size);
        case PypilotValueId::GpsSpeed: return state_.gps.speed.format(out, out_size);
        case PypilotValueId::GpsTrack: return state_.gps.track.format(out, out_size);
        case PypilotValueId::GpsSource: return state_.gps.source.format(out, out_size);
        case PypilotValueId::WindDirection: return state_.wind.direction.format(out, out_size);
        case PypilotValueId::WindSpeed: return state_.wind.speed.format(out, out_size);
        case PypilotValueId::WindSource: return state_.wind.source.format(out, out_size);
        default: return false;
        }
    }

    bool format_named_value(const char* name, char* out, size_t out_size) const { return format_value(parse_value_name(name), out, out_size); }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!copy_cstr(out, out_size, "values={")) return false;
        if (!append_cstr(out, out_size, "\"ap.enabled\":{\"type\":\"BooleanValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.mode\":{\"type\":\"StringValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.pilot\":{\"type\":\"StringValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.heading_command\":{\"type\":\"RangeProperty\",\"min\":0,\"max\":360,\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"ap.heading\":{\"type\":\"SensorValue\"},\"ap.heading_error\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.heading\":{\"type\":\"SensorValue\"},\"imu.roll\":{\"type\":\"SensorValue\"},\"imu.pitch\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.heading_lowpass\":{\"type\":\"SensorValue\"},\"imu.alignmentCounter\":{\"type\":\"SensorValue\"},\"imu.uptime\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"servo.command\":{\"type\":\"RangeProperty\",\"min\":-1,\"max\":1,\"writable\":true},\"servo.engaged\":{\"type\":\"BooleanValue\",\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"servo.state\":{\"type\":\"StringValue\"},\"servo.flags\":{\"type\":\"StringValue\"},\"servo.voltage\":{\"type\":\"SensorValue\"},\"servo.current\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"servo.controller\":{\"type\":\"StringValue\"},\"servo.amp_hours\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"gps.speed\":{\"type\":\"SensorValue\"},\"gps.track\":{\"type\":\"SensorValue\"},\"gps.source\":{\"type\":\"StringValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"wind.direction\":{\"type\":\"SensorValue\"},\"wind.speed\":{\"type\":\"SensorValue\"},\"wind.source\":{\"type\":\"StringValue\"}}\n")) return false;
        return true;
    }
private:
    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
