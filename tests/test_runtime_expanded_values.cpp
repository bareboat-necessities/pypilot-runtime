#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    model.wind.apparent.speed_kn.set(12.0f, 100);
    model.water.speed_kn.set(5.0f, 100);
    char out[128]{};
    assert(protocol.format_value("wind.speed", out, sizeof(out)));
    assert(std::strcmp(out, "wind.speed=12.0000\n") == 0);
    assert(protocol.format_value("water.speed", out, sizeof(out)));
    assert(std::strcmp(out, "water.speed=5.0000\n") == 0);
    return 0;
}
