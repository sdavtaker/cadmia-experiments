# SPDX-License-Identifier: BSD-2-Clause
# VDW14 tick-counter experiment — CadmIA IA-DEVS (three TIME variants)

cadmia_add_time_variants(vdw14 main.cpp)

# Tests (use decimal<3> only — the models themselves are TIME-agnostic)
add_executable(vdw14-tests test_models.cpp)
target_link_libraries(vdw14-tests PRIVATE cadmia::cadmia cdcommons Catch2::Catch2WithMain)
add_test(NAME vdw14-tests COMMAND vdw14-tests)
