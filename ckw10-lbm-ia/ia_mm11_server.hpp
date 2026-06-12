// SPDX-License-Identifier: BSD-2-Clause
#pragma once
/**
 * IA-DEVS M/M/1/1 Server for the CKW10 LBM experiment.
 *
 * STDEVS source: service time ~ Exp(mu) = -(1/mu) * ln(r); arriving tasks
 * are dropped when the server is busy (M/M/1/1 = no queue, finite capacity 1).
 * Support of Exp(mu) is (0, +inf) — strictly positive, open at zero.
 *
 * IA-DEVS spec (see ckw10-lbm-ia/spec.tex):
 *   state_t = double   (remaining service time; empty interval = idle)
 *   time_t  = double
 *   input_t = int      (arriving task signal)
 *   output_t = int     (completed task signal)
 *
 *   TA(empty)              = empty          (passive = idle)
 *   TA(s_tilde)            = s_tilde        (remaining service time)
 *   Delta_int(s)           = empty          (service completes; go idle)
 *   Delta_ext(empty, e, x) = (0, +inf)      (accept task; start service)
 *   Delta_ext(s, e, x)     = max(0, s - e)  (drop task; update remaining)
 *   Lambda(s)              = [1, 1]         (completed task signal)
 *
 * A loss event occurs when Delta_ext is called on a non-empty (busy) state.
 * The caller is responsible for detecting and counting loss events.
 */

#include <cadmia/concepts/iadevs_atomic_model.hpp>
#include <cadmia/modeling/interval.hpp>

#include <limits>

namespace ckw10_lbm_ia {

    class ia_mm11_server {
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

        // ── IA-DEVS functions (static, interval-valued) ───────────────────────

        [[nodiscard]] static state_i_t internal_transition(const state_i_t &) noexcept {
            return state_i_t::empty_interval(); // service complete; go idle
        }

        [[nodiscard]] static state_i_t external_transition(const state_i_t &s, const time_i_t &e,
                                                           const input_i_t &) noexcept {
            if (s.is_empty()) {
                // Idle: accept task; service time ~ Exp(mu), full support (0, +inf)
                return state_i_t::open(0.0, std::numeric_limits<state_t>::infinity());
            }
            // Busy: drop arriving task; update remaining service time
            return clamp_non_negative(s - e);
        }

        [[nodiscard]] static output_i_t output(const state_i_t &) noexcept {
            return output_i_t::closed(1, 1);
        }

        [[nodiscard]] static time_i_t time_advance(const state_i_t &s) noexcept {
            if (s.is_empty())
                return time_i_t::empty_interval(); // passive
            return s;                              // remaining service time
        }

      private:
        [[nodiscard]] static state_i_t clamp_non_negative(const state_i_t &in) noexcept {
            if (in.is_empty())
                return in;
            auto out = in;
            if (out.lower < 0.0) {
                out.lower        = 0.0;
                out.lower_closed = true;
            }
            if (out.upper < 0.0) {
                out.upper        = 0.0;
                out.upper_closed = true;
            }
            return out;
        }
    };

    static_assert(cadmia::IADEVSAtomicModel<ia_mm11_server>,
                  "ia_mm11_server must satisfy IADEVSAtomicModel");

} // namespace ckw10_lbm_ia
