#include <cassert>

#include <pypilot_runtime.hpp>

int main() {
    pypilot_event_loop::EventLoop<8, 128> loop;

    pypilot_runtime::AutopilotValues autopilot;
    pypilot_runtime::BoatImuValues boatimu;
    pypilot_runtime::ServoValues servo;
    pypilot_runtime::SensorValues sensors;
    pypilot_runtime::PilotValues pilots;
    pypilot_runtime::GpsValues gps;
    pypilot_runtime::WindValues wind;

    pypilot_runtime::PypilotRuntimeState state{autopilot, boatimu, sensors, servo, pilots, gps, wind};
    pypilot_runtime::PypilotRuntimeProtocol protocol(state);
    pypilot_runtime::PypilotRuntimeServer<2, 4> server(loop, protocol);

    pypilot_event_loop::TcpTimeoutOptions timeouts;
    timeouts.read_timeout_ms = 1000;
    timeouts.write_timeout_ms = 1000;
    server.set_tcp_timeouts(timeouts);

    pypilot_event_loop::TcpWatermarkOptions watermarks;
    watermarks.read_low = 1;
    watermarks.write_low = 0;
    server.set_tcp_watermarks(watermarks);
    server.set_max_output_bytes(4096);

    assert(!server.valid());
    return 0;
}
