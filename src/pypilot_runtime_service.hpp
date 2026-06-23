#pragma once

#include <stddef.h>
#include <stdint.h>

#include <pypilot_event_loop.hpp>
#include <pypilot_runtime_core.hpp>
#include <pypilot_runtime_server.hpp>

namespace pypilot_runtime {

struct PypilotRuntimeServiceOptions {
    const char* host = "0.0.0.0";
    uint16_t tcp_port = 23322;
    uint16_t udp_watch_port = 0;
    uint32_t publish_period_us = 50000u;
    size_t max_output_bytes = 32768u;
    const char* server_version = "pypilot-cpp";
    bool enable_tcp = true;
    bool enable_periodic_publish = true;
};

template<typename EventLoopType, size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeService final {
public:
    explicit PypilotRuntimeService(EventLoopType& loop)
        : loop_(loop),
          autopilot_values_(),
          boatimu_values_(),
          sensor_values_(),
          servo_values_(),
          pilot_values_(),
          gps_values_(),
          wind_values_(),
          state_{autopilot_values_, boatimu_values_, sensor_values_, servo_values_, pilot_values_, gps_values_, wind_values_},
          protocol_(state_),
          server_(loop_, protocol_) {}

    bool begin(const PypilotRuntimeServiceOptions& options = PypilotRuntimeServiceOptions{}) {
        options_ = options;
        fault_ = "";
        listening_ = false;
        listen_port_ = 0;

        state_.sensors.server_version.set(options_.server_version ? options_.server_version : "pypilot-cpp");
        state_.servo.engaged.set(false);
        state_.servo.state.set("runtime_ready");

        if (options_.enable_tcp && !start_server()) {
            return false;
        }

        if (options_.enable_periodic_publish && options_.publish_period_us != 0) {
            publish_handle_ = loop_.on_repeat_us(options_.publish_period_us, [this]() { publish_changed_values(); });
            if (!publish_handle_.assigned()) {
                set_fault("failed to register runtime publisher");
                return false;
            }
        }

        publish_changed_values();
        return true;
    }

    void stop() {
        server_.close();
        if (publish_handle_.assigned()) {
            loop_.remove(publish_handle_);
            publish_handle_ = pypilot_event_loop::EventHandle{};
        }
        listening_ = false;
        listen_port_ = 0;
    }

    PypilotRuntimeState& state() { return state_; }
    const PypilotRuntimeState& state() const { return state_; }

    PypilotRuntimeProtocol& protocol() { return protocol_; }
    const PypilotRuntimeProtocol& protocol() const { return protocol_; }

    PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection>& server() { return server_; }
    const PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection>& server() const { return server_; }

    bool listening() const { return listening_; }
    uint16_t port() const { return listen_port_; }
    const char* fault() const { return fault_; }

    void publish_changed_values() {
        state_.sensors.runtime_published_value_count.set(
            state_.sensors.runtime_published_value_count.get() + 1.0);
        server_.publish_changed_values();
    }

private:
    bool start_server() {
        PypilotRuntimeServerOptions server_options;
        server_options.max_output_bytes = options_.max_output_bytes;
        server_options.udp_watch_port = options_.udp_watch_port;
        if (!server_.configure(server_options)) {
            set_fault("failed to configure runtime server");
            return false;
        }
        if (!server_.listen(options_.host ? options_.host : "0.0.0.0", options_.tcp_port)) {
            set_fault("failed to listen on runtime TCP port");
            return false;
        }
        listening_ = true;
        listen_port_ = server_.port();
        return true;
    }

    void set_fault(const char* message) {
        fault_ = message ? message : "runtime fault";
        state_.sensors.status_faults.set(state_.sensors.status_faults.get() + 1.0);
        state_.servo.engaged.set(false);
        state_.servo.state.set(fault_);
    }

    EventLoopType& loop_;
    PypilotRuntimeServiceOptions options_{};
    pypilot_event_loop::EventHandle publish_handle_{};
    const char* fault_ = "";
    bool listening_ = false;
    uint16_t listen_port_ = 0;

    AutopilotValues autopilot_values_{};
    BoatImuValues boatimu_values_{};
    SensorValues sensor_values_{};
    ServoValues servo_values_{};
    PilotValues pilot_values_{};
    GpsValues gps_values_{};
    WindValues wind_values_{};
    PypilotRuntimeState state_;
    PypilotRuntimeProtocol protocol_;
    PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection> server_;
};

} // namespace pypilot_runtime
