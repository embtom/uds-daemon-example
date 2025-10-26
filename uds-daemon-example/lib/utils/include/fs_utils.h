#ifndef FS_UTILS_H
#define FS_UTILS_H

#include <random>
#include <sstream>
#include <string>


namespace fs_utils {

inline std::string random_suffix(std::size_t len = 6)
{
    static thread_local std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<int> dist(0, 15);

    std::ostringstream oss;
    for (std::size_t i = 0; i < len; ++i) {
        oss << std::hex << dist(rng);
    }
    return oss.str();
}

} // namespace fs_utils

#endif // FS_UTILS_H
