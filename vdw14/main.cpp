// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2025, Damian Vicino
 * Carleton University
 * All rights reserved.
 */

// VDW14 Tick-Counter Experiment — CadmIA IA-DEVS
//
// Coupled model:
//   tick_gen (period [100ms, 100ms], output [1,1])  ──┐
//                                                       ├──→ K (tick-counter)
//   reset_gen (period [1s, 1s],    output [0,0])  ──┘
//
// K counts non-zero inputs (ticks).  Input [0,0] triggers output and reset.
// With exact fixed-point arithmetic and SELECT ordering tick before reset,
// K outputs exactly [10,10] at every reset event.
//
// Reference: Vicino, Dalle, Wainer. SIMUTOOLS 2014. hal-01055555

#include <cadmia/engine/coordinator.hpp>
#include <cadmia/modeling/coupled.hpp>
#include <cadmia/modeling/decimal.hpp>
#include <cadmia/modeling/interval.hpp>
#include <cadmia/engine/engine.hpp>

#include <any>
#include <compare>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using dec3     = cadmia::modeling::decimal<3>;
using time_i_t = cadmia::modeling::interval<dec3>;

// ── tick_gen: fires every 100 ms, outputs [1, 1] ─────────────────────────────

struct tick_gen {
    using time_t   = dec3;
    using state_t  = int;
    using input_t  = int;
    using output_t = int;

    using time_i_t   = cadmia::modeling::interval<time_t>;
    using state_i_t  = cadmia::modeling::interval<state_t>;
    using input_i_t  = cadmia::modeling::interval<input_t>;
    using output_i_t = cadmia::modeling::interval<output_t>;

    static state_i_t internal_transition(const state_i_t &s) { return s; }

    static state_i_t external_transition(const state_i_t &s, const time_i_t &,
                                          const input_i_t &) {
        return s;  // never receives input
    }

    static output_i_t output(const state_i_t &) {
        return output_i_t::closed(1, 1);
    }

    static time_i_t time_advance(const state_i_t &) {
        const auto p = dec3::from_scaled(100);  // 100 ms
        return time_i_t::closed(p, p);
    }
};

// ── reset_gen: fires every 1000 ms, outputs [0, 0] ───────────────────────────

struct reset_gen {
    using time_t   = dec3;
    using state_t  = int;
    using input_t  = int;
    using output_t = int;

    using time_i_t   = cadmia::modeling::interval<time_t>;
    using state_i_t  = cadmia::modeling::interval<state_t>;
    using input_i_t  = cadmia::modeling::interval<input_t>;
    using output_i_t = cadmia::modeling::interval<output_t>;

    static state_i_t internal_transition(const state_i_t &s) { return s; }

    static state_i_t external_transition(const state_i_t &s, const time_i_t &,
                                          const input_i_t &) {
        return s;  // never receives input
    }

    static output_i_t output(const state_i_t &) {
        return output_i_t::closed(0, 0);
    }

    static time_i_t time_advance(const state_i_t &) {
        const auto p = dec3::from_scaled(1000);  // 1000 ms = 1 s
        return time_i_t::closed(p, p);
    }
};

// ── K: tick-counter — count = running sum; [0,0] input → output and reset ────

struct K {
    using time_t   = dec3;
    using input_t  = int;
    using output_t = int;

    enum class phase_t : int { idle = 0, ready = 1 };

    struct state_t {
        phase_t phase{phase_t::idle};
        int     count{0};

        constexpr state_t() noexcept = default;
        constexpr state_t(phase_t p, int c) noexcept : phase(p), count(c) {}

        [[nodiscard]] std::strong_ordering
        operator<=>(const state_t &o) const noexcept {
            if (auto cmp = phase <=> o.phase; cmp != 0) return cmp;
            return count <=> o.count;
        }
        bool operator==(const state_t &) const noexcept = default;
    };

    using time_i_t   = cadmia::modeling::interval<time_t>;
    using state_i_t  = cadmia::modeling::interval<state_t>;
    using input_i_t  = cadmia::modeling::interval<input_t>;
    using output_i_t = cadmia::modeling::interval<output_t>;

    static state_i_t internal_transition(const state_i_t &) {
        // Output done — reset count and go idle
        return state_i_t::closed(state_t{phase_t::idle, 0}, state_t{phase_t::idle, 0});
    }

    static state_i_t external_transition(const state_i_t &s, const time_i_t &,
                                          const input_i_t &x) {
        if (x.lower == 0 && x.upper == 0) {
            // Reset signal: transition to ready (output count)
            return state_i_t::closed(
                state_t{phase_t::ready, s.lower.count},
                state_t{phase_t::ready, s.upper.count});
        }
        // Tick: accumulate
        return state_i_t::closed(
            state_t{phase_t::idle, s.lower.count + x.lower},
            state_t{phase_t::idle, s.upper.count + x.upper});
    }

    static output_i_t output(const state_i_t &s) {
        return output_i_t::closed(s.lower.count, s.upper.count);
    }

    static time_i_t time_advance(const state_i_t &s) {
        if (s.lower.phase == phase_t::idle && s.upper.phase == phase_t::idle)
            return time_i_t::empty_interval();  // passive
        if (s.lower.phase == phase_t::ready && s.upper.phase == phase_t::ready)
            return time_i_t::closed(time_t{}, time_t{});  // [0, 0]
        // Mixed — shouldn't occur with exact degenerate intervals
        return time_i_t::right_open(time_t{}, cadmia::modeling::plus_inf);
    }
};

// ── Simulation driver ─────────────────────────────────────────────────────────

using coord_t = cadmia::engine::coordinator<dec3>;
using model_t = cadmia::modeling::CoupledModel<dec3>;

static std::vector<std::pair<int, int>> run_experiment(int n_resets) {
    const dec3 zero{};
    const auto zero_i = time_i_t::closed(zero, zero);

    // Initial states
    auto tg_s0 = cadmia::modeling::interval<tick_gen::state_t>::closed(0, 0);
    auto rg_s0 = cadmia::modeling::interval<reset_gen::state_t>::closed(0, 0);
    K::state_t ks{};
    auto k_s0  = K::state_i_t::closed(ks, ks);

    // Translations: both generators output int → K input int (identity)
    auto id = cadmia::engine::identity_translation();

    // SELECT: tick_gen fires before reset_gen when simultaneous
    auto select_fn = [](const std::unordered_set<std::string> &names) -> std::string {
        if (names.count("tick_gen")) return "tick_gen";
        if (names.count("reset_gen")) return "reset_gen";
        return *names.begin();
    };

    model_t::component_map comps;
    comps.emplace("K",         cadmia::modeling::make_atomic_component<K>(k_s0, zero_i));
    comps.emplace("tick_gen",  cadmia::modeling::make_atomic_component<tick_gen>(tg_s0, zero_i));
    comps.emplace("reset_gen", cadmia::modeling::make_atomic_component<reset_gen>(rg_s0, zero_i));

    model_t::influencer_map infl;
    infl["tick_gen"]  = {};
    infl["reset_gen"] = {};
    infl["K"]         = {"tick_gen", "reset_gen"};

    model_t::translation_map trans;
    trans[{"tick_gen",  "K"}] = id;
    trans[{"reset_gen", "K"}] = id;

    auto model = std::make_shared<const model_t>(
        std::move(comps), std::move(infl), std::move(trans), select_fn, zero);

    coord_t coord(model);
    coord.init(zero_i);

    std::vector<std::pair<int, int>> outputs;  // (lower, upper) of each K output
    int steps       = 0;
    const int limit = n_resets * 15 + 10;

    while (!coord.t_next().is_empty() && steps < limit &&
           static_cast<int>(outputs.size()) < n_resets) {

        auto t       = coord.t_next();
        auto actions = coord.compute_branches(t);
        if (actions.empty()) break;

        // With exact degenerate intervals and our select, always 1 action
        auto &action = actions[0];
        auto [comp_out, _] = coord.execute_branch(action);

        if (!action.engine_name.empty() && action.engine_name == "K" &&
            comp_out.has_value()) {
            auto val = std::any_cast<K::output_i_t>(*comp_out);
            outputs.emplace_back(val.lower, val.upper);
        }
        ++steps;
    }

    return outputs;
}

// ── Statistics ────────────────────────────────────────────────────────────────

static void print_stats(const std::vector<std::pair<int, int>> &outputs, int expected) {
    if (outputs.empty()) {
        std::cout << "  no outputs collected\n";
        return;
    }

    long long errors = 0;
    std::map<std::pair<int,int>, long long> hist;
    for (const auto &[lo, hi] : outputs) {
        ++hist[{lo, hi}];
        if (lo != expected || hi != expected) ++errors;
    }

    std::cout << "  resets:  " << outputs.size() << "\n"
              << "  errors:  " << errors
              << " (" << (100.0 * errors / static_cast<double>(outputs.size()))
              << " %)\n"
              << "  histogram:\n";
    for (const auto &[v, cnt] : hist)
        std::cout << "    [" << v.first << ", " << v.second << "] × " << cnt << "\n";
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    const int n_resets = 100;

    std::cout << "VDW14 Tick-Counter Experiment (CadmIA IA-DEVS)\n"
              << "tick_gen period=[100ms,100ms]  reset_gen period=[1s,1s]\n"
              << "expected counter output = [10, 10] at every reset\n"
              << "n_resets = " << n_resets << "\n\n";

    auto outputs = run_experiment(n_resets);
    print_stats(outputs, 10);

    return outputs.empty() ? 1 : 0;
}
