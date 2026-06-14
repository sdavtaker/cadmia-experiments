cmake_minimum_required(VERSION 3.28)

# Toolchain must be set before project() so CMake loads vcpkg during compiler detection.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "Path to vcpkg toolchain file")
    if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(FATAL_ERROR "VCPKG_ROOT is set to '$ENV{VCPKG_ROOT}' but toolchain file not found: ${CMAKE_TOOLCHAIN_FILE}")
    endif()
endif()

project(cadmia-experiments VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(cadmia CONFIG REQUIRED)
find_package(cdcommons CONFIG REQUIRED)

# Testing
find_package(Catch2 3 CONFIG REQUIRED)
include(CTest)
enable_testing()

# Helper: build one executable per TIME variant (decimal, rational, rsfp, mbfp, float, double).
# Usage: cadmia_add_time_variants(<name-prefix> <source>)
# Produces: <name-prefix>-decimal, <name-prefix>-rational, <name-prefix>-rsfp,
#           <name-prefix>-mbfp, <name-prefix>-float, <name-prefix>-double
function(cadmia_add_time_variants prefix source)
    foreach(variant IN ITEMS decimal rational rsfp mbfp float double)
        set(target "${prefix}-${variant}")
        string(TOUPPER "${variant}" variant_upper)
        add_executable(${target} ${source})
        target_compile_definitions(${target} PRIVATE "CADMIA_TIME_${variant_upper}")
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
        if(variant STREQUAL "float" OR variant STREQUAL "double")
            target_compile_options(${target} PRIVATE
                -frounding-math -fno-unsafe-math-optimizations)
        endif()
        target_link_libraries(${target} PRIVATE cadmia::cadmia)
        if(NOT (variant STREQUAL "float" OR variant STREQUAL "double"))
            target_link_libraries(${target} PRIVATE cdcommons::cdcommons)
        endif()
    endforeach()
endfunction()

add_subdirectory(vdw14)
add_subdirectory(vdw21)
add_subdirectory(ckw10-lbm-ia)

# Code Formatting
find_program(CLANG_FORMAT
    NAMES clang-format clang-format-18 clang-format-17 clang-format-16 clang-format-15
    DOC "Path to clang-format executable"
)

if(CLANG_FORMAT)
    file(GLOB_RECURSE EXPERIMENT_FORMAT_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/vdw14/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vdw14/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vdw21/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vdw21/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ckw10-lbm-ia/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ckw10-lbm-ia/*.cpp"
    )

    add_custom_target(experiments-format
        COMMAND ${CLANG_FORMAT} -i -style=file ${EXPERIMENT_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Applying clang-format to experiment sources"
        VERBATIM
    )

    add_custom_target(experiments-check-format
        COMMAND ${CLANG_FORMAT} -style=file --dry-run -Werror ${EXPERIMENT_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking clang-format compliance"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found; 'format' and 'check-format' targets unavailable")
endif()

# PDF Documentation
find_program(PDFLATEX pdflatex DOC "Path to pdflatex")
if(PDFLATEX)
    set(DOCS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCS_OUTPUT_DIR}
        COMMAND ${PDFLATEX} -interaction=nonstopmode -output-directory ${DOCS_OUTPUT_DIR} main.tex
        COMMAND ${PDFLATEX} -interaction=nonstopmode -output-directory ${DOCS_OUTPUT_DIR} main.tex
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
        COMMENT "Building PDF documentation -> build/docs/main.pdf"
        VERBATIM
    )
else()
    message(STATUS "pdflatex not found; 'docs' target unavailable")
endif()
