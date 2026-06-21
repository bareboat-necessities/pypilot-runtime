#pragma once

#include <pypilot_mdns.hpp>
#include <pypilot_runtime.hpp>

namespace pypilot_runtime {

class RuntimeMdnsService final {
public:
    bool begin(const char* hostname = "pypilot") {
        return mdns_.begin(hostname);
    }

    void end() {
        mdns_.end();
    }

    bool advertise_pypilot(uint16_t port = pypilot_mdns::PYPILOT_MDNS_DEFAULT_PYPILOT_PORT,
                           const char* instance = "pypilot",
                           const char* uid = "pypilot") {
        return mdns_.advertise_pypilot(port, instance, uid);
    }

    bool discover_signalk(pypilot_mdns::MdnsServiceInfo& out, uint32_t timeout_ms = 3000) {
        return mdns_.discover_signalk(out, timeout_ms);
    }

    const char* last_error() const {
        return mdns_.last_error();
    }

private:
    pypilot_mdns::PypilotMdns mdns_;
};

} // namespace pypilot_runtime
