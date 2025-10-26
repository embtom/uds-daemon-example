#include "socket_session_worker.h"
#include <array>
#include <span>
#include <spdlog/spdlog.h>
#include <format>
#include <byte_util.h>

namespace net {

SocketSessionWorker::SocketSessionWorker(SocketSession &&session)
 : session_(std::move(session))
 , running_(true)
 , thread_(&SocketSessionWorker::Run, this)
{
    spdlog::debug("SessionWorker started (fd={})", session_.getFd());
}

SocketSessionWorker::~SocketSessionWorker() { Stop(); }

void SocketSessionWorker::Stop() noexcept
{
    if (!running_.exchange(false))
        return;

    session_.unblockReceive();

    if (thread_.joinable())
        thread_.join();

    spdlog::debug("SessionWorker stopped");
}

void SocketSessionWorker::Run() const
{
    std::array<std::byte, 1024> buffer{};
    int rcvCount = 0;

    while (running_) {
        auto msg = session_.receive(std::span(buffer));
        if (!msg.has_value()) {
            spdlog::debug("Session disconnected (fd={})", session_.getFd());
            break;
        }
        std::string_view str = utils::from_bytes(std::span(buffer.data(), msg.value()));
        spdlog::info("Message Received {}", str);

        std::string response = std::format("{}-replay {}",rcvCount,str);
        rcvCount++;
        session_.send(std::span(response));
    }
}

} // namespace net
