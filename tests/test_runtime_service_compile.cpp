#include <cstring>
#include <pypilot_runtime.hpp>

int main() {
    pypilot_event_loop::EventLoop<64, 96> loop;
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeService<decltype(loop), 2, 4> runtime(loop, model);

    pypilot_runtime::PypilotRuntimeServiceOptions options;
    options.enable_tcp = false;
    options.enable_periodic_publish = false;
    options.server_version = "runtime-service-test";

    if (!runtime.begin(options)) return 1;
    if (std::strcmp(model.server.version, "runtime-service-test") != 0) return 2;
    model.ap.enabled.value = false;
    runtime.publish_changed_values();
    runtime.stop();
    return model.runtime_publication.published_value_count.valid ? 0 : 3;
}
