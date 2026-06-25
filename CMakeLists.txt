cmake_minimum_required(VERSION 3.16)
project(pypilot_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PYPILOT_EVENT_LOOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-event-loop" CACHE PATH "Path to pypilot-event-loop checkout")
set(PYPILOT_SETTINGS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-settings" CACHE PATH "Path to pypilot-settings checkout")
set(PYPILOT_MDNS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-mdns" CACHE PATH "Path to pypilot-mdns checkout")
set(PYPILOT_NMEA0183_CONNECTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-nmea0183-connector" CACHE PATH "Path to pypilot-nmea0183-connector checkout")
set(PYPILOT_SIGNALK_CONNECTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-signalk-connector" CACHE PATH "Path to pypilot-signalk-connector checkout")
set(PYPILOT_DATA_MODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-data-model/src" CACHE PATH "Path to pypilot-data-model src include dir")

if(EXISTS "${PYPILOT_EVENT_LOOP_DIR}/CMakeLists.txt")
    add_subdirectory("${PYPILOT_EVENT_LOOP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-event-loop")
else()
    message(FATAL_ERROR "pypilot-event-loop checkout not found at ${PYPILOT_EVENT_LOOP_DIR}")
endif()

if(EXISTS "${PYPILOT_SETTINGS_DIR}/CMakeLists.txt")
    add_subdirectory("${PYPILOT_SETTINGS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-settings")
else()
    message(FATAL_ERROR "pypilot-settings checkout not found at ${PYPILOT_SETTINGS_DIR}")
endif()

if(EXISTS "${PYPILOT_MDNS_DIR}/CMakeLists.txt")
    add_subdirectory("${PYPILOT_MDNS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-mdns")
else()
    message(FATAL_ERROR "pypilot-mdns checkout not found at ${PYPILOT_MDNS_DIR}")
endif()

if(EXISTS "${PYPILOT_NMEA0183_CONNECTOR_DIR}/CMakeLists.txt")
    set(PYPILOT_NMEA0183_CONNECTOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PYPILOT_NMEA0183_CONNECTOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${PYPILOT_NMEA0183_CONNECTOR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-nmea0183-connector")
else()
    message(FATAL_ERROR "pypilot-nmea0183-connector checkout not found at ${PYPILOT_NMEA0183_CONNECTOR_DIR}")
endif()

if(EXISTS "${PYPILOT_SIGNALK_CONNECTOR_DIR}/CMakeLists.txt")
    set(PYPILOT_SIGNALK_CONNECTOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PYPILOT_SIGNALK_CONNECTOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PYPILOT_MDNS_DIR "${PYPILOT_MDNS_DIR}" CACHE PATH "Path to pypilot-mdns checkout" FORCE)
    add_subdirectory("${PYPILOT_SIGNALK_CONNECTOR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-signalk-connector")
else()
    message(FATAL_ERROR "pypilot-signalk-connector checkout not found at ${PYPILOT_SIGNALK_CONNECTOR_DIR}")
endif()

if(NOT EXISTS "${PYPILOT_DATA_MODEL_DIR}/pypilot_data_model.hpp")
    message(FATAL_ERROR "pypilot-data-model checkout not found at ${PYPILOT_DATA_MODEL_DIR}")
endif()

add_library(pypilot_runtime INTERFACE)
target_include_directories(pypilot_runtime INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PYPILOT_DATA_MODEL_DIR}
)
target_link_libraries(pypilot_runtime INTERFACE
    pypilot_event_loop
    pypilot_settings
    pypilot_mdns
    pypilot_nmea0183_connector
    pypilot_signalk_connector
)
target_compile_definitions(pypilot_runtime INTERFACE
    PYPILOT_RUNTIME_WITH_DATA_MODEL=1
    PYPILOT_RUNTIME_WITH_SETTINGS=1
    PYPILOT_RUNTIME_WITH_MDNS=1
    PYPILOT_RUNTIME_WITH_NMEA0183_CONNECTOR=1
    PYPILOT_RUNTIME_WITH_SIGNALK_CONNECTOR=1
)

include(CTest)
enable_testing()

function(add_runtime_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE pypilot_runtime)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_runtime_test(test_runtime_protocol tests/test_runtime_protocol.cpp)
add_runtime_test(test_runtime_value_catalog tests/test_runtime_value_catalog.cpp)
add_runtime_test(test_runtime_server_compile tests/test_runtime_server_compile.cpp)
add_runtime_test(test_runtime_service_compile tests/test_runtime_service_compile.cpp)
add_runtime_test(test_runtime_local_io tests/test_runtime_loopback_min.cpp)
add_runtime_test(test_runtime_periodic_io tests/test_runtime_periodic_io.cpp)
add_runtime_test(test_runtime_watch_error tests/test_runtime_watch_error.cpp)
add_runtime_test(test_runtime_multi_client_watch tests/test_runtime_multi_client_watch.cpp)
add_runtime_test(test_runtime_periodic_disconnect tests/test_runtime_periodic_disconnect.cpp)
add_runtime_test(test_runtime_error_paths tests/test_runtime_error_paths.cpp)
add_runtime_test(test_runtime_expanded_values tests/test_runtime_expanded_values.cpp)
add_runtime_test(test_runtime_client_helper_compile tests/test_runtime_client_helper_compile.cpp)
add_runtime_test(test_runtime_udp_watch tests/test_runtime_udp_watch.cpp)
add_runtime_test(test_runtime_backpressure tests/test_runtime_backpressure.cpp)
add_runtime_test(test_runtime_settings_bridge tests/test_runtime_settings_bridge.cpp)
add_runtime_test(test_runtime_service_settings tests/test_runtime_service_settings.cpp)
add_runtime_test(test_runtime_mdns_compile tests/test_runtime_mdns_compile.cpp)

add_executable(runtime_server_example examples/RuntimeServerExample/RuntimeServerExample.cpp)
target_link_libraries(runtime_server_example PRIVATE pypilot_runtime)
