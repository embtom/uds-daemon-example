#include <sd_socket.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace systemd_socket {


std::map<int, std::filesystem::path> getSystemdUnixSockets() noexcept
{
    std::map<int, std::filesystem::path> result;

    int fd_count = sd_listen_fds(0);
    if (fd_count < 0) {
        std::error_code ec(-fd_count, std::system_category());
        spdlog::error("sd_listen_fds failed: {}", ec.message());
        return result;
    }

    if (fd_count == 0) {
        spdlog::warn("No systemd socket passed (LISTEN_FDS=0)");
        return result;
    }

    for (int i = 0; i < fd_count; ++i) {
        int fd = SD_LISTEN_FDS_START + i;

        int socktype = 0;
        socklen_t optlen = sizeof(socktype);
        if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &socktype, &optlen) < 0) {
            std::error_code ec(errno, std::system_category());
            spdlog::error("getsockopt failed for fd {}: {}", fd, ec.message());
            continue;
        }

        sockaddr_un addr{};
        socklen_t len = sizeof(addr);
        if (getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
            std::error_code ec(errno, std::system_category());
            spdlog::error("getsockname failed for fd {}: {}", fd, ec.message());
            continue;
        }

        // Only AF_UNIX sockets
        if (addr.sun_family != AF_UNIX) {
            spdlog::debug("fd {} is not an AF_UNIX socket", fd);
            continue;
        }

        if (addr.sun_path[0] == '\0') {
            spdlog::debug("fd {} is an abstract UNIX socket, ignoring", fd);
            continue;
        }

        result.emplace(fd, std::filesystem::path(addr.sun_path));
    }

    return result;
}

} // namespace systemd_socket
