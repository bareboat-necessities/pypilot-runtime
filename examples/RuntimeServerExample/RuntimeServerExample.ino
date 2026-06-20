#if defined(ARDUINO)
#define PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP
#include <WiFi.h>
#endif

#include <pypilot_event_loop.hpp>
#include <pypilot_runtime.hpp>

#ifndef PYPILOT_RUNTIME_EXAMPLE_PORT
#define PYPILOT_RUNTIME_EXAMPLE_PORT 23322
#endif

#if defined(ARDUINO)
static const char* WIFI_SSID = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";
#endif

pypilot_event_loop::EventLoop<32, 128> loop;

pypilot_runtime::AutopilotValues autopilot;
pypilot_runtime::BoatImuValues boatimu;
pypilot_runtime::ServoValues servo;
pypilot_runtime::SensorValues sensors;
pypilot_runtime::PilotValues pilots;
pypilot_runtime::GpsValues gps;
pypilot_runtime::WindValues wind;

pypilot_runtime::PypilotRuntimeState runtime_state{autopilot, boatimu, sensors, servo, pilots, gps, wind};
pypilot_runtime::PypilotRuntimeProtocol protocol(runtime_state);
pypilot_runtime::PypilotRuntimeServer<8, 16> server(loop, protocol);

static void print_line(const char* text) {
#if defined(ARDUINO)
    Serial.println(text);
#else
    puts(text);
#endif
}

static void setup_network() {
#if defined(ARDUINO)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
#else
    print_line("Linux network ready");
#endif
}

static void setup_runtime_example() {
#if defined(ARDUINO)
    Serial.begin(115200);
    delay(200);
#endif
    setup_network();

    if (!server.listen(PYPILOT_RUNTIME_EXAMPLE_PORT)) {
        print_line("failed to start pypilot runtime server");
        return;
    }

    loop.on_repeat(1000, [] {
        double heading = boatimu.heading.get() + 1.0;
        if (heading >= 360.0) {
            heading = 0.0;
        }
        boatimu.heading.set(heading);
        server.publish_changed_values();
    });

    print_line("pypilot runtime server listening");
}

#if defined(ARDUINO)
void setup() {
    setup_runtime_example();
}

void loop() {
    loop.tick();
}
#else
int main() {
    setup_runtime_example();
    loop.run_forever();
    return 0;
}
#endif
