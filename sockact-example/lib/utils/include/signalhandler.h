#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <initializer_list>
#include <signal.h>
#include <stdexcept>
#include <string>

namespace utils {

class SignalHandlerException : public std::runtime_error {
  public:
    SignalHandlerException() : std::runtime_error("SignalHandlerException")
    {
    }
    explicit SignalHandlerException(const std::string& msg) : std::runtime_error(msg)
    {
    }
};

class CSignalHandler {
  public:
    explicit CSignalHandler(const std::initializer_list<int>& signals);
    ~CSignalHandler() noexcept;

    CSignalHandler(const CSignalHandler&) = delete;
    CSignalHandler(CSignalHandler&&) = delete;
    CSignalHandler& operator=(const CSignalHandler&) = delete;
    CSignalHandler& operator=(CSignalHandler&&) = delete;

    int Sigwait() const;
    void EnableSigsegvHandler() const;

  private:
    static void signalHandler(int sig, siginfo_t* info, void* ctx);
    sigset_t m_sigSet{};
};

} // namespace utils

#endif // SIGNALHANDLER_H
