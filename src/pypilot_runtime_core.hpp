#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace pypilot_runtime {

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
    RuntimeString<24> preferred_mode{"ap.preferred_mode", "compass", true};
    RuntimeString<24> pilot{"ap.pilot", "basic", true};
    RuntimeNumber heading_command{"ap.heading_command", 0.0, 0.0, 360.0, true};
    RuntimeNumber heading{"ap.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber heading_error{"ap.heading_error", 0.0, -180.0, 180.0, false};
};

struct BoatImuValues {
    RuntimeNumber heading{"imu.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber roll{"imu.roll", 0.0, -180.0, 180.0, false};
    RuntimeNumber pitch{"imu.pitch", 0.0, -180.0, 180.0, false};
    RuntimeNumber heel{"imu.heel", 0.0, -180.0, 180.0, false};
    RuntimeNumber headingrate{"imu.headingrate", 0.0, -1000.0, 1000.0, false};
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
    RuntimeNumber position{"servo.position", 0.0, -180.0, 180.0, false};
    RuntimeNumber controller_temp{"servo.controller_temp", 0.0, -50.0, 200.0, false};
    RuntimeNumber motor_temp{"servo.motor_temp", 0.0, -50.0, 200.0, false};
    RuntimeString<32> controller{"servo.controller", "", false};
    RuntimeNumber amp_hours{"servo.amp_hours", 0.0, -1000000.0, 1000000.0, false};
};

struct SensorValues {
    RuntimeString<32> truewind_source{"truewind.source", "", false};
    RuntimeNumber truewind_direction{"truewind.direction", 0.0, 0.0, 360.0, false};
    RuntimeNumber truewind_speed{"truewind.speed", 0.0, 0.0, 200.0, false};
    RuntimeNumber water_speed{"water.speed", 0.0, 0.0, 200.0, false};
    RuntimeNumber rudder_angle{"rudder.angle", 0.0, -180.0, 180.0, false};
    RuntimeString<64> profile_name{"profile.name", "default", true};
    RuntimeString<32> server_version{"server.version", "", false};
    RuntimeNumber server_uptime{"server.uptime", 0.0, 0.0, 1000000000.0, false};
    RuntimeNumber status_faults{"status.faults", 0.0, 0.0, 4294967295.0, false};
    RuntimeNumber status_warnings{"status.warnings", 0.0, 0.0, 4294967295.0, false};
    RuntimeNumber runtime_published_value_count{"runtime.published_value_count", 0.0, 0.0, 4294967295.0, false};
};

struct PilotValues {
    RuntimeNumber tack_state{"ap.tack.state", 0.0, 0.0, 10.0, true};
    RuntimeNumber tack_direction{"ap.tack.direction", 0.0, 0.0, 2.0, true};
};

struct GpsValues {
    RuntimeNumber speed{"gps.speed", 0.0, 0.0, 200.0, false};
    RuntimeNumber track{"gps.track", 0.0, 0.0, 360.0, false};
    RuntimeNumber timestamp{"gps.timestamp", 0.0, 0.0, 1000000000.0, false};
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
    ApEnabled, ApMode, ApPreferredMode, ApPilot, ApHeadingCommand, ApHeading, ApHeadingError,
    ImuHeading, ImuRoll, ImuPitch, ImuHeel, ImuHeadingrate, ImuHeadingLowpass, ImuAlignmentCounter, ImuUptime,
    ServoCommand, ServoEngaged, ServoState, ServoFlags, ServoVoltage, ServoCurrent, ServoPosition, ServoControllerTemp, ServoMotorTemp, ServoController, ServoAmpHours,
    GpsSpeed, GpsTrack, GpsTimestamp, GpsSource,
    WindDirection, WindSpeed, WindSource,
    TrueWindDirection, TrueWindSpeed, TrueWindSource,
    WaterSpeed, RudderAngle,
    ApTackState, ApTackDirection,
    ProfileName, ServerVersion, ServerUptime, StatusFaults, StatusWarnings, RuntimePublishedValueCount,
    Unknown
};

static inline PypilotValueId parse_value_name(const char* name) {
    if (!name) return PypilotValueId::Unknown;
    if (strcmp(name, "ap.enabled") == 0) return PypilotValueId::ApEnabled;
    if (strcmp(name, "ap.mode") == 0) return PypilotValueId::ApMode;
    if (strcmp(name, "ap.preferred_mode") == 0) return PypilotValueId::ApPreferredMode;
    if (strcmp(name, "ap.pilot") == 0) return PypilotValueId::ApPilot;
    if (strcmp(name, "ap.heading_command") == 0) return PypilotValueId::ApHeadingCommand;
    if (strcmp(name, "ap.heading") == 0) return PypilotValueId::ApHeading;
    if (strcmp(name, "ap.heading_error") == 0) return PypilotValueId::ApHeadingError;
    if (strcmp(name, "imu.heading") == 0) return PypilotValueId::ImuHeading;
    if (strcmp(name, "imu.roll") == 0) return PypilotValueId::ImuRoll;
    if (strcmp(name, "imu.pitch") == 0) return PypilotValueId::ImuPitch;
    if (strcmp(name, "imu.heel") == 0) return PypilotValueId::ImuHeel;
    if (strcmp(name, "imu.headingrate") == 0) return PypilotValueId::ImuHeadingrate;
    if (strcmp(name, "imu.heading_lowpass") == 0) return PypilotValueId::ImuHeadingLowpass;
    if (strcmp(name, "imu.alignmentCounter") == 0) return PypilotValueId::ImuAlignmentCounter;
    if (strcmp(name, "imu.uptime") == 0) return PypilotValueId::ImuUptime;
    if (strcmp(name, "servo.command") == 0) return PypilotValueId::ServoCommand;
    if (strcmp(name, "servo.engaged") == 0) return PypilotValueId::ServoEngaged;
    if (strcmp(name, "servo.state") == 0) return PypilotValueId::ServoState;
    if (strcmp(name, "servo.flags") == 0) return PypilotValueId::ServoFlags;
    if (strcmp(name, "servo.voltage") == 0) return PypilotValueId::ServoVoltage;
    if (strcmp(name, "servo.current") == 0) return PypilotValueId::ServoCurrent;
    if (strcmp(name, "servo.position") == 0) return PypilotValueId::ServoPosition;
    if (strcmp(name, "servo.controller_temp") == 0) return PypilotValueId::ServoControllerTemp;
    if (strcmp(name, "servo.motor_temp") == 0) return PypilotValueId::ServoMotorTemp;
    if (strcmp(name, "servo.controller") == 0) return PypilotValueId::ServoController;
    if (strcmp(name, "servo.amp_hours") == 0) return PypilotValueId::ServoAmpHours;
    if (strcmp(name, "gps.speed") == 0) return PypilotValueId::GpsSpeed;
    if (strcmp(name, "gps.track") == 0) return PypilotValueId::GpsTrack;
    if (strcmp(name, "gps.timestamp") == 0) return PypilotValueId::GpsTimestamp;
    if (strcmp(name, "gps.source") == 0) return PypilotValueId::GpsSource;
    if (strcmp(name, "wind.direction") == 0) return PypilotValueId::WindDirection;
    if (strcmp(name, "wind.speed") == 0) return PypilotValueId::WindSpeed;
    if (strcmp(name, "wind.source") == 0) return PypilotValueId::WindSource;
    if (strcmp(name, "truewind.direction") == 0) return PypilotValueId::TrueWindDirection;
    if (strcmp(name, "truewind.speed") == 0) return PypilotValueId::TrueWindSpeed;
    if (strcmp(name, "truewind.source") == 0) return PypilotValueId::TrueWindSource;
    if (strcmp(name, "water.speed") == 0) return PypilotValueId::WaterSpeed;
    if (strcmp(name, "rudder.angle") == 0) return PypilotValueId::RudderAngle;
    if (strcmp(name, "ap.tack.state") == 0) return PypilotValueId::ApTackState;
    if (strcmp(name, "ap.tack.direction") == 0) return PypilotValueId::ApTackDirection;
    if (strcmp(name, "profile.name") == 0) return PypilotValueId::ProfileName;
    if (strcmp(name, "server.version") == 0) return PypilotValueId::ServerVersion;
    if (strcmp(name, "server.uptime") == 0) return PypilotValueId::ServerUptime;
    if (strcmp(name, "status.faults") == 0) return PypilotValueId::StatusFaults;
    if (strcmp(name, "status.warnings") == 0) return PypilotValueId::StatusWarnings;
    if (strcmp(name, "runtime.published_value_count") == 0) return PypilotValueId::RuntimePublishedValueCount;
    return PypilotValueId::Unknown;
}

class PypilotRuntimeProtocol final {
public:
    explicit PypilotRuntimeProtocol(PypilotRuntimeState& state) : state_(state) {}

    bool writable(PypilotValueId id) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.writable();
        case PypilotValueId::ApMode: return state_.autopilot.mode.writable();
        case PypilotValueId::ApPreferredMode: return state_.autopilot.preferred_mode.writable();
        case PypilotValueId::ApPilot: return state_.autopilot.pilot.writable();
        case PypilotValueId::ApHeadingCommand: return state_.autopilot.heading_command.writable();
        case PypilotValueId::ServoCommand: return state_.servo.command.writable();
        case PypilotValueId::ServoEngaged: return state_.servo.engaged.writable();
        case PypilotValueId::ApTackState: return state_.pilots.tack_state.writable();
        case PypilotValueId::ApTackDirection: return state_.pilots.tack_direction.writable();
        case PypilotValueId::ProfileName: return state_.sensors.profile_name.writable();
        default: return false;
        }
    }

    bool changed(PypilotValueId id) const {
#define PYPILOT_CHANGED(ID, FIELD) case PypilotValueId::ID: return FIELD.changed()
        switch (id) {
        PYPILOT_CHANGED(ApEnabled, state_.autopilot.enabled);
        PYPILOT_CHANGED(ApMode, state_.autopilot.mode);
        PYPILOT_CHANGED(ApPreferredMode, state_.autopilot.preferred_mode);
        PYPILOT_CHANGED(ApPilot, state_.autopilot.pilot);
        PYPILOT_CHANGED(ApHeadingCommand, state_.autopilot.heading_command);
        PYPILOT_CHANGED(ApHeading, state_.autopilot.heading);
        PYPILOT_CHANGED(ApHeadingError, state_.autopilot.heading_error);
        PYPILOT_CHANGED(ImuHeading, state_.boatimu.heading);
        PYPILOT_CHANGED(ImuRoll, state_.boatimu.roll);
        PYPILOT_CHANGED(ImuPitch, state_.boatimu.pitch);
        PYPILOT_CHANGED(ImuHeel, state_.boatimu.heel);
        PYPILOT_CHANGED(ImuHeadingrate, state_.boatimu.headingrate);
        PYPILOT_CHANGED(ImuHeadingLowpass, state_.boatimu.heading_lowpass);
        PYPILOT_CHANGED(ImuAlignmentCounter, state_.boatimu.alignment_counter);
        PYPILOT_CHANGED(ImuUptime, state_.boatimu.uptime);
        PYPILOT_CHANGED(ServoCommand, state_.servo.command);
        PYPILOT_CHANGED(ServoEngaged, state_.servo.engaged);
        PYPILOT_CHANGED(ServoState, state_.servo.state);
        PYPILOT_CHANGED(ServoFlags, state_.servo.flags);
        PYPILOT_CHANGED(ServoVoltage, state_.servo.voltage);
        PYPILOT_CHANGED(ServoCurrent, state_.servo.current);
        PYPILOT_CHANGED(ServoPosition, state_.servo.position);
        PYPILOT_CHANGED(ServoControllerTemp, state_.servo.controller_temp);
        PYPILOT_CHANGED(ServoMotorTemp, state_.servo.motor_temp);
        PYPILOT_CHANGED(ServoController, state_.servo.controller);
        PYPILOT_CHANGED(ServoAmpHours, state_.servo.amp_hours);
        PYPILOT_CHANGED(GpsSpeed, state_.gps.speed);
        PYPILOT_CHANGED(GpsTrack, state_.gps.track);
        PYPILOT_CHANGED(GpsTimestamp, state_.gps.timestamp);
        PYPILOT_CHANGED(GpsSource, state_.gps.source);
        PYPILOT_CHANGED(WindDirection, state_.wind.direction);
        PYPILOT_CHANGED(WindSpeed, state_.wind.speed);
        PYPILOT_CHANGED(WindSource, state_.wind.source);
        PYPILOT_CHANGED(TrueWindDirection, state_.sensors.truewind_direction);
        PYPILOT_CHANGED(TrueWindSpeed, state_.sensors.truewind_speed);
        PYPILOT_CHANGED(TrueWindSource, state_.sensors.truewind_source);
        PYPILOT_CHANGED(WaterSpeed, state_.sensors.water_speed);
        PYPILOT_CHANGED(RudderAngle, state_.sensors.rudder_angle);
        PYPILOT_CHANGED(ApTackState, state_.pilots.tack_state);
        PYPILOT_CHANGED(ApTackDirection, state_.pilots.tack_direction);
        PYPILOT_CHANGED(ProfileName, state_.sensors.profile_name);
        PYPILOT_CHANGED(ServerVersion, state_.sensors.server_version);
        PYPILOT_CHANGED(ServerUptime, state_.sensors.server_uptime);
        PYPILOT_CHANGED(StatusFaults, state_.sensors.status_faults);
        PYPILOT_CHANGED(StatusWarnings, state_.sensors.status_warnings);
        PYPILOT_CHANGED(RuntimePublishedValueCount, state_.sensors.runtime_published_value_count);
        default: return false;
        }
#undef PYPILOT_CHANGED
    }

    void clear_changed(PypilotValueId id) {
#define PYPILOT_CLEAR(ID, FIELD) case PypilotValueId::ID: FIELD.clear_changed(); break
        switch (id) {
        PYPILOT_CLEAR(ApEnabled, state_.autopilot.enabled);
        PYPILOT_CLEAR(ApMode, state_.autopilot.mode);
        PYPILOT_CLEAR(ApPreferredMode, state_.autopilot.preferred_mode);
        PYPILOT_CLEAR(ApPilot, state_.autopilot.pilot);
        PYPILOT_CLEAR(ApHeadingCommand, state_.autopilot.heading_command);
        PYPILOT_CLEAR(ApHeading, state_.autopilot.heading);
        PYPILOT_CLEAR(ApHeadingError, state_.autopilot.heading_error);
        PYPILOT_CLEAR(ImuHeading, state_.boatimu.heading);
        PYPILOT_CLEAR(ImuRoll, state_.boatimu.roll);
        PYPILOT_CLEAR(ImuPitch, state_.boatimu.pitch);
        PYPILOT_CLEAR(ImuHeel, state_.boatimu.heel);
        PYPILOT_CLEAR(ImuHeadingrate, state_.boatimu.headingrate);
        PYPILOT_CLEAR(ImuHeadingLowpass, state_.boatimu.heading_lowpass);
        PYPILOT_CLEAR(ImuAlignmentCounter, state_.boatimu.alignment_counter);
        PYPILOT_CLEAR(ImuUptime, state_.boatimu.uptime);
        PYPILOT_CLEAR(ServoCommand, state_.servo.command);
        PYPILOT_CLEAR(ServoEngaged, state_.servo.engaged);
        PYPILOT_CLEAR(ServoState, state_.servo.state);
        PYPILOT_CLEAR(ServoFlags, state_.servo.flags);
        PYPILOT_CLEAR(ServoVoltage, state_.servo.voltage);
        PYPILOT_CLEAR(ServoCurrent, state_.servo.current);
        PYPILOT_CLEAR(ServoPosition, state_.servo.position);
        PYPILOT_CLEAR(ServoControllerTemp, state_.servo.controller_temp);
        PYPILOT_CLEAR(ServoMotorTemp, state_.servo.motor_temp);
        PYPILOT_CLEAR(ServoController, state_.servo.controller);
        PYPILOT_CLEAR(ServoAmpHours, state_.servo.amp_hours);
        PYPILOT_CLEAR(GpsSpeed, state_.gps.speed);
        PYPILOT_CLEAR(GpsTrack, state_.gps.track);
        PYPILOT_CLEAR(GpsTimestamp, state_.gps.timestamp);
        PYPILOT_CLEAR(GpsSource, state_.gps.source);
        PYPILOT_CLEAR(WindDirection, state_.wind.direction);
        PYPILOT_CLEAR(WindSpeed, state_.wind.speed);
        PYPILOT_CLEAR(WindSource, state_.wind.source);
        PYPILOT_CLEAR(TrueWindDirection, state_.sensors.truewind_direction);
        PYPILOT_CLEAR(TrueWindSpeed, state_.sensors.truewind_speed);
        PYPILOT_CLEAR(TrueWindSource, state_.sensors.truewind_source);
        PYPILOT_CLEAR(WaterSpeed, state_.sensors.water_speed);
        PYPILOT_CLEAR(RudderAngle, state_.sensors.rudder_angle);
        PYPILOT_CLEAR(ApTackState, state_.pilots.tack_state);
        PYPILOT_CLEAR(ApTackDirection, state_.pilots.tack_direction);
        PYPILOT_CLEAR(ProfileName, state_.sensors.profile_name);
        PYPILOT_CLEAR(ServerVersion, state_.sensors.server_version);
        PYPILOT_CLEAR(ServerUptime, state_.sensors.server_uptime);
        PYPILOT_CLEAR(StatusFaults, state_.sensors.status_faults);
        PYPILOT_CLEAR(StatusWarnings, state_.sensors.status_warnings);
        PYPILOT_CLEAR(RuntimePublishedValueCount, state_.sensors.runtime_published_value_count);
        default: break;
        }
#undef PYPILOT_CLEAR
    }

    bool apply_set(const char* name, const char* payload, char* error, size_t error_size) {
        const PypilotValueId id = parse_value_name(name);
        if (id == PypilotValueId::Unknown) { snprintf(error, error_size, "error=unknown value %s\n", name ? name : ""); return false; }
        if (!writable(id)) { snprintf(error, error_size, "error=%s is not writable\n", name ? name : ""); return false; }
        bool ok = false;
        switch (id) {
        case PypilotValueId::ApEnabled: ok = state_.autopilot.enabled.parse_set(payload); break;
        case PypilotValueId::ApMode: ok = state_.autopilot.mode.parse_set(payload); break;
        case PypilotValueId::ApPreferredMode: ok = state_.autopilot.preferred_mode.parse_set(payload); break;
        case PypilotValueId::ApPilot: ok = state_.autopilot.pilot.parse_set(payload); break;
        case PypilotValueId::ApHeadingCommand: ok = state_.autopilot.heading_command.parse_set(payload); break;
        case PypilotValueId::ServoCommand: ok = state_.servo.command.parse_set(payload); break;
        case PypilotValueId::ServoEngaged: ok = state_.servo.engaged.parse_set(payload); break;
        case PypilotValueId::ApTackState: ok = state_.pilots.tack_state.parse_set(payload); break;
        case PypilotValueId::ApTackDirection: ok = state_.pilots.tack_direction.parse_set(payload); break;
        case PypilotValueId::ProfileName: ok = state_.sensors.profile_name.parse_set(payload); break;
        default: ok = false; break;
        }
        if (!ok) snprintf(error, error_size, "error=invalid value for %s\n", name ? name : "");
        return ok;
    }

    bool format_value(PypilotValueId id, char* out, size_t out_size) const {
#define PYPILOT_FORMAT(ID, FIELD) case PypilotValueId::ID: return FIELD.format(out, out_size)
        switch (id) {
        PYPILOT_FORMAT(ApEnabled, state_.autopilot.enabled);
        PYPILOT_FORMAT(ApMode, state_.autopilot.mode);
        PYPILOT_FORMAT(ApPreferredMode, state_.autopilot.preferred_mode);
        PYPILOT_FORMAT(ApPilot, state_.autopilot.pilot);
        PYPILOT_FORMAT(ApHeadingCommand, state_.autopilot.heading_command);
        PYPILOT_FORMAT(ApHeading, state_.autopilot.heading);
        PYPILOT_FORMAT(ApHeadingError, state_.autopilot.heading_error);
        PYPILOT_FORMAT(ImuHeading, state_.boatimu.heading);
        PYPILOT_FORMAT(ImuRoll, state_.boatimu.roll);
        PYPILOT_FORMAT(ImuPitch, state_.boatimu.pitch);
        PYPILOT_FORMAT(ImuHeel, state_.boatimu.heel);
        PYPILOT_FORMAT(ImuHeadingrate, state_.boatimu.headingrate);
        PYPILOT_FORMAT(ImuHeadingLowpass, state_.boatimu.heading_lowpass);
        PYPILOT_FORMAT(ImuAlignmentCounter, state_.boatimu.alignment_counter);
        PYPILOT_FORMAT(ImuUptime, state_.boatimu.uptime);
        PYPILOT_FORMAT(ServoCommand, state_.servo.command);
        PYPILOT_FORMAT(ServoEngaged, state_.servo.engaged);
        PYPILOT_FORMAT(ServoState, state_.servo.state);
        PYPILOT_FORMAT(ServoFlags, state_.servo.flags);
        PYPILOT_FORMAT(ServoVoltage, state_.servo.voltage);
        PYPILOT_FORMAT(ServoCurrent, state_.servo.current);
        PYPILOT_FORMAT(ServoPosition, state_.servo.position);
        PYPILOT_FORMAT(ServoControllerTemp, state_.servo.controller_temp);
        PYPILOT_FORMAT(ServoMotorTemp, state_.servo.motor_temp);
        PYPILOT_FORMAT(ServoController, state_.servo.controller);
        PYPILOT_FORMAT(ServoAmpHours, state_.servo.amp_hours);
        PYPILOT_FORMAT(GpsSpeed, state_.gps.speed);
        PYPILOT_FORMAT(GpsTrack, state_.gps.track);
        PYPILOT_FORMAT(GpsTimestamp, state_.gps.timestamp);
        PYPILOT_FORMAT(GpsSource, state_.gps.source);
        PYPILOT_FORMAT(WindDirection, state_.wind.direction);
        PYPILOT_FORMAT(WindSpeed, state_.wind.speed);
        PYPILOT_FORMAT(WindSource, state_.wind.source);
        PYPILOT_FORMAT(TrueWindDirection, state_.sensors.truewind_direction);
        PYPILOT_FORMAT(TrueWindSpeed, state_.sensors.truewind_speed);
        PYPILOT_FORMAT(TrueWindSource, state_.sensors.truewind_source);
        PYPILOT_FORMAT(WaterSpeed, state_.sensors.water_speed);
        PYPILOT_FORMAT(RudderAngle, state_.sensors.rudder_angle);
        PYPILOT_FORMAT(ApTackState, state_.pilots.tack_state);
        PYPILOT_FORMAT(ApTackDirection, state_.pilots.tack_direction);
        PYPILOT_FORMAT(ProfileName, state_.sensors.profile_name);
        PYPILOT_FORMAT(ServerVersion, state_.sensors.server_version);
        PYPILOT_FORMAT(ServerUptime, state_.sensors.server_uptime);
        PYPILOT_FORMAT(StatusFaults, state_.sensors.status_faults);
        PYPILOT_FORMAT(StatusWarnings, state_.sensors.status_warnings);
        PYPILOT_FORMAT(RuntimePublishedValueCount, state_.sensors.runtime_published_value_count);
        default: return false;
        }
#undef PYPILOT_FORMAT
    }

    bool format_named_value(const char* name, char* out, size_t out_size) const { return format_value(parse_value_name(name), out, out_size); }

    bool write_values_catalog(char* out, size_t out_size) const {
        if (!copy_cstr(out, out_size, "values={")) return false;
        const char* names[] = {
            "ap.enabled", "ap.mode", "ap.preferred_mode", "ap.pilot", "ap.heading_command", "ap.heading", "ap.heading_error",
            "imu.heading", "imu.roll", "imu.pitch", "imu.heel", "imu.headingrate", "imu.heading_lowpass", "imu.alignmentCounter", "imu.uptime",
            "servo.command", "servo.engaged", "servo.state", "servo.flags", "servo.voltage", "servo.current", "servo.position", "servo.controller_temp", "servo.motor_temp", "servo.controller", "servo.amp_hours",
            "gps.speed", "gps.track", "gps.timestamp", "gps.source",
            "wind.direction", "wind.speed", "wind.source", "truewind.direction", "truewind.speed", "truewind.source", "water.speed", "rudder.angle", "ap.tack.state", "ap.tack.direction", "profile.name", "server.version", "server.uptime", "status.faults", "status.warnings", "runtime.published_value_count"
        };
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
            if (i && !append_cstr(out, out_size, ",")) return false;
            if (!append_cstr(out, out_size, "\"")) return false;
            if (!append_cstr(out, out_size, names[i])) return false;
            if (!append_cstr(out, out_size, "\":{\"type\":\"Value\"}")) return false;
        }
        return append_cstr(out, out_size, "}\n");
    }
private:
    PypilotRuntimeState& state_;
};

} // namespace pypilot_runtime
