#ifndef CHESSLIB_RANDOM_ROMU_HPP
#define CHESSLIB_RANDOM_ROMU_HPP

#include <cstdint>
#include <limits>

namespace chesslib {
namespace detail {
    static inline constexpr auto rotl(uint64_t x, unsigned k) noexcept -> uint64_t
    {
        return (x << k) | (x >> (64U - k));
    }

    static inline constexpr auto split_mix64(uint64_t& state) noexcept -> uint64_t
    {
        uint64_t z = (state += UINT64_C(0x9e3779b97f4a7c15));
        z = (z ^ (z >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27U)) * UINT64_C(0x94d049bb133111eb);
        return z ^ (z >> 31U);
    }
}  // namespace detail

    class romu_trio final {
    public:
        using result_type = uint64_t;

        static inline constexpr uint64_t(min)() { return result_type { 0 }; }
        static inline constexpr uint64_t(max)() { return std::numeric_limits<result_type>::max(); }

        explicit romu_trio(uint64_t seed) noexcept
            : state_{ detail::split_mix64(seed), detail::split_mix64(seed), detail::split_mix64(seed) }
        {
            for (auto i = 0; i < 10; ++i) {
                operator()();
            }
        }

        // disallow copying (to prevent misuse)
        romu_trio(romu_trio const&) = delete;
        auto operator=(romu_trio const&) -> romu_trio& = delete;

        // allow moving
        romu_trio(romu_trio&&) noexcept = default;
        auto operator=(romu_trio&&) noexcept -> romu_trio& = default;

        ~romu_trio() noexcept = default;

        inline auto operator()() noexcept -> uint64_t
        {
            uint64_t xp = state_.x;
            uint64_t yp = state_.y;
            uint64_t zp = state_.z;
            state_.x = 15241094284759029579U * zp;
            state_.y = yp - xp;
            state_.y = detail::rotl(state_.y, 12U);
            state_.z = zp - yp;
            state_.z = detail::rotl(state_.z, 44U);
            return xp;
        }


    private:
        struct state {
            uint64_t x;
            uint64_t y;
            uint64_t z;
        } state_;
    };
} // namespace chesslib
#endif