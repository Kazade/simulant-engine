#
# Cross-compiles for iOS (or the iOS simulator) with upstream clang and
# cctools-port's ld64, as set up by platforms/ios/Dockerfile. Set:
#
#   IOS_SDK         Path to iPhoneOS.sdk (or iPhoneSimulator.sdk). Required.
#   IOS_ARCH        arm64 (the default) or x86_64 (the simulator on Intel Macs)
#   IOS_SIMULATOR   ON to build for the simulator. Defaults to whether
#                   IOS_SDK is a simulator SDK.
#   IOS_DEPLOYMENT_TARGET  The oldest iOS version to support (default 14.0)
#   IOS_TOOLCHAIN   Where cctools-port is installed (default /opt/ios-toolchain)
#
# Each of these can be set with -D or as an environment variable.
#

foreach(var IOS_SDK IOS_ARCH IOS_SIMULATOR IOS_DEPLOYMENT_TARGET IOS_TOOLCHAIN)
  if(NOT DEFINED ${var} AND DEFINED ENV{${var}})
    set(${var} "$ENV{${var}}")
  endif()
endforeach()

if(NOT IOS_SDK)
  message(FATAL_ERROR "Set IOS_SDK to the path of an iPhoneOS or iPhoneSimulator SDK")
endif()

if(NOT IOS_ARCH)
  set(IOS_ARCH arm64)
endif()

if(NOT DEFINED IOS_SIMULATOR)
  if(IOS_SDK MATCHES "iPhoneSimulator")
    set(IOS_SIMULATOR ON)
  else()
    set(IOS_SIMULATOR OFF)
  endif()
endif()

if(NOT IOS_DEPLOYMENT_TARGET)
  set(IOS_DEPLOYMENT_TARGET 14.0)
endif()

if(NOT IOS_TOOLCHAIN)
  set(IOS_TOOLCHAIN /opt/ios-toolchain)
endif()

# Checks (try_compile) run this file again in projects of their own, and not
# all of them are passed our variables, so they're left in the environment
foreach(var IOS_SDK IOS_ARCH IOS_SIMULATOR IOS_DEPLOYMENT_TARGET IOS_TOOLCHAIN)
  set(ENV{${var}} "${${var}}")
endforeach()

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_PROCESSOR ${IOS_ARCH})
set(CMAKE_OSX_SYSROOT ${IOS_SDK})
set(CMAKE_OSX_ARCHITECTURES ${IOS_ARCH})

# The deployment target goes in the target triple (which is also what says
# it's for the simulator), so CMake isn't given one to add its own flag for
set(IOS_TRIPLE ${IOS_ARCH}-apple-ios${IOS_DEPLOYMENT_TARGET})
if(IOS_SIMULATOR)
  set(IOS_TRIPLE ${IOS_TRIPLE}-simulator)
endif()

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${IOS_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${IOS_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${IOS_TRIPLE})

set(CMAKE_AR ${IOS_TOOLCHAIN}/bin/ar CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB ${IOS_TOOLCHAIN}/bin/ranlib CACHE FILEPATH "" FORCE)
set(CMAKE_LIPO ${IOS_TOOLCHAIN}/bin/lipo CACHE FILEPATH "" FORCE)
set(CMAKE_INSTALL_NAME_TOOL ${IOS_TOOLCHAIN}/bin/install_name_tool CACHE FILEPATH "" FORCE)

# Link with Apple's linker. Clang can't ask it for its version when it's not
# running on a Mac, and it needs to know to pass the newer flags.
set(IOS_LINKER_FLAGS "-fuse-ld=${IOS_TOOLCHAIN}/bin/ld64 -mlinker-version=956.6")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${IOS_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${IOS_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${IOS_LINKER_FLAGS}")

# Executables have to be signed app bundles to run, so checks only build
# static libraries
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH ${IOS_SDK})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Threads are part of libSystem. FindThreads can't link anything to find out
# (see above), so it's told.
set(CMAKE_HAVE_LIBC_PTHREAD 1 CACHE INTERNAL "")
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(THREADS_PREFER_PTHREAD_FLAG OFF)
