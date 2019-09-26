set(XLOG_LIB xlog)

set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 4)
set(PROJECT_VERSION_SO    1)

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# features
option(XLOG_FEATURE_ENABLE_COLOR "Enable colorful logging" ON)

option(XLOG_FEATURE_ENABLE_STATS "Enable statistics" OFF)
option(XLOG_FEATURE_ENABLE_STATS_PRINTER "Enable statistics for printer(s)" ON)
option(XLOG_FEATURE_ENABLE_STATS_MODULE "Enable statistics for module(s)" ON)
option(XLOG_FEATURE_ENABLE_DEFAULT_CONTEXT "Create default context if XLOG_CONTEXT is NULL" ON)

option(XLOG_VERSION_WITH_BUILDDATE "Append build-date to version" OFF)

# policies
option(XLOG_POLICY_ENABLE_RUNTIME_SAFE "Enable runtime safe policy" ON)

# bench configurations
option(XLOG_BENCH_NO_OUTPUT "Disable log output to test rate of logging formatting" OFF)

# include paths
include_directories(
	# current and subdirectories
	.
	
	# third-parties and dependencies
	${CMAKE_SOURCE_DIR}/include
)

# headers
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/xlog)

# sources
set(
	SOURCES
	
	xlog.c
	xlog_shell.c
	xlog_payload.c
	xlog_printer.c
	
	plugins/family_tree.c
	plugins/ringbuf.c
	plugins/hexdump.c
	plugins/getopt.c
	
	printers/stdio-basic.c
	printers/stdio-ringbuf.c
	printers/file-basic.c
	printers/file-daily.c
	printers/file-rotating.c
)

configure_file(
	"${CMAKE_SOURCE_DIR}/include/xlog/xlog_config.h.in"
	"${CMAKE_SOURCE_DIR}/include/xlog/xlog_config.h"
	@ONLY
)

option(BUILD_SHARED_AND_STATIC_LIBS "Build both shared and static libraries" ON)
option(XLOG_OVERRIDE_BUILD_SHARED_LIBS "Override BUILD_SHARED_LIBS with XLOG_BUILD_SHARED_LIBS" OFF)
option(XLOG_BUILD_SHARED_LIBS "Overrides BUILD_SHARED_LIBS if XLOG_OVERRIDE_BUILD_SHARED_LIBS is enabled" OFF)
if (NOT BUILD_SHARED_AND_STATIC_LIBS)
	if ((XLOG_OVERRIDE_BUILD_SHARED_LIBS AND XLOG_BUILD_SHARED_LIBS) OR ((NOT XLOG_OVERRIDE_BUILD_SHARED_LIBS) AND BUILD_SHARED_LIBS))
		set(XLOG_LIBRARY_TYPE SHARED)
	else()
		set(XLOG_LIBRARY_TYPE STATIC)
	endif()
	add_library("${XLOG_LIB}" "${XLOG_LIBRARY_TYPE}" "${HEADERS}" "${SOURCES}")
	
	if (NOT WIN32)
		target_link_libraries("${XLOG_LIB}" m pthread)
	endif()
	
	install(
		TARGETS "${XLOG_LIB}"
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	install(
		DIRECTORY ${CMAKE_SOURCE_DIR}/include/${XLOG_LIB} DESTINATION include
	)
else()
	# See https://cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F
	add_library("${XLOG_LIB}" STATIC "${HEADERS}" "${SOURCES}")
	add_library("${XLOG_LIB}-shared" SHARED "${HEADERS}" "${SOURCES}")
	set_target_properties("${XLOG_LIB}-shared" PROPERTIES OUTPUT_NAME "${XLOG_LIB}")
	set_target_properties("${XLOG_LIB}-shared" PROPERTIES PREFIX "lib")
	set_target_properties("${XLOG_LIB}-shared" PROPERTIES SOVERSION "${PROJECT_VERSION_SO}" VERSION "${PROJECT_VERSION}")
	if (NOT WIN32)
		target_link_libraries("${XLOG_LIB}" m pthread)
		target_link_libraries("${XLOG_LIB}-shared" m pthread)
	endif()
endif()

install(
	TARGETS "${XLOG_LIB}"
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/include/${XLOG_LIB} DESTINATION include
)
if (BUILD_SHARED_AND_STATIC_LIBS)
	install(
		TARGETS "${XLOG_LIB}-shared"
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
endif()
