#include <cassert>

#include <pypilot_runtime_client.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 128> loop;
    pypilot_runtime::PypilotRuntimeClient<> helper(loop);

    pypilot_event_loop::TcpTimeoutOptions timeouts;
    timeouts.read_timeout_ms = 1000;
    timeouts.write_timeout_ms = 1000;
    helper.set_tcp_timeouts(timeouts);

    pypilot_event_loop::TcpWatermarkOptions watermarks;
    watermarks.read_low = 1;
    helper.set_tcp_watermarks(watermarks);
    helper.set_max_output_bytes(4096);
    assert(helper.output_size() == 0);

    pypilot_runtime::PypilotClientValue value;
    bool b = false;
    double n = 0.0;
    char s[16]{};
    (void)value;
    (void)b;
    (void)n;
    (void)s;
    return 0;
}
