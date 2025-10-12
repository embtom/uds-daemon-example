#include <cstring>
#include <final_action.h>
#include <fs_utils.h>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <socket_session.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <test_uds_server.h>
#include <thread>
#include <tuple>
#include <uds_server.h>
#include <unistd.h>

using namespace net;

class UdsServerTest : public ::testing::Test {
  public:
    UdsServerTest()
     : server_(fs::temp_directory_path() /
               ("sockact-uds-test-" + fs_utils::random_suffix() + ".sock"))
    {
    }
    explicit UdsServerTest(UdsServer &&server)
     : server_(std::move(server))
    {
    }

    net::UdsServer &server() { return server_; }
    const net::UdsServer &server() const { return server_; }

  private:
    UdsServer server_;
};

TEST(UdsServerBasicTest, ConstructAndSocketPath)
{
    auto socketPath =
        fs::temp_directory_path() / ("sockact-uds-test-" + fs_utils::random_suffix() + ".sock");
    {
        UdsServer server(socketPath);
        EXPECT_TRUE(fs::exists(server.SocketPath()));
    }
    EXPECT_FALSE(fs::exists(socketPath));
}

TEST(UdsServerBasicTest, MoveConstruct)
{
    auto socketPath =
        fs::temp_directory_path() / ("sockact-uds-test-" + fs_utils::random_suffix() + ".sock");
    UdsServer serverCreation(socketPath);
    EXPECT_TRUE(fs::exists(serverCreation.SocketPath()));

    UdsServer server_moved(std::move(serverCreation));
    EXPECT_TRUE(fs::exists(server_moved.SocketPath()));

    EXPECT_FALSE(serverCreation.ServerSocket().isValid());
    EXPECT_FALSE(fs::exists(serverCreation.SocketPath()));

    EXPECT_TRUE(server_moved.ServerSocket().isValid());
    EXPECT_TRUE(fs::exists(server_moved.SocketPath()));
}

TEST(UdsServerBasicTest, MoveAssign)
{
    auto initialSocketCreation =
        fs::temp_directory_path() / ("sockact-uds-test-1-" + fs_utils::random_suffix() + ".sock");
    auto initialSocketMoved =
        fs::temp_directory_path() / ("sockact-uds-test-2-" + fs_utils::random_suffix() + ".sock");

    UdsServer serverCreation(initialSocketCreation);
    UdsServer serverMoved(initialSocketMoved);

    EXPECT_TRUE(fs::exists(serverCreation.SocketPath()));
    EXPECT_TRUE(fs::exists(serverMoved.SocketPath()));
    int serverCreationFd = serverCreation.ServerSocket().getFd();

    // Move-assign
    serverMoved = std::move(serverCreation);

    // serverCreation should be invalid and its socket path removed
    EXPECT_FALSE(serverCreation.ServerSocket().isValid());
    EXPECT_FALSE(fs::exists(serverCreation.SocketPath()));
    EXPECT_FALSE(fs::exists(initialSocketMoved));

    // serverMoved should now own initialSocketCreation
    EXPECT_TRUE(serverMoved.ServerSocket().isValid());
    EXPECT_EQ(serverMoved.SocketPath(), initialSocketCreation);
    EXPECT_TRUE(fs::exists(serverMoved.SocketPath()));
    EXPECT_EQ(serverMoved.ServerSocket().getFd(), serverCreationFd);
}

TEST_F(UdsServerTest, WaitForConnection)
{
    auto uds_path = server().SocketPath();
    EXPECT_TRUE(fs::exists(uds_path));

    std::thread server_thread([&]() {
        auto sock = server().WaitForConnection();
        EXPECT_TRUE(sock.has_value());
        EXPECT_TRUE(sock.value().isValid());
        EXPECT_GE(sock.value().getFd(), 0);
    });

    {
        int client_fd{-1};
        auto finally = utils::Finally([&client_fd]() {
            if (client_fd >= 0)
                close(client_fd);
        });

        client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_GT(client_fd, 0);

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, uds_path.c_str(), sizeof(addr.sun_path) - 1);

        int ret = ::connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        ASSERT_EQ(ret, 0) << "Client failed to connect";
    }

    server_thread.join();
}

TEST_F(UdsServerTest, SocketSession_ReceivesData)
{
    auto uds_path = server().SocketPath();
    ASSERT_TRUE(fs::exists(uds_path));

    std::promise<TestData> promise;
    auto future = promise.get_future();
    TestData to_send("test", 1, 2, 3);
    auto serialized = to_send.serialize();

    std::thread server_thread([&]() {
        auto session = server().WaitForConnection();
        EXPECT_TRUE(session.has_value());
        EXPECT_TRUE(session.value().isValid());
        EXPECT_GE(session.value().getFd(), 0);

        spdlog::info("Server accepted socket fd {}", session.value().getFd());
        std::array<std::byte, sizeof(TestData)> buffer{};

        auto ret = session.value().receive(std::span(buffer));
        ASSERT_TRUE(ret.has_value())
            << "Receive failed: " << std::make_error_code(ret.error()).message();

        EXPECT_EQ(ret.value(), serialized.size());
        promise.set_value(TestData::deserialize(buffer));
    });

    // --- client side ---
    int client_fd{-1};
    auto cleanup = utils::Finally([&client_fd]() {
        if (client_fd >= 0) {
            close(client_fd);
        }
    });

    client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GT(client_fd, 0);

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, uds_path.c_str(), sizeof(addr.sun_path) - 1);

    ASSERT_EQ(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)), 0)
        << "Client failed to connect";

    ssize_t written = ::send(client_fd, serialized.data(), serialized.size(), 0);
    ASSERT_EQ(written, static_cast<ssize_t>(serialized.size()));

    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready)
        << "Server did not receive data in time";

    auto received = future.get();

    EXPECT_EQ(to_send, received);
    spdlog::info("Data: {}", received);
    server_thread.join();
}

TEST_F(UdsServerTest, SocketSession_NoData)
{
    auto uds_path = server().SocketPath();
    ASSERT_TRUE(fs::exists(uds_path));

    std::promise<void> server_ready;
    auto ready_future = server_ready.get_future();

    std::thread server_thread([&]() {
        server_ready.set_value();
        auto session = server().WaitForConnection();
        EXPECT_TRUE(session.has_value());
        EXPECT_TRUE(session.value().isValid());
        ASSERT_GE(session.value().getFd(), 0);

        spdlog::info("Server accepted socket fd {}", session.value().getFd());
        std::array<std::byte, sizeof(TestData)> buffer{};

        auto ret = session.value().receive(std::span(buffer));
        ASSERT_FALSE(ret.has_value());
        EXPECT_EQ(ret.error(), std::errc::connection_reset);
    });

    ready_future.wait();

    {
        int client_fd{-1};
        auto cleanup = utils::Finally([&client_fd]() {
            if (client_fd >= 0) {
                close(client_fd);
            }
        });

        client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_GT(client_fd, 0);

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, uds_path.c_str(), sizeof(addr.sun_path) - 1);

        ASSERT_EQ(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)), 0)
            << "Client failed to connect";
    }
    server_thread.join();
}

TEST_F(UdsServerTest, SocketSession_SendsData)
{
    auto uds_path = server().SocketPath();
    ASSERT_TRUE(fs::exists(uds_path));

    std::promise<void> server_ready;
    auto ready_future = server_ready.get_future();

    TestData to_send("hello", 10, 20, 30);
    auto serialized = to_send.serialize();

    // --- Server side ---
    std::thread server_thread([&]() {
        server_ready.set_value();
        auto session = server().WaitForConnection();
        EXPECT_TRUE(session.has_value());
        EXPECT_TRUE(session.value().isValid());
        ASSERT_GE(session.value().getFd(), 0);

        spdlog::info("Server accepted socket fd {}", session.value().getFd());

        auto result = session.value().send(std::span(serialized));
        ASSERT_TRUE(result.has_value())
            << "Send failed: " << std::make_error_code(result.error()).message();
        EXPECT_EQ(result.value(), serialized.size());
    });

    // --- Client side ---
    ready_future.wait();

    int client_fd{-1};
    auto cleanup = utils::Finally([&client_fd]() {
        if (client_fd >= 0) {
            close(client_fd);
        }
    });

    client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GT(client_fd, 0);

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, uds_path.c_str(), sizeof(addr.sun_path) - 1);

    ASSERT_EQ(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)), 0)
        << "Client failed to connect";

    // --- Receive from server ---
    std::array<std::byte, sizeof(TestData)> recv_buffer{};
    ssize_t received = ::recv(client_fd, recv_buffer.data(), recv_buffer.size(), 0);
    ASSERT_EQ(received, static_cast<ssize_t>(serialized.size()));

    auto received_data = TestData::deserialize(recv_buffer);
    EXPECT_EQ(to_send, received_data);

    server_thread.join();
}

TEST_F(UdsServerTest, SocketSession_PingPong)
{
    auto uds_path = server().SocketPath();
    ASSERT_TRUE(fs::exists(uds_path));

    const int iterations = 5;

    std::thread server_thread([&]() {
        auto session = server().WaitForConnection();
        EXPECT_TRUE(session.has_value());
        ASSERT_TRUE(session.value().isValid());
        ASSERT_GE(session.value().getFd(), 0);
        spdlog::info("Server accepted socket fd {}", session.value().getFd());

        for (int i = 0; i < iterations; ++i) {
            std::array<std::byte, sizeof(TestData)> buffer{};

            // Receive from client
            auto ret = session.value().receive(std::span(buffer));
            ASSERT_TRUE(ret.has_value())
                << "Server receive failed: " << std::make_error_code(ret.error()).message();
            EXPECT_GE(ret.value(), 0);

            auto received_data = TestData::deserialize(buffer);
            spdlog::info("Server received: {}", received_data);

            // Echo back
            auto serialized = received_data.serialize();
            ret = session.value().send(std::span(serialized));
            ASSERT_TRUE(ret.has_value())
                << "Server send failed: " << std::make_error_code(ret.error()).message();
            EXPECT_EQ(ret.value(), serialized.size());
        }
    });

    int client_fd{-1};
    auto cleanup = utils::Finally([&client_fd]() {
        if (client_fd >= 0) {
            close(client_fd);
        }
    });

    client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GT(client_fd, 0);

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, uds_path.c_str(), sizeof(addr.sun_path) - 1);

    ASSERT_EQ(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)), 0)
        << "Client failed to connect";

    for (int i = 0; i < iterations; ++i) {
        TestData to_send("ping", static_cast<uint32_t>(i * 3), static_cast<uint16_t>(i * 2),
                         static_cast<uint8_t>(i));
        auto serialized = to_send.serialize();

        // Send to server
        ssize_t sent = ::send(client_fd, serialized.data(), serialized.size(), 0);
        ASSERT_EQ(sent, static_cast<ssize_t>(serialized.size()));

        // Receive echo from server
        std::array<std::byte, sizeof(TestData)> buffer{};
        ssize_t received = ::recv(client_fd, buffer.data(), buffer.size(), 0);
        ASSERT_EQ(received, static_cast<ssize_t>(serialized.size()));

        auto echoed = TestData::deserialize(buffer);
        EXPECT_EQ(to_send, echoed);
        spdlog::info("Client received: {}", echoed);
    }

    server_thread.join();
}
