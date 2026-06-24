#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    assert(protocol.value_exists("ap.enabled"));
    assert(!protocol.value_exists("not.a.value"));
    char error[128]{};
    assert(!protocol.apply_set("not.a.value", "1", error, sizeof(error)));
    assert(std::strstr(error, "unknown value") != nullptr);
    return 0;
}
