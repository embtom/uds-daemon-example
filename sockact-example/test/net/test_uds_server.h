#ifndef TESTDATA_HPP
#define TESTDATA_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <endian_convert.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <fmt/format.h>

struct TestData {
    TestData() noexcept = default;

    explicit TestData(std::string_view _name, uint32_t _data0, uint16_t _data1,
                      uint8_t _data2) noexcept
     : data0(_data0)
     , data1(_data1)
     , data2(_data2)
    {
        auto count = std::min(name.size() - 1, _name.size());
        std::copy_n(_name.begin(), count, name.begin());
        name[count] = '\0';
    }

    bool operator==(const TestData &rhs) const noexcept
    {
        return (data0 == rhs.data0) && (data1 == rhs.data1) && (data2 == rhs.data2) &&
               (std::string_view(name.data()) == std::string_view(rhs.name.data()));
    }

    std::vector<std::byte> serialize() const
    {
        std::vector<std::byte> buffer;
        buffer.reserve(name.size() + sizeof(data0) + sizeof(data1) + sizeof(data2));

        auto append_bytes = [&buffer]<typename T>(T data, std::size_t size) {
            const auto *ptr = reinterpret_cast<const std::byte *>(data);
            buffer.insert(buffer.end(), ptr, ptr + size);
        };

        auto push = [&buffer]<typename T>(T v) {
            T net = net::host_to_network(v);
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(net);
            buffer.insert(buffer.end(), bytes.begin(), bytes.end());
        };

        append_bytes(name.data(), name.size());
        push(data0);
        push(data1);
        push(data2);

        return buffer;
    }

    static TestData deserialize(std::span<const std::byte> buf)
    {
        TestData result;
        size_t offset = 0;

        std::memcpy(result.name.data(), buf.data(), result.name.size());
        offset += result.name.size();

        auto pull = [&buf, &offset]<typename T>(T &field) {
            if (offset + sizeof(T) > buf.size())
                throw std::runtime_error("Buffer too small while reading field");

            T net{};
            std::memcpy(&net, buf.data() + offset, sizeof(T));
            offset += sizeof(T);
            field = net::network_to_host(net);
        };

        pull(result.data0);
        pull(result.data1);
        pull(result.data2);

        return result;
    }

    std::array<char, 10> name{};
    uint32_t data0{0};
    uint16_t data1{0};
    uint8_t data2{0};
};

template <>
struct fmt::formatter<TestData> {
    // We don't need to parse any format specs (like {:x}), so leave it simple
    constexpr auto parse(const format_parse_context& ctx) const {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const TestData& data, FormatContext& ctx) const {
        std::string_view name_view(data.name.data(), strnlen(data.name.data(), data.name.size()));
        return fmt::format_to(ctx.out(),
                              "TestData{{ name='{}', data0={}, data1={}, data2={} }}",
                              name_view, data.data0, data.data1, data.data2);
    }
};
#endif /* TESTDATA_HPP */
