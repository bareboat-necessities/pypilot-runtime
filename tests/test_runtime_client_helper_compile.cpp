#include <cassert>

#include <pypilot_runtime_client.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 96> loop;
    pypilot_runtime::PypilotRuntimeClient<> client(loop);
    client.set_max_output_bytes(4096);
    assert(!client.connected());
    return 0;
}
