#include <cassert>
#include <cstring>
#include <unistd.h>

#include <pypilot_runtime.hpp>
#include <pypilot_runtime_client.hpp>

static void pump_for(pypilot_event_loop::EventLoop<64, 128>& runtime_loop, uint32_t milliseconds) {
    runtime_loop.on_delay(milliseconds, [&] { runtime_loop.request_exit(); });
    runtime_loop.run_forever();
}

static bool recv_udp_line(pypilot_event_loop::NativeUdpDatagramStream& udp, const char* expected) {
    uint8_t data[256]{};
    pypilot_event_loop::UdpEndpoint source{};
    for (int i = 0; i < 100; ++i) {
        const int n = udp.recv_from(data, sizeof(data) - 1, &source);
        if (n > 0) {
            data[n] = '\0';
            if (std::strcmp(reinterpret_cast<const char*>(data), expected) == 0) return true;
        }
        usleep(1000);
    }
    return false;
}

static bool drain_tcp_until(pypilot_runtime::PypilotRuntimeClient<>& client, const char* expected) {
    char line[160]{};
    while (client.read_line(line, sizeof(line))) {
        if (std::strcmp(line, expected) == 0) return true;
    }
    return false;
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

    pypilot_runtime::PypilotRuntimeServerOptions options;
    options.udp_watch_port = 24100;
    options.max_output_bytes = 4096;
    assert(server.configure(options));
    assert(server.udp_watch_enabled());
    assert(server.listen("127.0.0.1", 0));

    pypilot_event_loop::NativeUdpDatagramStream udp_client;
    assert(udp_client.bind(24101));

    pypilot_runtime::PypilotRuntimeClient<> client(runtime_loop);
    assert(client.open("127.0.0.1", server.port()));
    pump_for(runtime_loop, 25);
    assert(client.connected());

    assert(client.set_number("udp_port", 24101));
    pump_for(runtime_loop, 25);

    assert(client.watch("imu.heading", 0.0));
    pump_for(runtime_loop, 25);
    assert(recv_udp_line(udp_client, "imu.heading=0.0000\n"));
    assert(!drain_tcp_until(client, "imu.heading=0.0000"));

    boatimu.heading.set(42.0);
    server.publish_changed_values();
    pump_for(runtime_loop, 25);
    assert(recv_udp_line(udp_client, "imu.heading=42.0000\n"));

    assert(client.set_number("udp_port", 0.0));
    pump_for(runtime_loop, 25);
    boatimu.heading.set(43.0);
    server.publish_changed_values();
    pump_for(runtime_loop, 25);
    assert(drain_tcp_until(client, "imu.heading=43.0000"));

    client.close();
    server.close();
    udp_client.close();
    return 0;
}
