#include <cassert>

#include <pypilot_runtime.hpp>

int main() {
    async_event_loop::EventLoop<8, 96> loop;
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    pypilot_runtime::PypilotRuntimeServer<2, 4> server(loop, protocol);
    server.set_max_output_bytes(128);
    assert(!server.valid());
    return 0;
}
