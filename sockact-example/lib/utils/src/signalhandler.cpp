#include "signalhandler.h"
#include <cstdio>
#include <cstring>

namespace utils {

SignalHandler::SignalHandler(std::initializer_list<int> signals)
{
    if (sigemptyset(&sigSet) != 0)
        throw SignalError("sigemptyset failed");

    for (int s : signals) {
        if (sigaddset(&sigSet, s) != 0)
            throw SignalError("sigaddset failed");
    }

    // Block signals on all current threads
    if (pthread_sigmask(SIG_BLOCK, &sigSet, nullptr) != 0)
        throw SignalError("pthread_sigmask failed");
}

SignalHandler::~SignalHandler() { pthread_sigmask(SIG_UNBLOCK, &sigSet, nullptr); }

int SignalHandler::wait() const
{
    int sig{};
    if (sigwait(&sigSet, &sig) != 0)
        throw SignalError("sigwait failed");
    return sig;
}

void SignalHandler::enableSegfaultHandler()
{
    struct sigaction sa{};
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = &SignalHandler::segFaultHandler;

    sigaction(SIGSEGV, &sa, nullptr);
}

void SignalHandler::segFaultHandler(int sig, siginfo_t *info, void *)
{
    constexpr std::string_view msg = "Segmentation fault!\n";
    ssize_t ret = write(STDERR_FILENO, msg.data(), msg.size());
    (void)ret;

    if (info) {
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "Fault at: %p\n", info->si_addr);
        ret = write(STDERR_FILENO, buf, static_cast<size_t>(len));
    }

    // Restore default and crash properly for core dump/debugger
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

} // namespace utils
