project(xlog C)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE "Debug")

# install
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/lib)

include(${CMAKE_SOURCE_DIR}/cmake/platform_check.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/compiler_options.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/utilities.cmake)

option(ENABLE_COVERAGE_CHECK "Enable Coverage check" OFF)
if(ENABLE_COVERAGE_CHECK)
	add_definitions(-coverage)
endif()

# include paths
include_directories(
	# current and subdirectories
	.

	# top module

	# third-parties and dependencies

	# application
	${CMAKE_SOURCE_DIR}/include

	# nonstandard
)

add_definitions(
	-DPROJECT_PATH_PREFIX="${CMAKE_SOURCE_DIR}"
)

add_library(getopt STATIC getopt.c)

add_subdirectory(xlog)

link_directories(${LIBRARY_OUTPUT_PATH})

# cov-printer
set(SOURCES examples/cov-printer.c)
add_executable(cov-printer ${SOURCES})
target_link_libraries(cov-printer xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-printer gcov)
endif()
redefine_file_macro(cov-printer)

# cov-payload
set(SOURCES examples/cov-payload.c)
add_executable(cov-payload ${SOURCES})
target_link_libraries(cov-payload xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-payload gcov)
endif()
redefine_file_macro(cov-payload)

# demo-xlog
set(SOURCES examples/demo-xlog.c)
add_executable(demo-xlog ${SOURCES})
target_link_libraries(demo-xlog xlog pthread getopt)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(demo-xlog gcov)
endif()
redefine_file_macro(demo-xlog)

