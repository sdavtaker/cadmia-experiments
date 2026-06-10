// SPDX-License-Identifier: BSD-2-Clause
#pragma once
/**
 * IA-DEVS Load Generator for the CKW10 LBM experiment.
 *
 * STDEVS source: inter-departure time ~ Exp(lambda_s) = -(1/lambda_s) * ln(r).
 * UA-DEVS translation: reachable set = (0, +inf).
 * IA-DEVS bound: closed interval [sigma_L, sigma_U] from 90% quantile
 * truncation of Exp(lambda_s).
 *
 * IA-DEVS spec (see ckw10-lbm-ia/spec.tex):
 *   state_t   = double   (elapsed time since last output)
 *   time_t    = double
 *   input_t   = int      (no meaningful input)
 *   output_t  = int      (task signal = 1)
 *
 *   TA(sigma)           = [SIGMA_L, SIGMA_U] - sigma
 *   Delta_int(sigma)    = [0, 0]
 *   Delta_ext(s, e, x)  = s + e   (accumulate elapsed)
 *   Lambda(sigma)       = [1, 1]
 */

#include <cadmia/concepts/iadevs_atomic_model.hpp>
#include <cadmia/modeling/interval.hpp>

namespace ckw10_lbm_ia {

    class ia_load_generator {
      public:
        // ── Base type aliases (required by IADEVSAtomicModel) ─────────────────
        using state_t  = double;
        using time_t   = double;
        using input_t  = int;
        using output_t = int;

        // ── Interval aliases ──────────────────────────────────────────────────
        using state_i_t  = cadmia::modeling::interval<state_t>;
        using time_i_t   = cadmia::modeling::interval<time_t>;
        using input_i_t  = cadmia::modeling::interval<input_t>;
        using output_i_t = cadmia::modeling::interval<output_t>;

        // ── Inter-departure time interval (90% quantile of Exp(5)) ────────────
        // lambda_s = b_f * d_r = 0.5 * 10 = 5   (per server, TS1 b_f = 0.5)
        // sigma_L = q_{0.05}(5) = -ln(0.95)/5
        // sigma_U = q_{0.95}(5) = -ln(0.05)/5
        static constexpr double SIGMA_L = 0.010260; // ≈ -ln(0.95)/5
        static constexpr double SIGMA_U = 0.599146; // ≈ -ln(0.05)/5

        // ── IA-DEVS functions (static, interval-valued) ───────────────────────

        [[nodiscard]] static state_i_t internal_transition(const state_i_t &) noexcept {
            return state_i_t::closed(0.0, 0.0);
        }

        [[nodiscard]] static state_i_t external_transition(const state_i_t &s, const time_i_t &e,
                                                           const input_i_t &) noexcept {
            return s + e;
        }

        [[nodiscard]] static output_i_t output(const state_i_t &) noexcept {
            return output_i_t::closed(1, 1);
        }

        [[nodiscard]] static time_i_t time_advance(const state_i_t &s) noexcept {
            if (s.is_empty())
                return time_i_t::empty_interval();
            const auto period = time_i_t::closed(SIGMA_L, SIGMA_U);
            const auto raw    = period - s;
            // Clamp lower bound to 0 (cannot be negative)
            if (raw.is_empty())
                return raw;
            auto result         = raw;
            result.lower_closed = true;
            if (result.lower < 0.0)
                result.lower = 0.0;
            return result;
        }
    };

    static_assert(cadmia::IADEVSAtomicModel<ia_load_generator>,
                  "ia_load_generator must satisfy IADEVSAtomicModel");

} // namespace ckw10_lbm_ia
