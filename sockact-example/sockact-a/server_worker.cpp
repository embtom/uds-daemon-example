#include "server_worker.h"
#include <spdlog/spdlog.h>

namespace net {

UdsServerWorker::UdsServerWorker(UdsServer&& server)
 : udsServer_(std::move(server)), running_(true)
{
    acceptThread_ = std::thread(&UdsServerWorker::AcceptLoop, this);
    spdlog::info("UdsServerWorker started (socket: {})", udsServer_.SocketPath().string());
}

UdsServerWorker::~UdsServerWorker()
{
    Stop();
}

void UdsServerWorker::Stop() noexcept
{
    if (!running_.exchange(false))
        return; // already stopped

    spdlog::info("Stopping UdsServerWorker...");
    udsServer_.Unblock();

    if (acceptThread_.joinable())
        acceptThread_.join();

    spdlog::debug("Cleaning up session workers...");
    workers_.clear();
}

void UdsServerWorker::AcceptLoop()
{
    spdlog::info("Accept thread started — waiting for clients...");
    while (running_) {
        auto sessionResult = udsServer_.WaitForConnection();

        if (!sessionResult) {
            if (sessionResult.error() == std::errc::operation_canceled) {
                spdlog::debug("Accept loop unblocked — shutting down accept thread");
                break;
            }
            spdlog::error("Failed to accept connection: {}",
                          std::make_error_code(sessionResult.error()).message());
            continue;
        }

        spdlog::info("New client connected (fd={})", sessionResult->getFd());
        workers_.push_back(std::make_unique<SocketSessionWorker>(std::move(*sessionResult)));
    }

    spdlog::info("Accept thread exiting...");
}

} // namespace net
