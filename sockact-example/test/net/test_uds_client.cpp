#include <array>
#include <cstring>
#include <filesystem>
#include <fs_utils.h>
#include <gtest/gtest.h>
#include <socket.h>
#include <socket_session.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <uds_client.h>
#include <uds_server.h>
#include <string_utils.h>

namespace fs = std::filesystem;
using namespace net;

class UdsClientTest : public ::testing::Test {
  public:
    UdsClientTest()
     : server_(fs::temp_directory_path() /
               ("sockact-uds-test-" + fs_utils::random_suffix() + ".sock"))
    {
    }

    void SetUp() override
    {
        running_.store(true);

        server_thread_ = std::thread([this]() {
            while (running_.load()) {
                auto expected_session = server_.WaitForConnection();

                if (!expected_session.has_value()) {
                    // Graceful exit when Unblock() is called
                    if (expected_session.error() == std::errc::operation_canceled) {
                        spdlog::info("UdsServer unblocked, shutting down");
                        break;
                    }

                    spdlog::warn("UdsServer::WaitForConnection failed: {}",
                                 std::make_error_code(expected_session.error()).message());
                    continue; // try waiting again
                }

                auto session = std::move(expected_session.value());
                std::array<std::byte, 1024> buffer{};

                // Echo loop for this session
                while (running_.load()) {
                    auto ret = session.receive(std::span(buffer));
                    if (!ret.has_value() || ret.value() == 0)
                        break; // client disconnected or error

                    auto sent = session.send(std::span(buffer.data(), ret.value()));
                    if (!sent.has_value())
                        break; // send error → end session
                }
            }

            spdlog::info("UdsServer thread exiting");
        });
    }

    void TearDown() override
    {
        running_.store(false);
        server_.Unblock(); // unblock WaitForConnection()
        if (server_thread_.joinable())
            server_thread_.join();
    }

    fs::path socketPath() const noexcept { return server_.SocketPath(); }

  private:
    UdsServer server_;
    std::thread server_thread_;
    std::atomic_bool running_{false};
};

TEST_F(UdsClientTest, ConnectSendReceiveDisconnect)
{
    UdsClient client;
    client.connect(socketPath());
    std::string msg = "Hello";

    client.send(std::as_bytes(std::span(msg)));

    std::array<std::byte, 64> buffer{};
    auto ret = client.receive(std::span(buffer));

    ASSERT_TRUE(ret.has_value());
    std::string received(reinterpret_cast<char *>(buffer.data()), ret.value());
    EXPECT_EQ(received, msg);
}

TEST_F(UdsClientTest, ConnectFailsIfServerNotRunning)
{
    fs::path invalid = fs::temp_directory_path() / "nonexistent.sock";
    UdsClient client;
    auto err = client.connect(invalid);
    EXPECT_EQ(err, std::errc::no_such_file_or_directory);
}

TEST_F(UdsClientTest, ReconnectAfterDisconnect)
{
    UdsClient client;
    ASSERT_EQ(client.connect(socketPath()), std::errc{});

    client.disconnect();
    EXPECT_FALSE(client.isConnected());

    ASSERT_EQ(client.connect(socketPath()), std::errc{});
    EXPECT_TRUE(client.isConnected());
}

TEST_F(UdsClientTest, MultipleClientsConcurrent)
{
    constexpr int N = 5;
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this]() {
            UdsClient client;
            ASSERT_EQ(client.connect(socketPath()), std::errc{});
            std::string msg = "HelloServer";
            client.send(std::as_bytes(std::span(msg)));
            std::array<std::byte, 16> buf{};
            auto ret = client.receive(std::span(buf));
            ASSERT_TRUE(ret.has_value());
            EXPECT_EQ(utils::bytes_to_string(std::span(buf),ret.value()), msg);

            client.disconnect();
        });
    }

    for (auto &t : threads) t.join();
}
