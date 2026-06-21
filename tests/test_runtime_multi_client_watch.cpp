#include <cassert>
#include <cstring>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_client.hpp>

static void pump_for(pypilot_event_loop::EventLoop<96, 128>& runtime_loop, uint32_t milliseconds) {
    runtime_loop.on_delay(milliseconds, [&] { runtime_loop.request_exit(); });
    runtime_loop.run_forever();
}

static bool drain_until(pypilot_runtime::PypilotRuntimeClient<>& client, const char* expected) {
    char line[160]{};
    while (client.read_line(line, sizeof(line))) {
        if (std::strcmp(line, expected) == 0) return true;
    }
    return false;
}

int main() {
    pypilot_event_loop::EventLoop<96, 128> runtime_loop;
    pypilot_runtime::AutopilotValues autopilot;
    pypilot_runtime::BoatImuValues boatimu;
    pypilot_runtime::ServoValues servo;
    pypilot_runtime::SensorValues sensors;
    pypilot_runtime::PilotValues pilots;
    pypilot_runtime::GpsValues gps;
    pypilot_runtime::WindValues wind;
    pypilot_runtime::PypilotRuntimeState state{autopilot, boatimu, sensors, servo, pilots, gps, wind};
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);
    pypilot_runtime::PypilotRuntimeServer<4, 8> server(runtime_loop, protocol);
    assert(server.listen("127.0.0.1", 0));

    pypilot_runtime::PypilotRuntimeClient<> c1(runtime_loop);
    pypilot_runtime::PypilotRuntimeClient<> c2(runtime_loop);
    assert(c1.open("127.0.0.1", server.port()));
    assert(c2.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 50);
    assert(c1.connected());
    assert(c2.connected());

    assert(c1.watch("wind.speed", 0.0));
    assert(c2.watch("wind.speed", 0.0));
    pump_for(runtime_loop, 50);
    assert(drain_until(c1, "wind.speed=0.0000"));
    assert(drain_until(c2, "wind.speed=0.0000"));

    wind.speed.set(9.5);
    server.publish_changed_values();
    pump_for(runtime_loop, 50);
    assert(drain_until(c1, "wind.speed=9.5000"));
    assert(drain_until(c2, "wind.speed=9.5000"));

    c1.close();
    c2.close();
    pump_for(runtime_loop, 50);
    server.close();
    return 0;
}
