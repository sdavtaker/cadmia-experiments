// SPDX-License-Identifier: BSD-2-Clause
// VWD21 Section 7 case study: 4 Generators + 1 Processor (IA-DEVS)
#include <cadmia/engine/engine.hpp>
#include <cadmia/engine/root_coordinator.hpp>
#include <cadmia/modeling/coupled.hpp>
#include <cadmia/modeling/interval.hpp>

#include "generator.hpp"
#include "processor.hpp"
#include <any>
#include <cdcommons/time/decimal.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

using dec3   = cdcommons::time::decimal<3>;
using G      = vdw21::generator<dec3>;
using P      = vdw21::processor<dec3>;
using time_i = cadmia::modeling::interval<dec3>;

static cadmia::modeling::CoupledModel<dec3> build_4gp() {
    const dec3 zero{};
    const time_i zero_i = time_i::closed(zero, zero);

    auto g_s0 = G::state_i_t::closed(zero, zero);
    auto p_s0 = P::state_i_t::empty_interval();

    auto z = [](int job_id) {
        return cadmia::engine::make_translation<G::output_i_t, P::input_i_t>(
            std::function<P::input_i_t(G::output_i_t)>{
                [job_id](G::output_i_t) { return P::input_i_t::closed(job_id, job_id); }});
    };

    auto select_fn = [](const std::unordered_set<std::string> &names) -> std::string {
        for (const char *n : {"G1", "G2", "G3", "G4", "P"})
            if (names.count(n))
                return n;
        return *names.begin();
    };

    cadmia::modeling::CoupledModel<dec3>::component_map comps;
    comps.emplace("G1", cadmia::modeling::make_atomic_component<G>(g_s0, zero_i));
    comps.emplace("G2", cadmia::modeling::make_atomic_component<G>(g_s0, zero_i));
    comps.emplace("G3", cadmia::modeling::make_atomic_component<G>(g_s0, zero_i));
    comps.emplace("G4", cadmia::modeling::make_atomic_component<G>(g_s0, zero_i));
    comps.emplace("P", cadmia::modeling::make_atomic_component<P>(p_s0, zero_i));

    cadmia::modeling::CoupledModel<dec3>::influencer_map infl;
    infl["G1"] = {};
    infl["G2"] = {};
    infl["G3"] = {};
    infl["G4"] = {};
    infl["P"]  = {"G1", "G2", "G3", "G4"};

    cadmia::modeling::CoupledModel<dec3>::translation_map trans;
    trans[{"G1", "P"}] = z(1);
    trans[{"G2", "P"}] = z(2);
    trans[{"G3", "P"}] = z(3);
    trans[{"G4", "P"}] = z(4);

    return cadmia::modeling::CoupledModel<dec3>(std::move(comps), std::move(infl), std::move(trans),
                                                select_fn, zero);
}

int main() {
    std::cout << "VWD21 Section 7: 4G+P coupled model (CadmIA IA-DEVS)\n"
              << "TIME type : decimal<3>  (resolution 1 ms)\n"
              << "Expected  : paper Tables 1-5\n\n";

    auto model = build_4gp();

    cadmia::engine::RootCoordinator<dec3> rc;
    constexpr int MAX_STEPS    = 20;
    constexpr int MAX_BRANCHES = 100;

    const time_i zero_i = time_i::closed(dec3{}, dec3{});
    auto log            = rc.simulate(model, zero_i, MAX_STEPS, MAX_BRANCHES);

    std::cout << "Simulation log (" << log.size() << " entries):\n";
    std::cout << std::string(72, '-') << "\n";
    for (const auto &e : log) {
        std::cout << "  step=" << e.step << "  branch=" << e.branch << "  t=" << e.time
                  << "  kind=" << e.kind;
        if (!e.component.empty())
            std::cout << "  component=" << e.component;
        if (e.output.has_value())
            std::cout << "  output=" << *e.output;
        std::cout << "\n";
    }

    // Count processor firings and collect output job-id intervals
    int p_firings = 0;
    for (const auto &e : log) {
        if (e.component == "P" && e.raw_output.has_value())
            ++p_firings;
    }
    std::cout << "\nProcessor fired " << p_firings << " time(s).\n";
    std::cout << "Run with test binary for Table 1-5 validation.\n";

    return 0;
}
