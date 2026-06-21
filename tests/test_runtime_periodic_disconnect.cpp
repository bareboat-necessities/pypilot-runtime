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
    pypilot_runtime::PypilotRuntimeServer<2, 8> server(runtime_loop, protocol);
    assert(server.listen("127.0.0.1", 0));

    pypilot_runtime::PypilotRuntimeClient<> first(runtime_loop);
    assert(first.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 25);
    assert(first.connected());
    assert(first.watch("imu.heading", 0.20));
    pump_for(runtime_loop, 25);
    assert(drain_until(first, "imu.heading=0.0000"));
    first.close();
    pump_for(runtime_loop, 50);

    pypilot_runtime::PypilotRuntimeClient<> second(runtime_loop);
    assert(second.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 25);
    assert(second.connected());
    assert(second.watch("imu.heading", 0.0));
    pump_for(runtime_loop, 25);
    assert(drain_until(second, "imu.heading=0.0000"));

    boatimu.heading.set(77.0);
    server.publish_changed_values();
    pump_for(runtime_loop, 25);
    assert(drain_until(second, "imu.heading=77.0000"));

    second.close();
    pump_for(runtime_loop, 25);
    server.close();
    return 0;
}
