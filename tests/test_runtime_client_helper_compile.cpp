#include <pypilot_runtime_client.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 128> loop;
    pypilot_runtime::PypilotRuntimeClient<> helper(loop);
    pypilot_runtime::PypilotClientValue value;
    bool b = false;
    double n = 0.0;
    char s[16]{};
    (void)helper;
    (void)value;
    (void)b;
    (void)n;
    (void)s;
    return 0;
}
