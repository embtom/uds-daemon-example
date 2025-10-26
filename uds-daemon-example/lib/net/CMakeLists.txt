set(HEADERS
    "include/socket.h"
    "include/uds_server.h"
    "include/uds_client.h"
    "include/socket_session.h")

set(SOURCES
    "src/uds_server.cpp"
    "src/uds_client.cpp"
    "src/socket_session.cpp")

add_library(net STATIC ${HEADERS} ${SOURCES})

enable_strict_warnings(net)

target_include_directories(net PUBLIC "include")

target_link_libraries(net PUBLIC utils spdlog::spdlog)
