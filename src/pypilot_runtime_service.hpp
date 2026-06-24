#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pypilot_event_loop.hpp>
#include <pypilot_runtime_core.hpp>
#include <pypilot_runtime_server.hpp>

#if defined(PYPILOT_RUNTIME_WITH_SETTINGS)
#include <pypilot_settings.hpp>
#endif

namespace pypilot_runtime {

struct PypilotRuntimeServiceOptions {
    const char* host = "0.0.0.0";
    uint16_t tcp_port = 23322;
    uint16_t udp_watch_port = 0;
    uint32_t publish_period_us = 50000u;
    size_t max_output_bytes = 32768u;
    const char* server_version = "pypilot-cpp";
    bool enable_tcp = true;
    bool enable_periodic_publish = true;
};

#if defined(PYPILOT_RUNTIME_WITH_SETTINGS)
class PypilotRuntimeServiceSettings final {
public:
    static pypilot_settings::SettingsCatalog catalog() {
        return pypilot_settings::SettingsCatalog(descriptors(), descriptor_count());
    }

    static bool apply_process_environment(pypilot_settings::SettingsManager& settings) {
        bool ok = true;
        ok = save_env_if_set(settings, "PYPILOTD_RUNTIME_HOST", "runtime.tcp.host") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_HOST", "runtime.tcp.host") && ok;
        ok = save_env_if_set(settings, "PYPILOTD_RUNTIME_PORT", "runtime.tcp.port") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_PORT", "runtime.tcp.port") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_UDP_WATCH_PORT", "runtime.udp.watch.port") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_PUBLISH_PERIOD_US", "runtime.publish.period.us") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_MAX_OUTPUT_BYTES", "runtime.max.output.bytes") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_TCP_ENABLED", "runtime.tcp.enabled") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_PERIODIC_PUBLISH_ENABLED", "runtime.periodic.publish.enabled") && ok;
        ok = save_env_if_set(settings, "PYPILOT_RUNTIME_SERVER_VERSION", "runtime.server.version") && ok;
        return ok;
    }

    static bool load(pypilot_settings::SettingsManager& settings,
                     PypilotRuntimeServiceOptions& options,
                     char* host_storage,
                     size_t host_storage_size,
                     char* server_version_storage,
                     size_t server_version_storage_size) {
        bool ok = true;
        ok = load_string(settings, "runtime.tcp.host", host_storage, host_storage_size) && ok;
        ok = load_u16(settings, "runtime.tcp.port", options.tcp_port) && ok;
        ok = load_u16(settings, "runtime.udp.watch.port", options.udp_watch_port) && ok;
        ok = load_u32(settings, "runtime.publish.period.us", options.publish_period_us) && ok;
        ok = load_size(settings, "runtime.max.output.bytes", options.max_output_bytes) && ok;
        ok = load_bool(settings, "runtime.tcp.enabled", options.enable_tcp) && ok;
        ok = load_bool(settings, "runtime.periodic.publish.enabled", options.enable_periodic_publish) && ok;
        ok = load_string(settings, "runtime.server.version", server_version_storage, server_version_storage_size) && ok;
        if (ok) {
            options.host = host_storage;
            options.server_version = server_version_storage;
        }
        return ok;
    }

private:
    static bool save_env_if_set(pypilot_settings::SettingsManager& settings, const char* env_name, const char* setting_name) {
        const char* value = getenv(env_name);
        if (!value || !*value) return true;
        char error[160]{};
        return settings.save_value(setting_name, value, error, sizeof(error));
    }

    static bool load_string(pypilot_settings::SettingsManager& settings, const char* name, char* out, size_t out_size) {
        char value[160]{};
        if (!settings.load_value(name, value, sizeof(value))) return false;
        return pypilot_settings::settings_strip_quotes(value, out, out_size);
    }

    static bool load_bool(pypilot_settings::SettingsManager& settings, const char* name, bool& out) {
        char value[32]{};
        if (!settings.load_value(name, value, sizeof(value))) return false;
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) { out = true; return true; }
        if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) { out = false; return true; }
        return false;
    }

    static bool load_u16(pypilot_settings::SettingsManager& settings, const char* name, uint16_t& out) {
        uint32_t value = 0;
        if (!load_u32(settings, name, value)) return false;
        if (value > 65535u) return false;
        out = static_cast<uint16_t>(value);
        return true;
    }

    static bool load_u32(pypilot_settings::SettingsManager& settings, const char* name, uint32_t& out) {
        char value[32]{};
        if (!settings.load_value(name, value, sizeof(value))) return false;
        double parsed = 0.0;
        if (!pypilot_settings::settings_parse_number(value, parsed)) return false;
        if (parsed < 0.0 || parsed > 4294967295.0) return false;
        out = static_cast<uint32_t>(parsed);
        return true;
    }

    static bool load_size(pypilot_settings::SettingsManager& settings, const char* name, size_t& out) {
        uint32_t value = 0;
        if (!load_u32(settings, name, value)) return false;
        out = static_cast<size_t>(value);
        return true;
    }

    static const pypilot_settings::SettingDescriptor* descriptors() {
        static const pypilot_settings::SettingDescriptor values[] = {
            {"runtime.tcp.host", pypilot_settings::SettingType::String, pypilot_settings::SettingScope::Runtime, true, true, "0.0.0.0", 0.0, 0.0, 63, nullptr, 0},
            {"runtime.tcp.port", pypilot_settings::SettingType::Number, pypilot_settings::SettingScope::Runtime, true, true, "23322", 0.0, 65535.0, 0, nullptr, 0},
            {"runtime.udp.watch.port", pypilot_settings::SettingType::Number, pypilot_settings::SettingScope::Runtime, true, true, "0", 0.0, 65535.0, 0, nullptr, 0},
            {"runtime.publish.period.us", pypilot_settings::SettingType::Number, pypilot_settings::SettingScope::Runtime, true, true, "50000", 0.0, 60000000.0, 0, nullptr, 0},
            {"runtime.max.output.bytes", pypilot_settings::SettingType::Number, pypilot_settings::SettingScope::Runtime, true, true, "32768", 1024.0, 1048576.0, 0, nullptr, 0},
            {"runtime.tcp.enabled", pypilot_settings::SettingType::Bool, pypilot_settings::SettingScope::Runtime, true, true, "true", 0.0, 0.0, 0, nullptr, 0},
            {"runtime.periodic.publish.enabled", pypilot_settings::SettingType::Bool, pypilot_settings::SettingScope::Runtime, true, true, "true", 0.0, 0.0, 0, nullptr, 0},
            {"runtime.server.version", pypilot_settings::SettingType::String, pypilot_settings::SettingScope::Runtime, true, true, "pypilot-cpp", 0.0, 0.0, 63, nullptr, 0}
        };
        return values;
    }

    static size_t descriptor_count() { return 8; }
};
#endif

template<typename EventLoopType, size_t MaxConnections = 8, size_t MaxWatchesPerConnection = 16>
class PypilotRuntimeService final {
public:
    explicit PypilotRuntimeService(EventLoopType& loop)
        : loop_(loop),
          owned_state_(),
          state_(owned_state_),
          protocol_(state_),
          server_(loop_, protocol_) {}

    PypilotRuntimeService(EventLoopType& loop, PypilotRuntimeState& state)
        : loop_(loop),
          owned_state_(),
          state_(state),
          protocol_(state_),
          server_(loop_, protocol_) {}

    bool begin() {
#if defined(PYPILOT_RUNTIME_WITH_SETTINGS)
        pypilot_settings::MemorySettingsStore<16, 64, 128> store;
        pypilot_settings::SettingsCatalog service_catalog = PypilotRuntimeServiceSettings::catalog();
        pypilot_settings::SettingsManager settings(service_catalog, store);
        if (!PypilotRuntimeServiceSettings::apply_process_environment(settings)) {
            set_fault("failed to apply runtime service environment settings");
            return false;
        }
        return begin(settings);
#else
        return begin(PypilotRuntimeServiceOptions{});
#endif
    }

#if defined(PYPILOT_RUNTIME_WITH_SETTINGS)
    bool begin(pypilot_settings::SettingsManager& settings) {
        PypilotRuntimeServiceOptions options;
        if (!PypilotRuntimeServiceSettings::load(settings,
                                                 options,
                                                 settings_host_,
                                                 sizeof(settings_host_),
                                                 settings_server_version_,
                                                 sizeof(settings_server_version_))) {
            set_fault("failed to load runtime service settings");
            return false;
        }
        return begin(options);
    }
#endif

    bool begin(const PypilotRuntimeServiceOptions& options) {
        options_ = options;
        fault_ = "";
        listening_ = false;
        listen_port_ = 0;

        copy_cstr(state_.server.version, sizeof(state_.server.version), options_.server_version ? options_.server_version : "pypilot-cpp");
        if (state_.server.profile_name[0] == '\0') copy_cstr(state_.server.profile_name, sizeof(state_.server.profile_name), "default");
        state_.servo.engaged.value = false;

        if (options_.enable_tcp && !start_server()) {
            return false;
        }

        if (options_.enable_periodic_publish && options_.publish_period_us != 0) {
            publish_handle_ = loop_.on_repeat_us(options_.publish_period_us, [this]() { publish_changed_values(); });
            if (!publish_handle_.assigned()) {
                set_fault("failed to register runtime publisher");
                return false;
            }
        }

        publish_changed_values();
        return true;
    }

    void stop() {
        server_.close();
        if (publish_handle_.assigned()) {
            loop_.remove(publish_handle_);
            publish_handle_ = pypilot_event_loop::EventHandle{};
        }
        listening_ = false;
        listen_port_ = 0;
    }

    PypilotRuntimeState& state() { return state_; }
    const PypilotRuntimeState& state() const { return state_; }

    PypilotRuntimeProtocol& protocol() { return protocol_; }
    const PypilotRuntimeProtocol& protocol() const { return protocol_; }

    PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection>& server() { return server_; }
    const PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection>& server() const { return server_; }

    bool listening() const { return listening_; }
    uint16_t port() const { return listen_port_; }
    const char* fault() const { return fault_; }

    void publish_changed_values() {
        const uint64_t now_us = loop_.clock().micros();
        const uint32_t previous = state_.runtime_publication.published_value_count.valid
            ? state_.runtime_publication.published_value_count.value
            : 0u;
        state_.runtime_publication.published_value_count.set(previous + 1u, now_us);
        server_.publish_changed_values();
    }

private:
    bool start_server() {
        PypilotRuntimeServerOptions server_options;
        server_options.max_output_bytes = options_.max_output_bytes;
        server_options.udp_watch_port = options_.udp_watch_port;
        if (!server_.configure(server_options)) {
            set_fault("failed to configure runtime server");
            return false;
        }
        if (!server_.listen(options_.host ? options_.host : "0.0.0.0", options_.tcp_port)) {
            set_fault("failed to listen on runtime TCP port");
            return false;
        }
        listening_ = true;
        listen_port_ = server_.port();
        return true;
    }

    void set_fault(const char* message) {
        fault_ = message ? message : "runtime fault";
        ++state_.status.faults.value;
        state_.servo.engaged.value = false;
    }

    EventLoopType& loop_;
    PypilotRuntimeState owned_state_{};
    PypilotRuntimeState& state_;
    PypilotRuntimeServiceOptions options_{};
    pypilot_event_loop::EventHandle publish_handle_{};
    const char* fault_ = "";
    bool listening_ = false;
    uint16_t listen_port_ = 0;
#if defined(PYPILOT_RUNTIME_WITH_SETTINGS)
    char settings_host_[64]{};
    char settings_server_version_[64]{};
#endif

    PypilotRuntimeProtocol protocol_;
    PypilotRuntimeServer<MaxConnections, MaxWatchesPerConnection> server_;
};

} // namespace pypilot_runtime
