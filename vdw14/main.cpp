// SPDX-License-Identifier: BSD-2-Clause
#include <cadmia/engine/coordinator.hpp>
#include <cadmia/engine/engine.hpp>
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

using dec3   = cadmia::modeling::decimal<3>;
using time_i = cadmia::modeling::interval<dec3>;

using TG = vdw14::tick_gen<dec3>;
using RG = vdw14::reset_gen<dec3>;
using K  = vdw14::k_counter<dec3>;

// ── Simulation driver ─────────────────────────────────────────────────────────

static std::vector<std::pair<int, int>> run_experiment(int n_resets) {
    const dec3 zero{};
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

    cadmia::modeling::CoupledModel<dec3>::component_map comps;
    comps.emplace("K", cadmia::modeling::make_atomic_component<K>(k_s0, zero_i));
    comps.emplace("tick_gen", cadmia::modeling::make_atomic_component<TG>(tg_s0, zero_i));
    comps.emplace("reset_gen", cadmia::modeling::make_atomic_component<RG>(rg_s0, zero_i));

    cadmia::modeling::CoupledModel<dec3>::influencer_map infl;
    infl["tick_gen"]  = {};
    infl["reset_gen"] = {};
    infl["K"]         = {"tick_gen", "reset_gen"};

    cadmia::modeling::CoupledModel<dec3>::translation_map trans;
    trans[{"tick_gen", "K"}]  = id;
    trans[{"reset_gen", "K"}] = id;

    auto model = std::make_shared<const cadmia::modeling::CoupledModel<dec3>>(
        std::move(comps), std::move(infl), std::move(trans), select_fn, zero);

    cadmia::engine::coordinator<dec3> coord(model);
    coord.init(zero_i);

    std::vector<std::pair<int, int>> outputs;
    int steps       = 0;
    const int limit = n_resets * 15 + 10;

    while (!coord.t_next().is_empty() && steps < limit &&
           static_cast<int>(outputs.size()) < n_resets) {
        auto t       = coord.t_next();
        auto actions = coord.compute_branches(t);
        if (actions.empty())
            break;

        auto &action        = actions[0];
        auto [comp_out, _u] = coord.execute_branch(action);

        if (!action.engine_name.empty() && action.engine_name == "K" && comp_out.has_value()) {
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
    std::map<std::pair<int, int>, long long> hist;
    for (const auto &[lo, hi] : outputs) {
        ++hist[{lo, hi}];
        if (lo != expected || hi != expected)
            ++errors;
    }

    std::cout << "  resets:  " << outputs.size() << "\n"
              << "  errors:  " << errors << " ("
              << (100.0 * errors / static_cast<double>(outputs.size())) << " %)\n"
              << "  histogram:\n";
    for (const auto &[v, cnt] : hist)
        std::cout << "    [" << v.first << ", " << v.second << "] x " << cnt << "\n";
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
