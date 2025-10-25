#ifndef SOCKET_SESSION_WORKER_H_
#define SOCKET_SESSION_WORKER_H_

#include <socket_session.h>
#include <atomic>
#include <thread>

namespace net {

class SocketSessionWorker {
  public:
    explicit SocketSessionWorker(SocketSession&& session);
    ~SocketSessionWorker();

    // Non-copyable
    SocketSessionWorker(const SocketSessionWorker&) = delete;
    SocketSessionWorker& operator=(const SocketSessionWorker&) = delete;

    // Movable
    SocketSessionWorker(SocketSessionWorker&&) noexcept = default;
    SocketSessionWorker& operator=(SocketSessionWorker&&) noexcept = default;

    void Stop() noexcept;

  private:
    void Run() const;

    SocketSession session_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace net

#endif // SOCKET_SESSION_WORKER_H_
