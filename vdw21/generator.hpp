// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <cadmia/modeling/interval.hpp>

namespace vdw21 {

    // IA-DEVS Generator from VWD21 Section 7.
    // State: elapsed time since last output (accumulates between events).
    // Period: [997, 1005] ms   Output: [1, 2]  (job-id mapping done by Z translation fn)
    // internal_transition: reset elapsed to [0, 0]
    // external_transition: state = state + elapsed (ignore input)
    // time_advance: period - state (never passive)
    template <typename TIME> struct generator {
        using time_t   = TIME;
        using state_t  = TIME;
        using input_t  = int;
        using output_t = int;

        using time_i_t   = cadmia::modeling::interval<time_t>;
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        static state_i_t internal_transition(const state_i_t &) {
            return state_i_t::closed(TIME{}, TIME{});
        }

        static state_i_t external_transition(const state_i_t &s, const time_i_t &e,
                                             const input_i_t &) {
            return s + e;
        }

        static output_i_t output(const state_i_t &) {
            return output_i_t::closed(1, 2);
        }

        static time_i_t time_advance(const state_i_t &s) {
            const auto period = time_i_t::closed(TIME::from_scaled(997), TIME::from_scaled(1005));
            return period - s;
        }
    };

} // namespace vdw21
