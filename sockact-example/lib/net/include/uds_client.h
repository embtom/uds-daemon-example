#ifndef NET_UDS_CLIENT_H
#define NET_UDS_CLIENT_H

#include <expected>
#include <filesystem>
#include <optional>
#include <socket_session.h>
#include <system_error>

namespace fs = std::filesystem;

namespace net {

class UdsClient {
  public:
    explicit UdsClient();

    UdsClient(const UdsClient &) = delete;
    UdsClient &operator=(const UdsClient &) = delete;

    std::errc connect(const fs::path &socket_path);

    void disconnect() noexcept;

    bool isConnected() const noexcept;

    std::optional<fs::path> socketPath() const noexcept;

    template <typename T, std::size_t Extent = std::dynamic_extent>
        requires std::is_trivially_copyable_v<T>
    std::expected<std::size_t, std::errc> send(std::span<T, Extent> buffer) const noexcept
    {
        return session_.send(buffer);
    }

    template <typename T, std::size_t Extent = std::dynamic_extent>
        requires std::is_trivially_copyable_v<T>
    std::expected<std::size_t, std::errc>
    receive(std::span<T, Extent> buffer,
            const CallbackReceive &scanForEnd = defaultOneRead) const noexcept
    {
        return session_.receive(buffer, scanForEnd);
    }

  private:
    SocketSession session_;
    std::optional<fs::path> socket_path_;
};

}; // namespace net

#endif /* NET_UDS_CLIENT_H */
