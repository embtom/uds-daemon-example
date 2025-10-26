#ifndef NET_UDS_SERVER_H_
#define NET_UDS_SERVER_H_

#include <filesystem>
#include <socket_session.h>
#include <fdset.h>
#include <system_error>
#include <expected>
#include <sd_socket.h>

namespace fs = std::filesystem;

namespace net {

class UdsServerError : public std::system_error {
  public:
    explicit UdsServerError(const std::string &what, int errnum = errno)
     : std::system_error(errnum, std::generic_category(), what)
    {
    }
};

class UdsServer {
  public:
    UdsServer() = default;
    explicit UdsServer(const fs::path &socket_path);
    explicit UdsServer(const systemd_socket::SocketInfo &sd_socket_info);
    ~UdsServer();

    UdsServer(const UdsServer &) = delete;
    UdsServer &operator=(const UdsServer &) = delete;

    UdsServer(UdsServer &&) noexcept;
    UdsServer &operator=(UdsServer &&) noexcept;

    std::expected<SocketSession, std::errc> WaitForConnection() noexcept;

    void Unblock() const noexcept;
    fs::path SocketPath() const noexcept;
    const Socket &ServerSocket() const noexcept;

  private:
    Socket socket_;
    fs::path socket_path_;
    utils::FdSet fdSet_;
};

} // namespace net

#endif // NET_UDS_SERVER_H_
