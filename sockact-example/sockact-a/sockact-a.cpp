#include <sd_notify.h>
#include <sd_socket.h>
#include <signalhandler.h>
#include <uds_server.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <systemd/sd-daemon.h> // for sd_booted()

int main(int argc, const char **argv)
{
    (void)argc;
    (void)argv;

    bool theEnd = false;

    //--------------------------------------------------------------------------
    // Logger setup
    //--------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> logger;

    try {
        if (sd_booted() > 0) {
            auto systemd_sink = std::make_shared<spdlog::sinks::systemd_sink_mt>("sockact-a");
            logger = std::make_shared<spdlog::logger>("sockact-a", systemd_sink);
        } else {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            logger = std::make_shared<spdlog::logger>("sockact-a", console_sink);
        }

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Failed to initialize logger: %s\n", e.what());
        return EXIT_FAILURE;
    }

    //--------------------------------------------------------------------------
    // Signal handler setup
    //--------------------------------------------------------------------------
    utils::CSignalHandler signalHandler({SIGTERM, SIGINT, SIGHUP});

    try {
        signalHandler.EnableSigsegvHandler();
    } catch (const std::exception &e) {
        spdlog::error("Failed to enable SIGSEGV handler: {}", e.what());
        return EXIT_FAILURE;
    }

    //--------------------------------------------------------------------------
    // Create and initialize the UDS server
    //--------------------------------------------------------------------------
    net::UdsServer udsserver;
    try {
        if (sd_booted() > 0) {
            auto sockets = systemd_socket::getSystemdUnixSockets();
            if (sockets.empty()) {
                spdlog::warn("No systemd UNIX sockets found, falling back to manual bind()");
                udsserver = net::UdsServer("/run/sockact-local-a.sock");
            } else {
                for (const auto &[fd, path] : sockets) {
                    spdlog::info("Systemd provided socket: fd={} path={}", fd, path.string());
                }
                udsserver = net::UdsServer(*sockets.begin());
            }
        } else {
            udsserver = net::UdsServer("/run/sockact-local-a.sock");
        }
    } catch (const std::exception &e) {
        spdlog::critical("Failed to initialize UdsServer: {}", e.what());
        return EXIT_FAILURE;
    }

    //--------------------------------------------------------------------------
    // Main loop
    //--------------------------------------------------------------------------
    try {

        spdlog::info("Service started — listening on {}", udsserver.SocketPath().string());
        systemd_notify::ready();

        while (!theEnd) {
            const int receivedSignal = signalHandler.Sigwait();
            switch (receivedSignal) {
            case SIGTERM:
            case SIGINT:
                spdlog::info("Termination requested");
                systemd_notify::stopping();
                theEnd = true;
                break;

            case SIGHUP:
                spdlog::info("Reload configuration requested");
                systemd_notify::reloading();
                // TODO: re-read config if applicable
                systemd_notify::ready();
                break;

            default:
                spdlog::warn("Unhandled signal received: {}", receivedSignal);
                break;
            }
        }

        spdlog::info("Shutting down...");
    } catch (const std::exception &e) {
        spdlog::critical("Unhandled exception in main loop: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::critical("Unknown exception occurred in main()");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
