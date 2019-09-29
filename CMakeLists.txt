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
	
	# third-parties and dependencies
	${CMAKE_SOURCE_DIR}/include
)

add_definitions(
	-DPROJECT_PATH_PREFIX="${CMAKE_SOURCE_DIR}"
)

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

# cov-autobuf
set(SOURCES examples/cov-autobuf.c)
add_executable(cov-autobuf ${SOURCES})
target_link_libraries(cov-autobuf xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-autobuf gcov)
endif()
redefine_file_macro(cov-autobuf)

# cov-xlog-no-default-context
set(SOURCES examples/cov-not-default-context.c)
add_executable(cov-not-default-context ${SOURCES})
target_link_libraries(cov-not-default-context xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-not-default-context gcov)
endif()
redefine_file_macro(cov-not-default-context)

# cov-shell
set(SOURCES examples/cov-shell.c)
add_executable(cov-shell ${SOURCES})
target_link_libraries(cov-shell xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-shell gcov)
endif()
redefine_file_macro(cov-shell)

# cov-plugins
set(SOURCES examples/cov-plugins.c)
add_executable(cov-plugins ${SOURCES})
target_link_libraries(cov-plugins xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(cov-plugins gcov)
endif()
redefine_file_macro(cov-plugins)


# bench-multi-threads
set(SOURCES examples/bench-multi-threads.c)
add_executable(bench-multi-threads ${SOURCES})
target_link_libraries(bench-multi-threads xlog)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(bench-multi-threads gcov)
endif()
redefine_file_macro(bench-multi-threads)


# demo-xlog
set(SOURCES examples/demo-xlog.c)
add_executable(demo-xlog ${SOURCES})
target_link_libraries(demo-xlog xlog pthread)
if(ENABLE_COVERAGE_CHECK)
	target_link_libraries(demo-xlog gcov)
endif()
redefine_file_macro(demo-xlog)

