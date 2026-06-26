#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_client.hpp>

static void pump_for(async_event_loop::EventLoop<64, 128>& runtime_loop, uint32_t milliseconds) {
    runtime_loop.on_delay(milliseconds, [&] {
        runtime_loop.request_exit();
    });
    runtime_loop.run_forever();
}

static bool drain_until(pypilot_runtime::PypilotRuntimeClient<>& client, const char* expected) {
    char line[160]{};
    while (client.read_line(line, sizeof(line))) {
        if (std::strcmp(line, expected) == 0) {
            return true;
        }
    }
    return false;
}

int main() {
    async_event_loop::EventLoop<64, 128> runtime_loop;

    pypilot_runtime::PypilotRuntimeState state;
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);
    pypilot_runtime::PypilotRuntimeServer<2, 8> server(runtime_loop, protocol);

    async_event_loop::TcpTimeoutOptions server_timeouts;
    server_timeouts.read_timeout_ms = 1000;
    server_timeouts.write_timeout_ms = 1000;
    server.set_tcp_timeouts(server_timeouts);

    async_event_loop::TcpWatermarkOptions server_watermarks;
    server_watermarks.read_low = 1;
    server.set_tcp_watermarks(server_watermarks);
    server.set_max_output_bytes(4096);

    assert(server.listen("127.0.0.1", 0));

    pypilot_runtime::PypilotRuntimeClient<> client(runtime_loop);
    async_event_loop::TcpTimeoutOptions client_timeouts;
    client_timeouts.read_timeout_ms = 1000;
    client_timeouts.write_timeout_ms = 1000;
    client.set_tcp_timeouts(client_timeouts);
    client.set_max_output_bytes(4096);

    assert(client.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 25);
    assert(client.connected());

    assert(client.set_bool("ap.enabled", true));
    pump_for(runtime_loop, 25);
    assert(state.ap.enabled.value);

    assert(client.set_number("servo.command", -0.15));
    pump_for(runtime_loop, 25);
    assert(state.servo.command_norm.valid);
    assert(state.servo.command_norm.value < -0.149f && state.servo.command_norm.value > -0.151f);

    assert(client.watch("imu.heading", 0.0));
    pump_for(runtime_loop, 25);
    assert(drain_until(client, "imu.heading=0.0000"));

    state.imu.heading_deg.set(42.0f, 100);
    server.publish_changed_values();
    pump_for(runtime_loop, 25);
    assert(drain_until(client, "imu.heading=42.0000"));

    assert(client.unwatch("imu.heading"));
    pump_for(runtime_loop, 25);
    state.imu.heading_deg.set(43.0f, 100);
    server.publish_changed_values();
    pump_for(runtime_loop, 25);
    assert(!drain_until(client, "imu.heading=43.0000"));

    client.close();
    pump_for(runtime_loop, 25);
    server.close();
    return 0;
}
