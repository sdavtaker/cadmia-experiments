// SPDX-License-Identifier: BSD-2-Clause
#pragma once
/**
 * IA-DEVS Load Generator for the CKW10 LBM experiment.
 *
 * STDEVS source: inter-departure time ~ Exp(lambda_s) = -(1/lambda_s) * ln(r).
 * Support of Exp(lambda_s) is (0, +inf) — strictly positive, open at zero.
 * The exponential distribution is memoryless, so the remaining time is always
 * (0, +inf) regardless of elapsed time.
 *
 * IA-DEVS spec (see ckw10-lbm-ia/spec.tex):
 *   state_t   = double   (elapsed time since last output)
 *   time_t    = double
 *   input_t   = task_signal       (no meaningful input)
 *   output_t  = task_signal       (task_signal::arrived)
 *
 *   TA(s)               = (0, +inf)
 *   Delta_int(s)        = [0, 0]
 *   Delta_ext(s, e, x)  = s + e   (accumulate elapsed)
 *   Lambda(s)           = [arrived, arrived]
 */

#include <cadmia/concepts/iadevs_atomic_model.hpp>
#include <cadmia/modeling/interval.hpp>

#include "signals.hpp"
#include <limits>

namespace ckw10_lbm_ia {

    class ia_load_generator {
      public:
        // ── Base type aliases (required by IADEVSAtomicModel) ─────────────────
        using state_t  = double;
        using time_t   = double;
        using input_t  = task_signal;
        using output_t = task_signal;

        // ── Interval aliases ──────────────────────────────────────────────────
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using time_i_t   = cadmia::modeling::interval<time_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        // ── IA-DEVS functions (static, interval-valued) ───────────────────────

        [[nodiscard]] static state_i_t internal_transition(const state_i_t &) noexcept {
            return state_i_t::closed(0.0, 0.0);
        }

        [[nodiscard]] static state_i_t external_transition(const state_i_t &s, const time_i_t &e,
                                                           const input_i_t &) noexcept {
            return s + e;
        }

        [[nodiscard]] static output_i_t output(const state_i_t &) noexcept {
            return output_i_t::closed(task_signal::arrived, task_signal::arrived);
        }

        [[nodiscard]] static time_i_t time_advance(const state_i_t &s) noexcept {
            if (s.is_empty())
                return time_i_t::empty_interval();
            return time_i_t::open(0.0, std::numeric_limits<time_t>::infinity());
        }
    };

    static_assert(cadmia::IADEVSAtomicModel<ia_load_generator>,
                  "ia_load_generator must satisfy IADEVSAtomicModel");

} // namespace ckw10_lbm_ia
