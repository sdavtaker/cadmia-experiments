// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <cadmia/modeling/interval.hpp>

#include <concepts>

namespace vdw14 {

    // reset_gen fires every 1000 ms = 1 s, outputs [0, 0].
    // For decimal<N>: period 1.000 s is exact; returns a point interval.
    // For float/double: 1.0 is exactly representable in binary; returns a point interval.
    // Output [0, 0] is the reset signal recognized by k_counter.
    template <typename TIME> struct reset_gen {
        using time_t   = TIME;
        using state_t  = int;
        using input_t  = int;
        using output_t = int;

        using time_i_t   = cadmia::modeling::interval<time_t>;
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        static state_i_t internal_transition(const state_i_t &s) {
            return s;
        }

        static state_i_t external_transition(const state_i_t &s, const time_i_t &,
                                             const input_i_t &) {
            return s;
        }

        static output_i_t output(const state_i_t &) {
            return output_i_t::closed(0, 0);
        }

        static time_i_t time_advance(const state_i_t &) {
            if constexpr (requires { TIME::from_scaled(1000); }) {
                // decimal<Scale>: 1000 ms = 1 s is exact.
                const auto p = TIME::from_scaled(1000);
                return time_i_t::closed(p, p);
            } else if constexpr (!std::floating_point<TIME> &&
                                 !requires { TIME{std::int32_t{1}, std::int32_t{10}}; }) {
                // rsfp / mbfp: magnitude 10 = 10 × 100 ms = 1 s.
                const auto p = TIME{std::int32_t{10}};
                return time_i_t::closed(p, p);
            } else {
                // float / double: 1.0 is exact. rational: TIME{1} = 1/1 = 1 s.
                const TIME p{1};
                return time_i_t::closed(p, p);
            }
        }
    };

} // namespace vdw14
