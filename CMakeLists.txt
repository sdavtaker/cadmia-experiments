cmake_minimum_required(VERSION 3.28)
project(cadmia-experiments VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CADMIA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../simulators/cadmia"
    CACHE PATH "Path to cadmia source")

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

set(CADMIA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(CADMIA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CADMIA_DIR}" cadmia-build)

# add_subdirectory(vdw14)   # uncomment when vdw14 experiment is ready
