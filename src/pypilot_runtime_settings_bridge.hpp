#pragma once

#include <pypilot_runtime.hpp>
#include <pypilot_settings.hpp>

namespace pypilot_runtime {

class RuntimeSettingsBridge final {
public:
    static pypilot_settings::SettingsCatalog catalog() {
        return pypilot_settings::SettingsCatalog(descriptors(), descriptor_count());
    }

    static bool load(PypilotRuntimeProtocol& protocol, pypilot_settings::SettingsManager& settings) {
        bool ok = true;
        char value[128]{};
        char error[160]{};
        const pypilot_settings::SettingDescriptor* ds = descriptors();
        for (size_t i = 0; i < descriptor_count(); ++i) {
            if (!settings.load_value(ds[i].name, value, sizeof(value))) {
                ok = false;
                continue;
            }
            if (!settings.validate_value(ds[i].name, value, error, sizeof(error))) {
                ok = false;
                continue;
            }
            if (!protocol.apply_set(ds[i].name, value, error, sizeof(error))) {
                ok = false;
            }
        }
        return ok;
    }

    static bool save_if_known(const char* name,
                              const char* payload,
                              pypilot_settings::SettingsManager& settings,
                              char* error,
                              size_t error_size) {
        const pypilot_settings::SettingsCatalog runtime_catalog = catalog();
        if (!runtime_catalog.find(name)) {
            return true;
        }
        return settings.save_value(name, payload, error, error_size);
    }

    static bool apply_set_and_save(PypilotRuntimeProtocol& protocol,
                                   pypilot_settings::SettingsManager& settings,
                                   const char* name,
                                   const char* payload,
                                   char* error,
                                   size_t error_size) {
        const pypilot_settings::SettingsCatalog runtime_catalog = catalog();
        const bool known = runtime_catalog.find(name) != nullptr;
        if (known && !settings.validate_value(name, payload, error, error_size)) {
            return false;
        }
        if (!protocol.apply_set(name, payload, error, error_size)) {
            return false;
        }
        if (!known) {
            return true;
        }
        return settings.save_value(name, payload, error, error_size);
    }

private:
    static const pypilot_settings::SettingDescriptor* descriptors() {
        static const char* modes[] = {"compass", "gps", "wind"};
        static const pypilot_settings::SettingDescriptor values[] = {
            {"ap.mode", pypilot_settings::SettingType::Enum, pypilot_settings::SettingScope::Runtime, true, true, "compass", 0.0, 0.0, 0, modes, 3},
            {"ap.pilot", pypilot_settings::SettingType::String, pypilot_settings::SettingScope::Runtime, true, true, "basic", 0.0, 0.0, 32, nullptr, 0},
            {"servo.engaged", pypilot_settings::SettingType::Bool, pypilot_settings::SettingScope::Runtime, true, true, "false", 0.0, 0.0, 0, nullptr, 0}
        };
        return values;
    }

    static size_t descriptor_count() {
        return 3;
    }
};

} // namespace pypilot_runtime
