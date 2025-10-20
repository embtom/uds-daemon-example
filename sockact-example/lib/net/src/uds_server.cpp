#include <cstring>
#include <final_action.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <uds_server.h>

constexpr int maxConnectionBacklog = 5;

using namespace net;

inline const sockaddr *to_sockaddr(const sockaddr_un *addr) noexcept
{
    return reinterpret_cast<const sockaddr *>(addr);
}

UdsServer::UdsServer(const fs::path &socketPath)
 : socket_(ESocketMode::UNIX_STREAM)
 , socket_path_(socketPath)
{
    fs::remove(socketPath);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(socket_.getFd(), to_sockaddr(&addr), sizeof(addr)) == -1) {
        throw UdsServerError("bind failed");
    }

    if (::listen(socket_.getFd(), maxConnectionBacklog) == -1) {
        throw UdsServerError("listen failed");
    }
}

UdsServer::UdsServer(const systemd_socket::SocketInfo &sd_socket_info)
 : socket_(sd_socket_info.fd)
 , socket_path_(sd_socket_info.path)
{
    if(!socket_.isValid()) {
        throw UdsServerError("invalid systemd socket fd");
    }

    if (! fs::exists(socket_path_)) {
        throw UdsServerError("invalid systemd unix domain socket");
    }
}

UdsServer::~UdsServer()
{
    if (!socket_path_.empty()) {
        ::unlink(socket_path_.c_str());
    }
}

UdsServer::UdsServer(UdsServer &&other) noexcept
 : socket_(std::move(other.socket_))
 , socket_path_(std::move(other.socket_path_))
{
    other.socket_path_.clear();
}

UdsServer &UdsServer::operator=(UdsServer &&other) noexcept
{
    if (this == &other)
        return *this;

    if (!socket_path_.empty()) {
        ::unlink(socket_path_.c_str());
    }

    socket_ = std::move(other.socket_);
    socket_path_ = std::move(other.socket_path_);

    other.socket_path_.clear();
    return *this;
}

std::expected<SocketSession, std::errc> UdsServer::WaitForConnection() noexcept
{
    fdSet_.AddFd(socket_.getFd());
    auto finally = utils::Finally([this]() noexcept { fdSet_.RemoveFd(socket_.getFd()); });

    if (utils::FdSetRet ret = fdSet_.Select(); ret == utils::FdSetRet::UNBLOCK)
        return std::unexpected(std::errc::operation_canceled);

    int clientFd;
    do {
        clientFd = ::accept(socket_.getFd(), nullptr, nullptr);
    } while (clientFd < 0 && errno == EINTR);

    if (clientFd < 0)
        return std::unexpected(static_cast<std::errc>(errno));

    return SocketSession(clientFd);
}

void UdsServer::Unblock() const noexcept { fdSet_.UnBlock(); }

const Socket &UdsServer::ServerSocket() const noexcept { return socket_; }

fs::path UdsServer::SocketPath() const noexcept { return socket_path_; }
