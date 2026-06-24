#include <cassert>
#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    assert(protocol.value_exists("server.version"));
    return 0;
}
