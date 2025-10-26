#ifndef BYTE_UTIL_H
#define BYTE_UTIL_H

#include <array>
#include <concepts>
#include <cstddef> // std::byte
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <algorithm>

namespace utils {

template <typename T>
concept ByteLike = std::same_as<std::remove_cvref_t<T>, std::byte> ||
                   std::same_as<std::remove_cvref_t<T>, unsigned char> ||
                   std::same_as<std::remove_cvref_t<T>, uint8_t>;

template <typename T>
concept ByteContainer = std::ranges::contiguous_range<T> && ByteLike<std::ranges::range_value_t<T>>;

[[nodiscard]] constexpr std::string_view from_bytes(const ByteContainer auto &bytes)
{
    return std::string_view(reinterpret_cast<const char *>(std::ranges::data(bytes)),
                            std::ranges::size(bytes));
}

template <std::size_t N>
[[nodiscard]] constexpr auto to_bytes(const char (&str)[N]) noexcept -> std::array<std::byte, N>
{
    std::array<std::byte, N> bytes{};
    for (std::size_t i = 0; i < N; ++i)
        bytes[i] = static_cast<std::byte>(str[i]);
    return bytes;
}

[[nodiscard]] inline auto to_bytes(std::string_view str) noexcept -> std::vector<std::byte>
{
    std::vector<std::byte> bytes(str.size());
    std::ranges::transform(str.cbegin(), str.cend(), bytes.begin(),
                   [](const char c) { return static_cast<std::byte>(c); });
    return bytes;
}

} // namespace utils

#endif // BYTE_UTIL_H
