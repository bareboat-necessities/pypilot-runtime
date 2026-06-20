#include <cassert>
#include <unistd.h>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_client.hpp>

static void spin_loop(pypilot_event_loop::EventLoop<64, 128>& runtime_loop) {
    for (int i = 0; i < 80; ++i) {
        runtime_loop.run_once();
        usleep(1000);
    }
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
    assert(server.listen("localhost", 0));

    pypilot_runtime::PypilotRuntimeClient<> client(runtime_loop);
    assert(client.open("localhost", server.port()));
    spin_loop(runtime_loop);
    assert(client.connected());

    assert(client.set_bool("ap.enabled", true));
    spin_loop(runtime_loop);
    assert(autopilot.enabled.get());

    assert(client.set_number("servo.command", -0.15));
    spin_loop(runtime_loop);
    assert(servo.command.get() < -0.149 && servo.command.get() > -0.151);

    client.close();
    server.close();
    return 0;
}
