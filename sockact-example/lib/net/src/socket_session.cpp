#include "socket_session.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errormsg.h>

namespace net {

SocketSession::SocketSession() noexcept
 : socket_(-1)
{
}

SocketSession::SocketSession(int socketFd) noexcept
 : socket_(socketFd)
{
    if (socketFd >= 0)
        fdSet_.AddFd(socketFd);
}

SocketSession::SocketSession(Socket &&socket) noexcept
 : socket_(std::move(socket))
{
    if (socket_.isValid())
        fdSet_.AddFd(socket_.getFd());
}

SocketSession::SocketSession(SocketSession &&rhs) noexcept
 : fdSet_(std::move(rhs.fdSet_))
 , socket_(std::move(rhs.socket_))
{
}

SocketSession &SocketSession::operator=(SocketSession &&rhs) noexcept
{
    if (this != &rhs) {
        fdSet_ = std::move(rhs.fdSet_);
        socket_ = std::move(rhs.socket_);
    }
    return *this;
}

SocketSession::~SocketSession() noexcept
{
    if (!socket_.isValid())
        return;
    fdSet_.RemoveFd(socket_.getFd());
}

bool SocketSession::isValid() const noexcept { return socket_.isValid(); }

int SocketSession::getFd() const noexcept { return socket_.getFd(); }

//*****************************************************************************
// Send
//*****************************************************************************

std::expected<std::size_t, std::errc>
SocketSession::sendImpl(std::span<const std::byte> buffer) const noexcept
{
    std::size_t dataWritten = 0;
    const int fd = socket_.getFd();

    while (dataWritten < buffer.size_bytes()) {
        ssize_t put = ::send(fd, buffer.data() + dataWritten, buffer.size_bytes() - dataWritten, 0);

        if (put < 0) {
            std::error_code ec(errno, std::generic_category());

            if (ec == std::errc::interrupted) {
                spdlog::debug("SocketSession::send: EINTR — retrying");
                continue;
            }

            if (ec == std::errc::operation_would_block) {
                spdlog::debug("SocketSession::send: would block");
                return std::unexpected(std::errc::operation_would_block);
            }

            spdlog::warn("SocketSession::send: send() failed: {}", ec.message());
            return std::unexpected(std::errc::io_error);
        }

        dataWritten += static_cast<std::size_t>(put);
    }

    return dataWritten;
}

bool SocketSession::unblockReceive() const noexcept { return fdSet_.UnBlock(); }

std::expected<std::size_t, std::errc>
SocketSession::receiveImpl(std::span<std::byte> buffer,
                           const CallbackReceive &scanForEnd) const noexcept
{
    const int fd = socket_.getFd();
    utils::FdSetRet ret = fdSet_.Select([&fd](int readyFd) {
        if (readyFd == fd) {
            spdlog::debug("fd {} is readable", readyFd);
        } else {
            spdlog::warn("SocketSession::receive: unexpected fd {} signaled, expected {}", readyFd,
                         fd);
        }
    });

    switch (ret) {
    case utils::FdSetRet::UNBLOCK:
        return std::unexpected(std::errc::operation_canceled);
    case utils::FdSetRet::TIMEOUT:
        return std::unexpected(std::errc::timed_out);
    case utils::FdSetRet::OK: {
        auto result = receiveRaw(buffer, scanForEnd);
        if (!result.has_value()) {
            spdlog::warn("SocketSession::receiveRaw failed: {}",
                         std::make_error_code(result.error()).message());
        }
        return result; // propagate expected<std::size_t, errc>
    }
    default:
        return std::unexpected(std::errc::io_error);
    }
}

std::expected<std::size_t, std::errc>
SocketSession::receiveRaw(std::span<std::byte> &buffer,
                          const CallbackReceive &scanForEnd) const noexcept
{
    if (!socket_.isValid()) {
        spdlog::error("SocketSession::receiveRaw: invalid socket (moved)");
        return std::unexpected(std::errc::bad_file_descriptor);
    }

    std::size_t dataRead = 0;
    auto *readBuffer = buffer.data();
    int fd = socket_.getFd();

    spdlog::debug("SocketSession::receiveRaw: starting receive on fd {}", fd);

    {
        char probe;
        while (true) {
            ssize_t peek = ::recv(fd, &probe, 1, MSG_PEEK);
            if (peek == 0) {
                spdlog::info(
                    "SocketSession::receiveRaw: peer closed connection (MSG_PEEK returned 0)");
                return std::unexpected(std::errc::connection_reset);
            }
            if (peek < 0) {
                std::error_code ec(errno, std::generic_category());

                if (ec == std::errc::interrupted) {
                    spdlog::debug(
                        "SocketSession::receiveRaw: recv(MSG_PEEK) interrupted — retrying");
                    continue;
                }
                if (ec == std::errc::operation_would_block) {
                    spdlog::debug("SocketSession::receiveRaw: no data yet (MSG_PEEK would block)");
                    buffer = std::span(readBuffer, 0);
                    return 0;
                }

                spdlog::error("SocketSession::receiveRaw: MSG_PEEK failed: {}", ec.message());
                return std::unexpected(static_cast<std::errc>(ec.value()));
            }
            break; // success
        }
    }

    while (dataRead < buffer.size_bytes()) {
        ssize_t got = ::recv(fd, readBuffer + dataRead, buffer.size_bytes() - dataRead, 0);
        if (got < 0) {
            std::error_code ec(errno, std::generic_category());

            if (ec == std::errc::interrupted || ec == std::errc::resource_unavailable_try_again) {
                spdlog::debug("SocketSession::receiveRaw: temporary recv() error — retrying");
                continue;
            }

            spdlog::warn("SocketSession::receiveRaw: recv() failed: {}", ec.message());
            return std::unexpected(static_cast<std::errc>(ec.value()));
        }

        if (got == 0) {
            spdlog::info("SocketSession::receiveRaw: connection closed by peer");
            break;
        }

        dataRead += static_cast<std::size_t>(got);
        spdlog::debug("SocketSession::receiveRaw: read {} bytes (total {})", got, dataRead);

        if (scanForEnd(std::span(readBuffer, dataRead))) {
            spdlog::debug("SocketSession::receiveRaw: scanForEnd triggered stop condition");
            break;
        }
    }

    buffer = std::span(readBuffer, dataRead);
    spdlog::info("SocketSession::receiveRaw: finished reading {} bytes", dataRead);
    return dataRead;
}

} // namespace net
