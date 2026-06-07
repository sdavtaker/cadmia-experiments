# SPDX-License-Identifier: BSD-2-Clause
# VWD21 Section 7 case study: 4G+P coupled model — decimal<3> only

add_executable(vdw21-decimal main.cpp)
target_compile_options(vdw21-decimal PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(vdw21-decimal PRIVATE cadmia::cadmia cdcommons::cdcommons)

# Tests: atomic model unit tests + coordinator-level Table 1-5 validation
add_executable(vdw21-tests test_models.cpp)
target_compile_options(vdw21-tests PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(vdw21-tests PRIVATE cadmia::cadmia cdcommons::cdcommons Catch2::Catch2WithMain)
add_test(NAME vdw21-tests COMMAND vdw21-tests)
