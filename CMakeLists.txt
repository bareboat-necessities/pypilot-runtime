cmake_minimum_required(VERSION 3.16)
project(pypilot_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PYPILOT_EVENT_LOOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pypilot-event-loop" CACHE PATH "Path to pypilot-event-loop checkout")

if(EXISTS "${PYPILOT_EVENT_LOOP_DIR}/CMakeLists.txt")
    add_subdirectory("${PYPILOT_EVENT_LOOP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pypilot-event-loop")
else()
    message(FATAL_ERROR "pypilot-event-loop checkout not found at ${PYPILOT_EVENT_LOOP_DIR}")
endif()

add_library(pypilot_runtime INTERFACE)
target_include_directories(pypilot_runtime INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(pypilot_runtime INTERFACE pypilot_event_loop)

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
add_runtime_test(test_runtime_local_io tests/test_runtime_loopback_min.cpp)
add_runtime_test(test_runtime_periodic_io tests/test_runtime_periodic_io.cpp)
add_runtime_test(test_runtime_watch_error tests/test_runtime_watch_error.cpp)
add_runtime_test(test_runtime_multi_client_watch tests/test_runtime_multi_client_watch.cpp)
add_runtime_test(test_runtime_periodic_disconnect tests/test_runtime_periodic_disconnect.cpp)
add_runtime_test(test_runtime_error_paths tests/test_runtime_error_paths.cpp)
add_runtime_test(test_runtime_expanded_values tests/test_runtime_expanded_values.cpp)
add_runtime_test(test_runtime_client_helper_compile tests/test_runtime_client_helper_compile.cpp)

add_executable(runtime_server_example examples/RuntimeServerExample/RuntimeServerExample.cpp)
target_link_libraries(runtime_server_example PRIVATE pypilot_runtime)
