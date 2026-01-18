# Copyright 2024 The Chris Poovey. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set macOS deployment target to match Homebrew's Google Benchmark
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()

project(GrainflowBenchmarks)

# Find Google Benchmark
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(WARNING "Google Benchmark not found. Install with: brew install google-benchmark")
    message(WARNING "Skipping benchmark build.")
    return()
endif()

message(STATUS "Found Google Benchmark")

# Include GrainflowLib headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/AudioFile)

# Common function to create benchmark targets
function(add_grainflow_benchmark target_name source_file)
    add_executable(${target_name} ${source_file})

    target_link_libraries(${target_name}
        PRIVATE
        benchmark::benchmark
    )

    # Set compiler flags for optimization
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /O2 /W4)
    else()
        target_compile_options(${target_name} PRIVATE -O3 -Wall -Wextra)
    endif()

    # Enable C++17 features
    target_compile_features(${target_name} PRIVATE cxx_std_17)

    # On macOS, use native architecture only (Google Benchmark from Homebrew is arch-specific)
    if(APPLE)
        set_target_properties(${target_name} PROPERTIES
            OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}"
        )
    endif()

    # Install benchmark executable
    install(TARGETS ${target_name} RUNTIME DESTINATION bin)
endfunction()

# Create separate benchmark executables
add_grainflow_benchmark(benchmark_utils benchmark_utils.cpp)
add_grainflow_benchmark(benchmark_synthesis benchmark_synthesis.cpp)
add_grainflow_benchmark(benchmark_integration benchmark_integration.cpp)

# Create a custom target to run all benchmarks
add_custom_target(run_all_benchmarks
    COMMAND benchmark_utils
    COMMAND benchmark_synthesis
    COMMAND benchmark_integration
    DEPENDS benchmark_utils benchmark_synthesis benchmark_integration
    COMMENT "Running all Grainflow benchmarks"
)
