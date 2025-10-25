#include "byte_util.h"

#include <array>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

using namespace utils;

TEST(ByteUtilTest, ToBytesFromStringLiteral)
{
    constexpr auto bytes = to_bytes("Hi");
    static_assert(bytes.size() == 3);
    EXPECT_EQ(bytes[0], std::byte{'H'});
    EXPECT_EQ(bytes[1], std::byte{'i'});
    EXPECT_EQ(bytes[2], std::byte{'\0'});
}

TEST(ByteUtilTest, ToBytesFromStringView)
{
    const std::string_view str = "Hello";
    auto bytes = to_bytes(str);
    ASSERT_EQ(bytes.size(), str.size());
    for (std::size_t i = 0; i < str.size(); ++i)
        EXPECT_EQ(bytes[i], static_cast<std::byte>(str.at(i)));
}

TEST(ByteUtilTest, FromBytesToStringView)
{
    const auto bytes = to_bytes("World");
    const auto str_view = from_bytes(bytes);

    EXPECT_STREQ(str_view.data(), "World");
}

TEST(ByteUtilTest, FromBytesRoundTrip)
{
    const std::string_view original = "RoundTrip";
    auto bytes = to_bytes(original);
    auto reconstructed = from_bytes(bytes);
    EXPECT_EQ(reconstructed, original);
}

TEST(ByteUtilTest, WorksWithVectorUnsignedChar)
{
    std::vector<unsigned char> buf = {'A', 'B', 'C'};
    auto view = from_bytes(buf);
    EXPECT_EQ(view, "ABC");
}

TEST(ByteUtilTest, WorksWithArrayOfUInt8)
{
    std::array<std::uint8_t, 3> buf = {65, 66, 67}; // 'A', 'B', 'C'
    auto view = from_bytes(buf);
    EXPECT_EQ(view, "ABC");
}
