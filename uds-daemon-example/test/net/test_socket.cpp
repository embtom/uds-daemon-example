#include <gtest/gtest.h>
#include <socket.h>

using namespace net;

TEST(SocketTest, CreateInetStreamSocket)
{
    Socket s(ESocketMode::INET_STREAM);
    EXPECT_TRUE(s.isValid());
    EXPECT_GT(s.getFd(), 0);
}

TEST(SocketTest, CreateUnixStreamSocket)
{
    Socket s(ESocketMode::UNIX_STREAM);
    EXPECT_TRUE(s.isValid());
}

TEST(SocketTest, MoveConstructorTransfersOwnership)
{
    Socket s1(ESocketMode::INET_DGRAM);
    int fd = s1.getFd();

    Socket s2(std::move(s1));

    EXPECT_FALSE(s1.isValid()); // old one should be invalid
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(fd, s2.getFd());
}

TEST(SocketTest, MoveAssignmentTransfersOwnership)
{
    Socket s1(ESocketMode::INET_DGRAM);
    int fd1 = s1.getFd();

    Socket s2(ESocketMode::INET_STREAM);
    s2 = std::move(s1);

    EXPECT_FALSE(s1.isValid());
    EXPECT_TRUE(s2.isValid());
    EXPECT_EQ(fd1, s2.getFd());
}

TEST(SocketTest, SoReuseSocketSetsOption)
{
    Socket s(ESocketMode::INET_STREAM);
    EXPECT_NO_THROW(s.soReuseSocket());
}
