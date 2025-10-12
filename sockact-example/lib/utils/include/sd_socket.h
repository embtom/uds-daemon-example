#ifndef SD_SOCKET_H
#define SD_SOCKET_H

#include <filesystem>
#include <map>

namespace fs = std::filesystem;
namespace systemd_socket {

std::map<int, fs::path> getSystemdUnixSockets() noexcept;

} // namespace systemd_socket

#endif /* SD_SOCKET_H */
