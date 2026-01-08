cmake_minimum_required(VERSION 3.16)

project(frameflow
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(frameflow
    src/layout.cpp
)

add_library(frameflow::frameflow ALIAS frameflow)

target_include_directories(frameflow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(frameflow PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(frameflow PRIVATE /W4)
else()
    target_compile_options(frameflow PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(FRAMEFLOW_BUILD_TESTS "Build tests" OFF)
if (FRAMEFLOW_BUILD_TESTS)
    add_subdirectory(tests)
endif()
