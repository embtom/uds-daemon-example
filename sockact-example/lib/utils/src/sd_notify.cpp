#include <sd_notify.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <systemd/sd-daemon.h>

namespace systemd_notify {

void ready()
{
    if (sd_booted() > 0) {
        int ret = sd_notify(0, "READY=1");
        if (ret < 0) {
            spdlog::error("Failed to send READY=1 to systemd: {}", strerror(ret));
        } else {
            spdlog::info("Sent READY=1 to systemd");
        }
    } else {
        spdlog::debug("System not booted with systemd, skipping READY notify");
    }
}

void stopping()
{
    if (sd_booted() > 0) {
        int ret = sd_notify(0, "STOPPING=1");
        if (ret < 0) {
            spdlog::error("Failed to send STOPPING=1 to systemd: {}", strerror(ret));
        } else {
            spdlog::info("Sent STOPPING=1 to systemd");
        }
    } else {
        spdlog::debug("System not booted with systemd, skipping STOPPING notify");
    }
}

void reloading(std::string_view msg)
{
    if (sd_booted() > 0) {
        int ret = sd_notifyf(0, "RELOADING=1\nSTATUS=%s", std::string(msg).c_str());
        if (ret < 0) {
            spdlog::error("Failed to send RELOADING notify: {}", strerror(ret));
        } else {
            spdlog::info("Sent RELOADING=1 with status: {}", msg);
        }
    } else {
        spdlog::debug("System not booted with systemd, skipping RELOADING notify");
    }
}

void status(std::string_view msg)
{
    if (sd_booted() > 0) {
        int ret = sd_notifyf(0, "STATUS=%s", std::string(msg).c_str());
        if (ret < 0) {
            spdlog::error("Failed to send STATUS notify: {}", strerror(ret));
        } else {
            spdlog::debug("Updated systemd status: {}", msg);
        }
    } else {
        spdlog::debug("System not booted with systemd, skipping STATUS notify");
    }
}

} // namespace systemd_notify
