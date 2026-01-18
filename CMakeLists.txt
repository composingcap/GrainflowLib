# Copyright 2024 The Chris Poovey. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Force static runtime on MSVC (required for Max SDK compatibility)
# Must be set before project() call in CMake 3.15+
if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(GrainflowLib)

# Apply static runtime to all targets (for CMake < 3.15 compatibility and explicit control)
if(MSVC)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

# Disable AudioFile tests and examples
set(BUILD_TESTS OFF CACHE BOOL "Disable AudioFile tests" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable AudioFile examples" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/AudioFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/AudioFile)


# SAF configuration - must be set before adding SAF subdirectory
if(APPLE)
	set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE_LP64" CACHE STRING "" FORCE)
elseif(MSVC)
	# Use Intel MKL on Windows (full installer provides /MT-compatible libraries)
	# MKL must be installed via oneAPI installer - NuGet packages use /MD which conflicts with Max SDK's /MT
	if(NOT DEFINED INTEL_MKL_DIR)
		set(INTEL_MKL_DIR "C:/Program Files (x86)/Intel/oneAPI/mkl/latest")
	endif()

	set(INTEL_MKL_HEADER_PATH "${INTEL_MKL_DIR}/include" CACHE STRING "" FORCE)
	set(INTEL_MKL_LIB "${INTEL_MKL_DIR}/lib/mkl_intel_lp64.lib;${INTEL_MKL_DIR}/lib/mkl_sequential.lib;${INTEL_MKL_DIR}/lib/mkl_core.lib" CACHE STRING "" FORCE)

	set(SAF_PERFORMANCE_LIB "SAF_USE_INTEL_MKL_LP64" CACHE STRING "" FORCE)
else()
	set(SAF_PERFORMANCE_LIB "SAF_USE_OPEN_BLAS_AND_LAPACKE" CACHE STRING "" FORCE)
endif()

# Disable SAF tests, examples, and extras
set(SAF_BUILD_TESTS OFF CACHE BOOL "Disable SAF tests" FORCE)
set(SAF_BUILD_EXAMPLES OFF CACHE BOOL "Disable SAF examples" FORCE)
set(SAF_BUILD_EXTRAS OFF CACHE BOOL "Disable SAF extras" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/Spatial_Audio_Framework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/Spatial_Audio_Framework)

file(GLOB_RECURSE GRAINFLOW_HEADERS
	 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	 ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_custom_target(GRAINFLOW_LIB ALL
	SOURCES ${GRAINFLOW_HEADERS} "${CMAKE_CURRENT_SOURCE_DIR}/lib/AudioFile/AudioFile.h"
)

# Optional: Build benchmarks if BUILD_BENCHMARKS is enabled
option(BUILD_BENCHMARKS "Build benchmark suite" ON)

if(BUILD_BENCHMARKS)
	message(STATUS "Building GrainflowLib benchmarks")
	add_subdirectory(benchmarks)
endif()
