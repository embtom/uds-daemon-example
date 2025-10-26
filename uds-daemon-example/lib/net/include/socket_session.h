#ifndef SOCKETSESSION_HPP
#define SOCKETSESSION_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <fdset.h>
#include <functional>
#include <memory>
#include <socket.h>
#include <span>
#include <system_error>

namespace net {

using CallbackReceive = std::function<bool(std::span<const std::byte>)>;

constexpr auto defaultOneRead = [](std::span<const std::byte>) noexcept { return true; };

class SocketSessionError : public std::system_error {
  public:
    explicit SocketSessionError(const std::string &what, int errnum = errno)
     : std::system_error(errnum, std::generic_category(), what)
    {
    }
};

//*****************************************************************************
//! \brief SocketSession
//! Thin communication wrapper around a Socket object.
//! Used by both clients and servers after a connection is established.
class SocketSession {
  public:
    enum class ERet { OK, NODATA, ERROR, UNBLOCK };

    SocketSession() noexcept;

    explicit SocketSession(int socketFd) noexcept;
    explicit SocketSession(Socket &&socket) noexcept;

    SocketSession(SocketSession const &) = delete;
    SocketSession &operator=(SocketSession const &) = delete;

    SocketSession(SocketSession &&rhs) noexcept;
    SocketSession &operator=(SocketSession &&rhs) noexcept;

    ~SocketSession() noexcept;

    bool isValid() const noexcept;

    int getFd() const noexcept;

    //-------------------------------------------------------------------------
    // Send
    //-------------------------------------------------------------------------

    template <typename T, std::size_t Extent = std::dynamic_extent>
        requires std::is_trivially_copyable_v<T>
    std::expected<std::size_t, std::errc> send(std::span<T, Extent> buffer) const noexcept
    {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::byte>) {
            return sendImpl(buffer);
        } else {
            return sendImpl(std::as_bytes(buffer));
        }
    }

    //-------------------------------------------------------------------------
    // Receive
    //-------------------------------------------------------------------------

    template <typename T, std::size_t Extent = std::dynamic_extent>
        requires std::is_trivially_copyable_v<T>
    std::expected<std::size_t, std::errc>
    receive(std::span<T, Extent> buffer,
            const CallbackReceive &scanForEnd = defaultOneRead) const noexcept
    {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::byte>) {
            return receiveImpl(buffer, scanForEnd);
        } else {
            return receiveImpl(std::as_writable_bytes(buffer), scanForEnd);
        }
    }

    //! Unblocks a blocking receive.
    bool unblockReceive() const noexcept;

  private:
    std::expected<std::size_t, std::errc>
    sendImpl(std::span<const std::byte> buffer) const noexcept;

    std::expected<std::size_t, std::errc>
    receiveImpl(std::span<std::byte> buffer,
                const CallbackReceive &scanForEnd = defaultOneRead) const noexcept;

    std::expected<std::size_t, std::errc>
    receiveRaw(std::span<std::byte> &buffer, const CallbackReceive &scanForEnd) const noexcept;

    utils::FdSet fdSet_;
    Socket socket_;
};

} // namespace net

#endif // SOCKETSESSION_HPP
