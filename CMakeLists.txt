# Copyright 2024 The Chris Poovey. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.10)
project(GrainflowLib)

file(GLOB_RECURSE GRAINFLOW_HEADERS
	 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	 ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
add_custom_target( GRAINFLOW_LIB ALL
	SOURCES ${GRAINFLOW_HEADERS}
)


