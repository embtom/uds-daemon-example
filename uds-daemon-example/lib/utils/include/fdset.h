#ifndef FDSET_H
#define FDSET_H

#include <chrono>
#include <functional>
#include <poll.h>
#include <vector>
#include <system_error>

namespace utils {

using Callback = std::function<void(int fd)>;

struct FdEntry {
    pollfd fd;
    Callback cb;
};

enum class FdSetRet {
    OK,
    UNBLOCK,
    TIMEOUT
};

class FdSetError : public std::system_error {
  public:
    explicit FdSetError(const std::string& what, int errnum = errno)
     : std::system_error(errnum, std::generic_category(), what)
    {
    }
};

class FdSet final {
  public:
    FdSet(const FdSet&) = delete;
    FdSet& operator=(const FdSet&) = delete;
    FdSet(FdSet&&) noexcept;
    FdSet& operator=(FdSet&&) noexcept;

    explicit FdSet();
    ~FdSet();

    void AddFd(int fd, const Callback& cb);
    void AddFd(int fd);
    bool RemoveFd(int fd) noexcept;

    FdSetRet Select(std::chrono::milliseconds timeout) const;
    FdSetRet Select(const Callback& cb, std::chrono::milliseconds timeout) const;
    FdSetRet Select() const;
    FdSetRet Select(const Callback& cb) const;

    bool UnBlock() const noexcept;

  private:
    int unBlockFd_{-1};
    std::vector<FdEntry> entries_;
};

} // namespace utils

#endif // FDSET_H
