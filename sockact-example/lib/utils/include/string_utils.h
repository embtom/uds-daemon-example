#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <cstddef>
#include <span>
#include <string>

namespace utils {

inline std::string bytes_to_string(std::span<const std::byte> data, size_t count)
{
    if (count > data.size())
        count = data.size();
    return std::string(reinterpret_cast<const char *>(data.data()), count);
}

}; // namespace utils

#endif
