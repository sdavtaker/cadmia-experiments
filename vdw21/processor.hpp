// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <cadmia/modeling/interval.hpp>

#include <compare>
#include <vector>

namespace vdw21 {

    // Processor state: TOCJ (time on current job) + FIFO job-id queue.
    // Empty queue means the processor is passive (time_advance returns empty_interval).
    template <typename TIME> struct proc_state {
        TIME tocj{};
        std::vector<int> qj{};

        [[nodiscard]] bool empty() const noexcept {
            return qj.empty();
        }

        [[nodiscard]] std::strong_ordering operator<=>(const proc_state &o) const noexcept {
            if (auto c = tocj <=> o.tocj; c != 0)
                return c;
            if (qj.size() != o.qj.size())
                return qj.size() < o.qj.size() ? std::strong_ordering::less
                                               : std::strong_ordering::greater;
            for (size_t i = 0; i < qj.size(); ++i) {
                if (auto c = qj[i] <=> o.qj[i]; c != 0)
                    return c;
            }
            return std::strong_ordering::equal;
        }

        bool operator==(const proc_state &) const = default;
    };

    // IA-DEVS Processor from VWD21 Section 7.
    // Processing time: [250, 250] ms  (point interval — deterministic service time)
    // FIFO queue of integer job-ids.  Passive when queue is empty.
    // internal_transition: pop front job, reset tocj to 0; passive if queue becomes empty.
    // external_transition: if passive → new job with tocj=0; if active → append job, tocj += e.
    // output: interval of front job-ids [lower.qj[0], upper.qj[0]].
    // time_advance: empty if passive; closed(proc_time - tocj_hi, proc_time - tocj_lo) otherwise.
    template <typename TIME> struct processor {
        using time_t   = TIME;
        using state_t  = proc_state<TIME>;
        using input_t  = int;
        using output_t = int;

        using time_i_t   = cadmia::modeling::interval<time_t>;
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        static TIME proc_time() noexcept {
            return TIME::from_scaled(250);
        }

        static state_i_t internal_transition(const state_i_t &s) {
            auto lo = s.lower;
            auto hi = s.upper;
            lo.qj.erase(lo.qj.begin());
            hi.qj.erase(hi.qj.begin());
            lo.tocj = TIME{};
            hi.tocj = TIME{};
            if (lo.empty())
                return state_i_t::empty_interval(); // passive
            return state_i_t::closed(lo, hi);
        }

        static state_i_t external_transition(const state_i_t &s, const time_i_t &e,
                                             const input_i_t &x) {
            if (s.is_empty()) {
                // Passive: start new job; tocj stays 0 (do not accumulate elapsed)
                return state_i_t::closed(state_t{TIME{}, {x.lower}}, state_t{TIME{}, {x.upper}});
            }
            // Active: advance tocj by elapsed, enqueue new job
            auto lo = s.lower;
            auto hi = s.upper;
            lo.tocj = lo.tocj + e.lower;
            hi.tocj = hi.tocj + e.upper;
            lo.qj.push_back(x.lower);
            hi.qj.push_back(x.upper);
            return state_i_t::closed(lo, hi);
        }

        static output_i_t output(const state_i_t &s) {
            return output_i_t::closed(s.lower.qj.front(), s.upper.qj.front());
        }

        static time_i_t time_advance(const state_i_t &s) {
            if (s.is_empty())
                return time_i_t::empty_interval();
            const TIME pt = proc_time();
            return time_i_t::closed(pt - s.upper.tocj, pt - s.lower.tocj);
        }
    };

} // namespace vdw21
