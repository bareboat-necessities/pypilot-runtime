#include <pypilot_runtime.hpp>

int main() {
    pypilot_event_loop::EventLoop<64, 96> loop;
    pypilot_runtime::PypilotRuntimeService<decltype(loop), 2, 4> runtime(loop);

    pypilot_runtime::PypilotRuntimeServiceOptions options;
    options.enable_tcp = false;
    options.enable_periodic_publish = false;
    options.server_version = "runtime-service-test";

    if (!runtime.begin(options)) return 1;
    runtime.state().autopilot.enabled.set(false);
    runtime.publish_changed_values();
    runtime.stop();
    return runtime.state().sensors.runtime_published_value_count.get() > 0.0 ? 0 : 2;
}
