#ifndef SOCKET_H
#define SOCKET_H

#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace net {

enum class ESocketMode {
    INET_DGRAM,
    INET_STREAM,
    INET6_DGRAM,
    INET6_STREAM,
    UNIX_DGRAM,
    UNIX_STREAM,
    NO_MODE
};

class Socket {
  public:
    Socket() = default;
    explicit Socket(ESocketMode opMode)
    {
        auto [domain, type] = getDomainAndType(opMode);
        int fd = ::socket(domain, type, 0);
        if (fd < 0) {
            throw std::system_error(errno, std::system_category(), "socket() failed");
        }
        m_socketFd = fd;
    }
    explicit Socket(int fd) noexcept : m_socketFd(fd)
    {
    }

    ~Socket()
    {
        close();
    }

    // no copy
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : m_socketFd(std::exchange(other.m_socketFd, -1))
    {
    }

    Socket& operator=(Socket&& other) noexcept
    {
        if (this != &other) {
            ::close(m_socketFd);
            m_socketFd = std::exchange(other.m_socketFd, -1);
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_socketFd >= 0;
    }
    [[nodiscard]] int getFd() const noexcept
    {
        return m_socketFd;
    }

    void close() noexcept
    {
        if (isValid()) {
            ::close(m_socketFd);
            m_socketFd = -1;
        }
    }

    void soReuseSocket() const
    {
        int optval = 1;
        if (::setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            throw std::system_error(
                errno, std::system_category(), "setsockopt(SO_REUSEADDR) failed");
        }
    }

  private:
    int m_socketFd{-1};

    static std::pair<int, int> getDomainAndType(ESocketMode mode)
    {
        switch (mode) {
        case ESocketMode::INET_DGRAM: return {AF_INET, SOCK_DGRAM};
        case ESocketMode::INET_STREAM: return {AF_INET, SOCK_STREAM};
        case ESocketMode::INET6_DGRAM: return {AF_INET6, SOCK_DGRAM};
        case ESocketMode::INET6_STREAM: return {AF_INET6, SOCK_STREAM};
        case ESocketMode::UNIX_DGRAM: return {AF_UNIX, SOCK_DGRAM};
        case ESocketMode::UNIX_STREAM: return {AF_UNIX, SOCK_STREAM};
        case ESocketMode::NO_MODE:
        default: throw std::invalid_argument("Invalid socket mode");
        }
    }
};

} // namespace net

#endif
