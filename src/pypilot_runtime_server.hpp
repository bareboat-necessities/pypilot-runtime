#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <async_event_loop.hpp>
#include <async_event_loop/native_tcp.hpp>
#include <pypilot_runtime_core.hpp>
#include <pypilot_runtime_watch_period.hpp>

namespace pypilot_runtime {

struct PypilotRuntimeServerOptions {
    async_event_loop::TcpTimeoutOptions tcp_timeouts{};
    async_event_loop::TcpWatermarkOptions tcp_watermarks{};
    size_t max_output_bytes = 0;
    uint16_t udp_watch_port = 0;
    bool udp_watch_reuse_address = true;
};

#if defined(__linux__) || (defined(ARDUINO) && defined(ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP))

template<size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeServer final : public async_event_loop::ITcpServerHandler {
public:
    template<typename EventLoopType>
    PypilotRuntimeServer(EventLoopType& loop, PypilotRuntimeProtocol& protocol)
        : server_(loop.scheduler(), MaxConnections), protocol_(protocol), clock_(&loop.clock()) {}

    bool configure(const PypilotRuntimeServerOptions& options) {
        set_tcp_timeouts(options.tcp_timeouts);
        set_tcp_watermarks(options.tcp_watermarks);
        set_max_output_bytes(options.max_output_bytes);
        if (options.udp_watch_port != 0) {
            return enable_udp_watch_stream(options.udp_watch_port, options.udp_watch_reuse_address);
        }
        return true;
    }

    bool listen(const char* host, uint16_t port) {
        async_event_loop::TcpListenOptions options;
        options.host = host;
        options.port = port;
        return server_.listen(options, *this);
    }

    bool listen(uint16_t port) { return listen("0.0.0.0", port); }
    bool valid() const { return server_.valid(); }
    uint16_t port() const { return server_.port(); }
    size_t connection_count() const { return server_.connection_count(); }

    void set_tcp_timeouts(const async_event_loop::TcpTimeoutOptions& options) { tcp_timeouts_ = options; }
    void set_tcp_watermarks(const async_event_loop::TcpWatermarkOptions& options) { tcp_watermarks_ = options; }
    void set_max_output_bytes(size_t max_output_bytes) { max_output_bytes_ = max_output_bytes; }

    bool enable_udp_watch_stream(uint16_t port, bool reuse_address = true) {
#if defined(__linux__) || (defined(ARDUINO) && defined(ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP))
        udp_watch_enabled_ = udp_watch_stream_.bind(port, reuse_address);
        return udp_watch_enabled_;
#else
        (void)port;
        (void)reuse_address;
        udp_watch_enabled_ = false;
        return false;
#endif
    }

    void disable_udp_watch_stream() {
#if defined(__linux__) || (defined(ARDUINO) && defined(ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP))
        udp_watch_stream_.close();
#endif
        udp_watch_enabled_ = false;
        for (size_t i = 0; i < MaxConnections; ++i) slots_[i].has_udp_peer = false;
    }

    bool udp_watch_enabled() const { return udp_watch_enabled_; }

    void close() {
        server_.close();
        reset_slots();
    }

    void publish_changed_values() {
        flush_due_periodic(now());
    }

    void on_accept(async_event_loop::ITcpConnection& connection, const async_event_loop::TcpPeerInfo& peer) override {
        ConnectionSlot* slot = slot_for(nullptr);
        if (!slot) {
            connection.close();
            return;
        }
        apply_tcp_options(connection);
        slot->connection = &connection;
        slot->peer = peer;
        slot->watch_count = 0;
        slot->has_udp_peer = false;
    }

    void on_data(async_event_loop::ITcpConnection& connection) override {
        char line[320]{};
        while (connection.read_line(line, sizeof(line))) handle_line(connection, line);
    }

    void on_close(async_event_loop::ITcpConnection& connection) override { clear_connection(connection); }
    void on_error(async_event_loop::ITcpConnection& connection, int) override { clear_connection(connection); }

private:
    static constexpr size_t ValuesCatalogBufferSize = 8192;

    struct ConnectionSlot {
        async_event_loop::ITcpConnection* connection = nullptr;
        async_event_loop::TcpPeerInfo peer{};
        async_event_loop::UdpEndpoint udp_peer{};
        bool has_udp_peer = false;
        RuntimeWatchPeriod watches[MaxWatchesPerConnection]{};
        size_t watch_count = 0;
    };

    uint64_t now() const { return clock_ ? clock_->micros() : 0; }

    bool has_tcp_timeouts() const { return tcp_timeouts_.read_timeout_ms != 0 || tcp_timeouts_.write_timeout_ms != 0; }
    bool has_tcp_watermarks() const {
        return tcp_watermarks_.read_low != 0 || tcp_watermarks_.read_high != 0 ||
               tcp_watermarks_.write_low != 0 || tcp_watermarks_.write_high != 0;
    }

    void apply_tcp_options(async_event_loop::ITcpConnection& connection) {
        if (has_tcp_timeouts()) connection.set_timeouts(tcp_timeouts_);
        if (has_tcp_watermarks()) connection.set_watermarks(tcp_watermarks_);
    }

    ConnectionSlot* slot_for(async_event_loop::ITcpConnection* connection) {
        for (size_t i = 0; i < MaxConnections; ++i) if (slots_[i].connection == connection) return &slots_[i];
        return nullptr;
    }

    void reset_slots() {
        for (size_t i = 0; i < MaxConnections; ++i) {
            slots_[i].connection = nullptr;
            slots_[i].watch_count = 0;
            slots_[i].has_udp_peer = false;
        }
    }

    void clear_connection(async_event_loop::ITcpConnection& connection) {
        ConnectionSlot* slot = slot_for(&connection);
        if (slot) {
            slot->connection = nullptr;
            slot->watch_count = 0;
            slot->has_udp_peer = false;
        }
    }

    bool output_limit_exceeded(async_event_loop::ITcpConnection& connection) const {
        return max_output_bytes_ != 0 && connection.output_size() > max_output_bytes_;
    }

    bool send_line(async_event_loop::ITcpConnection& connection, const char* line) {
        if (max_output_bytes_ != 0 && connection.output_size() >= max_output_bytes_) {
            clear_connection(connection);
            connection.close();
            return false;
        }
        const int written = connection.write(reinterpret_cast<const uint8_t*>(line), strlen(line));
        if (written < 0) return false;
        if (output_limit_exceeded(connection)) {
            clear_connection(connection);
            connection.close();
            return false;
        }
        return true;
    }

    bool parse_udp_port(const char* text, uint16_t& port) const {
        if (!text) return false;
        unsigned value = 0;
        bool any = false;
        while (*text >= '0' && *text <= '9') {
            any = true;
            value = value * 10u + static_cast<unsigned>(*text - '0');
            if (value > 65535u) return false;
            ++text;
        }
        if (!any) return false;
        port = static_cast<uint16_t>(value);
        return true;
    }

    void handle_udp_port(async_event_loop::ITcpConnection& connection, const char* text) {
        ConnectionSlot* slot = slot_for(&connection);
        if (!slot) return;
        uint16_t udp_port = 0;
        if (!parse_udp_port(text, udp_port)) {
            send_line(connection, "error=invalid udp_port\n");
            return;
        }
        if (udp_port == 0) {
            slot->has_udp_peer = false;
            return;
        }
        runtime_copy_text(slot->udp_peer.host, sizeof(slot->udp_peer.host), slot->peer.host[0] ? slot->peer.host : "127.0.0.1", strlen(slot->peer.host[0] ? slot->peer.host : "127.0.0.1"));
        slot->udp_peer.port = udp_port;
        slot->has_udp_peer = true;
    }

    bool send_watch_line(ConnectionSlot& slot, const char* line) {
        if (!slot.connection || !slot.connection->valid()) return false;
#if defined(__linux__) || (defined(ARDUINO) && defined(ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP))
        if (udp_watch_enabled_ && slot.has_udp_peer) {
            const int n = udp_watch_stream_.send_to(reinterpret_cast<const uint8_t*>(line), strlen(line), slot.udp_peer);
            if (n == static_cast<int>(strlen(line))) return true;
        }
#endif
        return send_line(*slot.connection, line);
    }

    void handle_line(async_event_loop::ITcpConnection& connection, const char* line) {
        if (strncmp(line, "udp_port=", 9) == 0) {
            handle_udp_port(connection, line + 9);
            return;
        }
        if (strncmp(line, "watch=", 6) == 0) {
            handle_watch(connection, line + 6);
            return;
        }
        if (strcmp(line, "values") == 0 || strcmp(line, "values=true") == 0) {
            char out[ValuesCatalogBufferSize]{};
            if (protocol_.write_values_catalog(out, sizeof(out))) send_line(connection, out);
            else send_line(connection, "error=values catalog too large\n");
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

    RuntimeWatchPeriod* find_watch(ConnectionSlot& slot, const char* name) {
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (strcmp(slot.watches[i].name, name) == 0) return &slot.watches[i];
        }
        return nullptr;
    }

    bool add_watch(ConnectionSlot& slot, const char* name, double period_seconds) {
        if (!name || !protocol_.value_exists(name)) return false;
        RuntimeWatchPeriod* watch = find_watch(slot, name);
        if (!watch) {
            if (slot.watch_count >= MaxWatchesPerConnection) return false;
            watch = &slot.watches[slot.watch_count++];
        }
        *watch = make_runtime_watch_period(name, period_seconds, now());
        return true;
    }

    void remove_watch(ConnectionSlot& slot, const char* name) {
        for (size_t i = 0; i < slot.watch_count; ++i) {
            if (strcmp(slot.watches[i].name, name) == 0) {
                for (size_t j = i + 1; j < slot.watch_count; ++j) slot.watches[j - 1] = slot.watches[j];
                --slot.watch_count;
                return;
            }
        }
    }

    void handle_watch(async_event_loop::ITcpConnection& connection, const char* payload) {
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
            if (!protocol_.value_exists(name)) {
                char error[128]{};
                snprintf(error, sizeof(error), "error=unknown watch %s\n", name);
                send_line(connection, error);
            } else if (strncmp(colon, "false", 5) == 0) {
                remove_watch(*slot, name);
            } else {
                double period = 0.0;
                (void)runtime_parse_number(colon, period);
                if (add_watch(*slot, name, period)) send_formatted_value(*slot, name);
            }
            p = colon;
        }
    }

    void send_formatted_value(ConnectionSlot& slot, const char* name) {
        char out[160]{};
        if (protocol_.format_value(name, out, sizeof(out))) send_watch_line(slot, out);
    }

    void flush_due_periodic(uint64_t now_us) {
        for (size_t i = 0; i < MaxConnections; ++i) {
            ConnectionSlot& slot = slots_[i];
            if (!slot.connection || !slot.connection->valid()) continue;
            for (size_t j = 0; j < slot.watch_count; ++j) {
                RuntimeWatchPeriod& watch = slot.watches[j];
                if (runtime_watch_due(watch, now_us)) send_formatted_value(slot, watch.name);
            }
        }
    }

    async_event_loop::NativeTcpServer server_;
    PypilotRuntimeProtocol& protocol_;
    async_event_loop::IClock* clock_ = nullptr;
    async_event_loop::TcpTimeoutOptions tcp_timeouts_{};
    async_event_loop::TcpWatermarkOptions tcp_watermarks_{};
    size_t max_output_bytes_ = 0;
    bool udp_watch_enabled_ = false;
#if defined(__linux__) || (defined(ARDUINO) && defined(ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP))
    async_event_loop::NativeUdpDatagramStream udp_watch_stream_{};
#endif
    ConnectionSlot slots_[MaxConnections]{};
};

#else

template<size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeServer final {
public:
    explicit PypilotRuntimeServer(PypilotRuntimeProtocol&) {}
    template<typename EventLoopType>
    PypilotRuntimeServer(EventLoopType&, PypilotRuntimeProtocol&) {}
    bool configure(const PypilotRuntimeServerOptions&) { return false; }
    bool listen(const char*, uint16_t) { return false; }
    bool listen(uint16_t) { return false; }
    bool valid() const { return false; }
    uint16_t port() const { return 0; }
    size_t connection_count() const { return 0; }
    void close() {}
    void publish_changed_values() {}
};

#endif

} // namespace pypilot_runtime
