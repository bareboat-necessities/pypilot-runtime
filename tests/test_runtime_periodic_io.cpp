#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    model.server.uptime_s.set(12.5f, 100);
    char out[128]{};
    assert(protocol.format_value("server.uptime", out, sizeof(out)));
    assert(std::strcmp(out, "server.uptime=12.5000\n") == 0);
    return 0;
}
