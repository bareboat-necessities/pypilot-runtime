#include <cassert>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 96> loop;
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    pypilot_runtime::PypilotRuntimeServer<2, 4> server(loop, protocol);
    assert(!server.udp_watch_enabled());
    return 0;
}
