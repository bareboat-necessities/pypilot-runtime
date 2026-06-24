#include <cassert>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 128> loop;
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    pypilot_runtime::PypilotRuntimeServer<2, 4> server(loop, protocol);

    pypilot_runtime::PypilotRuntimeServerOptions options;
    options.tcp_timeouts.read_timeout_ms = 1000;
    options.tcp_timeouts.write_timeout_ms = 1000;
    options.tcp_watermarks.read_low = 1;
    options.max_output_bytes = 4096;
    assert(server.configure(options));

    pypilot_event_loop::TcpTimeoutOptions timeouts;
    timeouts.read_timeout_ms = 1000;
    timeouts.write_timeout_ms = 1000;
    server.set_tcp_timeouts(timeouts);

    pypilot_event_loop::TcpWatermarkOptions watermarks;
    watermarks.read_low = 1;
    server.set_tcp_watermarks(watermarks);
    server.set_max_output_bytes(4096);

    assert(!server.valid());
    return 0;
}
