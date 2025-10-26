#ifndef ERRORMSG_H_
#define ERRORMSG_H_

#include <format>
#include <string>

namespace utils
{

// Single shot: provide a format string + args
template <typename... Args>
[[nodiscard]] inline std::string ErrMsgBuild(std::string_view fmt, Args&&... args)
{
    return std::vformat(fmt, std::make_format_args(std::forward<const Args>(args)...));
}

} // namespace utils

#endif // ERRORMSG_H_
