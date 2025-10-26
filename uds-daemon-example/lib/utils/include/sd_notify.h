#ifndef SD_NOTIFY_H
#define SD_NOTIFY_H

#include <string_view>

namespace systemd_notify {

void ready();
void stopping();
void reloading(std::string_view msg = "Reloading");
void status(std::string_view msg);

} // namespace systemd_notify

#endif /* SD_NOTIFY_H */
