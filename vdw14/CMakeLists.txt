# SPDX-License-Identifier: BSD-2-Clause
# VDW14 tick-counter experiment — CadmIA IA-DEVS

add_executable(vdw14 main.cpp)
target_link_libraries(vdw14 PRIVATE cadmia::cadmia)
