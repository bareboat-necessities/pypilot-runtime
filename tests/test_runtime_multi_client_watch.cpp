#include <cassert>
#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    model.ap.enabled.value = true;
    char out[128]{};
    assert(protocol.format_value("ap.enabled", out, sizeof(out)));
    return 0;
}
