#ifndef NET_UDS_SERVER_WORKER_H_
#define NET_UDS_SERVER_WORKER_H_

#include <uds_server.h>
#include <socket_session_worker.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace net {

class UdsServerWorker {
  public:
    explicit UdsServerWorker(UdsServer&& server);
    ~UdsServerWorker();

    UdsServerWorker(const UdsServerWorker&) = delete;
    UdsServerWorker& operator=(const UdsServerWorker&) = delete;

    UdsServerWorker(UdsServerWorker&&) noexcept = default;
    UdsServerWorker& operator=(UdsServerWorker&&) noexcept = default;

    void Stop() noexcept;

  private:
    void AcceptLoop();

    UdsServer udsServer_;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;
    std::vector<std::unique_ptr<SocketSessionWorker>> workers_;
};

} // namespace net

#endif // NET_UDS_SERVER_WORKER_H_
