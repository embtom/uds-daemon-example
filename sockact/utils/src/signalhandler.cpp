#include "signalhandler.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

#include "errormsg.h"
//#include "to_array.h"

#ifdef ENABLE_LOCAL_UNWIND
#include <absl/debugging/internal/demangle.h>
#include <libunwind.h>
#endif

namespace utils
{

#ifdef ENABLE_LOCAL_UNWIND
void plotBacktrace()
{
    unw_context_t context;
    if (unw_getcontext(&context) != 0) return;
    unw_cursor_t cursor;
    if (unw_init_local(&cursor, &context) != 0) return;

    int idx{0};
    while (unw_step(&cursor))
    {
        unw_word_t ip, off;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        std::array<char, 128> mangled{};
        std::array<char, mangled.size()> demangled{};

        if (!unw_get_proc_name(&cursor, mangled.data(), mangled.size(), &off))
        {
            ++idx;
            bool valid = absl::debugging_internal::Demangle(
                mangled.data(), demangled.data(), demangled.size());

            printf(
                "#%-2d 0x%016" PRIxPTR " %s + 0x%" PRIxPTR "\n", idx,
                static_cast<uintptr_t>(ip),
                valid ? demangled.data() : mangled.data(),
                static_cast<uintptr_t>(off));
        }
    }
}
#endif // ENABLE_LOCAL_UNWIND

CSignalHandler::CSignalHandler(const std::initializer_list<int>& signals)
{
    if (sigemptyset(&m_sigSet) != 0)
        throw SignalHandlerException(
            ErrMsgBuild("[CSignalHandler] Failed sigemptyset: ", strerror(errno)));

    for (int signum : signals)
    {
        if (sigaddset(&m_sigSet, signum) != 0)
        {
            throw SignalHandlerException(
                "[CSignalHandler] sigaddset invalid signal " + std::to_string(signum));
        }
    }

    if (pthread_sigmask(SIG_BLOCK, &m_sigSet, nullptr) != 0)
    {
        throw SignalHandlerException(
            ErrMsgBuild("[CSignalHandler] pthread_sigmask: ", strerror(errno)));
    }
}

CSignalHandler::~CSignalHandler() noexcept
{
    pthread_sigmask(SIG_UNBLOCK, &m_sigSet, nullptr);
}

int CSignalHandler::Sigwait() const
{
    int signum{0};
    if (::sigwait(&m_sigSet, &signum) != 0)
    {
        throw SignalHandlerException(
            ErrMsgBuild("[CSignalHandler] sigwait: ", strerror(errno)));
    }
    return signum;
}

void CSignalHandler::EnableSigsegvHandler() const
{
    struct sigaction action {};
    action.sa_handler = CSignalHandler::signalHandler;

    if (sigaction(SIGSEGV, &action, nullptr) != 0)
    {
        throw SignalHandlerException(
            ErrMsgBuild("[CSignalHandler] sigaction: ", strerror(errno)));
    }
}

void CSignalHandler::signalHandler(int signum)
{
    switch (signum)
    {
        case SIGSEGV:
        {
            constexpr auto msg = to_array("Segmentation fault occurred\n");
            write(STDOUT_FILENO, msg.data(), msg.size());

#ifdef ENABLE_LOCAL_UNWIND
            plotBacktrace();
#endif
            signal(SIGSEGV, SIG_DFL);
            kill(getpid(), SIGSEGV);
            break;
        }
        default:
            break;
    }
}

} // namespace utils
