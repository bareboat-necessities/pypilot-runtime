#include <stdlib.h>

#include <pypilot_runtime.hpp>
#include <pypilot_settings.hpp>

namespace {

bool set_env(const char* name, const char* value) {
#if defined(_WIN32)
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

} // namespace

int main() {
    pypilot_event_loop::EventLoop<64, 96> loop;
    pypilot_runtime::PypilotRuntimeService<decltype(loop), 2, 4> runtime(loop);

    pypilot_settings::MemorySettingsStore<16, 64, 128> store;
    pypilot_settings::SettingsCatalog catalog = pypilot_runtime::PypilotRuntimeServiceSettings::catalog();
    pypilot_settings::SettingsManager settings(catalog, store);

    char error[160]{};
    if (!settings.save_value("runtime.tcp.enabled", "false", error, sizeof(error))) return 1;
    if (!settings.save_value("runtime.publish.period.us", "0", error, sizeof(error))) return 2;
    if (!settings.save_value("runtime.server.version", "runtime-settings-test", error, sizeof(error))) return 3;

    if (!runtime.begin(settings)) return 4;
    if (runtime.listening()) return 5;
    if (runtime.port() != 0) return 6;
    if (runtime.state().sensors.server_version.get() != std::string("runtime-settings-test")) return 7;
    runtime.stop();

    if (!set_env("PYPILOTD_RUNTIME_PORT", "0")) return 8;
    if (!set_env("PYPILOTD_RUNTIME_HOST", "127.0.0.1")) return 9;
    pypilot_settings::MemorySettingsStore<16, 64, 128> env_store;
    pypilot_settings::SettingsManager env_settings(catalog, env_store);
    if (!pypilot_runtime::PypilotRuntimeServiceSettings::apply_process_environment(env_settings)) return 10;
    char value[128]{};
    if (!env_settings.load_value("runtime.tcp.host", value, sizeof(value))) return 11;
    if (std::string(value) != "127.0.0.1") return 12;
    if (!env_settings.load_value("runtime.tcp.port", value, sizeof(value))) return 13;
    if (std::string(value) != "0") return 14;

    return 0;
}
