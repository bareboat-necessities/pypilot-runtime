#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_settings_bridge.hpp>
#include <pypilot_settings.hpp>

int main() {
    pypilot_runtime::PypilotRuntimeState model;
    pypilot_runtime::PypilotRuntimeProtocol protocol(model);

    pypilot_settings::MemorySettingsStore<8, 64, 128> store;
    pypilot_settings::SettingsCatalog catalog = pypilot_runtime::RuntimeSettingsBridge::catalog();
    pypilot_settings::SettingsManager settings(catalog, store);

    char error[160]{};
    assert(settings.save_value("ap.mode", "wind", error, sizeof(error)));
    assert(settings.save_value("ap.pilot", "basic", error, sizeof(error)));
    assert(settings.save_value("servo.engaged", "true", error, sizeof(error)));
    assert(pypilot_runtime::RuntimeSettingsBridge::load(protocol, settings));
    assert(model.ap.mode.value == ship_data_model::AutopilotMode::wind);
    assert(model.servo.engaged.value);

    assert(pypilot_runtime::RuntimeSettingsBridge::apply_set_and_save(protocol, settings, "ap.mode", "gps", error, sizeof(error)));
    assert(model.ap.mode.value == ship_data_model::AutopilotMode::gps);

    return 0;
}
