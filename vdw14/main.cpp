// SPDX-License-Identifier: BSD-2-Clause
#include <cadmia/engine/root_coordinator.hpp>
#include <cadmia/modeling/coupled.hpp>
#include <cadmia/modeling/decimal.hpp>
#include <cadmia/modeling/interval.hpp>

#include "k_counter.hpp"
#include "reset_gen.hpp"
#include "tick_gen.hpp"
#include <any>
#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>

// ── TIME variant selection ────────────────────────────────────────────────────
// Each variant is compiled as a separate executable by CMake.

#if defined(CADMIA_TIME_FLOAT)
using SimTime                        = float;
static constexpr const char *VARIANT = "float";
static constexpr int N_RESETS        = 20;   // fewer: float intervals overlap sooner,
static constexpr int MAX_STEPS       = 2000; // causing BFS branching per reset cycle
static constexpr int MAX_BRANCHES    = 5000;

#elif defined(CADMIA_TIME_DOUBLE)
using SimTime                        = double;
static constexpr const char *VARIANT = "double";
static constexpr int N_RESETS        = 20;
static constexpr int MAX_STEPS       = 2000;
static constexpr int MAX_BRANCHES    = 5000;

#elif defined(CADMIA_TIME_DECIMAL)
using SimTime                        = cadmia::modeling::decimal<3>;
static constexpr const char *VARIANT = "decimal<3>";
static constexpr int N_RESETS        = 100;
static constexpr int MAX_STEPS       = N_RESETS * 20;
static constexpr int MAX_BRANCHES    = 1000;

#else
#error "Define CADMIA_TIME_DECIMAL, CADMIA_TIME_FLOAT, or CADMIA_TIME_DOUBLE"
#endif

// ── Model types ───────────────────────────────────────────────────────────────

using TG     = vdw14::tick_gen<SimTime>;
using RG     = vdw14::reset_gen<SimTime>;
using K      = vdw14::k_counter<SimTime>;
using time_i = cadmia::modeling::interval<SimTime>;

// ── Model construction ────────────────────────────────────────────────────────

static cadmia::modeling::CoupledModel<SimTime> build_model() {
    const SimTime zero{};
    const time_i zero_i = time_i::closed(zero, zero);

    auto tg_s0 = cadmia::modeling::interval<TG::state_t>::closed(0, 0);
    auto rg_s0 = cadmia::modeling::interval<RG::state_t>::closed(0, 0);
    K::state_t ks{};
    auto k_s0 = K::state_i_t::closed(ks, ks);

    auto id = cadmia::engine::identity_translation();

    auto select_fn = [](const std::unordered_set<std::string> &names) -> std::string {
        if (names.count("tick_gen"))
            return "tick_gen";
        if (names.count("reset_gen"))
            return "reset_gen";
        return *names.begin();
    };

    cadmia::modeling::CoupledModel<SimTime>::component_map comps;
    comps.emplace("K", cadmia::modeling::make_atomic_component<K>(k_s0, zero_i));
    comps.emplace("tick_gen", cadmia::modeling::make_atomic_component<TG>(tg_s0, zero_i));
    comps.emplace("reset_gen", cadmia::modeling::make_atomic_component<RG>(rg_s0, zero_i));

    cadmia::modeling::CoupledModel<SimTime>::influencer_map infl;
    infl["tick_gen"]  = {};
    infl["reset_gen"] = {};
    infl["K"]         = {"tick_gen", "reset_gen"};

    cadmia::modeling::CoupledModel<SimTime>::translation_map trans;
    trans[{"tick_gen", "K"}]  = id;
    trans[{"reset_gen", "K"}] = id;

    return cadmia::modeling::CoupledModel<SimTime>(std::move(comps), std::move(infl),
                                                   std::move(trans), select_fn, zero);
}

// ── Experiment runner ─────────────────────────────────────────────────────────

static std::vector<std::pair<int, int>> run_experiment(int n_resets) {
    const SimTime zero{};
    const time_i zero_i = time_i::closed(zero, zero);

    auto model = build_model();

    cadmia::engine::RootCoordinator<SimTime> rc;
    auto log = rc.simulate(model, zero_i, MAX_STEPS, MAX_BRANCHES);

    std::vector<std::pair<int, int>> outputs;
    for (const auto &entry : log) {
        if (entry.component == "K" && entry.raw_output.has_value()) {
            auto val = std::any_cast<K::output_i_t>(*entry.raw_output);
            outputs.emplace_back(val.lower, val.upper);
            if (static_cast<int>(outputs.size()) >= n_resets)
                break;
        }
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
    std::map<std::pair<int, int>, long long> hist;
    for (const auto &[lo, hi] : outputs) {
        ++hist[{lo, hi}];
        if (lo != expected || hi != expected)
            ++errors;
    }

    std::cout << "  resets collected : " << outputs.size() << "\n"
              << "  non-[10,10] outputs: " << errors << " ("
              << (100.0 * errors / static_cast<double>(outputs.size())) << " %)\n"
              << "  histogram:\n";
    for (const auto &[v, cnt] : hist)
        std::cout << "    [" << v.first << ", " << v.second << "] x " << cnt << "\n";
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "VDW14 Tick-Counter Experiment (CadmIA IA-DEVS)\n"
              << "TIME type       : " << VARIANT << "\n"
              << "tick_gen period : 100 ms   reset_gen period: 1 s\n"
              << "expected output : [10, 10] at every reset\n"
              << "n_resets target : " << N_RESETS << "\n\n";

    auto outputs = run_experiment(N_RESETS);
    print_stats(outputs, 10);

    return outputs.empty() ? 1 : 0;
}
