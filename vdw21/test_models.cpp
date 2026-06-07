// SPDX-License-Identifier: BSD-2-Clause
#include <cadmia/engine/coordinator.hpp>
#include <cadmia/engine/engine.hpp>
#include <cadmia/modeling/coupled.hpp>
#include <cadmia/modeling/interval.hpp>

#include "generator.hpp"
#include "processor.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/decimal.hpp>
#include <functional>
#include <memory>
#include <unordered_set>

using dec3 = cdcommons::time::decimal<3>;

using G = vdw21::generator<dec3>;
using P = vdw21::processor<dec3>;

namespace {
    dec3 ms(int n) {
        return dec3::from_scaled(n);
    }

    G::state_i_t gs(int lo_ms, int hi_ms) {
        return G::state_i_t::closed(ms(lo_ms), ms(hi_ms));
    }
    G::time_i_t gt(int lo_ms, int hi_ms) {
        return G::time_i_t::closed(ms(lo_ms), ms(hi_ms));
    }
    P::state_i_t passive_p() {
        return P::state_i_t::empty_interval();
    }
    P::state_i_t active_p(int tocj_lo_ms, int tocj_hi_ms, std::vector<int> qj_lo,
                          std::vector<int> qj_hi) {
        return P::state_i_t::closed(P::state_t{ms(tocj_lo_ms), std::move(qj_lo)},
                                    P::state_t{ms(tocj_hi_ms), std::move(qj_hi)});
    }
} // namespace

// ── generator ─────────────────────────────────────────────────────────────────

TEST_CASE("generator time_advance from zero is period [997,1005]ms", "[generator]") {
    auto ta = G::time_advance(gs(0, 0));
    REQUIRE(ta.lower == ms(997));
    REQUIRE(ta.upper == ms(1005));
    REQUIRE(ta.lower_closed);
    REQUIRE(ta.upper_closed);
}

TEST_CASE("generator output is always [1,2]", "[generator]") {
    auto out = G::output(gs(0, 0));
    REQUIRE(out.lower == 1);
    REQUIRE(out.upper == 2);
}

TEST_CASE("generator internal_transition resets to [0,0]", "[generator]") {
    auto s2 = G::internal_transition(gs(0, 0));
    REQUIRE(s2.lower == ms(0));
    REQUIRE(s2.upper == ms(0));
}

TEST_CASE("generator external_transition adds elapsed to state", "[generator]") {
    auto s0        = gs(0, 0);
    auto e         = gt(10, 20);
    G::input_i_t x = G::input_i_t::closed(0, 0);
    auto s1        = G::external_transition(s0, e, x);
    REQUIRE(s1.lower == ms(10));
    REQUIRE(s1.upper == ms(20));
}

TEST_CASE("generator time_advance diminishes with elapsed state", "[generator]") {
    // state = [10, 20]ms → TA = [997-20, 1005-10] = [977, 995]
    auto ta = G::time_advance(gs(10, 20));
    REQUIRE(ta.lower == ms(977));
    REQUIRE(ta.upper == ms(995));
}

// ── processor ─────────────────────────────────────────────────────────────────

TEST_CASE("processor passive state gives empty time_advance", "[processor]") {
    REQUIRE(P::time_advance(passive_p()).is_empty());
}

TEST_CASE("processor with one job: time_advance [250,250]ms from tocj=0", "[processor]") {
    auto s  = active_p(0, 0, {1}, {1});
    auto ta = P::time_advance(s);
    REQUIRE(ta.lower == ms(250));
    REQUIRE(ta.upper == ms(250));
}

TEST_CASE("processor time_advance contracts with tocj: tocj=[0,24] → [226,250]", "[processor]") {
    // tocj=[0,24]ms → ta=[250-24, 250-0]=[226,250]ms
    auto s  = active_p(0, 24, {1, 2, 3, 4}, {1, 2, 3, 4});
    auto ta = P::time_advance(s);
    REQUIRE(ta.lower == ms(226));
    REQUIRE(ta.upper == ms(250));
}

TEST_CASE("processor external_transition from passive starts new job with tocj=0", "[processor]") {
    auto s0        = passive_p();
    P::time_i_t e  = P::time_i_t::closed(ms(997), ms(1005)); // elapsed doesn't matter
    P::input_i_t x = P::input_i_t::closed(1, 1);
    auto s1        = P::external_transition(s0, e, x);
    REQUIRE_FALSE(s1.is_empty());
    REQUIRE(s1.lower.tocj == ms(0));
    REQUIRE(s1.upper.tocj == ms(0));
    REQUIRE(s1.lower.qj == std::vector<int>{1});
    REQUIRE(s1.upper.qj == std::vector<int>{1});
}

TEST_CASE("processor external_transition from active appends job and advances tocj",
          "[processor]") {
    auto s0        = active_p(0, 0, {1}, {1});
    P::time_i_t e  = P::time_i_t::closed(ms(0), ms(8));
    P::input_i_t x = P::input_i_t::closed(2, 2);
    auto s1        = P::external_transition(s0, e, x);
    REQUIRE(s1.lower.tocj == ms(0));
    REQUIRE(s1.upper.tocj == ms(8));
    REQUIRE(s1.lower.qj == (std::vector<int>{1, 2}));
    REQUIRE(s1.upper.qj == (std::vector<int>{1, 2}));
}

TEST_CASE("processor output returns front job-id interval", "[processor]") {
    auto s   = active_p(0, 0, {1}, {1});
    auto out = P::output(s);
    REQUIRE(out.lower == 1);
    REQUIRE(out.upper == 1);
}

TEST_CASE("processor internal_transition pops front job and resets tocj", "[processor]") {
    auto s0 = active_p(0, 24, {1, 2, 3, 4}, {1, 2, 3, 4});
    auto s1 = P::internal_transition(s0);
    REQUIRE_FALSE(s1.is_empty());
    REQUIRE(s1.lower.tocj == ms(0));
    REQUIRE(s1.upper.tocj == ms(0));
    REQUIRE(s1.lower.qj == (std::vector<int>{2, 3, 4}));
    REQUIRE(s1.upper.qj == (std::vector<int>{2, 3, 4}));
}

TEST_CASE("processor internal_transition of last job yields passive state", "[processor]") {
    auto s0 = active_p(0, 0, {4}, {4});
    auto s1 = P::internal_transition(s0);
    REQUIRE(s1.is_empty());
    REQUIRE(P::time_advance(s1).is_empty());
}

// ── 4GP coupled model (Table 1-5 from VWD21 Section 7) ───────────────────────

namespace {
    using time_i = cadmia::modeling::interval<dec3>;

    // interval has no operator==; compare all four fields
    static bool iv_eq(const time_i &a, const time_i &b) noexcept {
        return a.lower == b.lower && a.upper == b.upper && a.lower_closed == b.lower_closed &&
               a.upper_closed == b.upper_closed;
    }

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

        return cadmia::modeling::CoupledModel<dec3>(std::move(comps), std::move(infl),
                                                    std::move(trans), select_fn, zero);
    }

    static cadmia::engine::coordinator<dec3> make_coord() {
        auto model = std::make_shared<const cadmia::modeling::CoupledModel<dec3>>(build_4gp());
        cadmia::engine::coordinator<dec3> coord(model);
        coord.init(time_i::closed(dec3{}, dec3{}));
        return coord;
    }

    // Fire first non-skip branch; all 4 generators are symmetric so order doesn't matter.
    static void fire_first(cadmia::engine::coordinator<dec3> &coord) {
        auto branches = coord.compute_branches(coord.t_next());
        for (auto &b : branches) {
            if (!b.engine_name.empty()) {
                coord.execute_branch(b);
                return;
            }
        }
    }
} // namespace

// ── Table 1: Initialization ───────────────────────────────────────────────────

TEST_CASE("Table1: all generators t_next=[997,1005] after init", "[table1]") {
    auto coord          = make_coord();
    const time_i period = time_i::closed(ms(997), ms(1005));
    const time_i zero_i = time_i::closed(ms(0), ms(0));
    for (const auto &name : {"G1", "G2", "G3", "G4"}) {
        CHECK(iv_eq(coord.engines().at(name)->t_next(), period));
        CHECK(iv_eq(coord.engines().at(name)->t_last(), zero_i));
    }
}

TEST_CASE("Table1: processor is passive after init", "[table1]") {
    auto coord = make_coord();
    REQUIRE(coord.engines().at("P")->t_next().is_empty());
}

TEST_CASE("Table1: coordinator t_next=[997,1005]", "[table1]") {
    auto coord = make_coord();
    REQUIRE(iv_eq(coord.t_next(), time_i::closed(ms(997), ms(1005))));
}

// ── Table 2: After one generator fires ───────────────────────────────────────
// All 4 generators are symmetric; we check aggregate state, not which one fired.

TEST_CASE("Table2: exactly one generator has t_next=[1994,2010] after first fire", "[table2]") {
    auto coord = make_coord();
    fire_first(coord);

    const time_i advanced = time_i::closed(ms(1994), ms(2010));
    const time_i period   = time_i::closed(ms(997), ms(1005));
    int n_advanced = 0, n_unchanged = 0;
    for (const auto &name : {"G1", "G2", "G3", "G4"}) {
        if (iv_eq(coord.engines().at(name)->t_next(), advanced))
            ++n_advanced;
        else if (iv_eq(coord.engines().at(name)->t_next(), period))
            ++n_unchanged;
    }
    REQUIRE(n_advanced == 1);
    REQUIRE(n_unchanged == 3);
}

TEST_CASE("Table2: processor t_next=[1247,1255] after first generator fires", "[table2]") {
    auto coord = make_coord();
    fire_first(coord);

    REQUIRE(iv_eq(coord.engines().at("P")->t_last(), time_i::closed(ms(997), ms(1005))));
    REQUIRE(iv_eq(coord.engines().at("P")->t_next(), time_i::closed(ms(1247), ms(1255))));
}

// ── Table 3: After all four generators fire ───────────────────────────────────
// NOTE: cadmia's simulator.hpp x() uses t_next.lower instead of t_last.lower
// for the elapsed upper bound — these tests expose that bug.

TEST_CASE("Table3: all generators t_next=[1994,2010] after all four fire", "[table3]") {
    auto coord = make_coord();
    for (int i = 0; i < 4; ++i)
        fire_first(coord);

    const time_i expected = time_i::closed(ms(1994), ms(2010));
    for (const auto &name : {"G1", "G2", "G3", "G4"}) {
        CHECK(iv_eq(coord.engines().at(name)->t_next(), expected));
    }
}

TEST_CASE("Table3: processor t_next=[1223,1255] after all four generators fire", "[table3]") {
    auto coord = make_coord();
    for (int i = 0; i < 4; ++i)
        fire_first(coord);

    // tocj=[0,24]ms → TA=[226,250]ms, t_last=[997,1005] → t_next=[1223,1255]
    REQUIRE(iv_eq(coord.engines().at("P")->t_next(), time_i::closed(ms(1223), ms(1255))));
}

// ── Table 4: After P fires three times ────────────────────────────────────────

TEST_CASE("Table4: processor times after G1-G4-P-P-P", "[table4]") {
    auto coord = make_coord();
    for (int i = 0; i < 7; ++i)
        fire_first(coord);

    CHECK(iv_eq(coord.engines().at("P")->t_last(), time_i::closed(ms(1723), ms(1755))));
    CHECK(iv_eq(coord.engines().at("P")->t_next(), time_i::closed(ms(1973), ms(2005))));
}

TEST_CASE("Table4: generators at t_next=[1994,2010] after 7 steps", "[table4]") {
    auto coord = make_coord();
    for (int i = 0; i < 7; ++i)
        fire_first(coord);

    const time_i expected = time_i::closed(ms(1994), ms(2010));
    for (const auto &name : {"G1", "G2", "G3", "G4"}) {
        CHECK(iv_eq(coord.engines().at(name)->t_next(), expected));
    }
}

// ── Table 5: Nothing branch (skip) ────────────────────────────────────────────

TEST_CASE("Table5: step 8 produces P-fire and skip branches", "[table5]") {
    auto coord = make_coord();
    for (int i = 0; i < 7; ++i)
        fire_first(coord);

    auto branches = coord.compute_branches(coord.t_next());
    REQUIRE(branches.size() == 2);
    bool has_p = false, has_skip = false;
    for (auto &b : branches) {
        if (b.engine_name == "P")
            has_p = true;
        if (b.engine_name.empty())
            has_skip = true;
    }
    REQUIRE(has_p);
    REQUIRE(has_skip);
}

TEST_CASE("Table5: skip branch restricts P.t_next lower to 1994ms", "[table5]") {
    auto coord = make_coord();
    for (int i = 0; i < 7; ++i)
        fire_first(coord);

    auto branches = coord.compute_branches(coord.t_next());
    for (auto &b : branches) {
        if (b.engine_name.empty()) {
            coord.execute_branch(b);
            break;
        }
    }

    auto p_tn = coord.engines().at("P")->t_next();
    REQUIRE(p_tn.lower == ms(1994));
    REQUIRE(p_tn.upper == ms(2005));
}
