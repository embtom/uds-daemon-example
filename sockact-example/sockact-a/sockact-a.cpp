#include <sd_notify.h>
#include <sd_socket.h>
#include <signalhandler.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>
#include <systemd/sd-daemon.h> // für sd_booted()

int main(int argc, const char **argv)
{
    (void)argc;
    (void)argv;
    bool theEnd{false};

    std::shared_ptr<spdlog::logger> logger;

    if (sd_booted() > 0) {
        auto systemd_sink = std::make_shared<spdlog::sinks::systemd_sink_mt>("sockact-a");
        logger = std::make_shared<spdlog::logger>("", systemd_sink);
    } else {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        logger = std::make_shared<spdlog::logger>("", console_sink);
    }

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);

    utils::CSignalHandler signalHandler({SIGTERM, SIGINT, SIGHUP});


    try {
        signalHandler.EnableSigsegvHandler();
    } catch (const std::exception &e) {
        spdlog::error("Error occurred {}", e.what());
        theEnd = true;
    }

    if (sd_booted() > 0) {
        auto sockets = systemd_socket::getSystemdUnixSockets();
        if (sockets.empty()) {
            spdlog::warn("No systemd UNIX sockets found, falling back to manual bind()");
        } else {
            for (const auto &[fd, path] : sockets) {
                spdlog::info("Systemd provided socket: fd={} path={}", fd, path.string());
            }
        }
    }


    spdlog::info("App start");
    systemd_notify::ready();
    while (!theEnd) {
        const int receivedSignal = signalHandler.Sigwait();
        switch (receivedSignal) {
        case SIGTERM:
            [[fallthrough]];
        case SIGINT: {
            spdlog::info("Termination requested");
            systemd_notify::stopping();
            theEnd = true;
            break;
        }
        case SIGHUP: {
            systemd_notify::reloading();
            spdlog::info("Hook for reloading configuration");
            systemd_notify::ready();
            break;
        }
        default: {
            spdlog::warn("Unhandled signal received {}", receivedSignal);
        }
        }
    }

    spdlog::info("Exiting...");
    return EXIT_SUCCESS;
}
