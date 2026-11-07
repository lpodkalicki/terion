cmake_minimum_required(VERSION 3.28)
project(terion VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)

option(BUILD_TESTING "Build testing" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

if (BUILD_TESTING)
    enable_testing()
endif ()

add_subdirectory(libterion)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
