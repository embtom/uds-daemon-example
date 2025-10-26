#include <fcntl.h>
#include <pipe.h>
#include <utility>

using namespace utils;

Pipe::Pipe()
{
    std::array<int, 2> fds;
    if (::pipe(fds.data()) == -1)
        throw PipeError("pipe creation failed");

    readFd_ = fds[0];
    writeFd_ = fds[1];

    int flags = fcntl(readFd_, F_GETFL, 0);
    if (flags == -1)
        throw PipeError("fcntl(F_GETFL) failed");

    if (fcntl(readFd_, F_SETFL, flags | O_NONBLOCK) == -1)
        throw PipeError("fcntl(F_SETFL) failed");
}

Pipe::~Pipe()
{
    if (readFd_ >= 0)
        ::close(readFd_);
    if (writeFd_ >= 0)
        ::close(writeFd_);
}

Pipe::Pipe(Pipe&& other) noexcept
 : readFd_(std::exchange(other.readFd_, -1)), writeFd_(std::exchange(other.writeFd_, -1))
{
}

Pipe& Pipe::operator=(Pipe&& other) noexcept
{
    if (this == &other)
        return *this;

    if (readFd_ >= 0)
        close(readFd_);
    if (writeFd_ >= 0)
        close(writeFd_);

    readFd_ = std::exchange(other.readFd_, -1);
    writeFd_ = std::exchange(other.writeFd_, -1);
    return *this;
}

int Pipe::readFd() const noexcept
{
    return readFd_;
}

int Pipe::writeFd() const noexcept
{
    return writeFd_;
}

ssize_t Pipe::writeString(std::string_view sv) noexcept
{
    return writeData(std::span<const char>{sv.data(), sv.size()});
}

std::expected<std::string, std::error_code> Pipe::readString() const noexcept
{
    std::string str(1024, '\0');
    auto n = readData(std::span<char>{str.data(), str.size()});
    if (n < 0)
        return std::unexpected(std::error_code(errno, std::generic_category()));

    str.resize(static_cast<size_t>(n));
    return str;
}
