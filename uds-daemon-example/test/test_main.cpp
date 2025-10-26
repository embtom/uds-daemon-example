#include <chrono>
#include <gtest/gtest.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>
#include <memory>

class ElapsedTimeFlag : public spdlog::custom_flag_formatter {
public:
    explicit ElapsedTimeFlag(std::chrono::steady_clock::time_point start)
        : start_time_(start) {}

    void format(const spdlog::details::log_msg &,
                const std::tm &, spdlog::memory_buf_t &dest) override {
        using namespace std::chrono;
        const auto elapsed_ms =
            duration_cast<milliseconds>(steady_clock::now() - start_time_).count();
        fmt::format_to(std::back_inserter(dest), "+{:6.3f}s", static_cast<double>(elapsed_ms) / 1000.0);
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<ElapsedTimeFlag>(start_time_);
    }

private:
    std::chrono::steady_clock::time_point start_time_;
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    static const auto start_time = std::chrono::steady_clock::now();

    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<ElapsedTimeFlag>('E', start_time);
    formatter->set_pattern("  [%E][T%t][%^%L%$] %v");
    spdlog::set_formatter(std::move(formatter));

    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::info);
    spdlog::flush_on(spdlog::level::debug);

    return RUN_ALL_TESTS();
}
