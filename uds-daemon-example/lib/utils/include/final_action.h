#ifndef FINAL_ACTION_H
#define FINAL_ACTION_H

#include <concepts>
#include <utility>

namespace utils {

template <std::invocable F>
class FinalAction {
  public:
    explicit FinalAction(F&& f) noexcept : function_(std::forward<F>(f))
    {
    }
    FinalAction(FinalAction&& other) noexcept
     : function_(std::move(other.function_)), active_(std::exchange(other.active_, false))
    {
        other.active_ = false;
    }

    ~FinalAction() noexcept
    {
        if (active_)
            function_();
    }

    FinalAction(const FinalAction&) = delete;
    FinalAction& operator=(const FinalAction&) = delete;
    FinalAction& operator=(FinalAction&&) = delete;

    void release() noexcept
    {
        active_ = false;
    }

  private:
    F function_;
    bool active_{true};
};

template <typename F>
decltype(auto) Finally(F&& f) noexcept
{
    // Use std::decay to remove references, cv-qualifiers, and array/function types.
    // This ensures FinalAction always owns a safe, copyable/movable callable type.
    using Func = std::decay_t<F>;
    return FinalAction<Func>(std::forward<F>(f));
}

} // namespace utils

#endif // FINAL_ACTION_H
