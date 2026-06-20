#pragma once

#include <stdio.h>
#include <string.h>

#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

#if defined(__linux__) || (defined(ARDUINO) && defined(PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP))

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

    void on_connect(pypilot_event_loop::ITcpConnection& connection, const pypilot_event_loop::TcpPeerInfo&) override {
        connection_ = &connection;
    }

    void on_close(pypilot_event_loop::ITcpConnection&) override {
        connection_ = nullptr;
    }

    void on_error(int) override {
        connection_ = nullptr;
    }

private:
    bool send_line(const char* line) {
        return connection_ && connection_->write(reinterpret_cast<const uint8_t*>(line), strlen(line)) >= 0;
    }

    pypilot_event_loop::NativeTcpClient client_;
    pypilot_event_loop::ITcpConnection* connection_ = nullptr;
};

#endif

} // namespace pypilot_runtime
