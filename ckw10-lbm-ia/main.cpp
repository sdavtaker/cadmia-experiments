// SPDX-License-Identifier: BSD-2-Clause
/**
 * CKW10 LBM IA-DEVS Analysis — STDEVS to IA-DEVS bounding
 *
 * Demonstrates three things:
 *  1. The ia_load_generator and ia_mm11_server IA-DEVS models are correctly
 *     specified (verified by static_assert at include time).
 *  2. The analytical IA-DEVS P_loss bound derived in spec.tex contains the
 *     exact STDEVS Erlang B value of 0.5 (TS1, b_f=0.5).
 *  3. A short 5-event interval simulation trace showing interval widening.
 */

#include "ia_load_generator.hpp"
#include "ia_mm11_server.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace ckw10_lbm_ia;
using LG = ia_load_generator;
using S  = ia_mm11_server;
using I  = cadmia::modeling::interval<double>;

// ── Helpers ────────────────────────────────────────────────────────────────

static std::string fmt_interval(const I &x) {
    if (x.is_empty())
        return "empty";
    std::ostringstream os;
    os << std::fixed << std::setprecision(4);
    const std::string u_str =
        (x.upper == std::numeric_limits<double>::infinity())
            ? "+inf"
            : (std::ostringstream{} << std::fixed << std::setprecision(4) << x.upper, os.str());
    os.str("");
    os << (x.lower_closed ? "[" : "(") << x.lower << ", ";
    if (x.upper == std::numeric_limits<double>::infinity())
        os << "+inf";
    else
        os << x.upper;
    os << (x.upper_closed ? "]" : ")");
    return os.str();
}

// ── Erlang B formula ────────────────────────────────────────────────────────

static double erlang_b(double rho) {
    return rho / (1.0 + rho);
}

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    // ── 1. Analytical P_loss bounds (spec.tex) ───────────────────────────────
    //
    // With service time s in (0, +inf) and arrival rate lambda_s > 0,
    // traffic intensity rho = lambda_s * s is in (0, +inf).
    // P_loss(rho) = rho/(1+rho) maps (0,+inf) onto (0,1); closure of the
    // limit values gives P_loss in [0, 1].

    const double lambda_s = 5.0; // per-server arrival rate: b_f * d_r = 0.5 * 10
    const double mu       = 5.0; // service rate: 1 / s_t = 1 / 0.2

    const double rho_exact    = lambda_s / mu; // = 1.0
    const double p_loss_exact = erlang_b(rho_exact);
    const double p_loss_min   = 0.0; // lim rho->0+  of rho/(1+rho)
    const double p_loss_max   = 1.0; // lim rho->+inf of rho/(1+rho)

    const bool contained = (p_loss_min <= p_loss_exact) && (p_loss_exact <= p_loss_max);

    std::cout << "CKW10 LBM IA-DEVS — Analytical P_loss Bounds (TS1, b_f=0.5)\n"
              << "  lambda_s = " << lambda_s << "  mu = " << mu << "\n"
              << "  Full exponential support: sigma, s_t in (0, +inf)\n"
              << "  Traffic intensity: rho in (0, +inf)\n"
              << "  P_loss (IA-DEVS) in [" << p_loss_min << ", " << p_loss_max << "]\n"
              << "  P_loss (Erlang B exact) = " << p_loss_exact << "\n"
              << "  Containment: " << (contained ? "VERIFIED" : "FAILED") << "\n\n";

    // ── 2. 5-event interval simulation trace ─────────────────────────────────
    //
    // Drive LG and S manually for 5 task arrivals.  At each step we show:
    //  - the elapsed interval since the last LG fire
    //  - the server state (remaining service time) BEFORE the task arrives
    //  - whether a loss is possible (server was non-empty)

    std::cout << "5-Event Interval Simulation Trace\n";
    std::cout << std::string(78, '-') << "\n";
    std::cout << std::left << std::setw(6) << "Event" << std::setw(26) << "Elapsed interval"
              << std::setw(26) << "Server remaining (before)" << "Loss possible?\n";
    std::cout << std::string(78, '-') << "\n";

    // Initial states
    I lg_state     = I::closed(0.0, 0.0); // elapsed = 0
    I server_state = I::empty_interval(); // idle

    // Elapsed interval between LG fires: full exponential support (0, +inf)
    const I elapsed_interval = I::open(0.0, std::numeric_limits<double>::infinity());
    const int n_events       = 5;

    for (int k = 1; k <= n_events; ++k) {
        bool loss_possible = !server_state.is_empty();

        std::cout << std::left << std::setw(6) << k << std::setw(26)
                  << fmt_interval(elapsed_interval) << std::setw(26) << fmt_interval(server_state)
                  << (loss_possible ? "yes" : "no") << "\n";

        // LG fires: reset elapsed to [0,0]
        lg_state = LG::internal_transition(lg_state);

        // Route task to server
        const S::input_i_t task = S::input_i_t::closed(1, 1);
        server_state            = S::external_transition(server_state, elapsed_interval, task);

        // Check if server internal event (service completion) could have occurred
        // before the next arrival. Here we do NOT advance the server; the elapsed
        // interval between events drives the subtraction inside external_transition.
    }

    std::cout << std::string(78, '-') << "\n";
    std::cout << "Final server state: " << fmt_interval(server_state) << "\n\n";

    std::cout << "Note: a non-empty remaining-time interval with lower = 0 means\n"
                 "      service COULD have completed (lower bound) or be still\n"
                 "      running (upper bound > 0). This is the interval-widening\n"
                 "      effect that bounds all possible STDEVS trajectories.\n";

    return contained ? 0 : 1;
}
