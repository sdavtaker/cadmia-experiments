// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <cadmia/modeling/decimal.hpp>
#include <cadmia/modeling/interval.hpp>

#include <compare>

namespace vdw14 {

    // k_counter: counts non-zero inputs; zero input triggers output and reset.
    // Encoding: input == [0,0] → reset signal; input with nonzero bounds → tick.
    // State (phase, count): idle = accumulating; ready = output pending.
    template <typename TIME> struct k_counter {
        using time_t   = TIME;
        using input_t  = int;
        using output_t = int;

        enum class phase_t : int { idle = 0, ready = 1 };

        struct state_t {
            phase_t phase{phase_t::idle};
            int count{0};

            constexpr state_t() noexcept = default;
            constexpr state_t(phase_t p, int c) noexcept : phase(p), count(c) {}

            [[nodiscard]] std::strong_ordering operator<=>(const state_t &o) const noexcept {
                if (auto cmp = phase <=> o.phase; cmp != 0)
                    return cmp;
                return count <=> o.count;
            }
            bool operator==(const state_t &) const noexcept = default;
        };

        using time_i_t   = cadmia::modeling::interval<time_t>;
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        static state_i_t internal_transition(const state_i_t &) {
            return state_i_t::closed(state_t{phase_t::idle, 0}, state_t{phase_t::idle, 0});
        }

        static state_i_t external_transition(const state_i_t &s, const time_i_t &,
                                             const input_i_t &x) {
            if (x.lower == 0 && x.upper == 0) {
                return state_i_t::closed(state_t{phase_t::ready, s.lower.count},
                                         state_t{phase_t::ready, s.upper.count});
            }
            return state_i_t::closed(state_t{phase_t::idle, s.lower.count + x.lower},
                                     state_t{phase_t::idle, s.upper.count + x.upper});
        }

        static output_i_t output(const state_i_t &s) {
            return output_i_t::closed(s.lower.count, s.upper.count);
        }

        static time_i_t time_advance(const state_i_t &s) {
            if (s.lower.phase == phase_t::idle && s.upper.phase == phase_t::idle)
                return time_i_t::empty_interval();
            if (s.lower.phase == phase_t::ready && s.upper.phase == phase_t::ready)
                return time_i_t::closed(time_t{}, time_t{});
            return time_i_t::right_open(time_t{}, cadmia::modeling::plus_inf);
        }
    };

} // namespace vdw14
