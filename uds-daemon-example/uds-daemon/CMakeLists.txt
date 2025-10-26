add_executable(
    uds-daemon
    "uds-daemon.cpp"
    "server_worker.h"
    "server_worker.cpp"
    "socket_session_worker.h"
    "socket_session_worker.cpp")

target_compile_features(uds-daemon PRIVATE cxx_std_23)

target_include_directories(uds-daemon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(
    uds-daemon
    PRIVATE libsystemd::libsystemd
    PRIVATE spdlog::spdlog
    PRIVATE cxxopts::cxxopts
    PRIVATE utils
    PRIVATE net)

enable_strict_warnings(uds-daemon)

install(TARGETS uds-daemon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
