# In this file we are doing all of our 'configure' checks. Things like checking
# for headers, functions, libraries, types and size of types.
INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckCXXSourceCompiles.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)

# check for include files
CHECK_INCLUDE_FILE("stdarg.h" HAVE_STDARG_H)

# check the size of primitive types
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
math(EXPR BITS_PER_LONG "8 * ${SIZEOF_LONG}")

# check for functions: set/get thread name
# list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
# list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
# CHECK_SYMBOL_EXISTS(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
# CHECK_SYMBOL_EXISTS(pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP)
# list(REMOVE_ITEM CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

# CHECK_FUNCTION_EXISTS("pthread_setname_np" HAVE_PTHREAD_SETNAME_NP)
# CHECK_FUNCTION_EXISTS("pthread_getname_np" HAVE_PTHREAD_GETNAME_NP)

# CHECK_SYMBOL_EXISTS(prctl "sys/prctl.h" HAVE_PRCTL)

# CHECK_INCLUDE_FILE("sys/prctl.h" HAVE_SYS_PRCTL_H)
# CHECK_FUNCTION_EXISTS("prctl" HAVE_PRCTL)
