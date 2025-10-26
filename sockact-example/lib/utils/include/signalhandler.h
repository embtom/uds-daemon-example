#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <cerrno>
#include <csignal>
#include <initializer_list>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace utils {

class SignalError : public std::system_error {
  public:
    explicit SignalError(const std::string &what, int errnum = errno)
     : std::system_error(errnum, std::generic_category(), what)
    {
    }
};

class SignalHandler {
  public:
    explicit SignalHandler(std::initializer_list<int> signals);
    ~SignalHandler();

    int wait() const;
    static void enableSegfaultHandler();

  private:
    static void segFaultHandler(int sig, siginfo_t *info, void *ctx);
    sigset_t sigSet{};
};

} // namespace utils

#endif // SIGNALHANDLER_H
