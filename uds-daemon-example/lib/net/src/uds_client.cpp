#include "uds_client.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace net {

inline const sockaddr *to_sockaddr(const sockaddr_un *addr) noexcept
{
    return reinterpret_cast<const sockaddr *>(addr);
}

UdsClient::UdsClient()
 : session_(Socket(ESocketMode::UNIX_STREAM))
{
}

std::errc UdsClient::connect(const fs::path &socket_path)
{
    if (!fs::exists(socket_path)) {
        spdlog::error("UdsClient::connect: socket path '{}' does not exist", socket_path.string());
        return std::errc::no_such_file_or_directory;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (session_.getFd() < 0) {
        session_ = SocketSession(Socket(ESocketMode::UNIX_STREAM));
    }

    int fd = session_.getFd();
    if (fd < 0) {
        spdlog::error("UdsClient::connect: invalid socket descriptor");
        return std::errc::bad_file_descriptor;
    }

    if (::connect(fd, to_sockaddr(&addr), sizeof(addr)) < 0) {
        std::error_code ec(errno, std::generic_category());
        spdlog::error("UdsClient::connect: failed to connect to '{}': {}", socket_path.string(),
                      ec.message());
        return static_cast<std::errc>(ec.value());
    }

    spdlog::info("UdsClient connected to '{}'", socket_path.string());
    socket_path_ = socket_path;
    return std::errc{};
}

void UdsClient::disconnect() noexcept
{
    if (!isConnected()) {
        return;
    }

    if (int fd = session_.getFd(); fd < 0) {
        ::shutdown(fd, SHUT_RDWR);
        spdlog::info("UdsClient disconnected from '{}'", socket_path_->string());
    }

    socket_path_.reset();
    session_ = SocketSession{};
}

bool UdsClient::isConnected() const noexcept
{
    return socket_path_.has_value() && session_.getFd() >= 0;
}

std::optional<fs::path> UdsClient::socketPath() const noexcept { return socket_path_; }

} // namespace net
