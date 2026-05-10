// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <cadmia/modeling/decimal.hpp>
#include <cadmia/modeling/interval.hpp>

#include <cfenv>
#include <concepts>

namespace vdw14 {

    // tick_gen fires every 100 ms, outputs [1, 1].
    // For decimal<N>: period 0.100 s is exact; returns a point interval.
    // For float/double: 0.1 is not exactly representable in binary; the period
    // is computed as 1/10 with directed rounding to produce an enclosing interval.
    // Compile float/double variants with -frounding-math -fno-unsafe-math-optimizations.
    template <typename TIME> struct tick_gen {
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
            return output_i_t::closed(1, 1);
        }

        static time_i_t time_advance(const state_i_t &) {
            if constexpr (requires { TIME::from_scaled(100); }) {
                // Exact fixed-point decimal: period is representable without rounding.
                const auto p = TIME::from_scaled(100);
                return time_i_t::closed(p, p);
            } else {
                // Floating-point: compute 1/10 with directed rounding so the
                // resulting interval encloses the true period 0.1 s.
                const TIME one{1};
                const TIME ten{10};
                std::fesetround(FE_DOWNWARD);
                const TIME lo = one / ten;
                std::fesetround(FE_UPWARD);
                const TIME hi = one / ten;
                std::fesetround(FE_TONEAREST);
                return time_i_t::closed(lo, hi);
            }
        }
    };

} // namespace vdw14
