#include <byte_util.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <uds_client.h>
#include <vector>

namespace fs = std::filesystem;

struct CliArgs {
    fs::path socket_path;
    spdlog::level::level_enum log_level;
    std::string message;
};

static CliArgs parse_arguments(int argc, char *argv[])
{
    cxxopts::Options options("sockact-cli",
                             "Simple UDS client for communicating with sockact service");

    options.add_options()(
        "s,socket", "Path to UNIX domain socket",
        cxxopts::value<std::string>()->default_value("/run/sockact-a.sock"))(
        "l,log-level", "Log level (trace, debug, info, warn, error, critical, off)",
        cxxopts::value<std::string>()->default_value("info"))(
        "m,message", "Message to send to the server",
        cxxopts::value<std::string>()->default_value("ping"))("h,help", "Print usage");

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

    const std::string level_str = result["log-level"].as<std::string>();
    spdlog::level::level_enum log_level;

    try {
        log_level = spdlog::level::from_str(level_str);
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Warning: Invalid log level '" << level_str << "' (" << ex.what()
                  << "), falling back to 'info'\n";
        log_level = spdlog::level::info;
    }

    CliArgs args;
    args.socket_path = result["socket"].as<std::string>();
    args.log_level = log_level;
    args.message = result["message"].as<std::string>();
    return args;
}

int main(int argc, char *argv[])
{
    auto args = parse_arguments(argc, argv);

    try {

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("sockact-client", console_sink);
        spdlog::set_default_logger(logger);

        spdlog::set_level(args.log_level);
        spdlog::info("Starting client with log level '{}'",
                     spdlog::level::to_string_view(spdlog::get_level()));

        // ---------------------------------------------------------------------
        // Connect to UDS server
        // ---------------------------------------------------------------------
        spdlog::info("Connecting to socket {}", args.socket_path.string());

        net::UdsClient client;
        if (auto connect_result = client.connect(args.socket_path); connect_result != std::errc{}) {
            spdlog::error("Failed to connect to {}: {}", args.socket_path.string(),
                          std::make_error_code(connect_result).message());
            return EXIT_FAILURE;
        }

        spdlog::info("Connected — sending message: '{}'", args.message);

        // Convert string message to bytes
        std::vector<std::byte> msg_bytes = utils::to_bytes(args.message);
        client.send(std::span(msg_bytes));

        std::array<std::byte, 1024> buffer{};
        if (auto response = client.receive(std::span(buffer)); response.has_value()) {
            std::string_view resp_str = utils::from_bytes(std::span(buffer.data(), response.value()));
            spdlog::info("Received response: '{}'", resp_str);
        } else {
            spdlog::warn("No response or connection closed");
        }
        client.disconnect();
        spdlog::info("Client exiting normally");
        return EXIT_SUCCESS;

    } catch (const std::exception &e) {
        spdlog::critical("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::critical("Unknown fatal error");
        return EXIT_FAILURE;
    }
}
