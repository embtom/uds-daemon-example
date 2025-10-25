add_executable(
    sockact-a
    "sockact-a.cpp"
    "server_worker.h"
    "server_worker.cpp"
    "socket_session_worker.h"
    "socket_session_worker.cpp")

target_compile_features(sockact-a PRIVATE cxx_std_23)

target_include_directories(sockact-a PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(
    sockact-a
    PRIVATE libsystemd::libsystemd
    PRIVATE spdlog::spdlog
    PRIVATE cxxopts::cxxopts
    PRIVATE utils
    PRIVATE net)

enable_strict_warnings(sockact-a)

install(TARGETS sockact-a RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
