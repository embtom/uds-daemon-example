#include <errno.h>
#include <fdset.h>
#include <future>
#include <gtest/gtest.h>
#include <pipe.h>

using namespace utils;



//*****************************
// Test construction and destruction
TEST(FdSetTest, ConstructDestruct)
{
    FdSet set;
}

//*****************************
// Test adding and removing fds
TEST(FdSetTest, AddRemoveFd)
{
    FdSet set;
    Pipe p;

    set.AddFd(p.readFd());
    EXPECT_EQ(set.RemoveFd(p.readFd()), true);
    EXPECT_EQ(set.RemoveFd(p.readFd()), false);
}

//*****************************

TEST(FdSetTest, SelectPipeReadable)
{
    FdSet set;
    Pipe p;
    std::string testStr("Test_Data");

    bool callbackCalled = false;
    set.AddFd(p.readFd(), [&p, &callbackCalled, &testStr](int fd) {
        callbackCalled = true;
        EXPECT_EQ(fd, p.readFd());
        auto res = p.readString();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(*res, testStr);
    });

    p.writeString(std::string_view(testStr));
    FdSetRet ret = set.Select(std::chrono::milliseconds(1000));
    EXPECT_EQ(ret, FdSetRet::OK);
    EXPECT_TRUE(callbackCalled);
}

//*****************************
// Test timeout
TEST(FdSetTest, SelectTimeout)
{
    FdSet set;
    FdSetRet ret = set.Select(std::chrono::milliseconds(100));
    EXPECT_EQ(ret, FdSetRet::TIMEOUT);
}

//*****************************
// Test unblock
TEST(FdSetTest, Unblock)
{
    FdSet set;

    std::promise<FdSetRet> promise;
    auto future = promise.get_future();

    std::thread t([&promise, &set] {
        FdSetRet ret = set.Select();
        promise.set_value(ret);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500LL));
    set.UnBlock();

    // Wait with timeout
    auto status = future.wait_for(std::chrono::seconds(1LL));
    ASSERT_EQ(status, std::future_status::ready) << "Select did not return in time";

    EXPECT_EQ(future.get(), FdSetRet::UNBLOCK);

    t.join();
}
