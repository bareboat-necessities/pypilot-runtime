#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);
    char error[128]{};
    assert(!protocol.apply_set("imu.heading", "22.0", error, sizeof(error)));
    assert(std::strstr(error, "not writable") != nullptr);
    assert(!protocol.apply_set("ap.enabled", "maybe", error, sizeof(error)));
    assert(std::strstr(error, "bad bool") != nullptr);
    return 0;
}
