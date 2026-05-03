# SPDX-License-Identifier: BSD-2-Clause
# VDW14 tick-counter experiment — CadmIA IA-DEVS

add_executable(vdw14 main.cpp)
target_link_libraries(vdw14 PRIVATE cadmia::cadmia)

# Tests
add_executable(vdw14-tests test_models.cpp)
target_link_libraries(vdw14-tests PRIVATE cadmia::cadmia Catch2::Catch2WithMain)
add_test(NAME vdw14-tests COMMAND vdw14-tests)
