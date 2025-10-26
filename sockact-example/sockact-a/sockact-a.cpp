#include <csignal>
#include <cstdlib>
#include <cxxopts.hpp>
#include <iostream>
#include <memory>
#include <sd_notify.h>
#include <sd_socket.h>
#include <server_worker.h>
#include <signalhandler.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <systemd/sd-daemon.h> // for sd_booted()
#include <uds_server.h>

struct CliArgs {
    spdlog::level::level_enum log_level;
    bool interactive;
};

static CliArgs parse_arguments(int argc, const char *argv[])
{
    cxxopts::Options options("sockact-a", "Unix domain socket service");
    auto opts = options.add_options();
    opts("l,log-level", "Log verbosity: trace | debug | info | warn | error | critical | off",
         cxxopts::value<std::string>()->default_value("info"));

    opts("i,interactive", "Force interactive mode (disable systemd/daemon mode)");
    opts("h,help", "Show help message");

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            std::exit(EXIT_SUCCESS);
        }

    } catch (const cxxopts::exceptions::exception &ex) {
        std::cerr << "Error parsing options: " << ex.what() << "\n\n"
                  << options.help() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const bool interactive = result.count("interactive") > 0;
    const std::string level_str = result["log-level"].as<std::string>();
    spdlog::level::level_enum log_level;

    try {
        log_level = spdlog::level::from_str(level_str);
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Warning: Invalid log level '" << level_str << "' (" << ex.what()
                  << "), falling back to 'info'\n";
        log_level = spdlog::level::info;
    }

    CliArgs args{
        .log_level = log_level,
        .interactive = interactive,
    };
    return args;
}

int main(int argc, const char *argv[])
{
    auto args = parse_arguments(argc, argv);

    bool theEnd = false;

    //--------------------------------------------------------------------------
    // Logger setup
    //--------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> logger;

    try {
        if ((sd_booted() > 0) && (!args.interactive)) {
            auto systemd_sink = std::make_shared<spdlog::sinks::systemd_sink_mt>("sockact-a");
            logger = std::make_shared<spdlog::logger>("sockact-a", systemd_sink);
        } else {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            logger = std::make_shared<spdlog::logger>("sockact-a", console_sink);
        }

        spdlog::set_default_logger(logger);
        spdlog::set_level(args.log_level);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Failed to initialize logger: %s\n", e.what());
        return EXIT_FAILURE;
    }

    //--------------------------------------------------------------------------
    // Signal handler setup
    //--------------------------------------------------------------------------
    utils::SignalHandler signalHandler(args.interactive
                                           ? std::initializer_list<int>{SIGTERM, SIGINT, SIGHUP}
                                           : std::initializer_list<int>{SIGTERM, SIGHUP});

    try {
        signalHandler.enableSegfaultHandler();
    } catch (const std::exception &e) {
        spdlog::error("Failed to enable SIGSEGV handler: {}", e.what());
        return EXIT_FAILURE;
    }

    //--------------------------------------------------------------------------
    // Create and initialize the UDS server
    //--------------------------------------------------------------------------
    net::UdsServer udsserver;
    try {
        if ((sd_booted() > 0) && (!args.interactive)) {
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
        net::UdsServerWorker udsServerWorker(std::move(udsserver));
        systemd_notify::ready();

        while (!theEnd) {
            const int receivedSignal = signalHandler.wait();
            switch (receivedSignal) {
            case SIGTERM:
            case SIGINT:
                spdlog::info("Termination requested {}", receivedSignal);
                systemd_notify::stopping();
                theEnd = true;
                break;

            case SIGHUP:
                spdlog::info("Reload configuration requested");
                systemd_notify::reloading();
                // Config could be reloaded
                systemd_notify::ready();
                break;

            default:
                spdlog::warn("Unhandled signal received: {}", receivedSignal);
                break;
            }
        }

        spdlog::info("Shutting down...");
        udsServerWorker.Stop();

    } catch (const std::exception &e) {
        spdlog::critical("Unhandled exception in main loop: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::critical("Unknown exception occurred in main()");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
