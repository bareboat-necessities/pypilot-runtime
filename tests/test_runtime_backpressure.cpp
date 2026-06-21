#include <cassert>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_client.hpp>

static void pump_for(pypilot_event_loop::EventLoop<64, 128>& runtime_loop, uint32_t milliseconds) {
    runtime_loop.on_delay(milliseconds, [&] { runtime_loop.request_exit(); });
    runtime_loop.run_forever();
}

int main() {
    pypilot_event_loop::EventLoop<64, 128> runtime_loop;

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
    server.set_max_output_bytes(1);
    assert(server.listen("127.0.0.1", 0));

    pypilot_runtime::PypilotRuntimeClient<> client(runtime_loop);
    assert(client.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 25);
    assert(client.connected());

    assert(client.watch("imu.heading", 0.0));
    pump_for(runtime_loop, 25);

    assert(server.connection_count() == 0 || !client.connected());
    client.close();
    server.close();
    return 0;
}
