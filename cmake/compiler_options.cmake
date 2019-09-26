# compiler flags
set(custom_compiler_flags)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D__DEBUG__)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
            -g
            -O0
        )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        list(APPEND custom_compiler_flags
            /O0
        )
    endif()
else()
    add_definitions(-D__RELEASE__ -DNDEBUG)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
            -O3
        )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        list(APPEND custom_compiler_flags
            /O3
        )
    endif()
endif()

option(ENABLE_CUSTOM_COMPILER_FLAGS "Enables custom compiler flags" ON)
if (ENABLE_CUSTOM_COMPILER_FLAGS)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
            -std=gnu99
            -pedantic
            -Wall
            #-Werror
            -Wno-pragmas
            -Wno-unknown-pragmas
            -Wcast-align
            -Wcast-qual
            -Wswitch-enum
            -Wextra
            -Wstrict-prototypes
            -Wwrite-strings
            -Wshadow
            -Winit-self
            -Wformat=2
            -Wmissing-prototypes
            -Wstrict-overflow=2
            -Wundef
            -Wswitch-default
            -Wconversion
            -Wc++-compat
            -fstack-protector-strong
            -Wcomma
            -Wdouble-promotion
            -Wparentheses
            -Wformat-overflow
            -Wunused-macros
            -Wmissing-variable-declarations
            -Wused-but-marked-unused
        )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Disable warning c4001 - nonstandard extension 'single line comment' was used
        # Define _CRT_SECURE_NO_WARNINGS to disable deprecation warnings for "insecure" C library functions
        list(APPEND custom_compiler_flags
            /GS
            /Za
            /sdl
            /W4
            /wd4001
            /D_CRT_SECURE_NO_WARNINGS
        )
    endif()
endif()

option(ENABLE_SANITIZERS "Enables AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
if (ENABLE_SANITIZERS)
    list(APPEND custom_compiler_flags
        -fno-omit-frame-pointer
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
        #-fsanitize-address-use-after-scope
        -fsanitize=integer
        -01
        #-fno-sanitize-recover
        -fno-sanitize-recover=all
    )
endif()

option(ENABLE_SAFE_STACK "Enables the SafeStack instrumentation pass by the Code Pointer Integrity Project" OFF)
if (ENABLE_SAFE_STACK)
    if (ENABLE_SANITIZERS)
        message(FATAL_ERROR "ENABLE_SAFE_STACK cannot be used in combination with ENABLE_SANITIZERS")
    endif()
    list(APPEND custom_compiler_flags
        -fsanitize=safe-stack
    )
endif()

option(ENABELE_POSITION_INDEPENDENT_CODE "Enable Position Independent Code" ON)
if (ENABELE_POSITION_INDEPENDENT_CODE)
    list(APPEND custom_compiler_flags -fPIC)
endif()

option(ENABLE_PUBLIC_SYMBOLS "Export library symbols." ON)
if (ENABLE_PUBLIC_SYMBOLS)
    list(APPEND custom_compiler_flags -fvisibility=hidden)
    add_definitions(-DXLOG_EXPORT_SYMBOLS -DXLOG_API_VISIBILITY)
endif()
option(ENABLE_HIDDEN_SYMBOLS "Hide library symbols." OFF)
if (ENABLE_HIDDEN_SYMBOLS)
    add_definitions(-DXLOG_HIDE_SYMBOLS -UXLOG_API_VISIBILITY)
endif()

# apply custom compiler flags
include(CheckCCompilerFlag)
set(supported_compiler_flags)
foreach(compiler_flag ${custom_compiler_flags})
    #remove problematic characters
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" current_variable ${compiler_flag})
    
    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_${current_variable}")
    if (OPTION_${current_variable})
        list(APPEND supported_compiler_flags ${compiler_flag})
    endif()
endforeach()

string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${supported_compiler_flags}")
