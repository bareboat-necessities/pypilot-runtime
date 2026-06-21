#include <cassert>

#include <pypilot_runtime_mdns.hpp>

int main() {
    pypilot_runtime::RuntimeMdnsService service;
    assert(service.last_error() != nullptr);
    return 0;
}
