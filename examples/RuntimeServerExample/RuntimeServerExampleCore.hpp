#pragma once

#if defined(ARDUINO)
#define ASYNC_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP
#include <WiFi.h>
#endif

#include <async_event_loop.hpp>
#include <pypilot_runtime.hpp>

#ifndef PYPILOT_RUNTIME_EXAMPLE_PORT
#define PYPILOT_RUNTIME_EXAMPLE_PORT 23322
#endif

#if defined(ARDUINO)
static const char* WIFI_SSID = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";
#endif

async_event_loop::EventLoop<32, 128> event_loop;
pypilot_runtime::PypilotRuntimeState runtime_state;
pypilot_runtime::PypilotRuntimeProtocol protocol(runtime_state);
pypilot_runtime::PypilotRuntimeServer<8, 16> server(event_loop, protocol);

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
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
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
    pypilot_runtime::runtime_copy_text(runtime_state.server.version, sizeof(runtime_state.server.version), "runtime-example", strlen("runtime-example"));

    if (!server.listen(PYPILOT_RUNTIME_EXAMPLE_PORT)) {
        print_line("failed to start pypilot runtime server");
        return;
    }

    event_loop.on_repeat(1000, [] {
        float heading = runtime_state.imu.heading_deg.valid ? runtime_state.imu.heading_deg.value + 1.0f : 1.0f;
        if (heading >= 360.0f) heading = 0.0f;
        runtime_state.imu.heading_deg.set(heading, event_loop.clock().micros());
        server.publish_changed_values();
    });

    print_line("pypilot runtime server listening");
}

static void run_runtime_example_once() {
    event_loop.tick();
}

#if !defined(ARDUINO)
int main() {
    setup_runtime_example();
    event_loop.run_forever();
    return 0;
}
#endif
