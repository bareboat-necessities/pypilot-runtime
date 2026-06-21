#pragma once

#include <stdio.h>
#include <string.h>

#include <pypilot_event_loop.hpp>
#include <pypilot_event_loop/native_tcp.hpp>
#include <pypilot_runtime_core.hpp>
#include <pypilot_runtime_watch_period.hpp>

namespace pypilot_runtime {

#if defined(__linux__) || (defined(ARDUINO) && defined(PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP))

template<size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeServer final : public pypilot_event_loop::ITcpServerHandler {
public:
    template<typename EventLoopType>
    PypilotRuntimeServer(EventLoopType& loop, PypilotRuntimeProtocol& protocol)
        : server_(loop.scheduler(), MaxConnections), protocol_(protocol), clock_(&loop.clock()) {}

    bool listen(const char* host, uint16_t port) {
        pypilot_event_loop::TcpListenOptions options;
        options.host = host;
        options.port = port;
        return server_.listen(options, *this);
    }

    bool listen(uint16_t port) { return listen("0.0.0.0", port); }
    bool valid() const { return server_.valid(); }
    uint16_t port() const { return server_.port(); }
    size_t connection_count() const { return server_.connection_count(); }

    void set_tcp_timeouts(const pypilot_event_loop::TcpTimeoutOptions& options) {
        tcp_timeouts_ = options;
    }

    void set_tcp_watermarks(const pypilot_event_loop::TcpWatermarkOptions& options) {
        tcp_watermarks_ = options;
    }

    void set_max_output_bytes(size_t max_output_bytes) {
        max_output_bytes_ = max_output_bytes;
    }

    void close() {
        server_.close();
        reset_slots();
    }

    void publish_changed_values() {
        const uint64_t now_us = now();
        const PypilotValueId ids[] = {
            PypilotValueId::ApEnabled, PypilotValueId::ApMode, PypilotValueId::ApPilot,
            PypilotValueId::ApHeadingCommand, PypilotValueId::ApHeading, PypilotValueId::ApHeadingError,
            PypilotValueId::ImuHeading, PypilotValueId::ImuRoll, PypilotValueId::ImuPitch,
            PypilotValueId::ImuHeadingLowpass, PypilotValueId::ImuAlignmentCounter, PypilotValueId::ImuUptime,
            PypilotValueId::ServoCommand, PypilotValueId::ServoEngaged, PypilotValueId::ServoState,
            PypilotValueId::ServoFlags, PypilotValueId::ServoVoltage, PypilotValueId::ServoCurrent,
            PypilotValueId::ServoController, PypilotValueId::ServoAmpHours,
            PypilotValueId::GpsSpeed, PypilotValueId::GpsTrack, PypilotValueId::GpsSource,
            PypilotValueId::WindDirection, PypilotValueId::WindSpeed, PypilotValueId::WindSource
        };
        for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
            if (protocol_.changed(ids[i])) {
                mark_or_publish_changed(ids[i], now_us);
                protocol_.clear_changed(ids[i]);
            }
        }
        flush_due_periodic(now_us);
    }

    void on_accept(pypilot_event_loop::ITcpConnection& connection, const pypilot_event_loop::TcpPeerInfo&) override {
        ConnectionSlot* slot = slot_for(nullptr);
        if (!slot) {
            connection.close();
            return;
        }
        apply_tcp_options(connection);
        slot->connection = &connection;
        slot->watch_count = 0;
    }

    void on_data(pypilot_event_loop::ITcpConnection& connection) override {
        char line[320]{};
        while (connection.read_line(line, sizeof(line))) {
            handle_line(connection, line);
        }
    }

    void on_close(pypilot_event_loop::ITcpConnection& connection) override { clear_connection(connection); }
    void on_error(pypilot_event_loop::ITcpConnection& connection, int) override { clear_connection(connection); }

private:
    struct ConnectionSlot {
        pypilot_event_loop::ITcpConnection* connection = nullptr;
        RuntimeWatchPeriod watches[MaxWatchesPerConnection]{};
        size_t watch_count = 0;
    };

    uint64_t now() const { return clock_ ? clock_->micros() : 0; }

    bool has_tcp_timeouts() const {
        return tcp_timeouts_.read_timeout_ms != 0 || tcp_timeouts_.write_timeout_ms != 0;
    }

    bool has_tcp_watermarks() const {
        return tcp_watermarks_.read_low != 0 || tcp_watermarks_.read_high != 0 ||
               tcp_watermarks_.write_low != 0 || tcp_watermarks_.write_high != 0;
    }

    void apply_tcp_options(pypilot_event_loop::ITcpConnection& connection) {
        if (has_tcp_timeouts()) {
            connection.set_timeouts(tcp_timeouts_);
        }
        if (has_tcp_watermarks()) {
            connection.set_watermarks(tcp_watermarks_);
        }
    }

    ConnectionSlot* slot_for(pypilot_event_loop::ITcpConnection* connection) {
        for (size_t i = 0; i < MaxConnections; ++i) {
            if (slots_[i].connection == connection) return &slots_[i];
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

    bool output_limit_exceeded(pypilot_event_loop::ITcpConnection& connection) const {
        return max_output_bytes_ != 0 && connection.output_size() > max_output_bytes_;
    }

    bool send_line(pypilot_event_loop::ITcpConnection& connection, const char* line) {
        if (max_output_bytes_ != 0 && connection.output_size() >= max_output_bytes_) {
            clear_connection(connection);
            connection.close();
            return false;
        }
        const int written = connection.write(reinterpret_cast<const uint8_t*>(line), strlen(line));
        if (written < 0) {
            return false;
        }
        if (output_limit_exceeded(connection)) {
            clear_connection(connection);
            connection.close();
            return false;
        }
        return true;
    }

    void handle_line(pypilot_event_loop::ITcpConnection& connection, const char* line) {
        if (strncmp(line, "watch=", 6) == 0) {
            handle_watch(connection, line + 6);
            return;
        }
        if (strcmp(line, "values") == 0 || strcmp(line, "values=true") == 0) {
            char out[2048]{};
            if (protocol_.write_values_catalog(out, sizeof(out))) {
                send_line(connection, out);
            } else {
                send_line(connection, "error=values catalog too large\n");
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
        publish_changed_values();
    }

    RuntimeWatchPeriod* find_watch(ConnectionSlot& slot, PypilotValueId id) {
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (slot.watches[i].id == id) return &slot.watches[i];
        }
        return nullptr;
    }

    bool add_watch(ConnectionSlot& slot, PypilotValueId id, double period_seconds) {
        if (id == PypilotValueId::Unknown) return false;
        RuntimeWatchPeriod* watch = find_watch(slot, id);
        if (!watch) {
            if (slot.watch_count >= MaxWatchesPerConnection) return false;
            watch = &slot.watches[slot.watch_count++];
        }
        *watch = make_runtime_watch_period(id, period_seconds, now());
        return true;
    }

    void remove_watch(ConnectionSlot& slot, PypilotValueId id) {
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (slot.watches[i].id == id) {
                for (size_t j = i + 1; j < slot.watch_count; ++j) slot.watches[j - 1] = slot.watches[j];
                --slot.watch_count;
                return;
            }
        }
    }

    void handle_watch(pypilot_event_loop::ITcpConnection& connection, const char* payload) {
        ConnectionSlot* slot = slot_for(&connection);
        if (!slot) return;
        const char* p = payload;
        while ((p = strchr(p, '"')) != nullptr) {
            ++p;
            char name[80]{};
            size_t n = 0;
            while (*p && *p != '"' && n + 1 < sizeof(name)) name[n++] = *p++;
            name[n] = '\0';
            if (*p != '"') break;
            const char* colon = strchr(p, ':');
            if (!colon) break;
            ++colon;
            const PypilotValueId id = parse_value_name(name);
            if (id == PypilotValueId::Unknown) {
                char error[128]{};
                snprintf(error, sizeof(error), "error=unknown watch %s\n", name);
                send_line(connection, error);
            } else if (strncmp(colon, "false", 5) == 0) {
                remove_watch(*slot, id);
            } else {
                double period = 0.0;
                parse_number_text(colon, period);
                if (add_watch(*slot, id, period)) send_formatted_value(connection, id);
            }
            p = colon;
        }
    }

    void send_formatted_value(pypilot_event_loop::ITcpConnection& connection, PypilotValueId id) {
        char out[160]{};
        if (protocol_.format_value(id, out, sizeof(out))) send_line(connection, out);
    }

    void mark_or_publish_changed(PypilotValueId id, uint64_t now_us) {
        for (size_t i = 0; i < MaxConnections; ++i) {
            ConnectionSlot& slot = slots_[i];
            if (!slot.connection || !slot.connection->valid()) continue;
            for (size_t j = 0; j < slot.watch_count; ++j) {
                RuntimeWatchPeriod& watch = slot.watches[j];
                if (watch.id != id) continue;
                if (mark_runtime_watch_pending(watch, now_us)) {
                    send_formatted_value(*slot.connection, id);
                }
            }
        }
    }

    void flush_due_periodic(uint64_t now_us) {
        for (size_t i = 0; i < MaxConnections; ++i) {
            ConnectionSlot& slot = slots_[i];
            if (!slot.connection || !slot.connection->valid()) continue;
            for (size_t j = 0; j < slot.watch_count; ++j) {
                RuntimeWatchPeriod& watch = slot.watches[j];
                if (!watch.continuous && runtime_watch_due(watch, now_us)) {
                    send_formatted_value(*slot.connection, watch.id);
                }
            }
        }
    }

    pypilot_event_loop::NativeTcpServer server_;
    PypilotRuntimeProtocol& protocol_;
    pypilot_event_loop::IClock* clock_ = nullptr;
    pypilot_event_loop::TcpTimeoutOptions tcp_timeouts_{};
    pypilot_event_loop::TcpWatermarkOptions tcp_watermarks_{};
    size_t max_output_bytes_ = 0;
    ConnectionSlot slots_[MaxConnections]{};
};

#endif

} // namespace pypilot_runtime
