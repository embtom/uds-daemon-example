#include <concepts>
#include <errno.h>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <unistd.h>

#ifndef PIPE_H
#define PIPE_H

namespace utils {

class PipeError : public std::system_error {
  public:
    explicit PipeError(const std::string& what, int errnum = errno)
     : std::system_error(errnum, std::generic_category(), what)
    {
    }
};

class Pipe {
  public:
    Pipe();
    ~Pipe();

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe(Pipe&& other) noexcept;
    Pipe& operator=(Pipe&& other) noexcept;

    ssize_t writeString(std::string_view sv) noexcept;
    std::expected<std::string, std::error_code> readString() const noexcept;

    int readFd() const noexcept;
    int writeFd() const noexcept;

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    ssize_t writeData(std::span<const T> data) noexcept
    {
        auto bytes = std::as_bytes(data);
        size_t written{0};

        while (written < bytes.size()) {
            ssize_t ret = ::write(writeFd_, bytes.data() + written, bytes.size() - written);
            if (ret < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            written += static_cast<size_t>(ret);
        }
        return static_cast<ssize_t>(written);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    ssize_t readData(std::span<T> buffer) const noexcept
    {
        auto bytes = std::as_writable_bytes(buffer);
        size_t read{0};

        while (read < bytes.size()) {
            ssize_t ret = ::read(readFd_, bytes.data() + read, bytes.size() - read);
            bool done{false};
            if (ret < 0) {
                if (errno == EINTR)
                    continue;
                if (errno == EWOULDBLOCK)
                    done = true;
                else
                    return -1;
            } else if (ret == 0)
                done = true;
            else
                read += static_cast<size_t>(ret);

            if (done)
                break;
        }
        return static_cast<ssize_t>(read);
    }

  private:
    int readFd_{-1};
    int writeFd_{-1};
};

} // namespace utils

#endif
