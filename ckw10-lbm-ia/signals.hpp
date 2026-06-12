// SPDX-License-Identifier: BSD-2-Clause
#pragma once

#include <compare>
#include <type_traits>

namespace ckw10_lbm_ia {

    // Signal emitted by the load generator on each task departure.
    enum class task_signal { arrived };

    constexpr std::strong_ordering operator<=>(task_signal a, task_signal b) noexcept {
        return static_cast<std::underlying_type_t<task_signal>>(a) <=>
               static_cast<std::underlying_type_t<task_signal>>(b);
    }

    // Signal emitted by a server when it finishes processing a task.
    enum class completion_signal { task_completed };

    constexpr std::strong_ordering operator<=>(completion_signal a, completion_signal b) noexcept {
        return static_cast<std::underlying_type_t<completion_signal>>(a) <=>
               static_cast<std::underlying_type_t<completion_signal>>(b);
    }

} // namespace ckw10_lbm_ia
