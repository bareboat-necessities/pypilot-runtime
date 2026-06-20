#include <pypilot_runtime_client.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 128> loop;
    pypilot_runtime::PypilotRuntimeClient<> helper(loop);
    (void)helper;
    return 0;
}
