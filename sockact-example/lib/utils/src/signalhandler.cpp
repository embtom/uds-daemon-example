#include "signalhandler.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "errormsg.h"

namespace utils {

CSignalHandler::CSignalHandler(const std::initializer_list<int>& signals)
{
    if (sigemptyset(&m_sigSet) != 0)
        throw SignalHandlerException(ErrMsgBuild(
            "[CSignalHandler] Failed sigemptyset: ", std::string_view(strerror(errno))));

    for (int signum : signals) {
        if (sigaddset(&m_sigSet, signum) != 0) {
            throw SignalHandlerException(
                ErrMsgBuild("[CSignalHandler] sigaddset invalid signal ", std::to_string(signum)));
        }
    }

    if (pthread_sigmask(SIG_BLOCK, &m_sigSet, nullptr) != 0) {
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
    if (::sigwait(&m_sigSet, &signum) != 0) {
        throw SignalHandlerException(ErrMsgBuild("[CSignalHandler] sigwait: ", strerror(errno)));
    }
    return signum;
}

void CSignalHandler::EnableSigsegvHandler() const
{
    struct sigaction action{};
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = &CSignalHandler::signalHandler;

    if (sigaction(SIGSEGV, &action, nullptr) != 0) {
        throw SignalHandlerException(ErrMsgBuild("[CSignalHandler] sigaction: ", strerror(errno)));
    }
}

void CSignalHandler::signalHandler(int sig, siginfo_t* info, void* ctx)
{
    (void)ctx;
    switch (sig) {
    case SIGSEGV: {
        constexpr std::array msg = std::to_array("Segmentation fault occurred\n");
        ssize_t ret = write(STDERR_FILENO, msg.data(), msg.size());
        (void)ret;

        if (info) {
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "Faulting address: %p\n", info->si_addr);
            if (len > 0) {
                ret = write(STDERR_FILENO, buf, static_cast<size_t>(len));
                (void)ret;
            }
        }

        if (ctx) {
            ucontext_t* uctx = reinterpret_cast<ucontext_t*>(ctx);
#if defined(__x86_64__)
            int len;
            {
                void* rip = reinterpret_cast<void*>(uctx->uc_mcontext.gregs[REG_RIP]);
                char buf[128];
                len = snprintf(buf, sizeof(buf), "Instruction pointer: %p\n", rip);
                ret = write(STDERR_FILENO, buf, static_cast<size_t>(len));
                (void)ret;
            }
#elif defined(__aarch64__)
            int len;
            {
                void* pc = reinterpret_cast<void*>(uctx->uc_mcontext.pc);
                char buf[128];
                len = snprintf(buf, sizeof(buf), "Instruction pointer: %p\n", pc);
                ret = write(STDERR_FILENO, buf, static_cast<size_t>(len));
                (void)ret;
            }
#endif
        }
        signal(sig, SIG_DFL);
        kill(getpid(), sig);
    }
    default: break;
    }
}

} // namespace utils
