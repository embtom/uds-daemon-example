#include <algorithm>
#include <errno.h>
#include <fdset.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace utils;

FdSet::FdSet(FdSet &&other) noexcept
 : unBlockFd_(std::exchange(other.unBlockFd_, -1))
 , entries_(std::move(other.entries_))
{
}

FdSet &FdSet::operator=(FdSet &&other) noexcept
{
    if (this != &other) {
        ::close(unBlockFd_);
        unBlockFd_ = std::exchange(other.unBlockFd_, -1);
    }
    return *this;
}

FdSet::FdSet()
{
    unBlockFd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (unBlockFd_ == -1)
        throw FdSetError("eventfd creation failed");

    AddFd(unBlockFd_, nullptr);
}

FdSet::~FdSet()
{
    if (unBlockFd_ > 0) {
        UnBlock();
        close(unBlockFd_);
    }
}

void FdSet::AddFd(int fd, const Callback &cb)
{
    FdEntry entry{
        .fd = {fd, POLLIN, 0},
        .cb = cb,
    };
    entries_.push_back(entry);
}

void FdSet::AddFd(int fd) { AddFd(fd, nullptr); }

bool FdSet::RemoveFd(int fd) noexcept
{
    auto erased = std::erase_if(entries_, [fd](const FdEntry &p) { return p.fd.fd == fd; });
    return erased > 0;
}

FdSetRet FdSet::Select(std::chrono::milliseconds timeout) const { return Select(nullptr, timeout); }

FdSetRet FdSet::Select(const Callback &cb, std::chrono::milliseconds timeout) const
{
    int pollTimeout =
        (timeout == std::chrono::milliseconds::max()) ? -1 : static_cast<int>(timeout.count());
    int ret;

    std::vector<struct pollfd> fds(entries_.size());
    std::ranges::transform(entries_, fds.begin(), [](const FdEntry &e) { return e.fd; });

    do {
        ret = ::poll(fds.data(), fds.size(), pollTimeout);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1)
        throw FdSetError("poll failed");

    if (ret == 0)
        return FdSetRet::TIMEOUT;

    for (size_t i = 0; i < fds.size(); ++i) {
        if (!(fds[i].revents & POLLIN))
            continue;

        if (fds[i].fd == unBlockFd_) {
            uint64_t val;
            ssize_t n = ::read(unBlockFd_, &val, sizeof(val)); // read empty
            (void)n;
            return FdSetRet::UNBLOCK;
        }

        if (cb)
            cb(fds[i].fd);

        if (entries_[i].cb)
            entries_[i].cb(fds[i].fd);
    }

    return FdSetRet::OK;
}

FdSetRet FdSet::Select() const { return Select(nullptr, std::chrono::milliseconds::max()); }

FdSetRet FdSet::Select(const Callback &cb) const
{
    return Select(cb, std::chrono::milliseconds::max());
}

bool FdSet::UnBlock() const noexcept
{
    if (uint64_t val = 1; ::write(unBlockFd_, &val, sizeof(val)) == -1 && errno != EAGAIN) {
        spdlog::error("Failed to write to signalFd: {}", strerror(errno));
        return true;
    }
    return false;
}
