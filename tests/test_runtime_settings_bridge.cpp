#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_settings_bridge.hpp>

int main() {
    pypilot_runtime::AutopilotValues autopilot;
    pypilot_runtime::BoatImuValues boatimu;
    pypilot_runtime::ServoValues servo;
    pypilot_runtime::SensorValues sensors;
    pypilot_runtime::PilotValues pilots;
    pypilot_runtime::GpsValues gps;
    pypilot_runtime::WindValues wind;
    pypilot_runtime::PypilotRuntimeState state{autopilot, boatimu, sensors, servo, pilots, gps, wind};
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);

    pypilot_settings::SettingsCatalog catalog = pypilot_runtime::RuntimeSettingsBridge::catalog();
    pypilot_settings::MemorySettingsStore<8> store;
    pypilot_settings::SettingsManager settings(catalog, store);

    assert(store.save("ap.mode", "gps"));
    assert(store.save("ap.pilot", "advanced"));
    assert(store.save("servo.engaged", "true"));
    assert(pypilot_runtime::RuntimeSettingsBridge::load(protocol, settings));
    assert(std::strcmp(autopilot.mode.get(), "gps") == 0);
    assert(std::strcmp(autopilot.pilot.get(), "advanced") == 0);
    assert(servo.engaged.get());

    char error[160]{};
    assert(pypilot_runtime::RuntimeSettingsBridge::apply_set_and_save(protocol, settings, "ap.pilot", "basic", error, sizeof(error)));
    char saved[64]{};
    assert(store.load("ap.pilot", saved, sizeof(saved)));
    assert(std::strcmp(saved, "basic") == 0);
    assert(std::strcmp(autopilot.pilot.get(), "basic") == 0);

    assert(pypilot_runtime::RuntimeSettingsBridge::apply_set_and_save(protocol, settings, "ap.enabled", "true", error, sizeof(error)));
    assert(autopilot.enabled.get());
    assert(!store.load("ap.enabled", saved, sizeof(saved)));

    assert(!pypilot_runtime::RuntimeSettingsBridge::apply_set_and_save(protocol, settings, "ap.mode", "not-a-mode", error, sizeof(error)));
    return 0;
}
