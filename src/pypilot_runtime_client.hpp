#pragma once

#include <stdio.h>
#include <string.h>

#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

#if defined(__linux__) || (defined(ARDUINO) && defined(PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP))

struct PypilotClientValue {
    char name[80]{};
    char payload[160]{};
    PypilotValueId id = PypilotValueId::Unknown;
};

template<size_t LineSize = 256>
class PypilotRuntimeClient final : public pypilot_event_loop::ITcpClientHandler {
public:
    template<typename EventLoopType>
    explicit PypilotRuntimeClient(EventLoopType& loop) : client_(loop.scheduler()) {}

    bool open(const char* host, uint16_t port) {
        pypilot_event_loop::TcpConnectOptions options;
        options.host = host;
        options.port = port;
        return client_.connect(options, *this);
    }

    bool connected() const { return connection_ && connection_->valid(); }
    bool valid() const { return client_.valid(); }
    size_t output_size() const { return connection_ ? connection_->output_size() : 0; }

    void set_tcp_timeouts(const pypilot_event_loop::TcpTimeoutOptions& options) {
        tcp_timeouts_ = options;
        if (connection_) connection_->set_timeouts(tcp_timeouts_);
    }

    void set_tcp_watermarks(const pypilot_event_loop::TcpWatermarkOptions& options) {
        tcp_watermarks_ = options;
        if (connection_) connection_->set_watermarks(tcp_watermarks_);
    }

    void set_max_output_bytes(size_t max_output_bytes) {
        max_output_bytes_ = max_output_bytes;
    }

    void close() {
        client_.close();
        connection_ = nullptr;
    }

    bool set_bool(const char* name, bool value) {
        char line[LineSize]{};
        snprintf(line, sizeof(line), "%s=%s\n", name, value ? "true" : "false");
        return send_line(line);
    }

    bool set_number(const char* name, double value) {
        char line[LineSize]{};
        snprintf(line, sizeof(line), "%s=%.4f\n", name, value);
        return send_line(line);
    }

    bool set_string(const char* name, const char* value) {
        char line[LineSize]{};
        snprintf(line, sizeof(line), "%s=\"%s\"\n", name, value ? value : "");
        return send_line(line);
    }

    bool watch(const char* name, double period_seconds) {
        char line[LineSize]{};
        snprintf(line, sizeof(line), "watch={\"%s\":%.4f}\n", name, period_seconds);
        return send_line(line);
    }

    bool unwatch(const char* name) {
        char line[LineSize]{};
        snprintf(line, sizeof(line), "watch={\"%s\":false}\n", name);
        return send_line(line);
    }

    bool read_line(char* out, size_t out_size) {
        return connection_ && connection_->read_line(out, out_size);
    }

    bool read_value(PypilotClientValue& value) {
        char line[LineSize]{};
        if (!read_line(line, sizeof(line))) return false;
        const char* eq = strchr(line, '=');
        if (!eq || eq == line) return false;
        const size_t name_len = static_cast<size_t>(eq - line);
        if (name_len >= sizeof(value.name)) return false;
        memcpy(value.name, line, name_len);
        value.name[name_len] = '\0';
        if (!copy_cstr(value.payload, sizeof(value.payload), eq + 1)) return false;
        value.id = parse_value_name(value.name);
        return true;
    }

    bool read_bool(PypilotClientValue& value, bool& out) {
        return read_value(value) && parse_bool_text(value.payload, out);
    }

    bool read_number(PypilotClientValue& value, double& out) {
        return read_value(value) && parse_number_text(value.payload, out);
    }

    bool read_string(PypilotClientValue& value, char* out, size_t out_size) {
        return read_value(value) && strip_optional_quotes(value.payload, out, out_size);
    }

    void on_connect(pypilot_event_loop::ITcpConnection& connection, const pypilot_event_loop::TcpPeerInfo&) override {
        connection_ = &connection;
        apply_tcp_options();
    }

    void on_close(pypilot_event_loop::ITcpConnection&) override {
        connection_ = nullptr;
    }

    void on_error(int) override {
        connection_ = nullptr;
    }

private:
    bool has_tcp_timeouts() const {
        return tcp_timeouts_.read_timeout_ms != 0 || tcp_timeouts_.write_timeout_ms != 0;
    }

    bool has_tcp_watermarks() const {
        return tcp_watermarks_.read_low != 0 || tcp_watermarks_.read_high != 0 ||
               tcp_watermarks_.write_low != 0 || tcp_watermarks_.write_high != 0;
    }

    void apply_tcp_options() {
        if (!connection_) return;
        if (has_tcp_timeouts()) connection_->set_timeouts(tcp_timeouts_);
        if (has_tcp_watermarks()) connection_->set_watermarks(tcp_watermarks_);
    }

    bool send_line(const char* line) {
        if (!connection_) return false;
        if (max_output_bytes_ != 0 && connection_->output_size() >= max_output_bytes_) {
            connection_->close();
            connection_ = nullptr;
            return false;
        }
        const int written = connection_->write(reinterpret_cast<const uint8_t*>(line), strlen(line));
        if (written < 0) return false;
        if (max_output_bytes_ != 0 && connection_->output_size() > max_output_bytes_) {
            connection_->close();
            connection_ = nullptr;
            return false;
        }
        return true;
    }

    pypilot_event_loop::NativeTcpClient client_;
    pypilot_event_loop::ITcpConnection* connection_ = nullptr;
    pypilot_event_loop::TcpTimeoutOptions tcp_timeouts_{};
    pypilot_event_loop::TcpWatermarkOptions tcp_watermarks_{};
    size_t max_output_bytes_ = 0;
};

#endif

} // namespace pypilot_runtime
