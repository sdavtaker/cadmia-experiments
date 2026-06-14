add_executable(ckw10-lbm-ia main.cpp)
target_include_directories(ckw10-lbm-ia PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(ckw10-lbm-ia PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(ckw10-lbm-ia PRIVATE cadmia::cadmia)

add_test(NAME ckw10-lbm-ia-runs COMMAND ckw10-lbm-ia)
