#ifndef SD_SOCKET_H
#define SD_SOCKET_H

#include <filesystem>
#include <vector>

namespace systemd_socket {

struct SocketInfo {
    int fd;
    std::filesystem::path path;
};

std::vector<SocketInfo> getSystemdUnixSockets() noexcept;

} // namespace systemd_socket

#endif /* SD_SOCKET_H */
