#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pypilot_event_loop.hpp>
#include <pypilot_event_loop/native_tcp.hpp>

namespace pypilot_runtime {

static inline bool copy_cstr(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) {
        return false;
    }
    if (!src) {
        dst[0] = '\0';
        return true;
    }
    size_t i = 0;
    for (; i + 1 < dst_size && src[i]; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
    return src[i] == '\0';
}

static inline bool append_cstr(char* dst, size_t dst_size, const char* src) {
    if (!dst || !src || dst_size == 0) {
        return false;
    }
    const size_t used = strlen(dst);
    if (used >= dst_size) {
        return false;
    }
    return copy_cstr(dst + used, dst_size - used, src);
}

static inline bool parse_bool_text(const char* text, bool& out) {
    if (!text) {
        return false;
    }
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
    if (!text || !*text) {
        return false;
    }
    char* end = nullptr;
    const double value = strtod(text, &end);
    if (!end || *end != '\0') {
        return false;
    }
    out = value;
    return true;
}

static inline bool strip_optional_quotes(const char* text, char* out, size_t out_size) {
    if (!text || !out || out_size == 0) {
        return false;
    }
    const size_t len = strlen(text);
    if (len >= 2 && text[0] == '"' && text[len - 1] == '"') {
        const size_t copy_len = len - 2;
        if (copy_len + 1 > out_size) {
            return false;
        }
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
        if (!parse_bool_text(text, parsed)) {
            return false;
        }
        return set(parsed);
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
        if (value < min_ || value > max_) {
            return false;
        }
        if (value_ != value) {
            value_ = value;
            changed_ = true;
        }
        return true;
    }

    bool parse_set(const char* text) {
        double parsed = 0.0;
        if (!parse_number_text(text, parsed)) {
            return false;
        }
        return set(parsed);
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
        if (!strip_optional_quotes(value, stripped, sizeof(stripped))) {
            return false;
        }
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
};

struct BoatImuValues {
    RuntimeNumber heading{"imu.heading", 0.0, 0.0, 360.0, false};
    RuntimeNumber roll{"imu.roll", 0.0, -180.0, 180.0, false};
    RuntimeNumber pitch{"imu.pitch", 0.0, -180.0, 180.0, false};
};

struct ServoValues {
    RuntimeNumber command{"servo.command", 0.0, -1.0, 1.0, true};
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
    ImuHeading,
    ImuRoll,
    ImuPitch,
    ServoCommand,
    ServoState,
    GpsSpeed,
    WindDirection,
    Unknown
};

static inline PypilotValueId parse_value_name(const char* name) {
    if (!name) return PypilotValueId::Unknown;
    if (strcmp(name, "ap.enabled") == 0) return PypilotValueId::ApEnabled;
    if (strcmp(name, "ap.mode") == 0) return PypilotValueId::ApMode;
    if (strcmp(name, "imu.heading") == 0) return PypilotValueId::ImuHeading;
    if (strcmp(name, "imu.roll") == 0) return PypilotValueId::ImuRoll;
    if (strcmp(name, "imu.pitch") == 0) return PypilotValueId::ImuPitch;
    if (strcmp(name, "servo.command") == 0) return PypilotValueId::ServoCommand;
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
        case PypilotValueId::ServoCommand: return state_.servo.command.writable();
        default: return false;
        }
    }

    bool changed(PypilotValueId id) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.changed();
        case PypilotValueId::ApMode: return state_.autopilot.mode.changed();
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.changed();
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.changed();
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.changed();
        case PypilotValueId::ServoCommand: return state_.servo.command.changed();
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
        case PypilotValueId::ImuHeading: state_.boatimu.heading.clear_changed(); break;
        case PypilotValueId::ImuRoll: state_.boatimu.roll.clear_changed(); break;
        case PypilotValueId::ImuPitch: state_.boatimu.pitch.clear_changed(); break;
        case PypilotValueId::ServoCommand: state_.servo.command.clear_changed(); break;
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
        case PypilotValueId::ServoCommand: ok = state_.servo.command.parse_set(payload); break;
        default: ok = false; break;
        }
        if (!ok) {
            snprintf(error, error_size, "error=invalid value for %s\n", name ? name : "");
        }
        return ok;
    }

    bool format_value(PypilotValueId id, char* out, size_t out_size) const {
        switch (id) {
        case PypilotValueId::ApEnabled: return state_.autopilot.enabled.format(out, out_size);
        case PypilotValueId::ApMode: return state_.autopilot.mode.format(out, out_size);
        case PypilotValueId::ImuHeading: return state_.boatimu.heading.format(out, out_size);
        case PypilotValueId::ImuRoll: return state_.boatimu.roll.format(out, out_size);
        case PypilotValueId::ImuPitch: return state_.boatimu.pitch.format(out, out_size);
        case PypilotValueId::ServoCommand: return state_.servo.command.format(out, out_size);
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
        if (!append_cstr(out, out_size, "\"imu.heading\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.roll\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"imu.pitch\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"servo.command\":{\"type\":\"RangeProperty\",\"min\":-1,\"max\":1,\"writable\":true},")) return false;
        if (!append_cstr(out, out_size, "\"servo.state\":{\"type\":\"StringValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"gps.speed\":{\"type\":\"SensorValue\"},")) return false;
        if (!append_cstr(out, out_size, "\"wind.direction\":{\"type\":\"SensorValue\"}}\n")) return false;
        return true;
    }

private:
    PypilotRuntimeState& state_;
};

#if defined(__linux__) || (defined(ARDUINO) && defined(PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP))
template<size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeServer final : public pypilot_event_loop::ITcpServerHandler {
public:
    template<typename EventLoopType>
    PypilotRuntimeServer(EventLoopType& loop, PypilotRuntimeProtocol& protocol)
        : server_(loop.scheduler(), MaxConnections), protocol_(protocol) {}

    bool listen(const char* host, uint16_t port) {
        pypilot_event_loop::TcpListenOptions options;
        options.host = host;
        options.port = port;
        return server_.listen(options, *this);
    }

    bool listen(uint16_t port) {
        return listen("0.0.0.0", port);
    }

    bool valid() const { return server_.valid(); }
    uint16_t port() const { return server_.port(); }
    size_t connection_count() const { return server_.connection_count(); }
    void close() { server_.close(); reset_slots(); }

    void publish_changed_values() {
        const PypilotValueId ids[] = {
            PypilotValueId::ApEnabled,
            PypilotValueId::ApMode,
            PypilotValueId::ImuHeading,
            PypilotValueId::ImuRoll,
            PypilotValueId::ImuPitch,
            PypilotValueId::ServoCommand,
            PypilotValueId::ServoState,
            PypilotValueId::GpsSpeed,
            PypilotValueId::WindDirection
        };
        for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
            if (protocol_.changed(ids[i])) {
                publish_value(ids[i]);
                protocol_.clear_changed(ids[i]);
            }
        }
    }

    void on_accept(pypilot_event_loop::ITcpConnection& connection, const pypilot_event_loop::TcpPeerInfo&) override {
        ConnectionSlot* slot = slot_for(nullptr);
        if (!slot) {
            connection.close();
            return;
        }
        slot->connection = &connection;
        slot->watch_count = 0;
    }

    void on_data(pypilot_event_loop::ITcpConnection& connection) override {
        char line[320]{};
        while (connection.read_line(line, sizeof(line))) {
            handle_line(connection, line);
        }
    }

    void on_close(pypilot_event_loop::ITcpConnection& connection) override {
        clear_connection(connection);
    }

    void on_error(pypilot_event_loop::ITcpConnection& connection, int) override {
        clear_connection(connection);
    }

private:
    struct ConnectionSlot {
        pypilot_event_loop::ITcpConnection* connection = nullptr;
        PypilotValueId watches[MaxWatchesPerConnection]{};
        size_t watch_count = 0;
    };

    ConnectionSlot* slot_for(pypilot_event_loop::ITcpConnection* connection) {
        for (size_t i = 0; i < MaxConnections; ++i) {
            if (slots_[i].connection == connection) {
                return &slots_[i];
            }
        }
        return nullptr;
    }

    void reset_slots() {
        for (size_t i = 0; i < MaxConnections; ++i) {
            slots_[i].connection = nullptr;
            slots_[i].watch_count = 0;
        }
    }

    void clear_connection(pypilot_event_loop::ITcpConnection& connection) {
        ConnectionSlot* slot = slot_for(&connection);
        if (slot) {
            slot->connection = nullptr;
            slot->watch_count = 0;
        }
    }

    bool send_line(pypilot_event_loop::ITcpConnection& connection, const char* line) {
        return connection.write(reinterpret_cast<const uint8_t*>(line), strlen(line)) >= 0;
    }

    void handle_line(pypilot_event_loop::ITcpConnection& connection, const char* line) {
        if (strncmp(line, "watch=", 6) == 0) {
            handle_watch(connection, line + 6);
            return;
        }
        if (strcmp(line, "values") == 0 || strcmp(line, "values=true") == 0) {
            char out[768]{};
            if (protocol_.write_values_catalog(out, sizeof(out))) {
                send_line(connection, out);
            }
            return;
        }

        const char* eq = strchr(line, '=');
        if (!eq) {
            send_line(connection, "error=invalid command\n");
            return;
        }

        char name[80]{};
        const size_t name_len = static_cast<size_t>(eq - line);
        if (name_len == 0 || name_len >= sizeof(name)) {
            send_line(connection, "error=invalid value name\n");
            return;
        }
        memcpy(name, line, name_len);
        name[name_len] = '\0';

        char error[128]{};
        if (!protocol_.apply_set(name, eq + 1, error, sizeof(error))) {
            send_line(connection, error);
            return;
        }
        publish_value(parse_value_name(name));
    }

    bool add_watch(ConnectionSlot& slot, PypilotValueId id) {
        if (id == PypilotValueId::Unknown) {
            return false;
        }
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (slot.watches[i] == id) {
                return true;
            }
        }
        if (slot.watch_count >= MaxWatchesPerConnection) {
            return false;
        }
        slot.watches[slot.watch_count++] = id;
        return true;
    }

    void remove_watch(ConnectionSlot& slot, PypilotValueId id) {
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (slot.watches[i] == id) {
                for (size_t j = i + 1; j < slot.watch_count; ++j) {
                    slot.watches[j - 1] = slot.watches[j];
                }
                --slot.watch_count;
                return;
            }
        }
    }

    void handle_watch(pypilot_event_loop::ITcpConnection& connection, const char* payload) {
        ConnectionSlot* slot = slot_for(&connection);
        if (!slot) {
            return;
        }
        const char* p = payload;
        while ((p = strchr(p, '"')) != nullptr) {
            ++p;
            char name[80]{};
            size_t n = 0;
            while (*p && *p != '"' && n + 1 < sizeof(name)) {
                name[n++] = *p++;
            }
            name[n] = '\0';
            if (*p != '"') {
                break;
            }
            const char* colon = strchr(p, ':');
            if (!colon) {
                break;
            }
            ++colon;
            const PypilotValueId id = parse_value_name(name);
            if (strncmp(colon, "false", 5) == 0) {
                remove_watch(*slot, id);
            } else if (add_watch(*slot, id)) {
                char out[160]{};
                if (protocol_.format_value(id, out, sizeof(out))) {
                    send_line(connection, out);
                }
            }
            p = colon;
        }
    }

    void publish_value(PypilotValueId id) {
        if (id == PypilotValueId::Unknown) {
            return;
        }
        char out[160]{};
        if (!protocol_.format_value(id, out, sizeof(out))) {
            return;
        }
        for (size_t i = 0; i < MaxConnections; ++i) {
            ConnectionSlot& slot = slots_[i];
            if (!slot.connection || !slot.connection->valid()) {
                continue;
            }
            for (size_t j = 0; j < slot.watch_count; ++j) {
                if (slot.watches[j] == id) {
                    send_line(*slot.connection, out);
                    break;
                }
            }
        }
    }

    pypilot_event_loop::NativeTcpServer server_;
    PypilotRuntimeProtocol& protocol_;
    ConnectionSlot slots_[MaxConnections]{};
};
#endif

} // namespace pypilot_runtime
