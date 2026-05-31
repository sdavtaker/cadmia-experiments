// SPDX-License-Identifier: BSD-2-Clause
#include <cadmia/modeling/interval.hpp>

#include "k_counter.hpp"
#include "reset_gen.hpp"
#include "tick_gen.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/decimal.hpp>

using dec3 = cdcommons::time::decimal<3>;

using TG = vdw14::tick_gen<dec3>;
using RG = vdw14::reset_gen<dec3>;
using K  = vdw14::k_counter<dec3>;

namespace {
    dec3 ms(int n) {
        return dec3::from_scaled(n);
    }
} // namespace

// ── tick_gen ──────────────────────────────────────────────────────────────────

TEST_CASE("tick_gen period is 100 ms", "[tick_gen]") {
    TG::state_i_t s = TG::state_i_t::closed(0, 0);
    auto ta         = TG::time_advance(s);
    REQUIRE(ta.lower == ms(100));
    REQUIRE(ta.upper == ms(100));
}

TEST_CASE("tick_gen output is [1,1]", "[tick_gen]") {
    TG::state_i_t s = TG::state_i_t::closed(0, 0);
    auto out        = TG::output(s);
    REQUIRE(out.lower == 1);
    REQUIRE(out.upper == 1);
}

TEST_CASE("tick_gen internal_transition preserves state", "[tick_gen]") {
    TG::state_i_t s = TG::state_i_t::closed(0, 0);
    auto s2         = TG::internal_transition(s);
    REQUIRE(s2.lower == s.lower);
    REQUIRE(s2.upper == s.upper);
}

// ── reset_gen ─────────────────────────────────────────────────────────────────

TEST_CASE("reset_gen period is 1000 ms", "[reset_gen]") {
    RG::state_i_t s = RG::state_i_t::closed(0, 0);
    auto ta         = RG::time_advance(s);
    REQUIRE(ta.lower == ms(1000));
    REQUIRE(ta.upper == ms(1000));
}

TEST_CASE("reset_gen output is [0,0]", "[reset_gen]") {
    RG::state_i_t s = RG::state_i_t::closed(0, 0);
    auto out        = RG::output(s);
    REQUIRE(out.lower == 0);
    REQUIRE(out.upper == 0);
}

TEST_CASE("reset_gen internal_transition preserves state", "[reset_gen]") {
    RG::state_i_t s = RG::state_i_t::closed(0, 0);
    auto s2         = RG::internal_transition(s);
    REQUIRE(s2.lower == s.lower);
    REQUIRE(s2.upper == s.upper);
}

// ── k_counter ─────────────────────────────────────────────────────────────────

using phase_t = K::phase_t;

TEST_CASE("k_counter initial state is idle with count 0", "[k_counter]") {
    K::state_i_t s = K::state_i_t::closed(K::state_t{}, K::state_t{});
    REQUIRE(s.lower.phase == phase_t::idle);
    REQUIRE(s.lower.count == 0);
}

TEST_CASE("k_counter tick increments count", "[k_counter]") {
    K::state_i_t s0 =
        K::state_i_t::closed(K::state_t{phase_t::idle, 0}, K::state_t{phase_t::idle, 0});
    K::input_i_t x = K::input_i_t::closed(1, 1);
    K::time_i_t e  = K::time_i_t::closed(dec3{}, dec3{});
    auto s1        = K::external_transition(s0, e, x);
    REQUIRE(s1.lower.count == 1);
    REQUIRE(s1.upper.count == 1);
    REQUIRE(s1.lower.phase == phase_t::idle);
}

TEST_CASE("k_counter zero input transitions to ready", "[k_counter]") {
    K::state_i_t s0 =
        K::state_i_t::closed(K::state_t{phase_t::idle, 5}, K::state_t{phase_t::idle, 5});
    K::input_i_t x = K::input_i_t::closed(0, 0);
    K::time_i_t e  = K::time_i_t::closed(dec3{}, dec3{});
    auto s1        = K::external_transition(s0, e, x);
    REQUIRE(s1.lower.phase == phase_t::ready);
    REQUIRE(s1.lower.count == 5);
}

TEST_CASE("k_counter output emits count", "[k_counter]") {
    K::state_i_t s =
        K::state_i_t::closed(K::state_t{phase_t::ready, 10}, K::state_t{phase_t::ready, 10});
    auto out = K::output(s);
    REQUIRE(out.lower == 10);
    REQUIRE(out.upper == 10);
}

TEST_CASE("k_counter internal_transition resets to idle 0", "[k_counter]") {
    K::state_i_t s =
        K::state_i_t::closed(K::state_t{phase_t::ready, 10}, K::state_t{phase_t::ready, 10});
    auto s2 = K::internal_transition(s);
    REQUIRE(s2.lower.phase == phase_t::idle);
    REQUIRE(s2.lower.count == 0);
}

TEST_CASE("k_counter idle state is passive", "[k_counter]") {
    K::state_i_t s =
        K::state_i_t::closed(K::state_t{phase_t::idle, 0}, K::state_t{phase_t::idle, 0});
    REQUIRE(K::time_advance(s).is_empty());
}

TEST_CASE("k_counter ready state has ta [0,0]", "[k_counter]") {
    K::state_i_t s =
        K::state_i_t::closed(K::state_t{phase_t::ready, 5}, K::state_t{phase_t::ready, 5});
    auto ta = K::time_advance(s);
    REQUIRE(ta.lower == dec3{});
    REQUIRE(ta.upper == dec3{});
}
