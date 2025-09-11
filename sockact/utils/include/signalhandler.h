#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <signal.h>

namespace utils
{

class SignalHandlerException : public std::runtime_error
{
public:
    SignalHandlerException() : std::runtime_error("SignalHandlerException") {}
    explicit SignalHandlerException(const std::string& msg) : std::runtime_error(msg) {}
};

class CSignalHandler
{
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
    static void signalHandler(int signum);
    sigset_t m_sigSet{};
};

#ifdef ENABLE_LOCAL_UNWIND
void plotBacktrace();
#endif

} // namespace utils

#endif // SIGNALHANDLER_H
